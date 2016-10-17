/** @file wlan_mac_eth_util.c
 *  @brief Ethernet Framework
 *
 *  Contains code for using Ethernet, including encapsulation and de-encapsulation.
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

#include "stdlib.h"
#include "xparameters.h"
#include "xaxiethernet.h"
#include "xaxidma.h"
#include "xintc.h"

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
#include "WARP_ip_udp.h"
#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP

#include "wlan_mac_common.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_station_info.h"


/*********************** Global Variable Definitions *************************/

// Performance monitoring using SW GPIO
//     - Each of these defines if 1 will enable performance monitoring using the
//       wlan_mac_set_dbg_hdr_out() / wlan_mac_clear_dbg_hdr_out() functions.
//     - All of the times are approximate because GPIO functions take time to
//       execute and might increase the time you are trying to measure.
//
#define PERF_MON_ETH_PROCESS_RX                            0
#define PERF_MON_ETH_PROCESS_ALL_RX                        0
#define PERF_MON_ETH_UPDATE_DMA                            0

// Performance monitoring BD usage
//     - Code enabled by the define will water-mark the BD usage of the Ethernet
//       driver.
//
#define PERF_MON_ETH_BD                                    0


/*************************** Variable Definitions ****************************/

// Instance structure for Ethernet DMA
static XAxiDma               eth_dma_instance;

// Callback for top-level processing of Ethernet received packets
static function_ptr_t        eth_rx_callback;

// Ethernet encapsulation mode
//     See:  http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/EthEncap
static u8                    eth_encap_mode;

// Number of TX / RX buffer descriptors
static u32                   num_rx_bd;
static u32                   num_tx_bd;

// Global representing the schedule ID and pointer
static u32					 rx_schedule_id;
static dl_entry* 			 rx_schedule_dl_entry;

// Ethernet Station MAC Address
//
//   The station code's implementation of encapsulation and de-encapsulation has an important
// limitation: only one device may be plugged into the station's Ethernet port. The station
// does not provide NAT. It assumes that the last received Ethernet src MAC address is used
// as the destination MAC address on any Ethernet transmissions. This is fine when there is
// only one device on the station's Ethernet port, but will definitely not work if the station
// is plugged into a switch with more than one device.
//
static u8                    eth_sta_mac_addr[6];


// Ethernet packet processing variables
//
//   In order to have a 1-to-1 interrupt assertion to ISR execution, when the Ethernet ISR
// runs, we have to disable the interrupt, acknowledge the interrupt and then collect all
// available packets to be processed.  Given that we cannot block the system for the time
// it takes to process all packets, we will schedule an event that will process the packets
// as quickly as possible using the scheduler.
//
static XAxiDma_Bd          * bd_set_to_process_ptr;
static int                   bd_set_count;
static int                   max_pkts;
static u32                   irq_status;


#if PERF_MON_ETH_BD
static u32                   bd_high_water_mark;
#endif


/*************************** Functions Prototypes ****************************/

int      wlan_eth_dma_init();
int      init_rx_bd(XAxiDma_Bd * bd_ptr, tx_queue_element_t * tqe_ptr, u32 max_transfer_len);
int      wlan_eth_dma_send(u8* pkt_ptr, u32 length);
int      wlan_eth_encap(u8* mpdu_start_ptr, u8* eth_dest, u8* eth_src, u8* eth_start_ptr, u32 eth_rx_len);
void     eth_rx_interrupt_handler(void *callbarck_arg);
void     wlan_process_all_eth_pkts(u32 schedule_id);
void     wlan_process_eth_rx(XAxiDma_BdRing * rx_ring_ptr, XAxiDma_Bd * bd_ptr);


#if PERF_MON_ETH_BD
void     print_bd_high_water_mark() { xil_printf("BD HWM = %d\n", bd_high_water_mark); }
#endif


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * @brief Initialize the Ethernet sub-system
 *
 * Initialize Ethernet A hardware and framework for handling Ethernet Tx/Rx via
 * the DMA.  This function must be called once at boot, before any Ethernet
 * configuration or Tx/Rx operations.
 *
 * @return 0 on success
 */
int wlan_eth_init() {
    int                      status;
    XAxiEthernet_Config    * eth_cfg_ptr;
    XAxiEthernet             eth_instance;

    // Set global variables
    bd_set_to_process_ptr  = NULL;
    bd_set_count           = 0;
    max_pkts               = 2;                       // Value in [1 .. XAXIDMA_ALL_BDS]

    rx_schedule_id = SCHEDULE_ID_RESERVED_MAX;
    rx_schedule_dl_entry = NULL;

#if PERF_MON_ETH_BD
    bd_high_water_mark     = 0;
#endif

    // Check to see if we were given enough room by wlan_mac_high.h for our buffer descriptors
    if (ETH_TX_BD_SIZE < XAXIDMA_BD_MINIMUM_ALIGNMENT) {
        xil_printf("Only %d bytes allocated for Eth Tx BD. Must be at least %d bytes\n", ETH_TX_BD_SIZE, XAXIDMA_BD_MINIMUM_ALIGNMENT);
        cpu_error_halt(WLAN_ERROR_CODE_INSUFFICIENT_BD_SIZE);
    }

    if (ETH_RX_BD_SIZE < XAXIDMA_BD_MINIMUM_ALIGNMENT) {
        xil_printf("Only %d bytes allocated for Eth Rx BDs. Must be at least %d bytes\n", ETH_RX_BD_SIZE, XAXIDMA_BD_MINIMUM_ALIGNMENT);
        cpu_error_halt(WLAN_ERROR_CODE_INSUFFICIENT_BD_SIZE);
    }

    // Initialize buffer descriptor counts
    num_tx_bd = 1;
    xil_printf("%3d Eth Tx BDs placed in BRAM: using %d B\n", num_tx_bd, num_tx_bd*XAXIDMA_BD_MINIMUM_ALIGNMENT);

    num_rx_bd = ETH_RX_BD_SIZE / XAXIDMA_BD_MINIMUM_ALIGNMENT;
    xil_printf("%3d Eth Rx BDs placed in BRAM: using %d kB\n", num_rx_bd, num_rx_bd*XAXIDMA_BD_MINIMUM_ALIGNMENT/1024);

    // Initialize Callback
    eth_rx_callback = (function_ptr_t)wlan_null_callback;

    // Initialize Ethernet instance
    eth_cfg_ptr = XAxiEthernet_LookupConfig(WLAN_ETH_DEV_ID);
    status = XAxiEthernet_CfgInitialize(&eth_instance, eth_cfg_ptr, eth_cfg_ptr->BaseAddress);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiEthernet_CfgInitialize! Err = %d\n", status); return -1; };

    // Setup the Ethernet options
    //     NOTE:  This Ethernet driver does not support jumbo Ethernet frames.  Only 2KB is allocated for each buffer
    //         descriptor and there is a basic assumption that 1 Ethernet frame = 1 buffer descriptor.
    status  = XAxiEthernet_ClearOptions(&eth_instance, XAE_LENTYPE_ERR_OPTION | XAE_FLOW_CONTROL_OPTION | XAE_JUMBO_OPTION);
    status |= XAxiEthernet_SetOptions(&eth_instance, XAE_FCS_STRIP_OPTION | XAE_PROMISC_OPTION | XAE_MULTICAST_OPTION | XAE_BROADCAST_OPTION | XAE_FCS_INSERT_OPTION);
    status |= XAxiEthernet_SetOptions(&eth_instance, XAE_RECEIVER_ENABLE_OPTION | XAE_TRANSMITTER_ENABLE_OPTION);
    if (status != XST_SUCCESS) {xil_printf("Error in XAxiEthernet_Set/ClearOptions! Err = %d\n", status); return -1;};

    // Set the operating speed
    XAxiEthernet_SetOperatingSpeed(&eth_instance, WLAN_ETH_LINK_SPEED);

    // If the link speed is 1 Gbps, then only advertise and link to 1 Gbps connection
    //     See Ethernet PHY specification for documentation on the values used
    if (WLAN_ETH_LINK_SPEED == 1000) {
        XAxiEthernet_PhyWrite(&eth_instance, WLAN_ETH_MDIO_PHYADDR, 0, 0x0140);
        XAxiEthernet_PhyWrite(&eth_instance, WLAN_ETH_MDIO_PHYADDR, 0, 0x8140);
    }

    // Initialize the DMA for Ethernet A
    status = wlan_eth_dma_init();

    // Start the Ethernet controller
    XAxiEthernet_Start(&eth_instance);

    return 0;
}



