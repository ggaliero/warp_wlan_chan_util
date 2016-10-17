/** @file wlan_mac_queue.c
 *  @brief Transmit Queue Framework
 *
 *  This contains code for accessing the transmit queue.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

// Xilinx / Standard library includes
#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "xintc.h"
#include "string.h"

// WLAN includes
#include "wlan_mac_common.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_eth_util.h"

// WLAN Exp includes
#include "wlan_exp_common.h"


/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

// User callback to see if the higher-level framework can send a packet to
// the lower-level framework to be transmitted.
extern function_ptr_t        tx_poll_callback;
static function_ptr_t        queue_state_change_callback;


/*************************** Functions Prototypes ****************************/

/*************************** Variable Definitions ****************************/

// List to hold all of the empty, free entries
static dl_list               free_queue;


// The tx_queues variable is an array of lists that will be filled with queue
// entries from the free_queue list
//
// NOTE:  This implementation sparsely packs the tx_queues array to allow fast
//     indexing at the cost of some wasted memory. The tx_queues array will be
//     reallocated whenever the upper-level MAC asks to enqueue at an index
//     that is larger than the current size of the array. It is assumed that
//     this index will not continue to grow over the course of execution,
//     otherwise this array will continue to grow and eventually be unable to
//     be reallocated. Practically speaking, this means an AP needs to re-use
//     the AIDs it issues stations if it wants to use the AIDs as an index into
//     the tx queue.
//
static dl_list*              tx_queues;
static u16                   num_tx_queues;


// Total number of Tx queue entries
volatile static u32          total_tx_queue_entries;


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * @brief  Initialize the Tx Queue framework
 *
 * The number of Tx Queue elements we can initialize is limited by the smaller of two values:
 *     (1) The number of dl_entry structs we can squeeze into TX_QUEUE_DL_ENTRY_MEM_SIZE
 *     (2) The number of QUEUE_BUFFER_SIZE MPDU buffers we can squeeze into TX_QUEUE_BUFFER_SIZE
 *
 * @param  u8 dram_present        - Flag to indicate if DRAM is present
 *                                  (1 = present; other = not present)
 *
 *****************************************************************************/
void queue_init(u8 dram_present){
	u32            i;
	dl_entry     * dl_entry_base;

	// Check that DRAM is present; Fatal error if not.
	if(dram_present != 1){
		xil_printf("A working DRAM SODIMM has not been detected on this board.\n");
		xil_printf("DRAM is required for the wireless transmission queue.  Halting.\n");
		cpu_error_halt(WLAN_ERROR_CODE_DRAM_NOT_PRESENT);
	}

	// Set the total number of supported Tx Queue entries
	total_tx_queue_entries = min((TX_QUEUE_DL_ENTRY_MEM_SIZE / sizeof(dl_entry)),   // Max dl_entry
	                             (TX_QUEUE_BUFFER_SIZE / QUEUE_BUFFER_SIZE));       // Max buffers

	// Initialize the Tx queue
	//     NOTE:  tx_queues is initially NULL because it will be dynamically allocated and
	//         initialized.
	//
	tx_queues     = NULL;
	num_tx_queues = 0;

	queue_state_change_callback = (function_ptr_t)wlan_null_callback;

	// Initialize the free queue
	dl_list_init(&free_queue);

	// Zero all Tx queue elements
	bzero((void*)TX_QUEUE_BUFFER_BASE, TX_QUEUE_BUFFER_SIZE);

	// Allocate the memory space into Tx Queue entries
	//
	// All Tx Queue entries are initially part of the free queue.  Each Tx Queue entry consists of:
	//     (1) Buffer to hold data
	//     (1) dl_entry to describe the buffer
	//
	// NOTE:  The code below exploits the fact that the starting state of all Tx Queue entries is
	//     sequential.  Therefore, it can use matrix addressing.  Matrix addressing is not safe
	//     once the queue is used and the insert/remove helper functions should be used instead.
	//
	dl_entry_base = (dl_entry*)(TX_QUEUE_DL_ENTRY_MEM_BASE);

	for (i = 0; i < total_tx_queue_entries; i++) {
		// Segment the buffer in QUEUE_BUFFER_SIZE pieces
		dl_entry_base[i].data = (void*)(TX_QUEUE_BUFFER_BASE + (i * QUEUE_BUFFER_SIZE));

		// Insert new dl_entry into the free queue
		dl_entry_insertEnd(&free_queue, &(dl_entry_base[i]));
	}

	// Print status
	xil_printf("Tx Queue of %d placed in DRAM: using %d kB\n", total_tx_queue_entries,
	           ((total_tx_queue_entries * QUEUE_BUFFER_SIZE) / 1024));
}



