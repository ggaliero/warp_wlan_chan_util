/** @file wlan_mac_ibss.c
 *  @brief Station
 *
 *  This contains code for the 802.11 IBSS node (ad hoc).
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

// Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_scan.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_ibss.h"

// WLAN Exp includes
#include "wlan_exp.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ibss.h"
#include "wlan_exp_transport.h"
#include "wlan_exp_user.h"


/*************************** Constant Definitions ****************************/

#define  WLAN_EXP_ETH                            TRANSPORT_ETH_B
#define  WLAN_EXP_NODE_TYPE                      WLAN_EXP_TYPE_DESIGN_80211_CPU_HIGH_IBSS


#define  WLAN_DEFAULT_CHANNEL                    1
#define  WLAN_DEFAULT_TX_PWR                     15
#define  WLAN_DEFAULT_TX_ANTENNA                 TX_ANTMODE_SISO_ANTA
#define  WLAN_DEFAULT_RX_ANTENNA                 RX_ANTMODE_SISO_ANTA
#define  WLAN_DEFAULT_BEACON_INTERVAL_TU         100

#define  WLAN_DEFAULT_SCAN_TIMEOUT_USEC          5000000

// WLAN_DEFAULT_USE_HT
//
// The WLAN_DEFAULT_USE_HT define will set the default unicast TX phy mode
// to:  1 --> HTMF  or  0 --> NONHT.  It will also be used as the default
// value for the HT_CAPABLE capability of the BSS in configure_bss() when
// moving from a NULL to a non-NULL BSS and the ht_capable parameter is not
// specified.  This does not affect the ability of the node to send and
// receive HT packets.   All WARP nodes are HT capable (ie they can send
// and receive both HTMF and NONHT packets).
#define  WLAN_DEFAULT_USE_HT                     1


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// If you want this station to try to associate to a known IBSS at boot, type
//   the string here. Otherwise, let it be an empty string.
static char                       default_ssid[SSID_LEN_MAX + 1] = "WARP-IBSS";
// static char                       default_ssid[SSID_LEN_MAX + 1] = "";


// Common TX header for 802.11 packets
mac_header_80211_common           tx_header_common;

// Default transmission parameters
tx_params_t                       default_unicast_mgmt_tx_params;
tx_params_t                       default_unicast_data_tx_params;
tx_params_t                       default_multicast_mgmt_tx_params;
tx_params_t                       default_multicast_data_tx_params;

// Top level IBSS state
bss_info_t*                       active_bss_info;

// Tx queue variables;
static u32                        max_queue_size;
volatile u8                       pause_data_queue;

// MAC address
static u8 	                      wlan_mac_addr[MAC_ADDR_LEN];

// Beacon configuration
static	beacon_txrx_configure_t	  gl_beacon_txrx_config;


/*************************** Functions Prototypes ****************************/

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
int  wlan_exp_process_user_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);
#endif

void send_probe_req();
void ibss_set_beacon_ts_update_mode(u32 enable);


/******************************** Functions **********************************/

