/** @file wlan_exp_node_sta.h
 *  @brief Station WARPNet Experiment
 *
 *  This contains code for the 802.11 Station's WARPNet experiment interface.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */


/***************************** Include Files *********************************/
#include "wlan_exp_common.h"



/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_STA_H_
#define WLAN_EXP_NODE_STA_H_



// ****************************************************************************
// Define WLAN Exp Node Station Commands
//
#define CMDID_NODE_STA_CONFIG                              0x200000



#define CMD_PARAM_NODE_STA_BEACON_TS_UPDATE                0x00000001


// ****************************************************************************
// Define Node Station Parameters
//   - NOTE:  To add another parameter, add the define before "NODE_MAX_PARAMETER"
//     and then change the value of "NODE_MAX_PARAMETER" to be the largest value
//     in the list so it is easy to iterate over all parameters
//


/*********************** Global Structure Definitions ************************/



/*************************** Function Prototypes *****************************/

int wlan_exp_node_sta_init( u32 type, u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *hw_addr );

int wlan_exp_node_sta_processCmd( unsigned int cmdID, const wn_cmdHdr* cmdHdr, void* cmdArgs, wn_respHdr* respHdr, void* respArgs, void* pktSrc, unsigned int eth_dev_num);


#endif /* WLAN_EXP_NODE_H_ */