/*****************************************************************************/
/**
 * @brief  Total number of Tx Queue entries
 *
 * This is the sum of all free and occupied Tx Queue entries
 *
 * @return u32
 *
 *****************************************************************************/
u32 queue_total_size(){
	return total_tx_queue_entries;
}



/*****************************************************************************/
/**
 * @brief  Number of free Tx Queue entries
 *
 * @return u32
 *
 *****************************************************************************/
u32 queue_num_free(){
	return free_queue.length;
}



/*****************************************************************************/
/**
 * @brief  Number of Tx Queue entries in a given queue
 *
 * @param  u16 queue_sel          - ID of the queue
 *
 * @return u32
 *
 *****************************************************************************/
u32 queue_num_queued(u16 queue_sel){
	if((queue_sel+1) > num_tx_queues){
		return 0;
	} else {
		return tx_queues[queue_sel].length;
	}
}



/*****************************************************************************/
/**
 * @brief  Removes all Tx Queue entries in the selected queue
 *
 * This function removes all entries from the selected queue and returns them to
 * the free pool. Packets contained in the purged entries will be dropped. This
 * function should only be called when enqueued packets should no longer be
 * transmitted wirelessly, such as when a node leaves a BSS.
 *
 * @param  u16 queue_sel          - ID of the queue to purge
 *
 *****************************************************************************/
void purge_queue(u16 queue_sel){
	u32                        num_queued;
	u32                        i;
	tx_queue_element_t       * curr_tx_queue_element;
	volatile interrupt_state_t prev_interrupt_state;

	num_queued = queue_num_queued(queue_sel);

	if (num_queued > 0) {
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
		wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_queue, "Purging %d packets from queue %d\n", num_queued, queue_sel);
#endif

		// NOTE:  There is an interesting condition that can occur if an LTG is running (ie creating new TX queue
		//     entries) and purge_queue is called.  In this case, you have one process removing elements from the
		//     queue while another process is adding elements to the queue.  Therefore, purge_queue will only
		//     remove a fixed number of elements from the queue (ie all queued elements at the time the function
		//     is called).  If purge_queue used a while loop with no checking on the number of elements removed,
		//     then it could conceivably run forever.
		//
		// TODO:  Currently, this is a relatively slow implementation.  It could be sped up dramatically if
		//     it used the dl_entry_move() function.  However, this would need to be tested to make sure there
		//     were no corner cases where the framework was currently transmitting element from that queue.
		//
		for (i = 0; i < num_queued; i++) {
			// The queue purge is not interrupt safe
			//     NOTE:  Since there could be many elements in the queue, we need to toggle the interrupts
			//         inside the for loop so that we do not block CPU High for an extended period of time.
			//         This could result in CPU Low dropping a reception.
			//
			prev_interrupt_state = wlan_mac_high_interrupt_stop();

			curr_tx_queue_element = dequeue_from_head(queue_sel);
			queue_checkin(curr_tx_queue_element);

			// Re-enable interrupts
			wlan_mac_high_interrupt_restore_state(prev_interrupt_state);
		}
	}
}



/*****************************************************************************/
/**
 * @brief  Adds a queue entry to a specified queue
 *
 * Adds the queue entry pointed to by tqe to the queue with ID queue_sel. The
 * calling context must ensure tqe points to a queue entry containing a packet
 * ready for wireless transmission.  If a queue with ID quele_sel does not
 * already exist this function will create it, then add tqe to the new queue.
 *
 * @param  u16 queue_sel          - ID of the queue to which tqe is added. A new
 *                                  queue with ID queue_sel will be created if it
 *                                  does not already exist.
 * @param  tx_queue_element_t* tqe  - Tx Queue entry containing packet for transmission
 *
 *****************************************************************************/
void enqueue_after_tail(u16 queue_sel, tx_queue_element_t* tqe){
	u32 i;

	// Create queues up to and including queue_sel if they don't already exist
	// Queue IDs are low-valued integers, allowing for fast lookup by indexing the tx_queues array
	if ((queue_sel + 1) > num_tx_queues) {
		tx_queues = wlan_mac_high_realloc(tx_queues, ((queue_sel + 1) * sizeof(dl_list)));

		if(tx_queues == NULL){
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
			wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_queue, "Could not reallocate %d bytes for queue %d\n",
			                ((queue_sel + 1) * sizeof(dl_list)), queue_sel);
#endif
		}

		for(i = num_tx_queues; i <= queue_sel; i++){
			dl_list_init(&(tx_queues[i]));
		}

		num_tx_queues = queue_sel + 1;
	}

	// Insert the queue entry into the dl_list representing the selected queue
	dl_entry_insertEnd(&(tx_queues[queue_sel]), (dl_entry*)tqe);

	// Update the occupancy of the tx queue for the tx_queue_element
	//     NOTE:  This is the best place to record this value since it will catch all cases.  However,
	//         when populating the tx_frame_info, be careful to not overwrite this value.  Also, this
	//         field is set after the current tx queue element has been added to the queue, so the
	//         occupancy value includes itself.
	//
	((tx_queue_buffer_t*)(tqe->data))->tx_frame_info.queue_info.occupancy = (tx_queues[queue_sel].length & 0xFFFF);

	if(tx_queues[queue_sel].length == 1){
		//If the queue element we just added is now the only member of this queue, we should inform
		//the top-level MAC that the queue has transitioned from empty to non-empty.
		queue_state_change_callback(queue_sel, 1);
	}

    // Poll the TX queues to see if anything needs to be transmitted
	tx_poll_callback();

	return;
}



