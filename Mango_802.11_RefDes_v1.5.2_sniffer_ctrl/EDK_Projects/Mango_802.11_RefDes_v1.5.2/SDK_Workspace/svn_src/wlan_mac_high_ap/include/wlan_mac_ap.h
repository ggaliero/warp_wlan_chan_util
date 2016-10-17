/** @file wlan_mac_ap.h
 *  @brief Access Point
 *
 *  This contains code for the 802.11 Access Point.
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

#include "wlan_mac_mgmt_tags.h"
#include "wlan_mac_scan.h"
#include "wlan_mac_station_info.h"

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_AP_H_
#define WLAN_MAC_AP_H_


//-----------------------------------------------
// Enable the WLAN UART Menu
#define WLAN_USE_UART_MENU


//-----------------------------------------------
// Allow Ethernet transmission of packets received by an associated station
// destined for another associated station
//#define ALLOW_ETH_TX_OF_WIRELESS_TX


//-----------------------------------------------
// Common Defines
#define MAX_TX_QUEUE_LEN                                   150       /// Maximum number of entries in any Tx queue

#define MAX_NUM_ASSOC                                      10        /// Maximum number of associations allowed
#define MAX_NUM_AUTH                                       10        /// Maximum number of authentications allowed
CASSERT( (MAX_NUM_ASSOC ) <= STATION_INFO_DL_ENTRY_MEM_NUM, insufficient_WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO_for_max_associations );


//-----------------------------------------------
// Tx queue IDs
#define MCAST_QID                                          0
#define MANAGEMENT_QID                                     1
#define STATION_ID_TO_QUEUE_ID(x)                        ((x) + 1)   /// Map association ID to Tx queue ID; min AID is 1
#define QID_TO_AID(x)                                    ((x) - 1)


//-----------------------------------------------
// Timing Parameters

// Period for checking association table for stale associations
#define ASSOCIATION_CHECK_INTERVAL_MS                     (1000)
#define ASSOCIATION_CHECK_INTERVAL_US                     (ASSOCIATION_CHECK_INTERVAL_MS * 1000)

// Timeout for last reception for an association; timed-out associations are subject to de-association
#define ASSOCIATION_TIMEOUT_S                             (300)
#define ASSOCIATION_TIMEOUT_US                            (ASSOCIATION_TIMEOUT_S * 1000000)

// Interval to allow associations after entering ASSOCIATION_ALLOW_TEMPORARY mode
#define ASSOCIATION_ALLOW_INTERVAL_MS                     (30000)
#define ASSOCIATION_ALLOW_INTERVAL_US                     (ASSOCIATION_ALLOW_INTERVAL_MS * 1000)

// Blinking period for hex displays, when used to show association mode
#define ANIMATION_RATE_US                                 (100000)


/*********************** Global Structure Definitions ************************/

typedef struct{
	u8  enable;
	u8  dtim_period;
	u8  dtim_count;
	u8  dtim_mcast_buffer_enable;
	u64 dtim_timestamp;
	u64 dtim_mcast_allow_window;
} ps_conf;

/*************************** Function Prototypes *****************************/
int  main();

void ltg_event(u32 id, void* callback_arg);

int  ethernet_receive(tx_queue_element_t* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length);

u32 					mpdu_rx_process(void* pkt_buf_addr, station_info_t* station_info, rx_common_entry* rx_event_log_entry);
bss_info_t* 			active_bss_info_getter();
void 					process_scan_state_change(scan_state_t scan_state);
void 					set_power_save_configuration(ps_conf power_save_configuration);
volatile ps_conf* 		get_power_save_configuration();

void queue_state_change(u32 QID, u8 queue_len);
inline void update_tim_tag_aid(u8 aid, u8 bit_val_in);
void update_tim_tag_all(u32 sched_id);

void beacon_transmit_done( tx_frame_info_t* tx_frame_info, wlan_mac_low_tx_details_t* tx_low_details, tx_low_entry* tx_low_event_log_entry );

void poll_tx_queues();
void purge_all_data_tx_queue();

void enable_associations();
void disable_associations();
void remove_inactive_station_infos();
void association_timestamp_adjust(s64 timestamp_diff);

u32  deauthenticate_station( station_info_t* station_info );
void deauthenticate_all_stations();
void handle_cpu_low_reboot();
u32  configure_bss(bss_config_t* bss_config);
void mpdu_dequeue(tx_queue_element_t* packet);

void up_button();

void uart_rx(u8 rxByte);

void ap_update_hex_display(u8 val);

#endif /* WLAN_MAC_AP_H_ */
