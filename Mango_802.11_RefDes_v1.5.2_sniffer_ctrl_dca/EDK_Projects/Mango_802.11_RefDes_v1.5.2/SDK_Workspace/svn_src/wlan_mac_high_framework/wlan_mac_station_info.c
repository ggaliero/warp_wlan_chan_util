/** @file wlan_mac_station_info.c
 *  @brief Station Information Metadata Subsystem
 *
 *  This contains code tracking metadata about stations.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

#include "xil_types.h"
#include "stdlib.h"
#include "stdio.h"
#include "xparameters.h"
#include "string.h"

#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_addr_filter.h"

/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

static dl_list               station_info_free;              ///< Free Counts

/// The counts_txrx_list is stored chronologically from .first being oldest
/// and .last being newest. The "find" function search from last to first
/// to minimize search time for new BSSes you hear from often.
static dl_list               station_info_list;              ///< Filled Counts


/*************************** Functions Prototypes ****************************/

dl_entry* station_info_find_oldest();


/******************************** Functions **********************************/

void station_info_init(u8 dram_present){

	u32       i;
	u32       num_station_info;
	dl_entry* dl_entry_base;

	dl_list_init(&station_info_free);
	dl_list_init(&station_info_list);

	if(dram_present){
		// Clear the memory in the dram used for bss_infos
		bzero((void*)STATION_INFO_BUFFER_BASE, STATION_INFO_BUFFER_SIZE);

		// The number of elements we can initialize is limited by the smaller of two values:
		//     (1) The number of dl_entry structs we can squeeze into STATION_INFO_DL_ENTRY_MEM_SIZE
		//     (2) The number of station_info_t structs we can squeeze into STATION_INFO_BUFFER_SIZE
		num_station_info = min(STATION_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry), STATION_INFO_BUFFER_SIZE/sizeof(station_info_t));

		// At boot, every dl_entry buffer descriptor is free
		// To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
		// This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
		dl_entry_base = (dl_entry*)(STATION_INFO_DL_ENTRY_MEM_BASE);

		for (i = 0; i < num_station_info; i++) {
			dl_entry_base[i].data = (void*)(STATION_INFO_BUFFER_BASE + (i*sizeof(station_info_t)));
			dl_entry_insertEnd(&station_info_free, &(dl_entry_base[i]));
		}

		xil_printf("Station Info list (len %d) placed in DRAM: using %d kB\n", num_station_info, (num_station_info*sizeof(station_info_t))/1024);

	} else {
		xil_printf("Error initializing Station Info subsystem\n");
	}
}



void station_info_init_finish(){
	//Will be called after interrupts have been started. Safe to use scheduler now.
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 10000000, SCHEDULE_REPEAT_FOREVER, (void*)station_info_timestamp_check);
}