/*****************************************************************************/
/**
 * @brief  Removes the head entry from the specified queue
 *
 * If queue_sel is not empty this function returns a tx_queue_element pointer
 * for the head entry in the queue. If the specified queue is empty this
 * function returns NULL.
 *
 * @param  u16 queue_sel          - ID of the queue from which to dequeue an entry
 *
 * @return tx_queue_element_t *     - Pointer to queue entry if available,
 *                                  NULL if queue is empty
 *
 *****************************************************************************/
tx_queue_element_t* dequeue_from_head(u16 queue_sel){
	dl_entry          * curr_dl_entry;

	if ((queue_sel + 1) > num_tx_queues) {
		// The specified queue does not exist; this can happen if a node has
		//     associated (has a valid AID = queue_sel) but no packet has ever
		//     been enqueued to it, as queues are created upon first insertion
		//     - see enqueue_after_tail()
		return NULL;
	} else {
		if (tx_queues[queue_sel].length == 0) {
			// Requested queue exists but is empty
			return NULL;
		} else {
			curr_dl_entry = (tx_queues[queue_sel].first);
			dl_entry_remove(&tx_queues[queue_sel], curr_dl_entry);

			if(tx_queues[queue_sel].length == 0){
				//If the queue element we just removed empties the queue, we should inform
				//the top-level MAC that the queue has transitioned from non-empty to empty.
				queue_state_change_callback(queue_sel, 0);
			}

			return (tx_queue_element_t *) curr_dl_entry;
		}
	}
}



/*****************************************************************************/
/**
 * @brief  Checks out one queue entry from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function
 * removes one entry from the free pool and returns it for use by the MAC
 * application. If the free pool is empty NULL is returned.
 *
 * @return tx_queue_element_t *     - Pointer to new queue entry if available,
 *                                  NULL if free queue is empty
 *
 *****************************************************************************/
tx_queue_element_t* queue_checkout(){
	tx_queue_element_t* tqe;

	if(free_queue.length > 0){
		tqe = ((tx_queue_element_t*)(free_queue.first));
		dl_entry_remove(&free_queue,free_queue.first);

		// Initialize the metadata type of the tx_queue entry since this
		// framework does not guarantee a Tx Queue entry is all zero on checkout
		((tx_queue_buffer_t*)(tqe->data))->metadata.metadata_type = QUEUE_METADATA_TYPE_IGNORE;

		// Set the Tx Packet Buffer state to uninitialized. This will be set to READY
		// after dequeue and CDMA into the actual Tx packet buffer
		((tx_queue_buffer_t*)(tqe->data))->tx_frame_info.tx_pkt_buf_state = TX_PKT_BUF_UNINITIALIZED;

		return tqe;
	} else {
		return NULL;
	}
}



/*****************************************************************************/
/**
 * @brief  Checks in one queue entry to the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function
 * returns one entry to the free pool.  tqe must be a valid pointer to a queue
 * entry which the MAC application no longer needs. The application must not
 * use the entry pointed to by tqe after calling this function.
 *
 * @param  tx_queue_element_t* tqe  - Pointer to queue entry to be returned to free pool
 *
 *****************************************************************************/
void queue_checkin(tx_queue_element_t* tqe){
	if (tqe != NULL) {
	    dl_entry_insertEnd(&free_queue, (dl_entry*) tqe);
	}
}



/*****************************************************************************/
/**
 * @brief Checks out multiple queue entries from the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function
 * attempts to check out num_tqe queue entries from the free pool. The number of
 * queue entries successfully checked out is returned. This may be less than
 * requested if the free pool had fewer than num_tqe entries available.
 *
 * @param   dl_list * new_list    - Pointer to dl_list to which queue entries are appended
 * @param   u16 num_tqe           - Number of queue entries requested
 *
 * @return  Number of queue entries successfully checked out and appended to new_list
 *
 *****************************************************************************/