/*****************************************************************************/
/**
 * @brief Initializes the axi_dma core that handles Tx/Rx of Ethernet packets on ETH A
 *
 * Refer to the axi_dma docs and axi_ethernet driver examples for more details on using
 * the axi_dma's scatter-gather mode to handle Ethernet Tx/Rx.
 *
 * @return 0 on success, -1 otherwise
 */
int wlan_eth_dma_init() {
    int                 i;
    int                 status;
    int                 bd_count;
    u32                 max_transfer_len;

    XAxiDma_Config    * eth_dma_cfg_ptr;

    XAxiDma_Bd          eth_dma_bd_template;
    XAxiDma_BdRing    * eth_tx_ring_ptr;
    XAxiDma_BdRing    * eth_rx_ring_ptr;

    XAxiDma_Bd        * first_bd_ptr;
    XAxiDma_Bd        * cur_bd_ptr;

    tx_queue_element_t* curr_tx_queue_element;

    // Initialize the DMA peripheral structures
    eth_dma_cfg_ptr = XAxiDma_LookupConfig(WLAN_ETH_DMA_DEV_ID);
    status = XAxiDma_CfgInitialize(&eth_dma_instance, eth_dma_cfg_ptr);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma_CfgInitialize! Err = %d\n", status); return -1; }

    // Zero-out the template buffer descriptor
    XAxiDma_BdClear(&eth_dma_bd_template);

    // Fetch handles to the Tx and Rx BD rings
    eth_tx_ring_ptr = XAxiDma_GetTxRing(&eth_dma_instance);
    eth_rx_ring_ptr = XAxiDma_GetRxRing(&eth_dma_instance);

    // Disable all Tx/Rx DMA interrupts
    XAxiDma_BdRingIntDisable(eth_tx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);
    XAxiDma_BdRingIntDisable(eth_rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    // Disable delays and coalescing by default
    //      NOTE:  We observed no performance increase with interrupt coalescing
    XAxiDma_BdRingSetCoalesce(eth_tx_ring_ptr, 1, 0);
    XAxiDma_BdRingSetCoalesce(eth_rx_ring_ptr, 1, 0);

    // Setup Tx/Rx buffer descriptor rings in memory
    status  = XAxiDma_BdRingCreate(eth_tx_ring_ptr, ETH_TX_BD_BASE, ETH_TX_BD_BASE, XAXIDMA_BD_MINIMUM_ALIGNMENT, num_tx_bd);
    status |= XAxiDma_BdRingCreate(eth_rx_ring_ptr, ETH_RX_BD_BASE, ETH_RX_BD_BASE, XAXIDMA_BD_MINIMUM_ALIGNMENT, num_rx_bd);
    if (status != XST_SUCCESS) { xil_printf("Error creating DMA BD Rings! Err = %d\n", status); return -1; }

    // Populate each ring with empty buffer descriptors
    status  = XAxiDma_BdRingClone(eth_tx_ring_ptr, &eth_dma_bd_template);
    status |= XAxiDma_BdRingClone(eth_rx_ring_ptr, &eth_dma_bd_template);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma_BdRingClone()! Err = %d\n", status); return -1; }

    // Start the DMA Tx channel
    //     NOTE:  No Eth packets are transmitted until actual Tx BD's are pushed to the DMA hardware
    status = XAxiDma_BdRingStart(eth_tx_ring_ptr);

    // Initialize the Rx buffer descriptors
    bd_count = XAxiDma_BdRingGetFreeCnt(eth_rx_ring_ptr);
    if (bd_count != num_rx_bd) { xil_printf("Error in Eth Rx DMA init - not all Rx BDs were free at boot\n"); }

    status = XAxiDma_BdRingAlloc(eth_rx_ring_ptr, bd_count, &first_bd_ptr);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma_BdRingAlloc()! Err = %d\n", status); return -1; }

    // Iterate over each Rx buffer descriptor
    cur_bd_ptr       = first_bd_ptr;
    max_transfer_len = eth_rx_ring_ptr->MaxTransferLen;

    for (i = 0; i < bd_count; i++) {
        // Check out queue element for Rx buffer descriptor
        curr_tx_queue_element = queue_checkout();

        if (curr_tx_queue_element == NULL) {
            xil_printf("Error during wlan_eth_dma_init: unable to check out sufficient tx_queue_element\n");
            return -1;
        }

        // Initialize the Rx buffer descriptor
        status = init_rx_bd(cur_bd_ptr, curr_tx_queue_element, max_transfer_len);

        if (status != XST_SUCCESS) {
            xil_printf("Error initializing Rx BD %d\n", i);
            return -1;
        }

        // Update cur_bd_ptr to the next BD in the chain for the next iteration
        cur_bd_ptr = XAxiDma_BdRingNext(eth_rx_ring_ptr, cur_bd_ptr);
    }

    // Push the Rx BD ring to hardware and start receiving
    status = XAxiDma_BdRingToHw(eth_rx_ring_ptr, bd_count, first_bd_ptr);

    // Enable Interrupts
    XAxiDma_BdRingIntEnable(eth_rx_ring_ptr, XAXIDMA_IRQ_ALL_MASK);

    status |= XAxiDma_BdRingStart(eth_rx_ring_ptr);
    if (status != XST_SUCCESS) { xil_printf("Error in XAxiDma BdRingToHw/BdRingStart! Err = %d\n", status); return -1; }

    return 0;
}



/*****************************************************************************/
/**
 * @brief Initializes an Rx buffer descriptor to use the given Tx queue entry
 *
 * @param XAxiDma_Bd * bd_ptr
 *  - Pointer to buffer descriptor to be initialized
 * @param tx_queue_element_t * tqe_ptr
 *  - Pointer to Tx queue element
 * @param u32 max_transfer_len
 *  - Max transfer length for Rx BD
 *
 * @return 0 on success, -1 otherwise
 */
int init_rx_bd(XAxiDma_Bd * bd_ptr, tx_queue_element_t * tqe_ptr, u32 max_transfer_len) {
    int  status;
    u32  buf_addr;

    if ((bd_ptr == NULL) || (tqe_ptr == NULL)) { return -1; }

    // Set the memory address for this BD's buffer to the corresponding Tx queue entry buffer
    //     NOTE:  This pointer is offset by the size of a MAC header and LLC header, which results
    //         in the Ethernet payload being copied to its post-encapsulated location. This
    //         speeds up the encapsulation process by skipping any re-copying of Ethernet payloads
    buf_addr = (u32)((void*)((tx_queue_buffer_t*)(tqe_ptr->data))->frame + sizeof(mac_header_80211) + sizeof(llc_header_t) - sizeof(ethernet_header_t));
    status   = XAxiDma_BdSetBufAddr(bd_ptr, buf_addr);
    if (status != XST_SUCCESS) { xil_printf("XAxiDma_BdSetBufAddr failed (addr 0x08x)! Err = %d\n", buf_addr, status); return -1; }

    // Set every Rx BD to max length (this assures 1 BD per Rx pkt)
    //     NOTE:  Jumbo Ethernet frames are not supported by the Ethernet device (ie the XAE_JUMBO_OPTION is cleared),
    //         so the WLAN_ETH_PKT_BUF_SIZE must be at least large enough to support standard MTUs (ie greater than
    //         1522 bytes) so the assumption of 1 BD = 1 Rx pkt is met.
    status = XAxiDma_BdSetLength(bd_ptr, WLAN_ETH_PKT_BUF_SIZE, max_transfer_len);
    if (status != XST_SUCCESS) { xil_printf("XAxiDma_BdSetLength failed (addr 0x08x)! Err = %d\n", buf_addr, status); return -1; }

    // Rx BD's don't need control flags before use; DMA populates these post-Rx
    XAxiDma_BdSetCtrl(bd_ptr, 0);

    // BD ID is arbitrary; use pointer to the dl_entry associated with this BD
    XAxiDma_BdSetId(bd_ptr, (u32)tqe_ptr);

    return 0;
}



/*****************************************************************************/
/**
 * @brief Sets the MAC callback to process Ethernet receptions
 *
 * The framework will call the MAC's callback for each Ethernet reception that is a candidate
 * for wireless transmission. The framework may reject some packets for malformed or unrecognized
 * Ethernet headers. The MAC may reject more, based on Ethernet address or packet contents.
 *
 * @param void(*callback)
 *  -Function pointer to the MAC's Ethernet Rx callback
 */
void wlan_mac_util_set_eth_rx_callback(void(*callback)()) {
    eth_rx_callback = (function_ptr_t)callback;
}



/*****************************************************************************/
/**
 * @brief Sets the wired-wireless encapsulation mode
 *
 * @param u8 mode
 *  -Must be ENCAP_MODE_AP or ENCAP_MODE_STA, indicating AP-style or STA-style encapsulation/de-encapsulation
 */
void wlan_mac_util_set_eth_encap_mode(u8 mode) {
    eth_encap_mode = mode;
}



/*****************************************************************************/
/**
 * @brief Returns the total number of axi_dma Rx buffer descriptors
 */
inline int eth_get_num_rx_bd() {
    return num_rx_bd;
}



/*****************************************************************************/
/**
 * @brief Configures and connects the axi_dma interrupt to the system interrupt controller
 *
 * @param XIntc* intc
 *  - axi_intc driver instance - this function must be called after the axi_intc is setup
 *
 * @return 0 on success, non-zero otherwise
 */
int wlan_eth_setup_interrupt(XIntc* intc){
    int                 status;
    XAxiDma_BdRing    * rx_ring_ptr;

    // The interrupt controller will remember an arbitrary value and pass it to the callback
    // when this interrupt fires. We use this to pass the axi_dma Rx BD ring pointer into
    // the eth_rx_interrupt_handler() callback
    rx_ring_ptr = XAxiDma_GetRxRing(&eth_dma_instance);

    // Connect the axi_dma interrupt
    status = XIntc_Connect(intc, WLAN_ETH_RX_INTR_ID, (XInterruptHandler)eth_rx_interrupt_handler, rx_ring_ptr);

    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Failed to connect axi_dma interrupt: (%d)\n", status);
        return XST_FAILURE;
    }

    XIntc_Enable(intc, WLAN_ETH_RX_INTR_ID);

    return 0;
}



