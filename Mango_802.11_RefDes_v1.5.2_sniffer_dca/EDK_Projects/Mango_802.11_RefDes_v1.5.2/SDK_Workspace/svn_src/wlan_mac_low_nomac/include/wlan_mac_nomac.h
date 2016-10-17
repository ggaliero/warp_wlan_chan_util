/** @file wlan_mac_nomac.c
 *  @brief Simple MAC that does nothing but transmit and receive
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_NOMAC_H_
#define WLAN_MAC_NOMAC_H_


//-----------------------------------------------
// WLAN Exp low parameter defines (NOMAC)
//     NOTE:  Need to make sure that these values do not conflict with any of the LOW PARAM
//     callback defines
//
// #define LOW_PARAM_NOMAC_                                  0x20000001


/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/
int  main();

int  frame_transmit(u8 pkt_buf, wlan_mac_low_tx_details_t* low_tx_details);
u32  frame_receive(u8 rx_pkt_buf, phy_rx_details_t* phy_details);

#endif /* WLAN_MAC_NOMAC_H_ */
