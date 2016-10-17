/** @file wlan_mac_sysmon_util.h
 *  @brief System Monitor Definitions
 *
 *  This contains system monitor definitions of required by both CPU_HIGH and
 *  CPU_LOW.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_SYSMON_UTIL_H_
#define WLAN_MAC_SYSMON_UTIL_H_


/*********************** Global Structure Definitions ************************/

/*************************** Function Prototypes *****************************/
void               init_sysmon();

u32                get_current_temp();
u32                get_min_temp();
u32                get_max_temp();


#endif