/*****************************************************************************/
/**
 * @brief Interrupt handler for ETH DMA receptions
 *
 * @param void* callbarck_arg
 *  - Argument passed in by interrupt controller (pointer to axi_dma Rx BD ring for Eth Rx)
 */
void eth_rx_interrupt_handler(void *callbarck_arg) {
    XAxiDma_BdRing    * rx_ring_ptr = (XAxiDma_BdRing *) callbarck_arg;

#ifdef _ISR_PERF_MON_EN_
    wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif

    irq_status = XAxiDma_BdRingGetIrq(rx_ring_ptr);

    if (!(irq_status & XAXIDMA_IRQ_ERROR_MASK)) {
        // At least one receptions is completed

        // Disable the interrupt and then acknowledge the interrupt
        XAxiDma_BdRingIntDisable(rx_ring_ptr, irq_status);
        XAxiDma_BdRingAckIrq(rx_ring_ptr, irq_status);

        // Get all the BDs that are available
        //     NOTE:  Use global variable so it is easy to process a sub-set of
        //         packets at a time.
        bd_set_count = XAxiDma_BdRingFromHw(rx_ring_ptr, XAXIDMA_ALL_BDS, &bd_set_to_process_ptr);

#if PERF_MON_ETH_BD
        // Update the BD high water mark
        if (bd_set_count > bd_high_water_mark) {
            bd_high_water_mark = bd_set_count;

            // Schedule a future event to print high water mark
            //     NOTE:  This is to minimize the impact of the print on what we are measuring.
            wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 0, 1, (void*)print_bd_high_water_mark);
        }
#endif

        // Process all Ethernet packets
        //     NOTE:  Interrupt will be re-enabled in this function when finished processing
        //         all Ethernet packets.
        wlan_process_all_eth_pkts(SCHEDULE_ID_RESERVED_MAX);

    } else {
        // Acknowledge the error interrupt
        XAxiDma_BdRingAckIrq(rx_ring_ptr, XAXIDMA_IRQ_ERROR_MASK);

        // !!! TODO:  Clean up from error condition !!!
    }

#ifdef _ISR_PERF_MON_EN_
    wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
    return;
}



/*****************************************************************************/
/**
 * @brief Process all Ethernet packets
 *
 * This function will be called by the ISR and the scheduler and will process
 * all the Ethernet packets and re-enable the interrupt if all packets are processed.
 *
 * This function tries to balance two goals:
 *   1) Don't stay in an ISR so long that the Tx PHY is starved by not processing
 *      TX_DONE messages waiting in the mailbox (i.e. only process "max_pkts" packets,
 *      then return)
 *
 *   2) Don't starve the Tx PHY by leaving it idle when taking too long to enqueue
 *      a ready-to-transmit packet received from Ethernet (i.e. process as many
 *      packets as it takes for the packet buffers to be full before returning)
 *
 * NOTE:  This function must be able to handle the case where bd_set_count = 0.
 */