inline station_info_t* station_info_tx_process(void* pkt_buf_addr) {
	tx_frame_info_t*      	tx_frame_info  	  = (tx_frame_info_t*)pkt_buf_addr;
	mac_header_80211* 		tx_80211_header   = (mac_header_80211*)((u8*)tx_frame_info + PHY_TX_PKT_BUF_MPDU_OFFSET);
	dl_entry*				curr_dl_entry;
	station_info_t*			curr_station_info;
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
	station_txrx_counts_t*	curr_txrx_counts;
	txrx_counts_sub_t*		txrx_counts_sub;
	u8						pkt_type;


	pkt_type = (tx_80211_header->frame_control_1 & MAC_FRAME_CTRL1_MASK_TYPE);
#endif

	curr_dl_entry = station_info_find_by_addr(tx_80211_header->address_1, NULL);

	if(curr_dl_entry != NULL){
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		// Remove entry from station_info_list; Will be added back at the bottom of the function
		// This serves to sort the list and keep the most recently updated entries at the tail.
		dl_entry_remove(&station_info_list, curr_dl_entry);
	} else {
		// We haven't seen this addr before, so we'll attempt to checkout a new dl_entry
		// struct from the free pool
		curr_dl_entry = station_info_checkout();

		if (curr_dl_entry == NULL){
			// No free dl_entry!
			// We'll have to reallocate the oldest entry in the filled list
			curr_dl_entry = station_info_find_oldest();

			if (curr_dl_entry != NULL) {
				dl_entry_remove(&station_info_list, curr_dl_entry);
			} else {
				xil_printf("Cannot create station_info_t.\n");
				return NULL;
			}
		}

		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		// Clear any old information from the Tx/Rx counts
		station_info_clear(curr_station_info);

		// Copy addr into station_info_t struct
		memcpy(curr_station_info->addr, tx_80211_header->address_1, MAC_ADDR_LEN);
	}

	// By this point in the function, curr_station_info is guaranteed to be pointing to a valid station_info_t struct
	// that we should update with this reception.

	// Update the latest TXRX time
	curr_station_info->latest_txrx_timestamp = get_system_time_usec();

	// If this transmission was a successful transmission, we implicitly know than an
	// ACK was received. So, we can update the latest Rx-only timestamp as well.
	// Note: The success <-> ACK Rx assumption holds here because this code
	// cannot be executed on the transmission of a multicast frame.
	// TODO: This structure doesn't work with NOMAC, where there is
	//   no ACK Rx on a successful unicast Data Tx.
	if(((tx_frame_info->tx_result) == TX_FRAME_INFO_RESULT_SUCCESS)){
		curr_station_info->latest_rx_timestamp = get_system_time_usec();
	}

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
	curr_txrx_counts = &(curr_station_info->txrx_counts);

	switch(pkt_type){
		default:
			//Unknown type
			return curr_station_info;
		break;
		case MAC_FRAME_CTRL1_TYPE_DATA:
			txrx_counts_sub = &(curr_txrx_counts->data);
		break;
		case MAC_FRAME_CTRL1_TYPE_MGMT:
			txrx_counts_sub = &(curr_txrx_counts->mgmt);
		break;
	}

	(txrx_counts_sub->tx_num_packets_total)++;
	(txrx_counts_sub->tx_num_bytes_total) += (tx_frame_info->length);
	(txrx_counts_sub->tx_num_attempts)    += (tx_frame_info->num_tx_attempts);

	if((tx_frame_info->tx_result) == TX_FRAME_INFO_RESULT_SUCCESS){
		(txrx_counts_sub->tx_num_packets_success)++;
		(txrx_counts_sub->tx_num_bytes_success) += tx_frame_info->length;
	}
#endif

	// Add Station Info into station_info_list
	dl_entry_insertEnd(&station_info_list, curr_dl_entry);

	return curr_station_info;

}

