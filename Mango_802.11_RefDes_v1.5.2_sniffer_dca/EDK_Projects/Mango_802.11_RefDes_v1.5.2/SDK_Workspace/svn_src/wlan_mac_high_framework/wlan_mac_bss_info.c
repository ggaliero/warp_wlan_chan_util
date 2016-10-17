/** @file wlan_mac_bss_info.c
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
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
#include "wlan_mac_bss_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_schedule.h"

/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

static dl_list               bss_info_free;                			///< Free BSS info

/// The bss_info_list is stored chronologically from .first being oldest
/// and .last being newest. The "find" function search from last to first
/// to minimize search time for new BSSes you hear from often.
static dl_list               bss_info_list;               	 		///< Filled BSS info

static dl_list               bss_info_matching_ssid_list;           ///< Filled BSS info that match the SSID provided to wlan_mac_high_find_bss_info_SSID


/*************************** Functions Prototypes ****************************/

dl_entry* wlan_mac_high_find_bss_info_oldest();


/******************************** Functions **********************************/

void bss_info_init(u8 dram_present){

	u32       i;
	u32       num_bss_info;
	dl_entry* dl_entry_base;

	dl_list_init(&bss_info_free);
	dl_list_init(&bss_info_list);
	dl_list_init(&bss_info_matching_ssid_list);

	if(dram_present){
		// Clear the memory in the dram used for bss_infos
		bzero((void*)BSS_INFO_BUFFER_BASE, BSS_INFO_BUFFER_SIZE);

		// The number of BSS info elements we can initialize is limited by the smaller of two values:
		//     (1) The number of dl_entry structs we can squeeze into BSS_INFO_DL_ENTRY_MEM_SIZE
		//     (2) The number of bss_info structs we can squeeze into BSS_INFO_BUFFER_SIZE
		num_bss_info = min(BSS_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry), BSS_INFO_BUFFER_SIZE/sizeof(bss_info_t));

		// At boot, every dl_entry buffer descriptor is free
		// To set up the doubly linked list, we exploit the fact that we know the starting state is sequential.
		// This matrix addressing is not safe once the queue is used. The insert/remove helper functions should be used
		dl_entry_base = (dl_entry*)(BSS_INFO_DL_ENTRY_MEM_BASE);

		for (i = 0; i < num_bss_info; i++) {
			dl_entry_base[i].data = (void*)(BSS_INFO_BUFFER_BASE + (i*sizeof(bss_info_t)));
			dl_entry_insertEnd(&bss_info_free, &(dl_entry_base[i]));
		}

		xil_printf("BSS Info list (len %d) placed in DRAM: using %d kB\n", num_bss_info, (num_bss_info*sizeof(bss_info_t))/1024);

	} else {
		xil_printf("Error initializing BSS info subsystem\n");
	}
}



void bss_info_init_finish(){
	//Will be called after interrupts have been started. Safe to use scheduler now.
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 10000000, SCHEDULE_REPEAT_FOREVER, (void*)bss_info_timestamp_check);
}