void wlan_process_all_eth_pkts(u32 schedule_id) {
    u32                 num_pkt_processed   = 0;
    XAxiDma_BdRing    * rx_ring_ptr         = XAxiDma_GetRxRing(&eth_dma_instance);

#if PERF_MON_ETH_PROCESS_ALL_RX
    wlan_mac_set_dbg_hdr_out(0x2);
#endif

    if(schedule_id != SCHEDULE_ID_RESERVED_MAX){
    	// This function was called from the context of the scheduler. We should disable the schedule
    	rx_schedule_dl_entry = wlan_mac_schedule_disable_id(SCHEDULE_FINE, schedule_id);
    }

    while (bd_set_count > 0) {
        // Process Ethernet packet
        wlan_process_eth_rx(rx_ring_ptr, bd_set_to_process_ptr);

        // Update to the next BD in the chain for the next iteration
        bd_set_to_process_ptr = XAxiDma_BdRingNext(rx_ring_ptr, bd_set_to_process_ptr);

        // Increment counters
        num_pkt_processed++;
        bd_set_count--;

        // Check stop condition
        if ((num_pkt_processed >= max_pkts) && (wlan_mac_high_is_dequeue_allowed() == 0)) {
            // Processed enough packets in this call and the Tx PHY isn't waiting on an
            // Ethernet packet to transmit.  Leave this ISR to handle any other higher
            // priority interrupts, such as IPC messages, then come back later to process
            // the next set of Ethernet BDs.
            break;
        }
    }

    // Reassign any free DMA buffer descriptors to a new queue entry
    wlan_eth_dma_update();

    if (bd_set_count > 0) {

    	if(rx_schedule_dl_entry == NULL){
			// Set up scheduled event for processing next packets
			//     NOTE:  All global variables have been updated
			wlan_mac_schedule_event_repeated(SCHEDULE_FINE, 0, SCHEDULE_REPEAT_FOREVER, (void*)wlan_process_all_eth_pkts);
    	} else {
    		// We have already previously configured this schedule, so we should just enable it again
    		wlan_mac_schedule_enable(SCHEDULE_FINE, rx_schedule_dl_entry);
    	}

    } else {
        // Finished all available Eth Rx - re-enable interrupt
        XAxiDma_BdRingIntEnable(rx_ring_ptr, irq_status);

        // Set global variable to NULL (for safety)
        bd_set_to_process_ptr = NULL;
    }

#if PERF_MON_ETH_PROCESS_ALL_RX
    wlan_mac_clear_dbg_hdr_out(0x02);
#endif
}



/*****************************************************************************/
/**
 * @brief Process an Ethernet packet that has been received by the ETH DMA
 *
 * This function processes an Ethernet DMA buffer descriptor.  This design assumes
 * a 1-to-1 correspondence between buffer descriptors and Ethernet packets.  For
 * each packet, this function encapsulates the Ethernet packet and calls the
 * MAC's callback to either enqueue (for eventual wireless Tx) or reject the packet.
 *
 * NOTE:  Processed ETH DMA buffer descriptors are freed but not resubmitted to
 * hardware for use by future Ethernet receptions.  This is the responsibility of
 * higher level code (see comment at the end of the function if this behavior needs
 * to change).
 *
 * This function requires the MAC implement a function (assigned to the eth_rx_callback
 * function pointer) that returns 0 or 1, indicating the MAC's handling of the packet:
 *     0: Packet was not enqueued and will not be processed by wireless MAC; framework
 *        should immediately discard
 *     1: Packet was enqueued for eventual wireless transmission; MAC will check in
 *        occupied queue entry when finished
 *
 * @param XAxiDma_BdRing * rx_ring_ptr
 *  - Pointer to RX ring containing the buffer descriptor
 * @param XAxiDma_Bd * bd_ptr
 *  - Pointer to the buffer descriptor to process
 *
 */
void wlan_process_eth_rx(XAxiDma_BdRing * rx_ring_ptr, XAxiDma_Bd * bd_ptr) {
    u8                * mpdu_start_ptr;
    u8                * eth_start_ptr;
    tx_queue_element_t* curr_tx_queue_element;
    u32                 eth_rx_len, eth_rx_buf;
    u32                 mpdu_tx_len;

    int                 status;
    int                 packet_is_queued;

    u8                  eth_dest[6];
    u8                  eth_src[6];

#if PERF_MON_ETH_PROCESS_RX
    wlan_mac_set_dbg_hdr_out(0x4);
#endif

    // Check arguments
    if ((bd_ptr == NULL) || (rx_ring_ptr == NULL)) {
        xil_printf("ERROR:  Tried to process NULL Ethernet packet\n");
        return;
    }

    // Process Ethernet packet
    packet_is_queued = 0;

    // Get the TX queue entry pointer
    //     NOTE:  The DMA "ID" field is used to hold a pointer to the Tx queue entry containing the packet contents
    curr_tx_queue_element = (tx_queue_element_t*)XAxiDma_BdGetId(bd_ptr);

    // Lookup length and data pointers from the DMA metadata
    eth_rx_len     = XAxiDma_BdGetActualLength(bd_ptr, rx_ring_ptr->MaxTransferLen);
    eth_rx_buf     = XAxiDma_BdGetBufAddr(bd_ptr);

    // After encapsulation, byte[0] of the MPDU will be at byte[0] of the queue entry frame buffer
    mpdu_start_ptr = (void*)((tx_queue_buffer_t*)(curr_tx_queue_element->data))->frame;
    eth_start_ptr  = (u8*)eth_rx_buf;

    // Encapsulate the Ethernet packet
    mpdu_tx_len    = wlan_eth_encap(mpdu_start_ptr, eth_dest, eth_src, eth_start_ptr, eth_rx_len);

    if (mpdu_tx_len == 0) {
        // Encapsulation failed (Probably because of an unknown ETHERTYPE value)
        //     Don't pass the invalid frame to the MAC - just cleanup and return
        packet_is_queued = 0;
    } else {
        // Call the MAC's callback to process the packet
        //     MAC will either enqueue the packet for eventual transmission or reject the packet
        packet_is_queued = eth_rx_callback(curr_tx_queue_element, eth_dest, eth_src, mpdu_tx_len);
    }

    // If the packet was not successfully enqueued, discard it and return its queue entry to the free pool
    //     For packets that are successfully enqueued, this cleanup is part of the post-wireless-Tx handler
    if (packet_is_queued == 0) {
        // Either the packet was invalid, or the MAC code failed to enqueue this packet
        //     The MAC will fail if the appropriate queue was full or the Ethernet addresses were not recognized.

        // Return the occupied queue entry to the free pool
        queue_checkin(curr_tx_queue_element);
    }

    // Free the ETH DMA buffer descriptor
    status = XAxiDma_BdRingFree(rx_ring_ptr, 1, bd_ptr);
    if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingFree of Rx BD! Err = %d\n", status); return;}

    // Reassign the just-freed DMA buffer descriptor to a new queue entry
    //     NOTE:  Due to processing overhead, we do not necessarily want to call this function
    //         for every packet.  Therefore, we are commenting this out and leaving that for
    //         a higher level function.
    //
    // wlan_eth_dma_update();

#if PERF_MON_ETH_PROCESS_RX
    wlan_mac_clear_dbg_hdr_out(0x4);
#endif
}



