/** @file wlan_exp_node_ap.h
 *  @brief Access Point WLAN Experiment
 *
 *  This contains code for the 802.11 Access Point's WLAN experiment interface.
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
#ifndef WLAN_EXP_NODE_AP_H_
#define WLAN_EXP_NODE_AP_H_



// ****************************************************************************
// Define Node Commands
//
// NOTE:  All Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// WLAN Exp Node AP Commands
//
#define CMDID_NODE_AP_CONFIG                                         0x100000
#define CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER                 0x100001

#define CMD_PARAM_NODE_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER          0x00000001

#define CMD_PARAM_AP_ASSOCIATE_FLAG_DISABLE_INACTIVITY_TIMEOUT       0x00000001
#define CMD_PARAM_AP_ASSOCIATE_FLAG_HT_CAPABLE_STA                   0x00000004

// ****************************************************************************
// Define Node AP Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//


/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int wlan_exp_process_node_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);


#endif /* WLAN_EXP_NODE_H_ */