int main() {
	u64                scan_start_timestamp;
	u8                 locally_administered_addr[MAC_ADDR_LEN];
	dl_list*           ssid_match_list = NULL;
	dl_entry*          temp_dl_entry = NULL;
	bss_info_t*          temp_bss_info = NULL;
	bss_config_t       bss_config;

	// Print initial message to UART
	xil_printf("\f");
	xil_printf("----- Mango 802.11 Reference Design -----\n");
	xil_printf("----- v1.5.2 ----------------------------\n");
	xil_printf("----- wlan_mac_ibss ---------------------\n");
	xil_printf("Compiled %s %s\n\n", __DATE__, __TIME__);

	// Heap Initialization
	//    The heap must be initialized before any use of malloc. This explicit
	//    init handles the case of soft-reset of the MicroBlaze leaving stale
	//    values in the heap RAM
	wlan_mac_high_heap_init();

	// Initialize the maximum TX queue size
	max_queue_size = MAX_TX_QUEUE_LEN;

	// Unpause the queue
	pause_data_queue       = 0;

	// Initialize beacon configuration
	gl_beacon_txrx_config.ts_update_mode = FUTURE_ONLY_UPDATE;
	bzero(gl_beacon_txrx_config.bssid_match, MAC_ADDR_LEN);
	gl_beacon_txrx_config.beacon_tx_mode = NO_BEACON_TX;

	// Zero out all TX params
	bzero(&default_unicast_data_tx_params, sizeof(tx_params_t));
	bzero(&default_unicast_mgmt_tx_params, sizeof(tx_params_t));
	bzero(&default_multicast_data_tx_params, sizeof(tx_params_t));
	bzero(&default_multicast_mgmt_tx_params, sizeof(tx_params_t));

	// New associations adopt these unicast params; the per-node params can be
	//   overridden via wlan_exp calls or by custom C code
	default_unicast_data_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_data_tx_params.phy.mcs            = 3;
#if WLAN_DEFAULT_USE_HT
	default_unicast_data_tx_params.phy.phy_mode       = PHY_MODE_HTMF;
#else
	default_unicast_data_tx_params.phy.phy_mode       = PHY_MODE_NONHT;
#endif
	default_unicast_data_tx_params.phy.antenna_mode   = WLAN_DEFAULT_TX_ANTENNA;

	default_unicast_mgmt_tx_params.phy.power          = WLAN_DEFAULT_TX_PWR;
	default_unicast_mgmt_tx_params.phy.mcs            = 0;
	default_unicast_mgmt_tx_params.phy.phy_mode       = PHY_MODE_NONHT;
	default_unicast_mgmt_tx_params.phy.antenna_mode   = WLAN_DEFAULT_TX_ANTENNA;

	// All multicast traffic (incl. broadcast) uses these default Tx params
	default_multicast_data_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_data_tx_params.phy.mcs          = 0;
	default_multicast_data_tx_params.phy.phy_mode     = PHY_MODE_NONHT;
	default_multicast_data_tx_params.phy.antenna_mode = WLAN_DEFAULT_TX_ANTENNA;

	default_multicast_mgmt_tx_params.phy.power        = WLAN_DEFAULT_TX_PWR;
	default_multicast_mgmt_tx_params.phy.mcs          = 0;
	default_multicast_mgmt_tx_params.phy.phy_mode     = PHY_MODE_NONHT;
	default_multicast_mgmt_tx_params.phy.antenna_mode = WLAN_DEFAULT_TX_ANTENNA;

	// Initialize the utility library
    wlan_mac_high_init();

    // IBSS is not currently a member of BSS
    configure_bss(NULL);

	// Initialize hex display to "No BSS"
	ibss_update_hex_display(0xFF);

	// Initialize callbacks
	wlan_mac_util_set_eth_rx_callback(          (void *) ethernet_receive);
	wlan_mac_high_set_mpdu_rx_callback(         (void *) mpdu_rx_process);
	wlan_mac_high_set_uart_rx_callback(         (void *) uart_rx);
	wlan_mac_high_set_poll_tx_queues_callback(  (void *) poll_tx_queues);
#if WLAN_SW_CONFIG_ENABLE_LTG
	wlan_mac_ltg_sched_set_callback(            (void *) ltg_event);
#endif //WLAN_SW_CONFIG_ENABLE_LTG
	wlan_mac_scan_set_tx_probe_request_callback((void *) send_probe_req);
	wlan_mac_scan_set_state_change_callback(    (void *) process_scan_state_change);
	wlan_mac_high_set_cpu_low_reboot_callback(  (void *) handle_cpu_low_reboot);

	// Set the Ethernet ecapsulation mode
	wlan_mac_util_set_eth_encap_mode(ENCAP_MODE_IBSS);

    wlan_mac_hw_info_t * hw_info;
    // Get the hardware info that has been collected from CPU low
    hw_info = get_mac_hw_info();

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

    // NOTE:  To use the WLAN Experiments Framework, it must be initialized after
    //        CPU low has populated the hw_info structure in the MAC High framework.

    // Reset all callbacks
    wlan_exp_reset_all_callbacks();

    // Set WLAN Exp callbacks
    wlan_exp_set_process_node_cmd_callback(         (void *) wlan_exp_process_node_cmd);
    wlan_exp_set_purge_all_data_tx_queue_callback(  (void *) purge_all_data_tx_queue);
    wlan_exp_set_tx_cmd_add_association_callback(   (void *) wlan_exp_ibss_tx_cmd_add_association);
    wlan_exp_set_process_user_cmd_callback(         (void *) wlan_exp_process_user_cmd);
    wlan_exp_set_beacon_ts_update_mode_callback(    (void *) ibss_set_beacon_ts_update_mode);
    wlan_exp_set_process_config_bss_callback(       (void *) configure_bss);
    wlan_exp_set_active_bss_info_getter_callback(   (void *) active_bss_info_getter);

    // Initialize WLAN Exp
    wlan_exp_node_init(hw_info->serial_number, hw_info->fpga_dna,
                       WLAN_EXP_ETH, hw_info->hw_addr_wlan_exp, hw_info->hw_addr_wlan);

    // Set CPU_HIGH Type in wlan_exp's node_info struct;
	wlan_exp_node_set_type_high(WLAN_EXP_NODE_TYPE);
#endif

	// CPU Low will pass HW information to CPU High as part of the boot process
	//   - Get necessary HW information
	memcpy((void*) &(wlan_mac_addr[0]), (void*) get_mac_hw_addr_wlan(), MAC_ADDR_LEN);

    // Set Header information
	tx_header_common.address_2 = &(wlan_mac_addr[0]);

	// Set the at-boot MAC Time to 0 usec
	set_mac_time_usec(0);

	wlan_mac_high_set_radio_channel(WLAN_DEFAULT_CHANNEL);
	wlan_mac_high_set_rx_ant_mode(WLAN_DEFAULT_RX_ANTENNA);
	wlan_mac_high_set_tx_ctrl_pow(WLAN_DEFAULT_TX_PWR);

	// Initialize interrupts
	wlan_mac_high_interrupt_init();

#if WLAN_SW_CONFIG_ENABLE_LOGGING
	// Reset the event log
	event_log_reset();
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING


	// Print Station information to the terminal
    xil_printf("------------------------\n");
    xil_printf("WLAN MAC IBSS boot complete: \n");
    xil_printf("  Serial Number : W3-a-%05d\n", hw_info->serial_number);
	xil_printf("  MAC Addr      : %02x:%02x:%02x:%02x:%02x:%02x\n\n", wlan_mac_addr[0], wlan_mac_addr[1], wlan_mac_addr[2], wlan_mac_addr[3], wlan_mac_addr[4], wlan_mac_addr[5]);

#ifdef WLAN_USE_UART_MENU
	xil_printf("\nPress the Esc key in your terminal to access the UART menu\n");
#endif

	// Start the interrupts
	wlan_mac_high_interrupt_restore_state(INTERRUPTS_ENABLED);

	// If there is a default SSID and the DIP switch allows it, start an active scan
	//     - Uses default scan parameters
	if ((strlen(default_ssid) > 0) && ((wlan_mac_high_get_user_io_state()&GPIO_MASK_DS_3) == 0)) {

		scan_start_timestamp = get_system_time_usec();

		wlan_mac_scan_start();

		while (((get_system_time_usec() < (scan_start_timestamp + WLAN_DEFAULT_SCAN_TIMEOUT_USEC))) &&
				(temp_dl_entry == NULL)) {
			// Only try to find a match if the IBSS has completed at least one full scan
			if (wlan_mac_scan_get_num_scans() > 0) {
				ssid_match_list = wlan_mac_high_find_bss_info_SSID(default_ssid);

				if (ssid_match_list->length > 0) {
					// Join the first entry in the list
					//     - This could be modified in the future to use some other selection,
					//       for example RX power.
					//
					temp_dl_entry = ssid_match_list->first;
				}
			}
		}

		wlan_mac_scan_stop();

		// Set the BSSID / SSID / Channel based on whether the scan was successful
		if (temp_dl_entry != NULL) {
			// Found an existing network matching the default SSID. Adopt that network's BSS configuration
			xil_printf("Found existing %s network. Matching BSS settings.\n", default_ssid);
			temp_bss_info = (bss_info_t*)(temp_dl_entry->data);

			memcpy(bss_config.bssid, temp_bss_info->bssid, MAC_ADDR_LEN);
			strncpy(bss_config.ssid, temp_bss_info->ssid, SSID_LEN_MAX);

			bss_config.chan_spec       = temp_bss_info->chan_spec;
			bss_config.beacon_interval = temp_bss_info->beacon_interval;

			if(temp_bss_info->capabilities & BSS_CAPABILITIES_HT_CAPABLE){
				bss_config.ht_capable  = 1;
			} else {
				bss_config.ht_capable  = 0;
			}

		} else {
			// Did not find an existing network matching the default SSID.  Create default BSS configuration
			xil_printf("Unable to find '%s' IBSS. Creating new network.\n", default_ssid);

			// Use node's wlan_mac_addr as BSSID
			//     - Raise the bit identifying this address as locally administered
			memcpy(locally_administered_addr, wlan_mac_addr, MAC_ADDR_LEN);
			locally_administered_addr[0] |= MAC_ADDR_MSB_MASK_LOCAL;

			memcpy(bss_config.bssid, locally_administered_addr, MAC_ADDR_LEN);
			strncpy(bss_config.ssid, default_ssid, SSID_LEN_MAX);

			bss_config.chan_spec.chan_pri  = WLAN_DEFAULT_CHANNEL;
			bss_config.chan_spec.chan_type = CHAN_TYPE_BW20;
			bss_config.beacon_interval     = WLAN_DEFAULT_BEACON_INTERVAL_TU;
			bss_config.ht_capable          = WLAN_DEFAULT_USE_HT;
		}

		// Set the rest of the bss_config fields
		bss_config.update_mask     = (BSS_FIELD_MASK_BSSID           |
									  BSS_FIELD_MASK_CHAN            |
									  BSS_FIELD_MASK_SSID            |
									  BSS_FIELD_MASK_BEACON_INTERVAL |
									  BSS_FIELD_MASK_HT_CAPABLE);

		// Set the BSS configuration
		configure_bss(&bss_config);
	}

	// Schedule Events
	wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, ASSOCIATION_CHECK_INTERVAL_US, SCHEDULE_REPEAT_FOREVER, (void*)remove_inactive_station_infos);


	while(1){
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
		// The wlan_exp Ethernet handling is not interrupt based. Periodic polls of the wlan_exp
		//     transport are required to service new commands. All other node activity (wired/wireless Tx/Rx,
		//     scheduled events, user interaction, etc) are handled via interrupt service routines
		transport_poll(WLAN_EXP_ETH);
#endif
	}

	// Unreachable, but non-void return keeps the compiler happy
	return -1;
}

/*****************************************************************************/
/**
 * @brief Send probe requet
 *
 * This function is part of the scan infrastructure and will be called whenever
 * the node needs to send a probe request.
 *
 * @param   None
 * @return  None
 *
 *****************************************************************************/