/*****************************************************************************/
/**
 * @brief Encapsulates Ethernet packets for wireless transmission
 *
 * This function implements the encapsulation process for 802.11 transmission of
 * Ethernet packets
 *
 * The encapsulation process depends on the node's role:
 *
 * AP:
 *     - Copy original packet's source and destination addresses to temporary space
 *     - Add an LLC header (8 bytes) in front of the Ethernet payload
 *         - LLC header includes original packet's ETHER_TYPE field; only IPV4 and ARP
 *           are currently supported
 *
 * STA:
 *    - Copy original packet's source and destination addresses to temporary space
 *    - Add an LLC header (8 bytes) in front of the Ethernet payload
 *        - LLC header includes original packet's ETHER_TYPE field; only IPV4 and ARP
 *          are currently supported
 *    - If packet is ARP Request, overwrite ARP header's source address with STA
 *      wireless MAC address
 *    - If packet is UDP packet containing a DHCP request
 *        - Assert DHCP header's BROADCAST flag
 *        - Disable the UDP packet checksum (otherwise it would be invliad after
 *          modifying the BROADCAST flag)
 *
 * Refer to the 802.11 Reference Design user guide for more details:
 *     http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/EthEncap
 *
 * @param u8* mpdu_start_ptr
 *  - Pointer to the first byte of the MPDU payload (first byte of the eventual MAC
 *    header)
 * @param u8* eth_dest
 *  - Pointer to 6 bytes of free memory; will be overwritten with Ethernet packet's
 *    destination address
 * @param u8* eth_src
 *  - Pointer to 6 bytes of free memory; will be overwritten with Ethernet packet's
 *    source address
 * @param u8* eth_start_ptr
 *  - Pointer to first byte of received Ethernet packet's header
 * @param u32 eth_rx_len
 *  - Length (in bytes) of the packet payload
 *
 * @return 0 for if packet type is unrecognized (failed encapsulation), otherwise
 *     returns length of encapsulated packet (in bytes)
 */
int wlan_eth_encap(u8* mpdu_start_ptr, u8* eth_dest, u8* eth_src, u8* eth_start_ptr, u32 eth_rx_len) {
	ethernet_header_t        * eth_hdr;
    ipv4_header_t            * ip_hdr;
    arp_ipv4_packet_t        * arp;
    udp_header_t             * udp;
    dhcp_packet            * dhcp;
    llc_header_t           * llc_hdr;
    u32                      mpdu_tx_len;

    // Calculate actual wireless Tx len (eth payload - eth header + wireless header)
    mpdu_tx_len = eth_rx_len - sizeof(ethernet_header_t) + sizeof(llc_header_t) + sizeof(mac_header_80211) + WLAN_PHY_FCS_NBYTES;

    // Helper pointers to interpret/fill fields in the new MPDU
    eth_hdr = (ethernet_header_t*)eth_start_ptr;
    llc_hdr = (llc_header_t*)(mpdu_start_ptr + sizeof(mac_header_80211));

    // Copy the src/dest addresses from the received Eth packet to temp space
    memcpy(eth_src, eth_hdr->src_mac_addr, 6);
    memcpy(eth_dest, eth_hdr->dest_mac_addr, 6);

    // Prepare the MPDU LLC header
    llc_hdr->dsap          = LLC_SNAP;
    llc_hdr->ssap          = LLC_SNAP;
    llc_hdr->control_field = LLC_CNTRL_UNNUMBERED;
    bzero((void *)(llc_hdr->org_code), 3); //Org Code 0x000000: Encapsulated Ethernet

    switch(eth_encap_mode) {
        // ------------------------------------------------
        case ENCAP_MODE_AP:
            switch(eth_hdr->ethertype) {
                case ETH_TYPE_ARP:
                    llc_hdr->type = LLC_TYPE_ARP;
                    arp = (arp_ipv4_packet_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                break;

                case ETH_TYPE_IP:
                    llc_hdr->type = LLC_TYPE_IP;
                break;

                default:
                    // Unknown/unsupported EtherType; don't process the Eth frame
                    return 0;
                break;
            }
        break;

        // ------------------------------------------------
        case ENCAP_MODE_IBSS:
        case ENCAP_MODE_STA:
            // Save this ethernet src address
            memcpy(eth_sta_mac_addr, eth_src, 6);
            memcpy(eth_src, get_mac_hw_addr_wlan(), 6);

            switch(eth_hdr->ethertype) {
                case ETH_TYPE_ARP:
                    llc_hdr->type = LLC_TYPE_ARP;

                    // Overwrite ARP request source MAC address field with the station's wireless MAC address.
                    arp = (arp_ipv4_packet_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                    memcpy(arp->sender_haddr, get_mac_hw_addr_wlan(), 6);
                break;

                case ETH_TYPE_IP:
                    llc_hdr->type = LLC_TYPE_IP;

                    // Check if IPv4 packet is a DHCP Discover in a UDP frame
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));

                    if (ip_hdr->protocol == IPV4_PROT_UDP) {
                        udp = (udp_header_t*)((void*)ip_hdr + 4*((u8)(ip_hdr->version_ihl) & 0xF));

                        // Check if this is a DHCP Discover packet, which contains the source hardware
                        // address deep inside the packet (in addition to its usual location in the Eth
                        // header). For STA encapsulation, we need to overwrite this address with the
                        // MAC addr of the wireless station.
                        if ((Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPC) ||
                            (Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPS)) {

                            // Disable the checksum since this will change the bytes in the packet
                            udp->checksum = 0;

                            dhcp = (dhcp_packet*)((void*)udp + sizeof(udp_header_t));

                            if (Xil_Ntohl(dhcp->magic_cookie) == DHCP_MAGIC_COOKIE) {
                                // Assert the DHCP Discover's BROADCAST flag; this signals to any DHCP
                                // severs that their responses should be sent to the broadcast address.
                                // This is necessary for the DHCP response to propagate back through the
                                // wired-wireless portal at the AP, through the STA Rx MAC filters, back
                                // out the wireless-wired portal at the STA, and finally into the DHCP
                                // listener at the wired device
                                dhcp->flags = Xil_Htons(DHCP_BOOTP_FLAGS_BROADCAST);

                            } // END is DHCP valid
                        } // END is DHCP
                    } // END is UDP
                break;

                default:
                    // Unknown/unsupported EtherType; don't process the Eth frame
                    return 0;
                break;
            } //END switch(pkt type)
        break;
    } // END switch(encap mode)

    // If we got this far, the packet was successfully encapsulated; return the post-encapsulation length
    return mpdu_tx_len;
}



