/** @file wlan_exp_node_sta.c
 *  @brief Station WLAN Experiment
 *
 *  This contains code for the 802.11 Station's WLAN experiment interface.
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

#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ibss.h"

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

// Xilinx includes
#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>

// Library includes
#include "string.h"
#include "stdlib.h"

// WLAN includes
#include "wlan_mac_event_log.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_ibss.h"
#include "wlan_mac_station_info.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/
extern tx_params_t                default_unicast_data_tx_params;
extern bss_info_t*                active_bss_info;

extern wlan_exp_function_ptr_t    wlan_exp_purge_all_data_tx_queue_callback;


/*************************** Variable Definitions ****************************/


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Process Node Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various node related commands.
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   from             - Pointer to socket address structure (struct sockaddr *) where command is from
 * @param   command          - Pointer to Command
 * @param   response         - Pointer to Response
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *                                 RESP_SENT    - A response has been sent
 *
 * @note    See on-line documentation for more information about the Ethernet
 *          packet structure:  www.warpproject.org
 *
 *****************************************************************************/
int wlan_exp_process_node_cmd(u32 cmd_id, int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len) {

    //
    // IMPORTANT ENDIAN NOTES:
    //     - command
    //         - header - Already endian swapped by the framework (safe to access directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the command)
    //     - response
    //         - header - Will be endian swapped by the framework (safe to write directly)
    //         - args   - Must be endian swapped as necessary by code (framework does not know the contents of the response)
    //

    // Standard variables
    u32                 resp_sent      = NO_RESP_SENT;

    u32               * cmd_args_32    = command->args;

    cmd_resp_hdr      * resp_hdr       = response->header;
    u32               * resp_args_32   = response->args;
    u32                 resp_index     = 0;

    //
    // NOTE: Response header cmd, length, and num_args fields have already been initialized.
    //

    switch(cmd_id){

//-----------------------------------------------------------------------------
// WLAN Exp Node Commands that must be implemented in child classes
//-----------------------------------------------------------------------------

        //---------------------------------------------------------------------
        case CMDID_NODE_RESET_STATE: {
            // NODE_RESET_STATE Packet Format:
            //   - cmd_args_32[0]  - Flags
            //                     [0] - NODE_RESET_LOG
            //                     [1] - NODE_RESET_TXRX_COUNTS
            //                     [2] - NODE_RESET_LTG
            //                     [3] - NODE_RESET_TX_DATA_QUEUE
            //                     [4] - NODE_RESET_ASSOCIATIONS
            //                     [5] - NODE_RESET_BSS_INFO
            //
            interrupt_state_t     prev_interrupt_state;
            u32                   status    = CMD_PARAM_SUCCESS;
            u32                   flags     = Xil_Ntohl(cmd_args_32[0]);

            // Disable interrupts so no packets interrupt the reset
            prev_interrupt_state = wlan_mac_high_interrupt_stop();
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            // Configure the LOG based on the flag bits
            if (flags & CMD_PARAM_NODE_RESET_FLAG_LOG) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_event_log, "Reset log\n");
                event_log_reset();
            }
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
            if (flags & CMD_PARAM_NODE_RESET_FLAG_TXRX_COUNTS) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_counts, "Reseting Counts\n");
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
                txrx_counts_zero_all();
#endif
            }

#if WLAN_SW_CONFIG_ENABLE_LTG
            if (flags & CMD_PARAM_NODE_RESET_FLAG_LTG) {
                status = ltg_sched_remove(LTG_REMOVE_ALL);

                if (status != 0) {
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to remove all LTGs\n");
                    status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Removing All LTGs\n");
                }
            }
#endif //WLAN_SW_CONFIG_ENABLE_LTG

            if (flags & CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_queue, "Purging all data transmit queues\n");
                wlan_exp_purge_all_data_tx_queue_callback();
            }

            if (flags & CMD_PARAM_NODE_RESET_FLAG_BSS) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Resetting BSS\n");

                // Set "active_bss_info" to NULL
                configure_bss(NULL);
            }

            if (flags & CMD_PARAM_NODE_RESET_FLAG_NETWORK_LIST) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Resetting Network List\n");
                wlan_mac_high_reset_network_list();
            }

            // Call MAC specific reset with the flags

            // Re-enable interrupts
            wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

            // Send response of success
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// IBSS Specific Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        default: {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown node command: 0x%x\n", cmd_id);
        }
        break;
    }

    return resp_sent;
}

/*****************************************************************************/
/**
 * Used by wlan_exp_cmd_add_association_callback in wlan_exp_node.c
 *
 * @param   mac_addr         - Pointer to MAC address association that will be added
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_exp_ibss_tx_cmd_add_association(u8* mac_addr) {

    if (active_bss_info != NULL) {
        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Adding association for:  ");
        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, mac_addr); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

        // Add station info
        //     - Set ht_capable argument to the HT_CAPABLE capability of the BSS.  Given that the node does not know
        //       the HT capabilities of the new station, it is reasonable to assume that they are the same as the BSS.
        //
        station_info_add(&(active_bss_info->members), mac_addr, ADD_STATION_INFO_ANY_ID, &default_unicast_data_tx_params,
                                       (active_bss_info->capabilities & BSS_CAPABILITIES_HT_CAPABLE));
    }
}


#endif