inline station_info_t* station_info_rx_process(void* pkt_buf_addr) {
	rx_frame_info_t*    	rx_frame_info   	     = (rx_frame_info_t*)pkt_buf_addr;
	void*               	mac_payload              = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	mac_header_80211*   	rx_80211_header          = (mac_header_80211*)((void *)mac_payload);
	dl_entry*				curr_dl_entry;
	station_info_t*			curr_station_info		 = NULL;
	u8						pkt_type;

	pkt_type = (rx_80211_header->frame_control_1 & MAC_FRAME_CTRL1_MASK_TYPE);

	if ( (rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD) &&
			(pkt_type !=  MAC_FRAME_CTRL1_TYPE_CTRL)) {
		// We will only consider good FCS receptions in our counts. We cannot
		// trust the address bytes themselves if the FCS is in error. Furthermore,
		// control frames will not be considered for counts either since the CTS
		// and ACK frames have no addr2 field.

		curr_dl_entry = station_info_find_by_addr(rx_80211_header->address_2, NULL);

		if(curr_dl_entry != NULL){
			curr_station_info = (station_info_t*)(curr_dl_entry->data);
			// Remove entry from station_info_list; Will be added back at the bottom of the function
			// This serves to sort the list and keep the most recently updated entries at the tail.
			dl_entry_remove(&station_info_list, curr_dl_entry);
		} else {
			// We haven't seen this addr before, so we'll attempt to checkout a new dl_entry
			// struct from the free pool
			curr_dl_entry = station_info_checkout();

			if (curr_dl_entry == NULL){
				// No free dl_entry!
				// We'll have to reallocate the oldest entry in the filled list
				curr_dl_entry = station_info_find_oldest();

				if (curr_dl_entry != NULL) {
					dl_entry_remove(&station_info_list, curr_dl_entry);
				} else {
					xil_printf("Cannot create station_info_t.\n");
					return NULL;
				}
			}

			curr_station_info = (station_info_t*)(curr_dl_entry->data);

			// Clear any old information from the Tx/Rx counts
			station_info_clear(curr_station_info);

			// Copy addr into station_info_t struct
			memcpy(curr_station_info->addr, rx_80211_header->address_2, MAC_ADDR_LEN);
		}

		// By this point in the function, curr_station_info is guaranteed to be pointing to a valid station_info_t struct
		// that we should update with this reception.

		// Update the latest TXRX time
		curr_station_info->latest_txrx_timestamp = get_system_time_usec();

		// Update the latest RX time
		curr_station_info->latest_rx_timestamp = get_system_time_usec();

		// Add Station Info into station_info_list
		dl_entry_insertEnd(&station_info_list, curr_dl_entry);
	}

	return curr_station_info;
}

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
void station_info_rx_process_counts(void* pkt_buf_addr, station_info_t* station_info, u32 option_flags) {
	rx_frame_info_t*    	rx_frame_info   	     = (rx_frame_info_t*)pkt_buf_addr;
	void*               	mac_payload              = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	mac_header_80211*   	rx_80211_header          = (mac_header_80211*)((void *)mac_payload);
	station_txrx_counts_t*	curr_txrx_counts;
	txrx_counts_sub_t*		txrx_counts_sub;
	u8						pkt_type;
	u16 					length   = rx_frame_info->phy_details.length;

	pkt_type = (rx_80211_header->frame_control_1 & MAC_FRAME_CTRL1_MASK_TYPE);

	if ( (station_info != NULL) && (rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD) &&
			(pkt_type !=  MAC_FRAME_CTRL1_TYPE_CTRL)) {
		// We will only consider good FCS receptions in our counts. We cannot
		// trust the address bytes themselves if the FCS is in error. Furthermore,
		// control frames will not be considered for counts either since the CTS
		// and ACK frames have no addr2 field.

		curr_txrx_counts = &(station_info->txrx_counts);

		switch(pkt_type){
			default:
				//Unknown type
				return;
			break;
			case MAC_FRAME_CTRL1_TYPE_DATA:
				txrx_counts_sub = &(curr_txrx_counts->data);
			break;
			case MAC_FRAME_CTRL1_TYPE_MGMT:
				txrx_counts_sub = &(curr_txrx_counts->mgmt);
			break;
		}

		(txrx_counts_sub->rx_num_packets_total)++;
		(txrx_counts_sub->rx_num_bytes_total) += (length - WLAN_PHY_FCS_NBYTES - sizeof(mac_header_80211));

		if( (option_flags & RX_PROCESS_COUNTS_OPTION_FLAG_IS_DUPLICATE) == 0){
			//Unique reception
			(txrx_counts_sub->rx_num_packets)++;
			(txrx_counts_sub->rx_num_bytes) += (length - WLAN_PHY_FCS_NBYTES - sizeof(mac_header_80211));
		}
	}
}
#endif