/*****************************************************************************/
/**
 * @brief Updates any free ETH Rx DMA buffer descriptors
 *
 * This function checks if any ETH DMA Rx buffer descriptors have been freed and
 * are ready to be reused.  For each free BD, this function attempts to checkout
 * a Tx queue entry and assign its payload to the BD. If successful, the BD is
 * then submitted to the DMA hardware for use by future Ethernet receptions. If
 * unsuccessful the BD is left free, to be recycled on the next iteration of this
 * function.
 *
 * The total number of ETH Rx buffer descriptors is set at boot during the DMA init.
 * The same number of Tx queue entries are effectively reserved by the MAC in its
 * queue size calculations. This function can handle the case of more ETH Rx BDs
 * than free Tx queue entries, though this should never happen.
 *
 * This function should be called under the following conditions:
 *   - A Rx buffer descriptor is finished being processed
 *   - A Tx queue entry has finished being processed
 * This will assure that enough Rx BDs are available to the DMA hardware.
 */
void wlan_eth_dma_update() {
    int                 status;
    int                 iter;
    int                 bd_count;
    u32                 bd_queue_pairs_to_process;
    u32                 bd_queue_pairs_processed;
    u32                 max_transfer_len;

    XAxiDma_BdRing    * rx_ring_ptr;
    XAxiDma_Bd        * first_bd_ptr;
    XAxiDma_Bd        * cur_bd_ptr;

    dl_list             checkout;
    dl_entry*           tx_queue_entry;

#if PERF_MON_ETH_UPDATE_DMA
    wlan_mac_set_dbg_hdr_out(0x2);
#endif

    // Get the Rx ring pointer and the number of free Rx buffer descriptors
    rx_ring_ptr = XAxiDma_GetRxRing(&eth_dma_instance);
    bd_count    = XAxiDma_BdRingGetFreeCnt(rx_ring_ptr);

    // If there are no BDs, then we are done
    if (bd_count == 0) {

#if PERF_MON_ETH_UPDATE_DMA
        wlan_mac_set_dbg_hdr_out(0x2);
#endif

        return;
    }

    // Initialize the list to checkout Tx queue entries
    dl_list_init(&checkout);

    // Attempt to checkout Tx queue entries for all free buffer descriptors
    queue_checkout_list(&checkout, bd_count);

    // If there were not enough Tx queue entries available, the length of the
    // checkout list will be the number of free Tx queue entries that were
    // found.  Only process buffer descriptors that have a corresponding Tx
    // queue entry.
    bd_queue_pairs_to_process = min(bd_count, checkout.length);

    if (bd_queue_pairs_to_process > 0) {

        // Initialize the number of buffer descriptors processed
        bd_queue_pairs_processed = 0;

        // Allocate the correct number of Rx buffer descriptors that have
        // been freed and have Tx queue entries available.
        status = XAxiDma_BdRingAlloc(rx_ring_ptr, bd_queue_pairs_to_process, &first_bd_ptr);
        if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingAlloc()! Err = %d\n", status); return;}

        // Initialize loop variables
        iter             = checkout.length;
        tx_queue_entry   = checkout.first;
        cur_bd_ptr       = first_bd_ptr;
        max_transfer_len = rx_ring_ptr->MaxTransferLen;

        while ((tx_queue_entry != NULL) && (iter-- > 0)) {

#if PERF_MON_ETH_UPDATE_DMA
            wlan_mac_set_dbg_hdr_out(0x4);
#endif

            // Initialize the Rx buffer descriptor
            status = init_rx_bd(cur_bd_ptr, tx_queue_entry, max_transfer_len);

            if (status != XST_SUCCESS) {
                // Clean up buffer descriptors and Tx queues
                //     NOTE:  Regardless of where we are in the update, we will check
                //         back in everything so the function can start fresh next invocation
                status = XAxiDma_BdRingUnAlloc(rx_ring_ptr, bd_queue_pairs_to_process, first_bd_ptr);
                if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingUnAlloc()! Err = %d\n", status);}

                queue_checkin_list(&checkout);

                return;
            }

            // Update the BD and queue entry pointers to the next list elements
            //     NOTE: This loop traverses both lists simultaneously
            cur_bd_ptr     = XAxiDma_BdRingNext(rx_ring_ptr, cur_bd_ptr);
            tx_queue_entry = dl_entry_next(tx_queue_entry);
            bd_queue_pairs_processed++;

#if PERF_MON_ETH_UPDATE_DMA
            wlan_mac_clear_dbg_hdr_out(0x4);
#endif
        }

        if (bd_queue_pairs_processed == bd_queue_pairs_to_process) {
            // Push the Rx BD ring to hardware and start receiving
            status = XAxiDma_BdRingToHw(rx_ring_ptr, bd_queue_pairs_to_process, first_bd_ptr);
            if(status != XST_SUCCESS) {xil_printf("XAxiDma_BdRingToHw failed! Err = %d\n", status);}
        } else {
            // Clean up buffer descriptors and Tx queues
            xil_printf("Error processing BD-queue pairs\n");

            status = XAxiDma_BdRingUnAlloc(rx_ring_ptr, bd_queue_pairs_to_process, first_bd_ptr);
            if(status != XST_SUCCESS) {xil_printf("Error in XAxiDma_BdRingUnAlloc()! Err = %d\n", status);}

            queue_checkin_list(&checkout);
        }
    }

#if PERF_MON_ETH_UPDATE_DMA
    wlan_mac_clear_dbg_hdr_out(0x2);
#endif
}



