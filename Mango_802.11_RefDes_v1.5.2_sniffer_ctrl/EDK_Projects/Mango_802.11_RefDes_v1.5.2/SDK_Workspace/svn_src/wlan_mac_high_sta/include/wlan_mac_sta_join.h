/** @file wlan_mac_sta_join.h
 *  @brief Join FSM
 *
 *  This contains code for the STA join process.
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


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_STA_JOIN_H_
#define WLAN_MAC_STA_JOIN_H_

// Join Timing Parameters
//     These defines set the join timing parameters at boot.
//
#define BSS_SEARCH_POLL_INTERVAL_USEC                      10000
#define BSS_ATTEMPT_POLL_INTERVAL_USEC                     50000


/*********************** Global Structure Definitions ************************/

typedef struct {
    char    * ssid;                    // Mandatory: SSID string
    u8        bssid[MAC_ADDR_LEN];     // Optional: BSSID address, 00-00-00-00-00-00 interpreted as "ignore"
    u8        channel;                 // Optional: Channel number, 0 interpreted as "ignore"
} join_parameters_t;


/*************************** Function Prototypes *****************************/

int  wlan_mac_sta_join_init();

void wlan_mac_sta_set_join_success_callback(function_ptr_t callback);

volatile join_parameters_t* wlan_mac_sta_get_join_parameters();
volatile bss_info_t* wlan_mac_sta_get_attempt_bss_info();

void wlan_mac_sta_join();
void wlan_mac_sta_join_return_to_idle();

void wlan_mac_sta_join_bss_attempt_poll(u32 aid);

u32  wlan_mac_sta_is_joining();
void wlan_mac_sta_successfully_authenticated(u8* bssid);
void wlan_mac_sta_successfully_associated(u8* bssid, u16 AID);

#endif