void	station_info_print(dl_list* list, u32 option_flags){
	// list == NULL : Print all in flat station_info_list
	// list !- NULL : Print only in provided list
	int       			iter;
	u32       			i;
	dl_list*			local_list;
	dl_entry* 			curr_dl_entry;
	station_info_t* 	curr_station_info;
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
	station_txrx_counts_t*		curr_txrx_counts;
#endif

	if(list != NULL){
		local_list = list;
	} else {
		local_list = &station_info_list;
	}

	i = 0;
	iter          = local_list->length;
	curr_dl_entry = local_list->first;

	// Print the header
	xil_printf("************************ Station Information *************************\n");

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_station_info = (station_info_t*)(curr_dl_entry->data);
		xil_printf("%d: [%d] %02x-%02x-%02x-%02x-%02x-%02x ", i, curr_station_info->ID, curr_station_info->addr[0],
															  curr_station_info->addr[1],curr_station_info->addr[2],curr_station_info->addr[3],
															  curr_station_info->addr[4],curr_station_info->addr[5]);


		if((list != NULL) && (curr_station_info->flags & STATION_INFO_FLAG_KEEP)){
			xil_printf("(KEEP)\n");
		} else {
			xil_printf("\n");
		}


#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
		if(option_flags & STATION_INFO_PRINT_OPTION_FLAG_INCLUDE_COUNTS){
			curr_txrx_counts = &(curr_station_info->txrx_counts);
			xil_printf("  Data   Rx Num Bytes:           %d\n", (u32)(curr_txrx_counts->data.rx_num_bytes));
			xil_printf("  Data   Rx Num Bytes Total:     %d\n", (u32)(curr_txrx_counts->data.rx_num_bytes_total));
			xil_printf("  Data   Tx Num Bytes Success:   %d\n", (u32)(curr_txrx_counts->data.tx_num_bytes_success));
			xil_printf("  Data   Tx Num Bytes Total:     %d\n", (u32)(curr_txrx_counts->data.tx_num_bytes_total));
			xil_printf("  Data   Rx Num Packets:         %d\n", (u32)(curr_txrx_counts->data.rx_num_packets));
			xil_printf("  Data   Rx Num Packets Total:   %d\n", (u32)(curr_txrx_counts->data.rx_num_packets_total));
			xil_printf("  Data   Tx Num Packets Success: %d\n", (u32)(curr_txrx_counts->data.tx_num_packets_success));
			xil_printf("  Data   Tx Num Packets Total:   %d\n", (u32)(curr_txrx_counts->data.tx_num_packets_total));
			xil_printf("  Data   Tx Num Attempts:        %d\n", (u32)(curr_txrx_counts->data.tx_num_attempts));
			xil_printf("  Mgmt.  Rx Num Bytes:           %d\n", (u32)(curr_txrx_counts->mgmt.rx_num_bytes));
			xil_printf("  Mgmt.  Rx Num Bytes Total:     %d\n", (u32)(curr_txrx_counts->mgmt.rx_num_bytes_total));
			xil_printf("  Mgmt.  Tx Num Bytes Success:   %d\n", (u32)(curr_txrx_counts->mgmt.tx_num_bytes_success));
			xil_printf("  Mgmt.  Tx Num Bytes Total:     %d\n", (u32)(curr_txrx_counts->mgmt.tx_num_bytes_total));
			xil_printf("  Mgmt.  Rx Num Packets:         %d\n", (u32)(curr_txrx_counts->mgmt.rx_num_packets));
			xil_printf("  Mgmt.  Rx Num Packets Total:   %d\n", (u32)(curr_txrx_counts->mgmt.rx_num_packets_total));
			xil_printf("  Mgmt.  Tx Num Packets Success: %d\n", (u32)(curr_txrx_counts->mgmt.tx_num_packets_success));
			xil_printf("  Mgmt.  Tx Num Packets Total:   %d\n", (u32)(curr_txrx_counts->mgmt.tx_num_packets_total));
			xil_printf("  Mgmt.  Tx Num Attempts:        %d\n", (u32)(curr_txrx_counts->mgmt.tx_num_attempts));
		}
#endif

		xil_printf("    Last update:   %d msec ago\n", (u32)((get_system_time_usec() - curr_station_info->latest_txrx_timestamp)/1000));


		curr_dl_entry = dl_entry_next(curr_dl_entry);
		i++;
	}
}


void station_info_reset_all_counts_txrx(){
	//This function will return all counts to 0.

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
	int       					iter;
	u32       					i;
	dl_entry* 					curr_dl_entry;
	station_info_t* 			curr_station_info;
	station_txrx_counts_t* 		curr_txrx_counts;

	i = 0;
	iter          = station_info_list.length;
	curr_dl_entry = station_info_list.last;


	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_station_info = (station_info_t*)(curr_dl_entry->data);
		curr_txrx_counts = &(curr_station_info->txrx_counts);

		bzero(&(curr_txrx_counts->data), sizeof(txrx_counts_sub_t));
		bzero(&(curr_txrx_counts->mgmt), sizeof(txrx_counts_sub_t));

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
		i++;
	}
#endif
}



