/** @file wlan_mac_sta.h
 *  @brief Station
 *
 *  This contains code for the 802.11 Station.
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

#include "wlan_mac_high.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_scan.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_entries.h"



/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_STA_H_
#define WLAN_MAC_STA_H_


//-----------------------------------------------
// Enable the WLAN UART Menu
#define WLAN_USE_UART_MENU


//-----------------------------------------------
// Common Defines

#define MAX_TX_QUEUE_LEN                                   150       /// Max number of entries in any Tx queue
#define MAX_NUM_ASSOC                                      1         /// Max number of associations the STA will attempt


//-----------------------------------------------
// Tx queue IDs
#define MCAST_QID                                          0
#define MANAGEMENT_QID                                     1
#define UNICAST_QID                                        2

//-----------------------------------------------
// Channels used in dual channel mode
//#define WLAN_SET_SEC_CHAN(x) (( x == WLAN_MAC_PRY_CHAN) ? )
#define WLAN_MAC_PRY_CHAN									36
#define WLAN_MAC_SEC_CHAN									48

//-----------------------------------------------
// Timing Parameters

// Timeout for last reception from the AP association
#define AP_ASSOCIATION_TIMEOUT_MS						   2000
#define AP_ASSOCIATION_TIMEOUT_US						   (AP_ASSOCIATION_TIMEOUT_MS * 1000)

// Time interval to check the existence of AP association
#define ASSOCIATION_CHECK_INTERVAL_MS					   3000
#define ASSOCIATION_CHECK_INTERVAL_US                      (ASSOCIATION_CHECK_INTERVAL_MS * 1000)

// Timeout for association request-response handshake
#define ASSOCIATION_TIMEOUT_US                             100000
#define ASSOCIATION_NUM_TRYS                               5

// Timeout for authentication handshake
#define AUTHENTICATION_TIMEOUT_US                          100000
#define AUTHENTICATION_NUM_TRYS                            5

// Number of probe requests to send per channel when active scanning
#define NUM_PROBE_REQ                                      5

// Time the active scan procedure will dwell on each channel before
// moving to the next channel (microseconds)
//
#define ACTIVE_SCAN_DWELL                                  100000

// The amount of time between full active scans when looking for a particular SSID
//
// NOTE: This value must be larger than the maximum amount of time it takes for
//     a single active scan. For an active scan over 11 channels, this value must be larger
//     than 11*ACTIVE_SCAN_DWELL.
//
#define ACTIVE_SCAN_UPDATE_RATE                            5000000


/*************************** Function Prototypes *****************************/
int  main();

void ltg_event(u32 id, void* callback_arg);

int  ethernet_receive(tx_queue_element_t* curr_tx_queue_element, u8* eth_dest, u8* eth_src, u16 tx_length);

u32 mpdu_rx_process(void* pkt_buf_addr, station_info_t* station_info, rx_common_entry* rx_event_log_entry);
void send_probe_req();
void process_scan_state_change(scan_state_t scan_state);
void poll_tx_queues();
void purge_all_data_tx_queue();
bss_info_t * 	   active_bss_info_getter();

int  sta_disassociate();
u32  configure_bss(bss_config_t* bss_config);

//Gabri
u8   is_valid_access_point_ssid();
void sta_associate_ssid();
void sta_associate();
void check_ap_association();
void start_periodic_check_association();
void stop_periodic_check_association();
//

void up_button();
void uart_rx(u8 rxByte);

void sta_update_hex_display(u8 val);

#endif /* WLAN_MAC_STA_H_ */