/*****************************************************************************/
/**
 * @brief De-encapsulates a wireless reception and prepares it for transmission
 *     via Ethernet
 *
 * This function implements the de-encapsulation process for the wireless-wired
 * portal. See the 802.11 Reference Design user guide for more details:
 *     http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/EthEncap
 *
 * In addition to de-encapsulation this function implements one extra behavior. When
 * the AP observes a DHCP request about to be transmitted via Ethernet, it inspects
 * the DHCP payload to extract the hostname field. When the source packet was a
 * wireless transmission from an associated STA, this hostname field reflects the
 * user-specified name of the device which generated the DHCP request. For most
 * devices this name is human readable (like "jacks-phone"). The AP copies this
 * hostname to the station_info.hostname field in the STA's entry in the association
 * table.
 *
 * This functionality is purely for user convenience (i.e. the AP only displays the
 * hostname, it never makes state decisions based on its value. The hostname can be
 * bogus or missing without affecting the behavior of the AP.
 *
 * @param u8* mpdu
 *  - Pointer to the first byte of the packet received from the wireless interface
 * @param u32 length
 *  - Length (in bytes) of the packet to send
 *
 * @return 0 for successful de-encapsulation, -1 otherwise; failure usually indicates
 *     malformed or unrecognized LLC header
*/
int wlan_mpdu_eth_send(void* mpdu, u16 length, u8 pre_llc_offset) {
    int                      status;
    u8                     * eth_mid_ptr;

    rx_frame_info_t        * rx_frame_info;

    mac_header_80211       * rx80211_hdr;
    llc_header_t           * llc_hdr;

    ethernet_header_t        * eth_hdr;
    ipv4_header_t            * ip_hdr;
    udp_header_t             * udp;

    arp_ipv4_packet_t        * arp;
    dhcp_packet            * dhcp;

    u8                       continue_loop;
    u8                       is_dhcp_req         = 0;

    u8                       addr_cache[6];
    u32                      len_to_send;

    u32                      min_pkt_len = sizeof(mac_header_80211) + sizeof(llc_header_t);

    if(length < (min_pkt_len + pre_llc_offset + WLAN_PHY_FCS_NBYTES)){
        xil_printf("Error in wlan_mpdu_eth_send: length of %d is too small... must be at least %d\n", length, min_pkt_len);
        return -1;
    }

    // Get helper pointers to various byte offsets in the packet payload
    rx80211_hdr = (mac_header_80211*)((void *)mpdu);
    llc_hdr     = (llc_header_t*)((void *)mpdu + sizeof(mac_header_80211) + pre_llc_offset);
    eth_hdr     = (ethernet_header_t*)((void *)mpdu + sizeof(mac_header_80211) + sizeof(llc_header_t) + pre_llc_offset - sizeof(ethernet_header_t));

    // Calculate length of de-encapsulated Ethernet packet
    len_to_send = length - min_pkt_len - pre_llc_offset - WLAN_PHY_FCS_NBYTES + sizeof(ethernet_header_t);

    // Perform de-encapsulation of wireless packet
    switch(eth_encap_mode) {
        // ------------------------------------------------
        case ENCAP_MODE_AP:
            // Map the 802.11 header address fields to Ethernet header address fields
            //     For AP de-encapsulation, (eth.dest == wlan.addr3) and (eth.src == wlan.addr2)
            memmove(eth_hdr->dest_mac_addr, rx80211_hdr->address_3, 6);
            memmove(eth_hdr->src_mac_addr,  rx80211_hdr->address_2, 6);

            // Set the ETHER_TYPE field in the Ethernet header
            switch(llc_hdr->type){
                case LLC_TYPE_ARP:
                    eth_hdr->ethertype = ETH_TYPE_ARP;
                break;

                case LLC_TYPE_IP:
                    eth_hdr->ethertype = ETH_TYPE_IP;

                    // Check if this is a DHCP discover packet in a UDP packet
                    //     If so, extract the hostname field from the DHCP discover payload and
                    //         update the corresponding STA association table entry
                    //     This hostname is purely for convenience- the hostname is easier to
                    //          recognize than the STA MAC address. The hostname can be blank
                    //          without affecting any AP functionality.
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));

                    if (ip_hdr->protocol == IPV4_PROT_UDP) {
                        udp = (udp_header_t*)((void*)ip_hdr + 4*((u8)(ip_hdr->version_ihl) & 0xF));

                        if ((Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPC) ||
                            (Xil_Ntohs(udp->src_port) == UDP_SRC_PORT_BOOTPS)) {
                            dhcp = (dhcp_packet*)((void*)udp + sizeof(udp_header_t));

                            if (Xil_Ntohl(dhcp->magic_cookie) == DHCP_MAGIC_COOKIE) {
                                eth_mid_ptr = (u8*)((void*)dhcp + sizeof(dhcp_packet));
\
                                // Iterate over all tagged parameters in the DHCP request, looking for the hostname parameter
                                //     NOTE:  Stop after 20 tagged parameters (handles case of mal-formed DHCP packets missing END tag)
                                continue_loop = 20;

                                while(continue_loop) {
                                    continue_loop--;
                                    switch(eth_mid_ptr[0]) {

                                        case DHCP_OPTION_TAG_TYPE:
                                            if((eth_mid_ptr[2] == DHCP_OPTION_TYPE_DISCOVER) ||
                                               (eth_mid_ptr[2] == DHCP_OPTION_TYPE_REQUEST)) {
                                                    is_dhcp_req = 1;
                                            }
                                        break;

                                        case DHCP_HOST_NAME:
                                            if (is_dhcp_req) {
                                                // Look backwards from the MPDU payload to find the wireless Rx pkt metadata (the rx_frame_info struct)
                                                rx_frame_info = (rx_frame_info_t*)((u8*)mpdu  - PHY_RX_PKT_BUF_MPDU_OFFSET);

                                                if (rx_frame_info->additional_info != NULL) {
                                                    // rx_frame_info has pointer to STA entry in association table - fill in that entry's hostname field

                                                    // Zero out the hostname field of the station_info
                                                    //     NOTE: This will effectively Null-terminate the string
                                                    bzero(((station_info_t*)(rx_frame_info->additional_info))->hostname, STATION_INFO_HOSTNAME_MAXLEN+1);

                                                    // Copy the string from the DHCP payload into the hostname field
                                                    memcpy(((station_info_t*)(rx_frame_info->additional_info))->hostname,
                                                            &(eth_mid_ptr[2]),
                                                            min(STATION_INFO_HOSTNAME_MAXLEN, eth_mid_ptr[1]));
                                                }
                                            }
                                        break;

                                        case DHCP_OPTION_END:
                                            continue_loop = 0;
                                        break;
                                    } // END switch(DHCP tag type)

                                    // Increment by size of current tagged parameter
                                    eth_mid_ptr += (2+eth_mid_ptr[1]);

                                } // END iterate over DHCP tags
                            }  // END is DHCP_MAGIC_COOKIE correct?
                        } // END is DHCP?
                    } // END is UDP?
                break; // END case(IP packet)

                default:
                    // Invalid or unsupported Eth type; give up and return
                    return -1;
                break;
            } // END switch(LLC type)
        break;

        // ------------------------------------------------
        case ENCAP_MODE_STA:
            if (wlan_addr_eq(rx80211_hdr->address_3, get_mac_hw_addr_wlan())) {
                // This case handles the behavior of an AP reflecting a station-sent broadcast
                // packet back out over the air.  Without this filtering, a station would forward
                // the packet it just transmitted back to its wired interface.  This messes up
                // DHCP and ARP behavior on the connected PC.
                return -1;
            }

            // Make temp copy of the 802.11 header address 3 field
            memcpy(addr_cache, rx80211_hdr->address_3, 6);

            // If this packet is addressed to this STA, use the wired device's MAC address as the Eth dest address
            if (wlan_addr_eq(rx80211_hdr->address_1, get_mac_hw_addr_wlan())) {
                memcpy(eth_hdr->dest_mac_addr, eth_sta_mac_addr, 6);
            } else {
                memmove(eth_hdr->dest_mac_addr, rx80211_hdr->address_1, 6);
            }

            // Insert the Eth source, from the 802.11 header address 1 field
            memcpy(eth_hdr->src_mac_addr, addr_cache, 6);

            switch(llc_hdr->type){
                case LLC_TYPE_ARP:
                    eth_hdr->ethertype = ETH_TYPE_ARP;

                    // If the ARP packet is addressed to this STA wireless address, replace the ARP dest address
                    // with the connected wired device's MAC address
                    arp = (arp_ipv4_packet_t *)((void*)eth_hdr + sizeof(ethernet_header_t));
                    if (wlan_addr_eq(arp->target_haddr, get_mac_hw_addr_wlan())) {
                        memcpy(arp->target_haddr, eth_sta_mac_addr, 6);
                    }
                break;

                case ETH_TYPE_IP:
                    eth_hdr->ethertype = ETH_TYPE_IP;
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                break;

                default:
                    // Invalid or unsupported Eth type; give up and return
                    return -1;
                break;
            }
        break;

        // ------------------------------------------------
        case ENCAP_MODE_IBSS:
            // Make temp copy of the 802.11 header address 2 field
            memcpy(addr_cache, rx80211_hdr->address_2, 6);

            // If this packet is addressed to this STA, use the wired device's MAC address as the Eth dest address
            if(wlan_addr_eq(rx80211_hdr->address_1, get_mac_hw_addr_wlan())) {
                memcpy(eth_hdr->dest_mac_addr, eth_sta_mac_addr, 6);
            } else {
                memmove(eth_hdr->dest_mac_addr, rx80211_hdr->address_1, 6);
            }

            // Insert the Eth source, from the 802.11 header address 2 field
            memcpy(eth_hdr->src_mac_addr, addr_cache, 6);

            switch(llc_hdr->type){
                case LLC_TYPE_ARP:
                    eth_hdr->ethertype = ETH_TYPE_ARP;

                    // If the ARP packet is addressed to this STA wireless address, replace the ARP dest address
                    // with the connected wired device's MAC address
                    arp = (arp_ipv4_packet_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                    if (wlan_addr_eq(arp->target_haddr, get_mac_hw_addr_wlan())) {
                        memcpy(arp->target_haddr, eth_sta_mac_addr, 6);
                    }
                break;

                case ETH_TYPE_IP:
                    eth_hdr->ethertype = ETH_TYPE_IP;
                    ip_hdr = (ipv4_header_t*)((void*)eth_hdr + sizeof(ethernet_header_t));
                break;

                default:
                    // Invalid or unsupported Eth type; give up and return
                    return -1;
                break;
            }
        break;
    }