void station_info_timestamp_check() {
	dl_entry* curr_dl_entry;
	station_info_t* curr_station_info;

	curr_dl_entry = station_info_list.first;

	while(curr_dl_entry != NULL){
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		if((get_system_time_usec() - curr_station_info->latest_txrx_timestamp) > STATION_INFO_TIMEOUT_USEC){
			if((curr_station_info->flags & STATION_INFO_FLAG_KEEP) == 0){
				station_info_clear(curr_station_info);
				dl_entry_remove(&station_info_list, curr_dl_entry);
				station_info_checkin(curr_dl_entry);
			}
		} else {
			// Nothing after this entry is older, so it's safe to quit
			return;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
}

dl_entry* station_info_checkout(){
	dl_entry* entry;

	if(station_info_free.length > 0){
		entry = ((dl_entry*)(station_info_free.first));
		dl_entry_remove(&station_info_free,station_info_free.first);
		return entry;
	} else {
		return NULL;
	}
}

void station_info_checkin(dl_entry* entry){
	dl_entry_insertEnd(&station_info_free, (dl_entry*)entry);
	return;
}

dl_entry* station_info_find_by_id(u32 id, dl_list* list){
	int       			iter;
	dl_entry* 			curr_dl_entry;
	station_info_t* 	curr_station_info;
	dl_list*			list_to_search;

	if(list == NULL){
		// Error. "List" must be provided. ID has no meaning for the
		//  flat "station_info_list" global.
		xil_printf("Error: station_info_find_by_id must be provided with a list argument\n");
		return NULL;
	} else {
		list_to_search = list;
	}

	iter          = list_to_search->length;
	curr_dl_entry = list_to_search->last;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		if (curr_station_info->ID == id) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
	}
	return NULL;
}

dl_entry* station_info_find_by_addr(u8* addr, dl_list* list){
	int       			iter;
	dl_entry* 			curr_dl_entry;
	station_info_t* 	curr_station_info;
	dl_list*			list_to_search;

	if(list == NULL){
		//Optional "list" argument not provided. Search will occur over
		// flat "station_info_list" global.
		list_to_search = &station_info_list;
	} else {
		list_to_search = list;
	}

	iter          = list_to_search->length;
	curr_dl_entry = list_to_search->last;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		if (wlan_addr_eq(addr,curr_station_info->addr)) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
	}
	return NULL;
}

dl_entry* station_info_find_oldest(){
	int       iter;
	dl_entry* curr_dl_entry;
	station_info_t* station_info;

	iter          = station_info_list.length;
	curr_dl_entry = station_info_list.first;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		station_info = (station_info_t*)(curr_dl_entry->data);

		if ((station_info->flags & STATION_INFO_FLAG_KEEP) == 0) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
	return NULL;
}



// Function will create a station_info_t and make sure that the address is unique
// in the flat station_info_list.
//
station_info_t* station_info_create(u8* addr){
	dl_entry* 		curr_dl_entry;
	station_info_t* curr_station_info = NULL;

	curr_dl_entry = station_info_find_by_addr(addr, NULL);

	if (curr_dl_entry != NULL){
		// Get the Tx/Rx Counts from the entry
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		// Remove the entry from the info list so it can be added back later
		dl_entry_remove(&station_info_list, curr_dl_entry);
	} else {
		// Have not seen this addr before; attempt to grab a new dl_entry
		// struct from the free pool
		curr_dl_entry = station_info_checkout();

		if (curr_dl_entry == NULL){
			// No free dl_entry; Re-allocate the oldest entry in the filled list
			curr_dl_entry = station_info_find_oldest();

			if (curr_dl_entry != NULL) {
				dl_entry_remove(&station_info_list, curr_dl_entry);
			} else {
				xil_printf("Cannot create station_info.\n");
				return NULL;
			}
		}

		// Get the Station Info from the entry
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		// Clear any old information
		station_info_clear(curr_station_info);

		// Copy the addr to the entry
		memcpy(curr_station_info->addr, addr, MAC_ADDR_LEN);
	}

	// Update the timestamp
	// TODO: This behavior is debatable. The idea here is that, since this
	// entry was just created, whoever called this function probably does
	// not want the framework to automatically purge it the next time
	// station_info_timestamp_check() is called. This shouldn't happen
	// if care is taken to either:
	//	(1) Update the timestamp after this function is called and before
	//		an interrupt context is left.
	//  (2) Set the STATION_INFO_FLAGS_KEEP bit to 1 after this fucntion
	//		is called and before an interrupt context is left.
	// This update is just for extra safety and makes it more difficult
	// to accidentally have the framework delete this struct.
	curr_station_info->latest_txrx_timestamp = get_system_time_usec();

	// Insert the updated entry into the network list
	dl_entry_insertEnd(&station_info_list, curr_dl_entry);

	return curr_station_info;
}

/**
 * @brief Reset List of Stations
 *
 * Reset all Station Infos except ones flagged to be kept
 *
 * @param  None
 * @return None
 */