void send_probe_req(){
	u16                             tx_length;
	tx_queue_element_t*             curr_tx_queue_element;
	tx_queue_buffer_t*              curr_tx_queue_buffer;
	volatile scan_parameters_t*     scan_parameters = wlan_mac_scan_get_parameters();

	// Check out queue element for packet
	curr_tx_queue_element = queue_checkout();

	// Create probe request
	if(curr_tx_queue_element != NULL){
		curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

		// Setup the TX header
		wlan_mac_high_setup_tx_header(&tx_header_common, (u8 *)bcast_addr, (u8 *)bcast_addr);

		// Fill in the data
		tx_length = wlan_create_probe_req_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, scan_parameters->ssid);

		// Setup the TX frame info
		wlan_mac_high_setup_tx_frame_info(&tx_header_common, curr_tx_queue_element, tx_length, 0, MANAGEMENT_QID);

		// Set the information in the TX queue buffer
		curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
		curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_multicast_mgmt_tx_params);
		curr_tx_queue_buffer->tx_frame_info.ID          = 0;

		// Put the packet in the queue
		enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

		// Poll the TX queues to possibly send the packet
		poll_tx_queues();
	}
}



/*****************************************************************************/
/**
 * @brief Handle state change in the network scanner
 *
 * This function is part of the scan infrastructure and will be called whenever
 * the scanner is started, stopped, paused, or resumed. This allows the STA
 * to revert the channel to a known-good state when the scanner has stopped and
 * also serves as notification to the project that it should stop dequeueing data
 * frames since it might be on a different channel than its intended recipient.
 *
 * @param   None
 * @return  None
 *
 *****************************************************************************/
void process_scan_state_change(scan_state_t scan_state){

	// ------------------------------------------------------------------------
	// Note on scanning:
	//
	//   Currently, scanning should only be done with active_bss_info = NULL, ie the
	// node is not currently in a BSS.  This is to avoid any corner cases.  The
	// IBSS needs to do the following things to make scanning safe when active_bss_info
	// is not NULL:
	//
	//     - Pause outgoing data queues
	//     - Pause beacon transmissions in CPU_LOW
	//     - Refuse to enqueue probe responses when a probe request is received off channel
	//     - Pause dequeue of probe responses when off channel
	//       - Note: Currently, this is difficult because probe responses share a
	//             queue with probe requests which are needed for active scans
	//
	// ------------------------------------------------------------------------

	switch(scan_state){
		case SCAN_IDLE:
		case SCAN_PAUSED:
			pause_data_queue = 0;
			if(active_bss_info != NULL){
				wlan_mac_high_set_radio_channel(
						wlan_mac_high_bss_channel_spec_to_radio_chan(active_bss_info->chan_spec));
			}
		break;
		case SCAN_RUNNING:
			pause_data_queue = 1;
		break;
	}
}



/*****************************************************************************/
/**
 * @brief Poll Tx queues to select next available packet to transmit
 *
 * @param   None
 * @return  None
 *****************************************************************************/
void poll_tx_queues(){
	u32 i,k;

	#define NUM_QUEUE_GROUPS 3
	typedef enum {BEACON_QGRP, MGMT_QGRP, DATA_QGRP} queue_group_t;

	// Remember the next group to poll between calls to this function
	//   This implements the ping-pong poll between the MGMT_QGRP and DATA_QGRP groups
	static queue_group_t next_queue_group = MGMT_QGRP;
	queue_group_t curr_queue_group;

	// Remember the last queue polled between calls to this function
	//   This implements the round-robin poll of queues in the DATA_QGRP group
	static dl_entry* next_station_info_entry = NULL;
	dl_entry* curr_station_info_entry;

	station_info_t* curr_station_info;

	if (wlan_mac_high_is_dequeue_allowed()) {
		for(k = 0; k < NUM_QUEUE_GROUPS; k++){
			curr_queue_group = next_queue_group;

			switch(curr_queue_group){
				case BEACON_QGRP:
					next_queue_group = MGMT_QGRP;
					if(dequeue_transmit_checkin(BEACON_QID)){
						return;
					}
				break;

				case MGMT_QGRP:
					next_queue_group = DATA_QGRP;
					if(dequeue_transmit_checkin(MANAGEMENT_QID)){
						return;
					}
				break;

				case DATA_QGRP:
					next_queue_group = BEACON_QGRP;
					curr_station_info_entry = next_station_info_entry;

					if(active_bss_info != NULL){
						for(i = 0; i < (active_bss_info->members.length + 1); i++) {
							// Loop through all associated stations' queues and the broadcast queue
							if(curr_station_info_entry == NULL){
								// Check the broadcast queue
								next_station_info_entry = active_bss_info->members.first;
								if(dequeue_transmit_checkin(MCAST_QID)){
									// Found a not-empty queue, transmitted a packet
									return;
								} else {
									curr_station_info_entry = next_station_info_entry;
								}
							} else {
								curr_station_info = (station_info_t*)(curr_station_info_entry->data);
								if( station_info_is_member(&active_bss_info->members, curr_station_info) ){
									if(curr_station_info_entry == active_bss_info->members.last){
										// We've reached the end of the table, so we wrap around to the beginning
										next_station_info_entry = NULL;
									} else {
										next_station_info_entry = dl_entry_next(curr_station_info_entry);
									}

									if(dequeue_transmit_checkin(STATION_ID_TO_QUEUE_ID(curr_station_info->ID))){
										// Found a not-empty queue, transmitted a packet
										return;
									} else {
										curr_station_info_entry = next_station_info_entry;
									}
								} else {
									// This curr_station_info is invalid. Perhaps it was removed from
									// the association table before poll_tx_queues was called. We will
									// start the round robin checking back at broadcast.
									next_station_info_entry = NULL;
									return;
								} // END if(is_valid_association)
							}
						} // END for loop over association table
					} else {
						if(dequeue_transmit_checkin(MCAST_QID)){
							// Found a not-empty queue, transmitted a packet
							return;
						}
					}
				break;
			} // END switch(queue group)
		} // END loop over queue groups
	} // END CPU low is ready
}



/*****************************************************************************/
/**
 * @brief Purges all packets from all Tx queues
 *
 * This function discards all currently en-queued packets awaiting transmission and returns all
 * queue entries to the free pool.
 *
 * This function does not discard packets already submitted to the lower-level MAC for transmission
 *
 * @param None
 * @return None
 *****************************************************************************/
void purge_all_data_tx_queue(){
	dl_entry*	    curr_station_info_entry;
	station_info_t* curr_station_info;
	int			  iter = active_bss_info->members.length;

	// Purge all data transmit queues
	purge_queue(MCAST_QID);                                    		// Broadcast Queue

	if(active_bss_info != NULL){
		curr_station_info_entry = active_bss_info->members.first;
		while( (curr_station_info_entry != NULL) && (iter-- > 0) ){
			curr_station_info = (station_info_t*)(curr_station_info_entry->data);
			purge_queue(STATION_ID_TO_QUEUE_ID(curr_station_info->ID));       		// Each unicast queue
			curr_station_info_entry = dl_entry_next(curr_station_info_entry);
		}
	}
}

/*****************************************************************************/
/**
 * @brief Callback to handle insertion of an Ethernet reception into the corresponding wireless Tx queue
 *
 * This function is called when a new Ethernet packet is received that must be transmitted via the wireless interface.
 * The packet must be encapsulated before it is passed to this function. Ethernet encapsulation is implemented in the mac_high framework.
 *
 * The tx_queue_list argument is a DL list, but must contain exactly one queue entry which contains the encapsulated packet
 * A list container is used here to ease merging of the list with the target queue.
 *
 * @param tx_queue_element_t* curr_tx_queue_element
 *  - A single queue element containing the packet to transmit
 * @param u8* eth_dest
 *  - 6-byte destination address from original Ethernet packet
 * @param u8* eth_src
 *  - 6-byte source address from original Ethernet packet
 * @param u16 tx_length
 *  - Length (in bytes) of the packet payload
 * @return 1 for successful enqueuing of the packet, 0 otherwise
 *****************************************************************************/