inline void bss_info_rx_process(void* pkt_buf_addr) {

	rx_frame_info_t*    rx_frame_info   	     = (rx_frame_info_t*)pkt_buf_addr;
	void*               mac_payload              = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mac_payload_ptr_u8       = (u8*)mac_payload;
	char*               ssid;
	u8                  ssid_length;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mac_payload_ptr_u8);
	dl_entry*			curr_dl_entry;
	bss_info_t*			curr_bss_info;
	u8					update_rx_power 		 = 0;
	u8					update_timestamp 		 = 0;

	u16 				length					 = rx_frame_info->phy_details.length;

	if ((rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD)) {
		switch(rx_80211_header->frame_control_1) {
			case (MAC_FRAME_CTRL1_SUBTYPE_BEACON):
				update_rx_power = 1;
				update_timestamp = 1;
			case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP):

				curr_dl_entry = wlan_mac_high_find_bss_info_BSSID(rx_80211_header->address_3);

				if(curr_dl_entry != NULL){
					curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

					// Remove entry from bss_info_list; Will be added back at the bottom of the function
					dl_entry_remove(&bss_info_list, curr_dl_entry);
				} else {
					// We haven't seen this BSS ID before, so we'll attempt to checkout a new dl_entry
					// struct from the free pool
					curr_dl_entry = bss_info_checkout();

					if (curr_dl_entry == NULL){
						// No free dl_entry!
						// We'll have to reallocate the oldest entry in the filled list
						curr_dl_entry = wlan_mac_high_find_bss_info_oldest();

						if (curr_dl_entry != NULL) {
							dl_entry_remove(&bss_info_list, curr_dl_entry);
						} else {
							xil_printf("Cannot create bss_info.\n");
							return;
						}
					}

					curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

					// Clear any old information from the BSS info
					wlan_mac_high_clear_bss_info(curr_bss_info);

					// Initialize the members stations list
					dl_list_init(&(curr_bss_info->members));

					// Copy BSSID into bss_info struct
					memcpy(curr_bss_info->bssid, rx_80211_header->address_3, MAC_ADDR_LEN);

				}

				// Move the packet pointer to after the header
				mac_payload_ptr_u8 += sizeof(mac_header_80211);

				// Set capabilities to zero
				//     - Capabilities of the BSS will be filled in below
				curr_bss_info->capabilities = 0;

				// Copy beacon capabilities into bss_info struct
				//     - Only a subset of beacon capabilities are recorded
				curr_bss_info->capabilities |= (((beacon_probe_frame*)mac_payload_ptr_u8)->capabilities & BSS_CAPABILITIES_BEACON_MASK);

				// Copy beacon interval into bss_info struct
				curr_bss_info->beacon_interval = ((beacon_probe_frame*)mac_payload_ptr_u8)->beacon_interval;

				// Copy the channel on which this packet was received into the bss_info struct
				//     - chan_spec will be overwritten later in this function if a HT
				//       capabilities tag is discovered
				curr_bss_info->chan_spec.chan_pri = rx_frame_info->channel;
				curr_bss_info->chan_spec.chan_type = CHAN_TYPE_BW20;

				// Move the packet pointer to after the beacon/probe frame
				mac_payload_ptr_u8 += sizeof(beacon_probe_frame);

				// Parse the tagged parameters
				while( (((u32)mac_payload_ptr_u8) - ((u32)mac_payload)) < (length - WLAN_PHY_FCS_NBYTES)) {

					// Parse each of the tags
					switch(mac_payload_ptr_u8[0]){

						//-------------------------------------------------
						case TAG_SSID_PARAMS:
							// SSID parameter set
							//
							ssid        = (char*)(&(mac_payload_ptr_u8[2]));
							ssid_length = min(mac_payload_ptr_u8[1],SSID_LEN_MAX);

							memcpy(curr_bss_info->ssid, ssid, ssid_length);

							// Terminate the string
							(curr_bss_info->ssid)[ssid_length] = 0;
						break;

						//-------------------------------------------------
						case TAG_HT_CAPABILITIES:
							curr_bss_info->capabilities |= BSS_CAPABILITIES_HT_CAPABLE;
						break;

						//-------------------------------------------------
						case TAG_HT_INFORMATION:
							curr_bss_info->chan_spec.chan_pri = mac_payload_ptr_u8[2];
							if(mac_payload_ptr_u8[2] & 0x4){
								// Channel widths larger than 20MHz are supported by this BSS
								if((mac_payload_ptr_u8[2] & 0x3) == 0x3){
									//Secondary Channel is below primary channel
									curr_bss_info->chan_spec.chan_type = CHAN_TYPE_BW40_SEC_BELOW;
								} else if((mac_payload_ptr_u8[2] & 0x3) == 0x1){
									//Secondary Channel is above primary channel
									curr_bss_info->chan_spec.chan_type = CHAN_TYPE_BW40_SEC_ABOVE;
								}
							}
						break;

						//-------------------------------------------------
						case TAG_DS_PARAMS:
							// DS Parameter set (e.g. channel)
							curr_bss_info->chan_spec.chan_pri = mac_payload_ptr_u8[2];
						break;
					}

					// Increment packet pointer to the next tag
					mac_payload_ptr_u8 += mac_payload_ptr_u8[1]+2;
				}

				// Update the beacon Rx power
				if (update_rx_power) curr_bss_info->latest_beacon_rx_power = rx_frame_info->rx_power;

				// Update the beacon timestamp
				if (update_timestamp) curr_bss_info->latest_beacon_rx_time = get_system_time_usec();

				// TODO: Potential here for a application-specific callback on new BSS capabilities

				// Add BSS info into bss_info_list
				dl_entry_insertEnd(&bss_info_list, curr_dl_entry);
			break;


			//---------------------------------------------------------------------
			default:
                // Only need to process MAC_FRAME_CTRL1_SUBTYPE_BEACON, MAC_FRAME_CTRL1_SUBTYPE_PROBE_RESP
				// to update BSS information.
			break;
		}
	}
}