void station_info_reset_all(){
	dl_entry * 		next_dl_entry = station_info_list.first;
	dl_entry * 		curr_dl_entry;
	station_info_t* curr_station_info;
    int		   		iter = station_info_list.length;

	while( (next_dl_entry != NULL) && (iter-- > 0) ){
		curr_dl_entry = next_dl_entry;
		next_dl_entry = dl_entry_next(curr_dl_entry);
		curr_station_info = (station_info_t*)(curr_dl_entry->data);

		if( (curr_station_info->flags & STATION_INFO_FLAG_KEEP) == 0){
			station_info_clear(curr_station_info);
			dl_entry_remove(&station_info_list, curr_dl_entry);
			station_info_checkin(curr_dl_entry);
		}
	}
}

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
/**
 * @brief Zero all Counts
 *
 * Zero all counts
 *
 * @param  None
 * @return None
 */
void txrx_counts_zero_all(){
	dl_entry * 				next_dl_entry = station_info_list.first;
	dl_entry * 				curr_dl_entry;
	station_info_t* 		curr_station_info;
	station_txrx_counts_t*	curr_txrx_counts;
    int		   				iter = station_info_list.length;

	while( (next_dl_entry != NULL) && (iter-- > 0) ){
		curr_dl_entry = next_dl_entry;
		next_dl_entry = dl_entry_next(curr_dl_entry);
		curr_station_info = (station_info_t*)(curr_dl_entry->data);
		curr_txrx_counts = &(curr_station_info->txrx_counts);
		station_info_clear_txrx_counts(curr_txrx_counts);
	}

}

void station_info_clear_txrx_counts(station_txrx_counts_t* txrx_counts){
	bzero(txrx_counts, sizeof(station_txrx_counts_t));
}
#endif

void station_info_clear(station_info_t* station_info){
	if (station_info != NULL){
        bzero(station_info, sizeof(station_info_t));
        //Set the latest Rx sequence number to an invalid value (sequence numbers are only 12 bits)
        //This will prevent erroneous de-duplications (since 0 is a valid sequence number)
        station_info->latest_rx_seq = 0xFFFF;
	}
}



inline dl_list* station_info_get_list(){
	return &station_info_list;
}


/**
 * @brief Find Station Information within a doubly-linked list from a station ID
 *
 * Given a doubly-linked list of station_info structures, this function will return
 * the pointer to a particular entry whose station ID field matches the argument
 * to this function.
 *
 * @param dl_list* list
 *  - Doubly-linked list of station_info structures
 * @param u32 id
 *  - ID to search for
 * @return curr_station_info_entry*
 *  - Returns the pointer to the entry in the doubly-linked list that has the
 *    provided AID.
 *  - Returns NULL if no station_info pointer is found that matches the search
 *    criteria
 *
 */
dl_entry* wlan_mac_high_find_station_info_ID(dl_list* list, u32 id){
	dl_entry*			curr_station_info_entry;
	station_info_t* 	station_info;

	curr_station_info_entry = list->first;
	int iter = list->length;

	while( (curr_station_info_entry != NULL) && (iter-- > 0) ){
		station_info = (station_info_t*)(curr_station_info_entry->data);

		if(station_info->ID == id){
			return curr_station_info_entry;
		} else {
			curr_station_info_entry = dl_entry_next(curr_station_info_entry);
		}
	}

	return NULL;
}


/**
 * @brief Add Station Info
 *
 * Function will add a station_info to the provided dl_list for the given address using the
 * requested station ID
 *
 * @param  dl_list* app_station_info_list
 *     - Station Info list pointer
 * @param  u8* addr
 *     - Address of station to add to the dl_list
 * @param  u16 requested_ID
 *     - Requested ID for the new station.  A value of 'ADD_STATION_INFO_ANY_ID' will use the next available AID.
 * @param  tx_params_t tx_params
 *     - Transmit parameters for the new station.
 * @param  u8 ht_capable
 *     - Is this station HT capable?  (This will set the station info HT_CAPABLE flag)
 * @return station_info *
 *     - Pointer to the station_info in the dl_list
 *     - NULL
 *
 *	TODO: This argument list should be reconsidered
 *
 * @note   This function will not perform any filtering on the addr field
 */