int ethernet_receive(tx_queue_element_t* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length){

	tx_queue_buffer_t* 	curr_tx_queue_buffer;
	station_info_t*     station_info;
	dl_entry*			station_info_entry;
	u32                 queue_sel;

	if(active_bss_info != NULL){

		// Send the pre-encapsulated Ethernet frame over the wireless interface
		//     NOTE:  The queue element has already been provided, so we do not need to check if it is NULL
		curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

		// Setup the TX header
		wlan_mac_high_setup_tx_header( &tx_header_common, eth_dest,active_bss_info->bssid);

		// Fill in the data
		wlan_create_data_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, 0);

		if( wlan_addr_mcast(eth_dest) ){
			// Setup the TX frame info
				queue_sel = MCAST_QID;
				wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, ( 0 ), queue_sel );

				// Set the information in the TX queue buffer
				curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
				curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_multicast_data_tx_params);
				curr_tx_queue_buffer->tx_frame_info.ID         = 0;
		} else {

			station_info_entry = station_info_find_by_addr(eth_dest, &active_bss_info->members);

			if(station_info_entry != NULL){
				station_info = (station_info_t*)station_info_entry->data;
			} else {
				// Add station info
				//     - Set ht_capable argument to the HT_CAPABLE capability of the BSS.  Given that the node does not know
				//       the HT capabilities of the new station, it is reasonable to assume that they are the same as the BSS.
				//
				station_info = station_info_add(&(active_bss_info->members), eth_dest, ADD_STATION_INFO_ANY_ID, &default_unicast_data_tx_params,
															  (active_bss_info->capabilities & BSS_CAPABILITIES_HT_CAPABLE));
				if(station_info != NULL){
					station_info->flags |= STATION_INFO_FLAG_KEEP;
				}

				ibss_update_hex_display(active_bss_info->members.length);
			}

			if(station_info == NULL){
				//If we don't have a station_info for this frame, we will enqueue it in the multicast queue as a catch all
				queue_sel = MCAST_QID;
				// Setup the TX frame info
				wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_REQ_TO), queue_sel );
				curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
				curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_data_tx_params);
				curr_tx_queue_buffer->tx_frame_info.ID         = 0;
			} else {
				queue_sel = STATION_ID_TO_QUEUE_ID(station_info->ID);
				// Setup the TX frame info
				wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_REQ_TO), queue_sel );
				curr_tx_queue_buffer->metadata.metadata_type  = QUEUE_METADATA_TYPE_STATION_INFO;
				curr_tx_queue_buffer->metadata.metadata_ptr   = (u32)station_info;
				curr_tx_queue_buffer->tx_frame_info.ID          = station_info->ID;
			}
		}

		if(queue_num_queued(queue_sel) < max_queue_size){
			// Put the packet in the queue
			enqueue_after_tail(queue_sel, curr_tx_queue_element);

		} else {
			// Packet was not successfully enqueued
			return 0;
		}

		// Packet was successfully enqueued
		return 1;
	} else {
		return 0;
	}
}



/*****************************************************************************/
/**
 * @brief Process received MPDUs
 *
 * This callback function will process all the received MPDUs..
 *
 * @param  void* pkt_buf_addr
 *     - Packet buffer address;  Contains the contents of the MPDU as well as other packet information from CPU low
 * @param  station_info_t * station_info
 *     - Pointer to metadata about the station from which this frame was received
 * @param  rx_common_entry* rx_event_log_entry
 * 	   - Pointer to the log entry created for this reception by the MAC High Framework
 * @return u32 flags
 *
 *****************************************************************************/
u32 mpdu_rx_process(void* pkt_buf_addr, station_info_t* station_info, rx_common_entry* rx_event_log_entry)  {

	rx_frame_info_t*    rx_frame_info            = (rx_frame_info_t*)pkt_buf_addr;
	void*               mac_payload              = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mac_payload_ptr_u8       = (u8*)mac_payload;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mac_payload_ptr_u8);

	u16                 rx_seq;

	tx_queue_element_t*   curr_tx_queue_element;
	tx_queue_buffer_t*    curr_tx_queue_buffer;

	u8                  unicast_to_me;
	u8                  to_multicast;
	u8					send_response			 = 0;
	u32					tx_length;
	u8					pre_llc_offset			 = 0;
	u32					return_val				 = 0;

	u16 				length				 = rx_frame_info->phy_details.length;

	// If this function was passed a CTRL frame (e.g., CTS, ACK), then we should just quit.
	// The only reason this occured was so that it could be logged in the line above.
	if((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_CTRL){
		goto mpdu_rx_process_end;
	}

	// Determine destination of packet
	unicast_to_me = wlan_addr_eq(rx_80211_header->address_1, wlan_mac_addr);
	to_multicast  = wlan_addr_mcast(rx_80211_header->address_1);

    // If the packet is good (ie good FCS) and it is destined for me, then process it
	if( (rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD)){

		// Sequence number is 12 MSB of seq_control field
		rx_seq         = ((rx_80211_header->sequence_control) >> 4) & 0xFFF;

		// Check if this was a duplicate reception
		//	 - Packet has the RETRY bit set to 1 in the second frame control byte
		//   - Received seq num matched previously received seq num for this STA
		if( station_info != NULL ){
			if( ((rx_80211_header->frame_control_2) & MAC_FRAME_CTRL2_FLAG_RETRY) && (station_info->latest_rx_seq == rx_seq) ) {
				if(rx_event_log_entry != NULL){
					rx_event_log_entry->flags |= RX_FLAGS_DUPLICATE;
					return_val |= MAC_RX_CALLBACK_RETURN_FLAG_DUP;
				}
			} else {
				station_info->latest_rx_seq = rx_seq;
			}
		}

		// Update the association information
		if(active_bss_info != NULL){
			if(wlan_addr_eq(rx_80211_header->address_3, active_bss_info->bssid)){
				if( station_info != NULL && station_info_is_member(&active_bss_info->members, station_info) == 0 ){
					// Add station info
					//     - Set ht_capable argument to the HT_CAPABLE capability of the BSS.  Given that the node does not know
					//       the HT capabilities of the new station, it is reasonable to assume that they are the same as the BSS.
					//
					// Note: we do not need the returned station_info_t* from this function since it is guaranteed to match
					//  the "station_info" argument to the mpdu_rx_process function
					station_info_add(&(active_bss_info->members), rx_80211_header->address_2, ADD_STATION_INFO_ANY_ID, &default_unicast_data_tx_params,
										(active_bss_info->capabilities & BSS_CAPABILITIES_HT_CAPABLE));
					station_info->flags |= STATION_INFO_FLAG_KEEP;
					ibss_update_hex_display(active_bss_info->members.length);
				}
			}
		}

		if(station_info != NULL) {


			// Check if this was a duplicate reception
			//   - Received seq num matched previously received seq num for this STA
			if( return_val & MAC_RX_CALLBACK_RETURN_FLAG_DUP) {
				// Finish the function
				goto mpdu_rx_process_end;
			}
		}

		if(unicast_to_me || to_multicast){
			// Process the packet
			switch(rx_80211_header->frame_control_1) {

				//---------------------------------------------------------------------
				case MAC_FRAME_CTRL1_SUBTYPE_QOSDATA:
					pre_llc_offset = sizeof(qos_control);
				case (MAC_FRAME_CTRL1_SUBTYPE_DATA):
					// Data packet
					//   - If the STA is associated with the AP and this is from the DS, then transmit over the wired network
					//
					if(active_bss_info != NULL){
						if(wlan_addr_eq(rx_80211_header->address_3, active_bss_info->bssid)) {
							// MPDU is flagged as destined to the DS - send it for de-encapsulation and Ethernet Tx (if appropriate)
							wlan_mpdu_eth_send(mac_payload, length, pre_llc_offset);
						}
					}
				break;

				//---------------------------------------------------------------------
				case (MAC_FRAME_CTRL1_SUBTYPE_PROBE_REQ):
					if(active_bss_info != NULL){
						if(wlan_addr_eq(rx_80211_header->address_3, bcast_addr)) {
							mac_payload_ptr_u8 += sizeof(mac_header_80211);

							// Loop through tagged parameters
							while(((u32)mac_payload_ptr_u8 -  (u32)mac_payload)<= (length - WLAN_PHY_FCS_NBYTES)){

								// What kind of tag is this?
								switch(mac_payload_ptr_u8[0]){
									//-----------------------------------------------------
									case TAG_SSID_PARAMS:
										// SSID parameter set
										if((mac_payload_ptr_u8[1]==0) || (memcmp(mac_payload_ptr_u8+2, (u8*)default_ssid, mac_payload_ptr_u8[1])==0)) {
											// Broadcast SSID or my SSID - send unicast probe response
											send_response = 1;
										}
									break;

									//-----------------------------------------------------
									case TAG_SUPPORTED_RATES:
										// Supported rates
									break;

									//-----------------------------------------------------
									case TAG_EXT_SUPPORTED_RATES:
										// Extended supported rates
									break;

									//-----------------------------------------------------
									case TAG_DS_PARAMS:
										// DS Parameter set (e.g. channel)
									break;
								}

								// Move up to the next tag
								mac_payload_ptr_u8 += mac_payload_ptr_u8[1]+2;
							}

							if(send_response) {
								// Create a probe response frame
								curr_tx_queue_element = queue_checkout();

								if(curr_tx_queue_element != NULL){
									curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

									// Setup the TX header
									wlan_mac_high_setup_tx_header( &tx_header_common, rx_80211_header->address_2, active_bss_info->bssid );

									// Fill in the data
									tx_length = wlan_create_probe_resp_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, active_bss_info);

									// Setup the TX frame info
									wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, tx_length, (TX_FRAME_INFO_FLAGS_FILL_TIMESTAMP | TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_REQ_TO), MANAGEMENT_QID );

									// Set the information in the TX queue buffer
									curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
									curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
									curr_tx_queue_buffer->tx_frame_info.ID         = 0;

									// Put the packet in the queue
									enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);
								}

								// Finish the function
								goto mpdu_rx_process_end;
							}
						}
					}
				break;

				//---------------------------------------------------------------------
				default:
					//This should be left as a verbose print. It occurs often when communicating with mobile devices since they tend to send
					//null data frames (type: DATA, subtype: 0x4) for power management reasons.
					wlan_printf(PL_VERBOSE, "Received unknown frame control type/subtype %x\n",rx_80211_header->frame_control_1);
				break;
			}
		}
		// Finish the function
		goto mpdu_rx_process_end;
	} else {
		// Process any Bad FCS packets
		goto mpdu_rx_process_end;
	}


	// Finish any processing for the RX MPDU process
	mpdu_rx_process_end:

    return return_val;
}