void print_bss_info(){
	int       iter;
	u32       i;
	dl_entry* curr_dl_entry;
	bss_info_t* curr_bss_info;


	i = 0;
	iter          = bss_info_list.length;
	curr_dl_entry = bss_info_list.last;

	// Print the header
	xil_printf("************************ BSS Info *************************\n");

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		xil_printf("[%d] SSID:     %s ", i, curr_bss_info->ssid);

		if(curr_bss_info->capabilities & BSS_CAPABILITIES_PRIVACY){
			xil_printf("(*)");
		}
		if(curr_bss_info->capabilities & BSS_CAPABILITIES_IBSS){
			xil_printf("(I)");
		}
		if(curr_bss_info->flags & BSS_FLAGS_KEEP){
			xil_printf("(my BSS)");
		}
		xil_printf("\n");

		xil_printf("    BSSID:         %02x-%02x-%02x-%02x-%02x-%02x\n", curr_bss_info->bssid[0],curr_bss_info->bssid[1],curr_bss_info->bssid[2],curr_bss_info->bssid[3],curr_bss_info->bssid[4],curr_bss_info->bssid[5]);
		xil_printf("    Channel:       %d\n", wlan_mac_high_bss_channel_spec_to_radio_chan(curr_bss_info->chan_spec));

		if((curr_bss_info->flags & BSS_FLAGS_KEEP) == 0){
			xil_printf("    Last update:   %d msec ago\n", (u32)((get_system_time_usec() - curr_bss_info->latest_beacon_rx_time)/1000));
		}
		xil_printf("    Capabilities:  0x%04x\n", curr_bss_info->capabilities);
		curr_dl_entry = dl_entry_prev(curr_dl_entry);
		i++;
	}
}