station_info_t*  station_info_add(dl_list* app_station_info_list, u8* addr, u16 requested_ID, tx_params_t* tx_params, u8 ht_capable){
	dl_entry*           entry;
	station_info_t*     station_info;
	dl_entry*           curr_station_info_entry;
	station_info_t*     station_info_temp;
	u16                 curr_ID;
	int                 iter;

	curr_ID = 0;

	if (requested_ID != ADD_STATION_INFO_ANY_ID) {
		// This call is requesting a particular ID.
		entry = station_info_find_by_id(requested_ID, app_station_info_list);

		if (entry != NULL) {
			station_info = (station_info_t*)(entry->data);
			// Found a station_info with this requested AID. Check if
			// the address matches the argument to this function call
			if(wlan_addr_eq(station_info->addr, addr)){
				// Already have this exact station_info, so just return a pointer to it.
				return station_info;
			} else {
				// The requested ID is already in use and it is used by a different
				// address. We cannot add this station_info.
				return NULL;
			}
		}
	}

	// Search for the station_info inside the provided list
	entry = station_info_find_by_addr(addr, app_station_info_list);

	if(entry != NULL){
		station_info = (station_info_t*)(entry->data);
		// This addr is already tied to an list entry. We'll just pass this call
		// the pointer to that entry back to the calling function without creating a new entry

		return station_info;
	} else {

		// This addr is new, so we'll have to add an entry into the list
		entry = wlan_mac_high_malloc(sizeof(dl_entry));
		if(entry == NULL){
			return NULL;
		}

		station_info = station_info_create(addr);
		if(station_info == NULL){
			free(entry);
			return NULL;
		}



		bzero(&(station_info->rate_info),sizeof(rate_selection_info_t));
		station_info->rate_info.rate_selection_scheme = RATE_SELECTION_SCHEME_STATIC;

		// Populate the entry
		entry->data = (void*)station_info;

		station_info->ID          = 0;
		station_info->hostname[0] = 0;
		station_info->flags       = 0;

		// Initialize the latest activity timestamp
		//     NOTE:  This is so we don't run into a race condition when we try to check the association timeout
		//
		station_info->latest_rx_timestamp = get_system_time_usec();

		// Set the last received sequence number to something invalid so we don't accidentally
		// de-duplicate the next reception if that sequence number is 0.
		station_info->latest_rx_seq = 0xFFFF; //Sequence numbers are only 12 bits long. This is intentionally invalid.

		// Do not allow WARP nodes to time out
		if(wlan_mac_addr_is_warp(addr)){
			// TODO: This doesn't belong here. This is an AP specific behavior.
			station_info->flags |= STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
		}

		// Set the HT_CAPABLE flag base on input parameter
		if (ht_capable) {
			station_info->flags |= STATION_INFO_FLAG_HT_CAPABLE;
		}

		// Set the TX parameters
		//     1) Do a blind copy of the TX parameters from the input argument.
		//     2) Update the (mcs, phy_mode) parameters.  This will adjust the TX rate of the station
		//        was not HT_CAPABLE and a HT rate was requested.
		//
		station_info->tx = *tx_params;

		station_info_update_rate(station_info, tx_params->phy.mcs, tx_params->phy.phy_mode);

		// Set up the station ID
		if(requested_ID == ADD_STATION_INFO_ANY_ID){
			// Find the minimum AID that can be issued to this station.
			curr_station_info_entry = app_station_info_list->first;
			iter = app_station_info_list->length;

			while((curr_station_info_entry != NULL) && (iter-- > 0)){

				station_info_temp = (station_info_t*)(curr_station_info_entry->data);

				if((station_info_temp->ID - curr_ID) > 1){
					// There is a hole in the list and we can re-issue a previously issued station ID.
					station_info->ID = station_info_temp->ID - 1;

					// Add this station into the list just before the curr_station_info
					dl_entry_insertBefore(app_station_info_list, curr_station_info_entry, entry);

					break;
				} else {
					curr_ID = station_info_temp->ID;
				}

				curr_station_info_entry = dl_entry_next(curr_station_info_entry);
			}

			if(station_info->ID == 0){
				// There was no hole in the list, so we just issue a new AID larger than the last AID in the table.

				if(app_station_info_list->length == 0){
					// This is the first entry in the list;
					station_info->ID = 1;
				} else {
					curr_station_info_entry = app_station_info_list->last;
					station_info_temp = (station_info_t*)(curr_station_info_entry->data);
					station_info->ID = (station_info_temp->ID)+1;
				}

				// Add this station into the list at the end
				dl_entry_insertEnd(app_station_info_list, entry);
			}
		} else {
			// Find the right place in the dl_list to insert this station_info with the requested AID
			curr_station_info_entry = app_station_info_list->first;
			iter = app_station_info_list->length;

			while( (curr_station_info_entry != NULL) && (iter-- > 0)){

				station_info_temp = (station_info_t*)(curr_station_info_entry->data);

				if(station_info_temp->ID > requested_ID){
					station_info->ID = requested_ID;
					// Add this station into the list just before the curr_station_info
					dl_entry_insertBefore(app_station_info_list, curr_station_info_entry, entry);
				}

				curr_station_info_entry = dl_entry_next(curr_station_info_entry);
			}

			if(station_info->ID == 0){
				// There was no hole in the list, so we insert it at the end
				station_info->ID = requested_ID;

				// Add this station into the list at the end
				dl_entry_insertEnd(app_station_info_list, entry);
			}
		}

		// Print our station_infos on the UART
		station_info_print(app_station_info_list, 0);
		return station_info;
	}
}