/*****************************************************************************/
/**
 * @brief Check the time since the station has interacted with another station
 *
 *
 * @param  None
 * @return None
 *****************************************************************************/
void remove_inactive_station_infos() {

	u64 				time_since_last_activity;
	station_info_t*     curr_station_info;
	dl_entry*           curr_station_info_entry;
	dl_entry*           next_station_info_entry;

	if(active_bss_info != NULL){
		next_station_info_entry = active_bss_info->members.first;

		while(next_station_info_entry != NULL) {
			curr_station_info_entry = next_station_info_entry;
			next_station_info_entry = dl_entry_next(curr_station_info_entry);

			curr_station_info        = (station_info_t*)(curr_station_info_entry->data);
			time_since_last_activity = (get_system_time_usec() - curr_station_info->latest_rx_timestamp);

			// De-authenticate the station if we have timed out and we have not disabled this check for the station
			if((time_since_last_activity > ASSOCIATION_TIMEOUT_US) && ((curr_station_info->flags & STATION_INFO_FLAG_DISABLE_ASSOC_CHECK) == 0)){
                purge_queue(STATION_ID_TO_QUEUE_ID(curr_station_info->ID));
                curr_station_info->flags &= ~STATION_INFO_FLAG_KEEP;
                station_info_remove( &active_bss_info->members, curr_station_info->addr );
				ibss_update_hex_display(active_bss_info->members.length);
			}
		}
	}
}


#if WLAN_SW_CONFIG_ENABLE_LTG
/*****************************************************************************/
/**
 * @brief Callback to handle new Local Traffic Generator event
 *
 * This function is called when the LTG scheduler determines a traffic generator should create a new packet. The
 * behavior of this function depends entirely on the LTG payload parameters.
 *
 * The reference implementation defines 3 LTG payload types:
 *  - LTG_PYLD_TYPE_FIXED: generate 1 fixed-length packet to single destination; callback_arg is pointer to ltg_pyld_fixed struct
 *  - LTG_PYLD_TYPE_UNIFORM_RAND: generate 1 random-length packet to signle destimation; callback_arg is pointer to ltg_pyld_uniform_rand struct
 *  - LTG_PYLD_TYPE_ALL_ASSOC_FIXED: generate 1 fixed-length packet to each associated station; callback_arg is poitner to ltg_pyld_all_assoc_fixed struct
 *
 * @param u32 id
 *  - Unique ID of the previously created LTG
 * @param void* callback_arg
 *  - Callback argument provided at LTG creation time; interpretation depends on LTG type
 * @return None
 *****************************************************************************/