int queue_checkout_list(dl_list* list, u16 num_tqe){
#if 0
	// Approx. performance metrics:
	//    incremental processing = 3.3 us / queue entry
	//    overhead               = 0.3 us
	//
	// Ex.  For one queue entry, function will take 3.6 us (ie (3.3 * 1) + 0.3 = 3.6)
	//
	u32       i;
	u32       num_checkout;
	dl_entry* curr_dl_entry;

	if(num_tqe <= free_queue.length){
		num_checkout = num_tqe;
	} else {
		num_checkout = free_queue.length;
	}

	//Traverse the free_queue and update the pointers
	for (i = 0; i < num_checkout; i++){
		curr_dl_entry = (free_queue.first);

		//Remove from free list
		dl_entry_remove(&free_queue, curr_dl_entry);

		//Add to new checkout list
		dl_entry_insertEnd(list, curr_dl_entry);
	}

	return num_checkout;
#else
	// Approx. performance metrics:
	//    incremental processing = 0.16 us / queue entry
	//    overhead               = 1.84 us
	//
	// Ex.  For one queue entry, function will take 2.0 us (ie (0.16 * 1) + 1.84 = 2.0)
	//
    return dl_entry_move(&free_queue, list, num_tqe);
#endif
}



/*****************************************************************************/
/**
 * @brief Checks in multiple queue entries into the free pool
 *
 * The queue framework maintains a pool of free queue entries. This function will
 * check in all queue entries from the provided list to the end of the free pool.
 *
 * @param   dl_list * new_list    - Pointer to dl_list from which queue entries will be checked in
 *
 * @return  Number of queue entries successfully checked in
 *
 *****************************************************************************/
int queue_checkin_list(dl_list * list) {
#if 0
	u32       i;
	u32       num_checkin;
	dl_entry* curr_dl_entry;

	num_checkin = list->length;

	// Traverse the list and update the pointers
	for (i = 0; i < num_checkin; i++){
		curr_dl_entry = (list->first);

		//Remove from free list
		dl_entry_remove(list, curr_dl_entry);

		//Add to new checkout list
		dl_entry_insertEnd(&free_queue, curr_dl_entry);
	}
#else
    return dl_entry_move(list, &free_queue, list->length);
#endif
}



/*****************************************************************************/
/**
 * @brief   Dequeues one packet and submits it the lower MAC for wireless transmission
 *
 * This function is the link between the Tx queue framework and the lower-level MAC.
 * The calling function has determined the next wireless transmission should come
 * from the queue with ID queue_sel.  This function attempts to checkout 1 packet
 * from that queue. If a packet is available it is passed to wlan_mac_high_mpdu_transmit(),
 * which handles the actual copy-to-pkt-buf and Tx process.
 *
 * When a packet is successfully de-queued and submitted for transmission the
 * corresponding queue entry (tx_queue_element) is returned to the free pool.
 *
 * This function returns 0 if no packets are available in the requested queue or if
 * the MAC state machine is not ready to submit a new packet to the lower MAC for
 * transmission.
 *
 * @param   u16 queue_sel         - Queue ID from which to dequeue packet
 *
 * @return  Number of successfully transmitted packets (0 or 1)
 *
 *****************************************************************************/
inline int dequeue_transmit_checkin(u16 queue_sel){
	int                 return_value             = 0;
	int                 tx_pkt_buf               = -1;
	tx_queue_element_t  * curr_tx_queue_element;

	tx_pkt_buf = wlan_mac_high_get_empty_tx_packet_buffer();

	if (tx_pkt_buf != -1) {
		// Get the Tx queue element to transmit
		curr_tx_queue_element = dequeue_from_head(queue_sel);

		if(curr_tx_queue_element != NULL){
			// Transmit the Tx Queue element
			//     NOTE:  This copies all the contents of the queue element to the
			//         packet buffer so that the queue element can be safely returned
			//         to the free pool
			wlan_mac_high_mpdu_transmit(curr_tx_queue_element, tx_pkt_buf);

			// Check in the Tx Queue element because it is not long being used
			queue_checkin(curr_tx_queue_element);

			// Update the Ethernet DMA
			//     NOTE:  Given that there is now another free Tx queue element, let
			//         the Ethernet sub-system know so that if it is ready, it can
			//         use this Tx queue element to store a packet from Ethernet.
			wlan_eth_dma_update();

			// Update return value
			return_value = 1;
		} else {
			// Release the packet buffer because there is no Tx queue element to transmit
			((tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf))->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
		}
	}

	return return_value;
}

inline void queue_set_state_change_callback(function_ptr_t callback){
	queue_state_change_callback = callback;
}