/**
 * @brief Remove Station Info
 *
 * Function will remove the station_info from the provided list for the given address
 *
 * @param  dl_list* app_station_info_list
 *     - Pointer to list of Station Info structs
 * @param  u8* addr
 *     - Address of station to remove from the provided list
 * @return int
 *     -  0  - Successfully removed the station
 *     - -1  - Failed to remove station
 */
int station_info_remove(dl_list* app_station_info_list, u8* addr){
	dl_entry* 		entry;

	entry = station_info_find_by_addr(addr, app_station_info_list);

	if(entry == NULL){
		// This addr doesn't refer to any station currently in the list,
		// so there is nothing to remove. We'll return an error to let the calling
		// function know that something is wrong.
		return -1;
	} else {

		// Remove station from the list;
		dl_entry_remove(app_station_info_list, entry);

		wlan_mac_high_free(entry);

		station_info_print(app_station_info_list, 0);

		return 0;
	}
}



/**
 * @brief Is the provided station a valid member of the provided list
 *
 * Function will check that the provided station is part of the dl_list provided as the first argument
 *
 * @param  dl_list* station_info_list
 *     - Pointer to list of Station Info structs
 * @param  station_info * station
 *     - Station info pointer to check
 * @return u8
 *     - 0  - Station is not a member of the provided list
 *     - 1  - Station is a member of the provided list
 */
u8	station_info_is_member(dl_list* app_station_info_list, station_info_t* station_info){
	dl_entry*	  	curr_station_info_entry;
	station_info_t* station_info_temp;
	int			  	iter = app_station_info_list->length;

	curr_station_info_entry = app_station_info_list->first;

	while( (curr_station_info_entry != NULL) && (iter-- > 0) ){

		station_info_temp = (station_info_t*)(curr_station_info_entry->data);

		if(station_info == station_info_temp){
			return 1;
		}

		curr_station_info_entry = dl_entry_next(curr_station_info_entry);
		station_info_temp = (station_info_t*)(curr_station_info_entry->data);
	}

	return 0;
}


/**
 * @brief Update station_info TX rate
 *
 * Function will update the TX rate based on the passed in parameters.  This function will
 * honor the ht_capable flag of the station info.  If the station is not ht_capable, then
 * the function will map the (mcs, phy_mode) to the nearest NONHT MCS:
 *     Requested HT MCS: 0 1 2 3 4 5 6 7
 *     Actual NONHT MCS: 0 2 3 4 5 6 7 7
 *
 * @return int     - Status
 *                     -  0 - Rate set was rate given
 *                     - -1 - Rate set was adjusted from given rate
 *
 */
int	station_info_update_rate(station_info_t* station_info, u8 mcs, u8 phy_mode) {
	int status          = 0;
	u8  tmp_mcs;

	if (station_info->flags & STATION_INFO_FLAG_HT_CAPABLE) {
		station_info->tx.phy.phy_mode = phy_mode;
		station_info->tx.phy.mcs      = mcs;
	} else {
		if (phy_mode == PHY_MODE_HTMF) {
			// Requested rate was HT, adjust the MCS corresponding to the table above
			if ((mcs == 0) || (mcs == 7)) {
				tmp_mcs = mcs;
			} else {
				tmp_mcs = mcs + 1;
			}

			status = -1;
		} else {
			// Requested rate was non-HT, so do not adjust MCS
			tmp_mcs = mcs;
		}

		station_info->tx.phy.phy_mode = PHY_MODE_NONHT;
		station_info->tx.phy.mcs      = tmp_mcs;
	}
	return status;
}