void ltg_event(u32 id, void* callback_arg){

	u32                 payload_length;
	u32                 min_ltg_payload_length;
	dl_entry*	        station_info_entry           = NULL;
	station_info_t*     station_info                 = NULL;
	u8*                 addr_da;
	u8                  is_multicast;
	u8                  queue_sel;
	tx_queue_element_t* curr_tx_queue_element        = NULL;
	tx_queue_buffer_t*  curr_tx_queue_buffer         = NULL;
	u8                  continue_loop;

	if(active_bss_info != NULL){
		switch(((ltg_pyld_hdr*)callback_arg)->type){
			case LTG_PYLD_TYPE_FIXED:
				payload_length = ((ltg_pyld_fixed*)callback_arg)->length;
				addr_da = ((ltg_pyld_fixed*)callback_arg)->addr_da;
				is_multicast = wlan_addr_mcast(addr_da);
				if(is_multicast){
					queue_sel = MCAST_QID;
				} else {
					station_info_entry = station_info_find_by_addr(addr_da, &active_bss_info->members);
					if(station_info_entry != NULL){
						station_info = (station_info_t*)(station_info_entry->data);
						queue_sel = STATION_ID_TO_QUEUE_ID(station_info->ID);
					} else {
						//Unlike the AP, this isn't necessarily a criteria for giving up on this LTG event.
						//In the IBSS, it's possible that there simply wasn't room in the heap for a station_info,
						//but we should still send it a packet. We'll use the multi-cast queue as a catch-all queue for these frames.
						queue_sel = MCAST_QID;
					}
				}
			break;

			case LTG_PYLD_TYPE_UNIFORM_RAND:
				payload_length = (rand()%(((ltg_pyld_uniform_rand*)(callback_arg))->max_length - ((ltg_pyld_uniform_rand*)(callback_arg))->min_length))+((ltg_pyld_uniform_rand*)(callback_arg))->min_length;
				addr_da = ((ltg_pyld_fixed*)callback_arg)->addr_da;

				is_multicast = wlan_addr_mcast(addr_da);
				if(is_multicast){
					queue_sel = MCAST_QID;
				} else {
					station_info_entry = station_info_find_by_addr(addr_da, &active_bss_info->members);
					if(station_info_entry != NULL){
						station_info = (station_info_t*)(station_info_entry->data);
						queue_sel = STATION_ID_TO_QUEUE_ID(station_info->ID);
					} else {
						//Unlike the AP, this isn't necessarily a criteria for giving up on this LTG event.
						//In the IBSS, it's possible that there simply wasn't room in the heap for a station_info,
						//but we should still send it a packet. We'll use the multi-cast queue as a catch-all queue for these frames.
						queue_sel = MCAST_QID;
					}
				}
			break;

			case LTG_PYLD_TYPE_ALL_ASSOC_FIXED:
				if(active_bss_info->members.length > 0){
					station_info_entry = active_bss_info->members.first;
					station_info = (station_info_t*)station_info_entry->data;
					addr_da = station_info->addr;
					queue_sel = STATION_ID_TO_QUEUE_ID(station_info->ID);
					is_multicast = 0;
					payload_length = ((ltg_pyld_all_assoc_fixed*)callback_arg)->length;
				} else {
					return;
				}
			break;

			default:
				xil_printf("ERROR ltg_event: Unknown LTG Payload Type! (%d)\n", ((ltg_pyld_hdr*)callback_arg)->type);
				return;
			break;
		}

		if(is_multicast == 0){
			station_info_entry = station_info_find_by_addr(addr_da, &active_bss_info->members);

			if(station_info_entry == NULL){
				// Add station info
				//     - Set ht_capable argument to the HT_CAPABLE capability of the BSS.  Given that the node does not know
				//       the HT capabilities of the new station, it is reasonable to assume that they are the same as the BSS.
				//
				station_info = station_info_add(&(active_bss_info->members), addr_da, ADD_STATION_INFO_ANY_ID, &default_unicast_data_tx_params,
															  (active_bss_info->capabilities & BSS_CAPABILITIES_HT_CAPABLE));
				if(station_info != NULL){
					station_info->flags |= STATION_INFO_FLAG_KEEP;
				}
				ibss_update_hex_display(active_bss_info->members.length);
			}
		}


		do{
			continue_loop = 0;

			if(queue_num_queued(queue_sel) < max_queue_size){
				// Checkout 1 element from the queue;
				curr_tx_queue_element = queue_checkout();
				if(curr_tx_queue_element != NULL){
					// Create LTG packet
					curr_tx_queue_buffer = ((tx_queue_buffer_t*)(curr_tx_queue_element->data));

					// Setup the MAC header
					wlan_mac_high_setup_tx_header( &tx_header_common, addr_da, active_bss_info->bssid );

					min_ltg_payload_length = wlan_create_ltg_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, MAC_FRAME_CTRL2_FLAG_FROM_DS, id);
					payload_length = max(payload_length+sizeof(mac_header_80211)+WLAN_PHY_FCS_NBYTES, min_ltg_payload_length);

					// Finally prepare the 802.11 header
					if (is_multicast) {
						wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, payload_length, (TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_FILL_UNIQ_SEQ), queue_sel);
					} else {
						wlan_mac_high_setup_tx_frame_info ( &tx_header_common, curr_tx_queue_element, payload_length, (TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_FILL_UNIQ_SEQ | TX_FRAME_INFO_FLAGS_REQ_TO), queue_sel);
					}

					// Update the queue entry metadata to reflect the new new queue entry contents
					if (is_multicast) {
						curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
						curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)&default_multicast_data_tx_params;
						curr_tx_queue_buffer->tx_frame_info.ID     = 0;
					} else if(station_info == NULL){
						curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
						curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)&default_unicast_data_tx_params;
						curr_tx_queue_buffer->tx_frame_info.ID     = 0;
					} else {

					    curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_STATION_INFO;
					    curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)station_info;
						curr_tx_queue_buffer->tx_frame_info.ID         = station_info->ID;
					}

					// Submit the new packet to the appropriate queue
					enqueue_after_tail(queue_sel, curr_tx_queue_element);


				} else {
					// There aren't any free queue elements right now.
					// As such, there probably isn't any point to continuing this callback.
					// We'll return and try again once it is called the next time.
					return;
				}
			}

			if(((ltg_pyld_hdr*)callback_arg)->type == LTG_PYLD_TYPE_ALL_ASSOC_FIXED){
				station_info_entry = dl_entry_next(station_info_entry);
				if(station_info_entry != NULL){
					station_info = (station_info_t*)station_info_entry->data;
					addr_da = station_info->addr;
					queue_sel = STATION_ID_TO_QUEUE_ID(station_info->ID);
					is_multicast = 0;
					continue_loop = 1;
				} else {
					continue_loop = 0;
				}
			} else {
				continue_loop = 0;
			}
		} while(continue_loop == 1);
	}
}
#endif //WLAN_SW_CONFIG_ENABLE_LTG

/*****************************************************************************/
/**
 * @brief Handle a reboot of CPU_LOW
 *
 * If CPU_LOW reboots, any parameters we had previously set in it will be lost.
 * This function is called to tell us that we should re-apply any previous
 * parameters we had set.
 *
 * @param  None
 * @return None
 *****************************************************************************/
void handle_cpu_low_reboot(){
	if(active_bss_info){
		// Re-apply any Beacon Tx configurations
		wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
	}
}

/*****************************************************************************/
/**
 *
 *****************************************************************************/