void bss_info_timestamp_check() {
	dl_entry* curr_dl_entry;
	bss_info_t* curr_bss_info;

	curr_dl_entry = bss_info_list.first;

	while(curr_dl_entry != NULL){
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		if((get_system_time_usec() - curr_bss_info->latest_beacon_rx_time) > BSS_INFO_TIMEOUT_USEC){
			if((curr_bss_info->flags & BSS_FLAGS_KEEP) == 0){
				wlan_mac_high_clear_bss_info(curr_bss_info);
				dl_entry_remove(&bss_info_list, curr_dl_entry);
				bss_info_checkin(curr_dl_entry);
			}
		} else {
			// Nothing after this entry is older, so it's safe to quit
			return;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
}



dl_entry* bss_info_checkout(){
	dl_entry* bsi;
	bss_info_t* curr_bss_info;

	if(bss_info_free.length > 0){
		bsi = ((dl_entry*)(bss_info_free.first));
		dl_entry_remove(&bss_info_free,bss_info_free.first);

		curr_bss_info = (bss_info_t*)(bsi->data);
		dl_list_init(&(curr_bss_info->members));
		return bsi;
	} else {
		return NULL;
	}
}



void bss_info_checkin(dl_entry* bsi){
	dl_entry_insertEnd(&bss_info_free, (dl_entry*)bsi);
	return;
}


dl_list* wlan_mac_high_find_bss_info_SSID(char* ssid){
	// This function will return a pointer to a dl_list that contains every
	// bss_info struct that matches the SSID argument.

    int       iter;
	dl_entry* curr_dl_entry_primary_list;
	dl_entry* curr_dl_entry_match_list;
	dl_entry* next_dl_entry_match_list;
	bss_info_t* curr_bss_info;

	// Remove/free any members of bss_info_matching_ssid_list that exist since the last time
	// this function was called

	iter                     = bss_info_matching_ssid_list.length;
	curr_dl_entry_match_list = bss_info_matching_ssid_list.first;

	while ((curr_dl_entry_match_list != NULL) && (iter-- > 0)) {
		next_dl_entry_match_list = dl_entry_next(curr_dl_entry_match_list);
		dl_entry_remove(&bss_info_matching_ssid_list, curr_dl_entry_match_list);
		wlan_mac_high_free(curr_dl_entry_match_list);
		curr_dl_entry_match_list = next_dl_entry_match_list;
	}

	// At this point in the code, bss_info_matching_ssid_list is guaranteed to be empty.
	// We will fill it with new dl_entry that point to existing bss_info structs that
	// match the SSID argument to this function. Note: these bss_info structs will continue
	// to be pointed to by the dl_entry

	iter                       = bss_info_list.length;
	curr_dl_entry_primary_list = bss_info_list.last;

	while ((curr_dl_entry_primary_list != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info_t*)(curr_dl_entry_primary_list->data);

		if (strcmp(ssid, curr_bss_info->ssid) == 0) {
			curr_dl_entry_match_list = wlan_mac_high_malloc(sizeof(dl_entry));
			curr_dl_entry_match_list->data = (void*)(curr_bss_info);
			dl_entry_insertEnd(&bss_info_matching_ssid_list, curr_dl_entry_match_list);
		}

		curr_dl_entry_primary_list = dl_entry_prev(curr_dl_entry_primary_list);
	}

	return &bss_info_matching_ssid_list;
}


dl_entry* wlan_mac_high_find_bss_info_BSSID(u8* bssid){
	int       iter;
	dl_entry* curr_dl_entry;
	bss_info_t* curr_bss_info;

	iter          = bss_info_list.length;
	curr_dl_entry = bss_info_list.last;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		if (wlan_addr_eq(bssid,curr_bss_info->bssid)) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_prev(curr_dl_entry);
	}
	return NULL;
}


dl_entry* wlan_mac_high_find_bss_info_oldest(){
	int       iter;
	dl_entry* curr_dl_entry;
	bss_info_t* curr_bss_info;

	iter          = bss_info_list.length;
	curr_dl_entry = bss_info_list.first;

	while ((curr_dl_entry != NULL) && (iter-- > 0)) {
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		if ((curr_bss_info->flags & BSS_FLAGS_KEEP) == 0) {
			return curr_dl_entry;
		}

		curr_dl_entry = dl_entry_next(curr_dl_entry);
	}
	return NULL;
}



// Function will create a bss_info and make sure that the BSS ID is unique
// in the bss_info list.
//
bss_info_t* wlan_mac_high_create_bss_info(u8* bssid, char* ssid, u8 chan){
	dl_entry * curr_dl_entry;
	bss_info_t * curr_bss_info = NULL;

	curr_dl_entry = wlan_mac_high_find_bss_info_BSSID(bssid);

	if (curr_dl_entry != NULL){
		// Get the BSS info from the entry
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		// Remove the entry from the info list so it can be added back later
		dl_entry_remove(&bss_info_list, curr_dl_entry);
	} else {
		// Have not seen this BSS ID before; attempt to grab a new dl_entry
		// struct from the free pool
		curr_dl_entry = bss_info_checkout();

		if (curr_dl_entry == NULL){
			// No free dl_entry; Re-allocate the oldest entry in the filled list
			curr_dl_entry = wlan_mac_high_find_bss_info_oldest();

			if (curr_dl_entry != NULL) {
				dl_entry_remove(&bss_info_list, curr_dl_entry);
			} else {
				xil_printf("Cannot create bss_info.\n");
				return NULL;
			}
		}

		// Get the BSS info from the entry
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		// Clear any old information from the BSS info
		wlan_mac_high_clear_bss_info(curr_bss_info);

		// Initialize the associated stations list
		dl_list_init(&(curr_bss_info->members));

		// Copy the BSS ID to the entry
		memcpy(curr_bss_info->bssid, bssid, MAC_ADDR_LEN);
	}

	// Update the fields of the BSS Info
	strncpy(curr_bss_info->ssid, ssid, SSID_LEN_MAX);
	curr_bss_info->chan_spec.chan_pri       = chan;
	curr_bss_info->chan_spec.chan_type      = CHAN_TYPE_BW20;
	curr_bss_info->latest_beacon_rx_time    = get_system_time_usec();

	//
	// The following fields have their previous value retained if the were
	// in the network list (i.e. wlan_mac_high_find_bss_info_BSSID() returned
	// a non-NULL entry):
	//    - latest_beacon_rx_power
	//    - capabilities
	//    - beacon_interval
	//    - flags
	//    - station_info_list
	//

	// Insert the updated entry into the network list
	dl_entry_insertEnd(&bss_info_list, curr_dl_entry);

	return curr_bss_info;
}

/**
 * @brief Reset List of Networks
 *
 * Reset all BSS Info except ones flagged to be kept
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_reset_network_list(){
	dl_entry * next_dl_entry = bss_info_list.first;
	dl_entry * curr_dl_entry;
    bss_info_t * curr_bss_info;
    int		   iter = bss_info_list.length;

	while( (next_dl_entry != NULL) && (iter-- > 0) ){
		curr_dl_entry = next_dl_entry;
		next_dl_entry = dl_entry_next(curr_dl_entry);
		curr_bss_info = (bss_info_t*)(curr_dl_entry->data);

		if( (curr_bss_info->flags & BSS_FLAGS_KEEP) == 0){
			wlan_mac_high_clear_bss_info(curr_bss_info);
			dl_entry_remove(&bss_info_list, curr_dl_entry);
			bss_info_checkin(curr_dl_entry);
		}
	}
}

//Gabri
void wlan_mac_high_delete_bss_from_list(char* ssid){
	dl_list*					ap_matching_ssid_list;
	dl_entry*					curr_dl_entry;
	dl_entry* 					next_dl_entry;
	bss_info_t*					curr_bss_info;
	int 						iter;

	ap_matching_ssid_list = wlan_mac_high_find_bss_info_SSID(ssid);
	if(ap_matching_ssid_list != NULL){

		iter = ap_matching_ssid_list->length;
		next_dl_entry = ap_matching_ssid_list->first;

		while((next_dl_entry != NULL) && (iter-- > 0)){
			curr_dl_entry = next_dl_entry;
			next_dl_entry = dl_entry_next(curr_dl_entry);
			curr_bss_info = (bss_info_t*)(curr_dl_entry->data);
			wlan_mac_high_clear_bss_info(curr_bss_info);
			dl_entry_remove(&bss_info_list, curr_dl_entry);
		}
	}
}

void wlan_mac_high_clear_bss_info(bss_info_t * info){
	int            iter;
	station_info_t * curr_station_info;
	dl_entry       * next_station_info_entry;
	dl_entry       * curr_station_info_entry;

	if (info != NULL){
        // Remove any station infos
		iter                    = info->members.length;
		next_station_info_entry = info->members.first;

		if (((info->flags & BSS_FLAGS_KEEP) == 0) && (next_station_info_entry != NULL)) {
            xil_printf("WARNING:  BSS info not flagged to be kept but contained station_info entries.\n");
		}

		// Set any members to ~STATION_INFO_FLAG_KEEP so the framework can remove them
		while ((next_station_info_entry != NULL) && (iter-- > 0)) {
			curr_station_info_entry = next_station_info_entry;
			next_station_info_entry = dl_entry_next(curr_station_info_entry);

			curr_station_info       = (station_info_t*)(curr_station_info_entry->data);

			// Remove the "keep" flag for this station_info so the framework can cleanup later.
			curr_station_info->flags &= ~STATION_INFO_FLAG_KEEP;
			station_info_remove(&info->members, curr_station_info->addr);
		}


		// Clear the bss_info
        bzero(info, sizeof(bss_info_t));

        // Set beacon_interval to "unknown"
        info->beacon_interval = BEACON_INTERVAL_UNKNOWN;
	}
}



inline dl_list* wlan_mac_high_get_bss_info_list(){
	return &bss_info_list;
}