#ifdef FMC_PKT_EN
    status = wlan_fmc_pkt_eth_send((u8*)eth_hdr, len_to_send);
    if (status != 0) { xil_printf("Error in wlan_fmc_pkt_eth_send! Err = %d\n", status); return -1; }
#else
    status = wlan_eth_dma_send((u8*)eth_hdr, len_to_send);
    if (status != 0) { xil_printf("Error in wlan_mac_send_eth! Err = %d\n", status); return -1; }
#endif

    return 0;
}



/*****************************************************************************/
/**
 * @brief Transmits a packet over Ethernet using the ETH DMA
 *
 * This function transmits a single packet via Ethernet using the axi_dma. The
 * packet passed via pkt_ptr must be a valid Ethernet packet, complete with
 * 14-byte Ethernet header. This function does not check for a valid header
 * (i.e. the calling function must ensure this).
 *
 * The packet must be stored in a memory location accessible by the axi_dma core.
 * The MicroBlaze DLMB is *not* accessible to the DMA. Thus packets cannot be
 * stored in malloc'd areas (the heap is in the DLMB).  In the reference implementation
 * all Ethernet transmissions start as wireless receptions. Thus, the Ethernet payloads
 * are stored in the wireless Rx packet buffer, which is accessible by the DMA.
 *
 * Custom code which needs to send Ethernet packets can use a spare wireless Tx/Rx
 * packet buffer, a spare Tx queue entry in DRAM or the user scratch space in DRAM
 * to create custom Ethernet payloads.
 *
 * This function blocks until the Ethernet transmission completes.
 *
 * @param u8* pkt_ptr
 *  - Pointer to the first byte of the Ethernet header of the packet to send; valid
 *    header must be created before calling this function
 * @param u32 length
 *  - Length (in bytes) of the packet to send
 *
 * @return 0 for successful Ethernet transmission, -1 otherwise
 */
int wlan_eth_dma_send(u8* pkt_ptr, u32 length) {
    int                 status;
    XAxiDma_BdRing    * tx_ring_ptr;
    XAxiDma_Bd        * cur_bd_ptr = NULL;

    if ((length == 0) || (length > 1518)) {
        xil_printf("ERROR: wlan_eth_dma_send length = %d\n", length);
        return -1;
    }

    // IMPORTANT:  If the data cache is enabled the cache must be flushed before
    // attempting to send a packet via the ETH DMA. The DMA will read packet
    // contents directly from RAM, bypassing any cache checking normally done by
    // the MicroBlaze.  The data cache is disabled by default in the reference
    // implementation.
    //
    // Xil_DCacheFlushRange((u32)TxPacket, MAX_PKT_LEN);

    // Check if the user-supplied pointer is in the DLMB, unreachable by the DMA
    if (((u32)pkt_ptr > XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_BASEADDR && (u32)pkt_ptr < XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_HIGHADDR) ||
        ((u32)pkt_ptr > XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_BASEADDR && (u32)pkt_ptr < XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_HIGHADDR)) {
        xil_printf("ERROR: Eth DMA send -- source address (0x%08x) not reachable by DMA\n", pkt_ptr);
        return -1;
    }

    // Get pointer to the axi_dma Tx buffer descriptor ring
    tx_ring_ptr = XAxiDma_GetTxRing(&eth_dma_instance);

    // Allocate and setup one Tx BD
    status  = XAxiDma_BdRingAlloc(tx_ring_ptr, 1, &cur_bd_ptr);
    status |= XAxiDma_BdSetBufAddr(cur_bd_ptr, (u32)pkt_ptr);
    status |= XAxiDma_BdSetLength(cur_bd_ptr, length, tx_ring_ptr->MaxTransferLen);

    if(status != XST_SUCCESS) {
        xil_printf("length = %d, max_transfer_len = %d\n", length, tx_ring_ptr->MaxTransferLen );
        xil_printf("ERROR setting ETH TX BD! Err = %d\n", status);
        status = XAxiDma_BdRingFree(tx_ring_ptr, 1, cur_bd_ptr);
        return -1;
    }

    // When using 1 BD for 1 pkt set both start-of-frame (SOF) and end-of-frame (EOF)
    XAxiDma_BdSetCtrl(cur_bd_ptr, (XAXIDMA_BD_CTRL_TXSOF_MASK | XAXIDMA_BD_CTRL_TXEOF_MASK));

    // Set arbitrary ID (DMA ignores this)
    XAxiDma_BdSetId(cur_bd_ptr, (u32)pkt_ptr);

    // Push the BD ring to hardware; this initiates the actual DMA transfer and Ethernet Tx
    status = XAxiDma_BdRingToHw(tx_ring_ptr, 1, cur_bd_ptr);
    if(status != XST_SUCCESS){
        xil_printf("ERROR: TX XAxiDma_BdRingToHw! Err = %d\n", status);
        status = XAxiDma_BdRingFree(tx_ring_ptr, 1, cur_bd_ptr);
        return -1;
    }

    // Wait for this DMA transfer to finish
    //     !!! TODO: replace with post-Tx ISR !!!
    while (XAxiDma_BdRingFromHw(tx_ring_ptr, 1, &cur_bd_ptr) == 0) { /*Do Nothing*/ }

    // Free the BD for future use
    status = XAxiDma_BdRingFree(tx_ring_ptr, 1, cur_bd_ptr);
    if(status != XST_SUCCESS) {xil_printf("ERROR: TX XAxiDma_BdRingFree! Err = %d\n", status); return -1;}

    return 0;
}