u32	configure_bss(bss_config_t* bss_config){
	u32					return_status 				= 0;
	u8					update_beacon_template 		= 0;
	u8					send_beacon_config_to_low 	= 0;
	u8					send_channel_switch_to_low	= 0;

	bss_info_t*			local_bss_info;
	interrupt_state_t   curr_interrupt_state;
	station_info_t* 	curr_station_info;
	dl_entry* 			next_station_info_entry;
	dl_entry* 			curr_station_info_entry;
	int					iter;

	//---------------------------------------------------------
	// 1. Check for any invalid inputs or combination of inputs
	//      First verify the requested update to the BSS configuration before
	//      modifying anything. This will prevent a partial update of BSS
	//      configuration with valid parameters before discovering an invalid
	//      parameter.

	if (bss_config != NULL) {
		if (bss_config->update_mask & BSS_FIELD_MASK_BSSID) {
			if (wlan_addr_eq(bss_config->bssid, zero_addr) == 0) {
				if ((active_bss_info != NULL) && wlan_addr_eq(bss_config->bssid, active_bss_info->bssid)) {
					// The caller of this function claimed that it was updating the BSSID,
					// but the new BSSID matches the one already specified in active_bss_info.
					// Complete the rest of this function as if that bit in the update mask
					// were not set
					bss_config->update_mask &= ~BSS_FIELD_MASK_BSSID;
				} else {
					// Changing the BSSID, perform necessary argument checks
					if ((bss_config->bssid[0] & MAC_ADDR_MSB_MASK_LOCAL ) == 0) {
						// In the IBSS implementation, the BSSID provided must be locally generated
						return_status |= BSS_CONFIG_FAILURE_BSSID_INVALID;
					}
					if (((bss_config->update_mask & BSS_FIELD_MASK_SSID) == 0) ||
						((bss_config->update_mask & BSS_FIELD_MASK_CHAN) == 0) ||
						((bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL) == 0)) {
						return_status |= BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS;
					}
				}
			}
		} else {
			if (active_bss_info == NULL) {
				// Cannot update BSS without specifying BSSID
				return_status |= BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS;
			}
		}
		if (bss_config->update_mask & BSS_FIELD_MASK_CHAN) {
			if (wlan_verify_channel(
					wlan_mac_high_bss_channel_spec_to_radio_chan(active_bss_info->chan_spec)) != XST_SUCCESS) {
				return_status |= BSS_CONFIG_FAILURE_CHANNEL_INVALID;
			}
		}
		if (bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL) {
			if ((bss_config->beacon_interval != BEACON_INTERVAL_NO_BEACON_TX) &&
				(bss_config->beacon_interval <  10)) {
				return_status |= BSS_CONFIG_FAILURE_BEACON_INTERVAL_INVALID;
			}
		}
		if (bss_config->update_mask & BSS_FIELD_MASK_HT_CAPABLE) {
			if (bss_config->ht_capable > 1) {
				return_status |= BSS_CONFIG_FAILURE_HT_CAPABLE_INVALID;
			}
		}
	}

	if (return_status == 0) {
		//---------------------------------------------------------
		// 2. Apply BSS configuration changes
		//      Now that the provided bss_config_t struct is valid, apply the changes.

		// Disable interrupts around these modifications to prevent state
		// changing out from underneath this context while the new BSS
		// configuration parameters are only partially updated.
		curr_interrupt_state = wlan_mac_high_interrupt_stop();

		if ((bss_config == NULL) || (bss_config->update_mask & BSS_FIELD_MASK_BSSID)) {
			// Adopting a new BSSID. This could mean either
			//    1) Shutting the BSS down
			// or 2) Shutting the BSS down and then starting a new BSS.
			//
			// In either case, first remove any station_info structs
			// that are members of the current active_bss_info and return to
			// a NULL active_bss_info state.
			//
			// This will not result in any OTA transmissions to the stations.

			if (active_bss_info != NULL) {
				// Remove all associations
				next_station_info_entry = active_bss_info->members.first;
				iter = active_bss_info->members.length;

				while ((next_station_info_entry != NULL) && (iter-- > 0)) {
					curr_station_info_entry = next_station_info_entry;
					next_station_info_entry = dl_entry_next(curr_station_info_entry);

					curr_station_info = (station_info_t*)(curr_station_info_entry->data);

					// Purge any data for the station
					purge_queue(STATION_ID_TO_QUEUE_ID(curr_station_info->ID));

					// Remove the association
					curr_station_info->flags &= ~STATION_INFO_FLAG_KEEP;
					station_info_remove(&active_bss_info->members, curr_station_info->addr);

					// Update the hex display to show station was removed
					ibss_update_hex_display(active_bss_info->members.length);
				}

				// Inform the MAC High Framework to no longer will keep this BSS Info. This will
				// allow it to be overwritten in the future to make space for new BSS Infos.
				active_bss_info->flags &= ~BSS_FLAGS_KEEP;

				// Set "active_bss_info" to NULL
				//     - All functions must be able to handle active_bss_info = NULL
				active_bss_info = NULL;

				// Disable beacons immediately
				((tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_BEACON))->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
				gl_beacon_txrx_config.beacon_tx_mode = NO_BEACON_TX;
				bzero(gl_beacon_txrx_config.bssid_match, MAC_ADDR_LEN);
				wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);

				// Set hex display to "No BSS"
				ibss_update_hex_display(0xFF);
			}

			// (bss_config == NULL) is one way to remove the BSS state of the node. This operation
			// was executed just above.  Rather that continuing to check non-NULLness of bss_config
			// throughout the rest of this function, just re-enable interrupts and return early.

			if(bss_config == NULL){
				wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
				return return_status;
			}

			// active_bss_info is guaranteed to be NULL at this point in the code
			// bss_config is guaranteed to be non-NULL at this point in the code

			// Update BSS
			//     - BSSID must not be zero_addr (reserved address)
			if (wlan_addr_eq(bss_config->bssid, zero_addr) == 0) {
				// Stop the scan state machine if it is running
				if (wlan_mac_scan_is_scanning()) {
					wlan_mac_scan_stop();
				}

				// Create a new bss_info or overwrite an existing one with matching BSSID.
				//     Note:  The wildcard SSID and 0-valued channel arguments are temporary. Because
				//         of the error checking at the top of this function, the bss_config will
				//         contain a valid SSID as well as channel. These fields will be updated
				//         in step 3).
				local_bss_info = wlan_mac_high_create_bss_info(bss_config->bssid, "", 0);

				if(local_bss_info != NULL){
					local_bss_info->flags |= BSS_FLAGS_KEEP;
#if WLAN_DEFAULT_USE_HT
					local_bss_info->capabilities = (BSS_CAPABILITIES_IBSS | BSS_CAPABILITIES_HT_CAPABLE);
#else
					local_bss_info->capabilities = (BSS_CAPABILITIES_IBSS);
#endif
					active_bss_info = local_bss_info;
				}

				// Set hex display
				ibss_update_hex_display(active_bss_info->members.length);
			}
		}

		//---------------------------------------------------------
		// 3. Clean up
		//      Now that active_bss_info has been updated, CPU_HIGH can communicate
		//      the changes to CPU_LOW so that the node is tuned to the correct channel,
		//      send beacons at the correct interval, and update the beacon
		//      template packet buffer.
		if (active_bss_info != NULL) {

			if (bss_config->update_mask & BSS_FIELD_MASK_CHAN) {
				active_bss_info->chan_spec = bss_config->chan_spec;
				send_channel_switch_to_low = 1;
				update_beacon_template = 1;
			}
			if (bss_config->update_mask & BSS_FIELD_MASK_SSID) {
				strncpy(active_bss_info->ssid, bss_config->ssid, SSID_LEN_MAX);
				update_beacon_template = 1;
			}
			if (bss_config->update_mask & BSS_FIELD_MASK_BEACON_INTERVAL) {
				active_bss_info->beacon_interval = bss_config->beacon_interval;
				update_beacon_template = 1;
				send_beacon_config_to_low = 1;
			}
			if (bss_config->update_mask & BSS_FIELD_MASK_HT_CAPABLE) {
				// In an IBSS network, the node does not know the HT capabilities of any of
				// the peer nodes in the BSS.  Therefore, when station infos are added to
				// track peer nodes, the node assumes that the peer node's HT capabilities
				// matches that of the BSS.  Given that changing the BSS capabilities does
				// not invalidate that assumption, the IBSS node does not update any station
				// info HT capabilities when the BSS capabilities change.  Therefore,
				// changing the BSS HT_CAPABLE capabilities only affects what is advertised in
				// the IBSS beacons.  Also, it should not change any of the default TX params
				// since the IBSS node is still capable of sending and receiving HT packets.

				if (bss_config->ht_capable) {
					active_bss_info->capabilities |= BSS_CAPABILITIES_HT_CAPABLE;
				} else {
					active_bss_info->capabilities &= ~BSS_CAPABILITIES_HT_CAPABLE;
				}

				// Update the beacon template to match capabilities
				update_beacon_template = 1;
			}

			// Update the beacon template
			//     In the event that CPU_LOW currently has the beacon packet buffer locked,
			//     block for now until it unlocks.  This will guarantee that beacon are updated
			//     before the function returns.
			if (update_beacon_template) {
				wlan_mac_high_setup_tx_header(&tx_header_common, (u8 *)bcast_addr, active_bss_info->bssid);
				while (wlan_mac_high_configure_beacon_tx_template(&tx_header_common, active_bss_info, &default_multicast_mgmt_tx_params, TX_FRAME_INFO_FLAGS_FILL_TIMESTAMP) != 0) {}
			}

			// Update the channel
			if (send_channel_switch_to_low) {
				wlan_mac_high_set_radio_channel(
						wlan_mac_high_bss_channel_spec_to_radio_chan(active_bss_info->chan_spec));
			}

			// Update Beacon configuration
			if (send_beacon_config_to_low) {

				memcpy(gl_beacon_txrx_config.bssid_match, active_bss_info->bssid, MAC_ADDR_LEN);

				if ((active_bss_info->beacon_interval == BEACON_INTERVAL_NO_BEACON_TX) ||
					(active_bss_info->beacon_interval == BEACON_INTERVAL_UNKNOWN)) {
					((tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_BEACON))->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
					gl_beacon_txrx_config.beacon_tx_mode = NO_BEACON_TX;
				} else {
					gl_beacon_txrx_config.beacon_tx_mode = IBSS_BEACON_TX;
				}

				gl_beacon_txrx_config.beacon_interval_tu = active_bss_info->beacon_interval;
				gl_beacon_txrx_config.beacon_template_pkt_buf = TX_PKT_BUF_BEACON;

				wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
			}

			// Print new IBSS information
			xil_printf("IBSS Details: \n");
			xil_printf("  BSSID           : %02x-%02x-%02x-%02x-%02x-%02x\n",active_bss_info->bssid[0],active_bss_info->bssid[1],active_bss_info->bssid[2],active_bss_info->bssid[3],active_bss_info->bssid[4],active_bss_info->bssid[5]);
			xil_printf("   SSID           : %s\n", active_bss_info->ssid);
			xil_printf("   Channel        : %d\n", wlan_mac_high_bss_channel_spec_to_radio_chan(active_bss_info->chan_spec));
			if (active_bss_info->beacon_interval == BEACON_INTERVAL_NO_BEACON_TX) {
				xil_printf("   Beacon Interval: No Beacon Tx\n");
			} else if (active_bss_info->beacon_interval == BEACON_INTERVAL_UNKNOWN) {
				xil_printf("   Beacon Interval: Unknown\n");
			} else {
				xil_printf("   Beacon Interval: %d TU (%d us)\n", active_bss_info->beacon_interval, active_bss_info->beacon_interval*1024);
			}
		}

		// Restore interrupts after all BSS changes
		wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
	}

	return return_status;
}



