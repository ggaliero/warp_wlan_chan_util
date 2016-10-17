/** @file wlan_exp_node_sta.h
 *  @brief Station WLAN Experiment
 *
 *  This contains code for the 802.11 Station's WLAN experiment interface.
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

#include "wlan_exp_common.h"



/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_STA_H_
#define WLAN_EXP_NODE_STA_H_



// ****************************************************************************
// Define WLAN Exp Node Station Commands
//
#define CMDID_NODE_STA_JOIN                                0x100000
#define CMDID_NODE_STA_JOIN_STATUS                         0x100001
#define CMDID_NODE_STA_SET_AID                             0x100002



// ****************************************************************************
// Define Node Station Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//


/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int  wlan_exp_process_node_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);


#endif /* WLAN_EXP_NODE_H_ */
