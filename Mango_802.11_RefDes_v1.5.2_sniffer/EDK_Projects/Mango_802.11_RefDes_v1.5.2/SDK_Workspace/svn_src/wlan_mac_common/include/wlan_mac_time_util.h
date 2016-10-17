/** @file wlan_mac_time_util.h
 *  @brief Time Definitions
 *
 *  This contains time definitions of required by both CPU_HIGH and CPU_LOW.
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
#ifndef WLAN_MAC_TIME_UTIL_H_
#define WLAN_MAC_TIME_UTIL_H_


//-----------------------------------------------
// WLAN MAC TIME HW register definitions
//
// RO:
#define WLAN_MAC_TIME_REG_SYSTEM_TIME_MSB                  XPAR_WLAN_MAC_TIME_HW_MEMMAP_SYSTEM_TIME_USEC_MSB
#define WLAN_MAC_TIME_REG_SYSTEM_TIME_LSB                  XPAR_WLAN_MAC_TIME_HW_MEMMAP_SYSTEM_TIME_USEC_LSB
#define WLAN_MAC_TIME_REG_MAC_TIME_MSB                     XPAR_WLAN_MAC_TIME_HW_MEMMAP_MAC_TIME_USEC_MSB
#define WLAN_MAC_TIME_REG_MAC_TIME_LSB                     XPAR_WLAN_MAC_TIME_HW_MEMMAP_MAC_TIME_USEC_LSB

// RW:
#define WLAN_MAC_TIME_REG_NEW_MAC_TIME_MSB                 XPAR_WLAN_MAC_TIME_HW_MEMMAP_NEW_MAC_TIME_MSB
#define WLAN_MAC_TIME_REG_NEW_MAC_TIME_LSB                 XPAR_WLAN_MAC_TIME_HW_MEMMAP_NEW_MAC_TIME_LSB
#define WLAN_MAC_TIME_REG_CONTROL                          XPAR_WLAN_MAC_TIME_HW_MEMMAP_CONTROL

// Control reg masks
#define WLAN_MAC_TIME_CTRL_REG_RESET_SYSTEM_TIME           0x00000001
#define WLAN_MAC_TIME_CTRL_REG_UPDATE_MAC_TIME             0x00000002


/*********************** Global Structure Definitions ************************/

/*************************** Function Prototypes *****************************/
u64                get_mac_time_usec();
void               set_mac_time_usec(u64 new_time);
void               apply_mac_time_delta_usec(s64 time_delta);
u64                get_system_time_usec();
void               usleep(u64 delay);



#endif
