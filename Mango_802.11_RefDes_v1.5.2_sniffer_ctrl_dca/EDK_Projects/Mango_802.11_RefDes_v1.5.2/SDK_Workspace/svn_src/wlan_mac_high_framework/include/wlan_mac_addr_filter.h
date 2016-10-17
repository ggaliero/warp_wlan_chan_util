/** @file wlan_mac_addr_mac_filter.h
 *  @brief Address Filter
 *
 *  This contains code for the filtering MAC addresses
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_ADDR_FILTER_H_
#define WLAN_MAC_ADDR_FILTER_H_

#include "wlan_mac_high_sw_config.h"

#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_common.h"


//-----------------------------------------------
// Address Filter defines
//
#define ADDR_FILTER_ADDR_NOT_ALLOWED                       0
#define ADDR_FILTER_ADDR_ALLOWED                           1



/*********************** Global Structure Definitions ************************/

// **********************************************************************
// Whitelist Range Structure
//
typedef struct {

	u8   mask[MAC_ADDR_LEN];
	u8   compare[MAC_ADDR_LEN];

} whitelist_range;



/*************************** Function Prototypes *****************************/

void  wlan_mac_addr_filter_init();
void  wlan_mac_addr_filter_reset();

int   wlan_mac_addr_filter_add(u8* mask, u8* compare);

u8    wlan_mac_addr_filter_is_allowed(u8* addr);
u8    wlan_mac_addr_is_warp(u8* addr);

#endif /* WLAN_MAC_ADDR_FILTER_H_ */