/*****************************************************************************/
/**
 * @brief Callback to handle beacon MAC time update mode
 *
 * @param  u32 enable        - Enable / Disable MAC time update from beacons
 * @return None
 *
 *****************************************************************************/
void ibss_set_beacon_ts_update_mode(u32 enable){
    if (enable) {
        gl_beacon_txrx_config.ts_update_mode = FUTURE_ONLY_UPDATE;
    } else {
        gl_beacon_txrx_config.ts_update_mode = NEVER_UPDATE;
    }

    // Push beacon configuration to CPU_LOW
    wlan_mac_high_config_txrx_beacon(&gl_beacon_txrx_config);
}



/*****************************************************************************/
/**
 * @brief Accessor methods for global variables
 *
 * These functions will return pointers to global variables
 *
 * @param  None
 * @return None
 *****************************************************************************/
dl_list * get_bss_member_list(){
	if(active_bss_info != NULL){
		return &(active_bss_info->members);
	} else {
		return NULL;
	}
}

u8         * get_wlan_mac_addr()     { return (u8 *)&wlan_mac_addr;      }
bss_info_t * active_bss_info_getter(){ return active_bss_info; }



/*****************************************************************************/
/**
 * @brief IBSS specific hex display update command
 *
 * This function update the hex display for the IBSS.  In general, this function
 * is a wrapper for standard hex display commands found in wlan_mac_misc_util.c.
 * However, this wrapper was implemented so that it would be easy to do other
 * actions when the IBSS needed to update the hex display.
 *
 * @param   val              - Value to be displayed (between 0 and 99)
 * @return  None
 *****************************************************************************/
void ibss_update_hex_display(u8 val) {

    // Use standard hex display write
    write_hex_display(val);
}



#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

// ****************************************************************************
// Define MAC Specific User Commands
//
// NOTE:  All User Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// MAC Specific User Commands
//
// #define CMDID_USER_<COMMAND_NAME>                       0x100000


//-----------------------------------------------
// MAC Specific User Command Parameters
//
// #define CMD_PARAM_USER_<PARAMETER_NAME>                 0x00000000



/*****************************************************************************/
/**
 * Process User Commands
 *
 * This function is part of the WLAN Exp framework and will process the framework-
 * level user commands.  This function intentionally does not implement any user
 * commands and it is left to the user to implement any needed functionality.   By
 * default, any commands not processed in this function will print an error to the
 * UART.
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   from             - Pointer to socket address structure (struct sockaddr *) where command is from
 * @param   command          - Pointer to Command
 * @param   response         - Pointer to Response
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *                                 RESP_SENT    - A response has been sent
 *
 * @note    See on-line documentation for more information:
 *          https://warpproject.org/trac/wiki/802.11/wlan_exp/Extending
 *
 *****************************************************************************/
int wlan_exp_process_user_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len) {

    //
    // IMPORTANT ENDIAN NOTES:
    //     - command
    //         - header - Already endian swapped by the framework (safe to access directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the command)
    //     - response
    //         - header - Will be endian swapped by the framework (safe to write directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the response)
    //

    // Standard variables
    //
    // Used for accessing command arguments and constructing the command response header/payload
    //
    // NOTE:  Some of the standard variables below have been commented out.  This was to remove
    //     compiler warnings for "unused variables" since the default implementation is empty.  As
    //     you add commands, you should un-comment the standard variables.
    //
    u32                 resp_sent      = NO_RESP_SENT;

#if 0
    cmd_resp_hdr      * cmd_hdr        = command->header;
    cmd_resp_hdr      * resp_hdr       = response->header;

    u32               * cmd_args_32    = command->args;
    u32               * resp_args_32   = response->args;

    u32                 resp_index     = 0;
#endif

    switch(cmd_id){

//-----------------------------------------------------------------------------
// MAC Specific User Commands
//-----------------------------------------------------------------------------

        // Template framework for a Command
        //
        // NOTE:  The WLAN Exp framework assumes that the Over-the-Wire format of the data is
        //     big endian.  However, the node processes data using little endian.  Therefore,
        //     any data received from the host must be properly endian-swapped and similarly,
        //     any data sent to the host must be properly endian-swapped.  The built-in Xilinx
        //     functions:  Xil_Ntohl() (Network to Host) and Xil_Htonl() (Host to Network) are
        //     used for this.
        //
#if 0
        //---------------------------------------------------------------------
        case CMDID_USER_<COMMAND_NAME>: {
            // Command Description
            //
            // Message format:
            //     cmd_args_32[0:N]    Document command arguments from the host
            //
            // Response format:
            //     resp_args_32[0:M]   Document response arguments from the node
            //
            // NOTE:  Variables are declared above.
            // NOTE:  Please take care of the endianness of the arguments (see comment above)
            //

            // Variables for template command
            int                 status;
            u32                 arg_0;
            interrupt_state_t   curr_interrupt_state;

            // Initialize variables
            status      = CMD_PARAM_SUCCESS;
            arg_0       = Xil_Ntohl(cmd_args_32[0]);              // Swap endianness of command argument

            // Do something with argument(s)
            xil_printf("Command argument 0: 0x%08x\n", arg_0);

            // If necessary, disable interrupts before processing the command
            //  Interrupts must be disabled if the command implementation relies on any state that might
            //  change during an interrupt service routine. See the user guide for more details
            //  https://warpproject.org/trac/wiki/802.11/wlan_exp/Extending
            curr_interrupt_state = wlan_mac_high_interrupt_stop();

            // Process command arguments and generate any response payload
            //     NOTE:  If interrupts were disabled above, take care to avoid any long-running code in this
            //         block (i.e. avoid xil_printf()). When interrupts are disabled, CPU High is unable to
            //         respond to CPU Low (ie CPU High will not send / receive packets) and execute scheduled
            //         tasks, such as LTGs.

            // Re-enable interrupts before returning (only do this if wlan_mac_high_interrupt_stop() is called above)
            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);

            // Send response
            //   NOTE:  It is good practice to send a status as the first argument of the response.
            //       This way it is easy to determine if the other data in the response is valid.
            //       Predefined status values are:  CMD_PARAM_SUCCESS, CMD_PARAM_ERROR
            //
            resp_args_32[resp_index++] = Xil_Htonl(status);       // Swap endianness of response arguments

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;
#endif


        //---------------------------------------------------------------------
        default: {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown IBSS user command: 0x%x\n", cmd_id);
        }
        break;
    }

    return resp_sent;
}

#endif

