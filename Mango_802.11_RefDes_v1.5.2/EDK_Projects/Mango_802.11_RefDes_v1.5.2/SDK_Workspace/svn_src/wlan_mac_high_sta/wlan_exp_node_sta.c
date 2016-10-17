/** @file wlan_exp_node_sta.c
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
#include "wlan_exp_node.h"
#include "wlan_exp_node_sta.h"

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

// Xilinx includes
#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>
#include "xintc.h"


// Library includes
#include "string.h"
#include "stdlib.h"

// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_sta_join.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_sta.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/
extern dl_list                    association_table;

extern u8                         my_aid;

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

                // Note: Interrupts are currently disabled

                // Stop any scan / join in progress
                wlan_mac_sta_join_return_to_idle();

                // STA disassociate command is the same for an individual AP or ALL
                sta_disassociate();

                // Set "my_bss_info" to NULL
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


        //---------------------------------------------------------------------
        case CMDID_NODE_DISASSOCIATE: {
            // Disassociate from the AP
            //
            // The MAC address argument is ignored and the STA will always disassociate
            // from the AP when this command is used.
            //
            // Message format:
            //     cmd_args_32[0:1]      MAC Address (All 0xFF means all station info)
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            interrupt_state_t     prev_interrupt_state;
            u32                   status         = CMD_PARAM_SUCCESS;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociate\n");

            // Stop the join state machine if it is running
            if (wlan_mac_sta_is_joining()) { wlan_mac_sta_join_return_to_idle(); }

            // Stop the scan state machine if it is running
            if (wlan_mac_scan_is_scanning()) { wlan_mac_scan_stop(); }

            // Disable interrupts so no packets interrupt the disassociate
            prev_interrupt_state = wlan_mac_high_interrupt_stop();

            // Disassociate STA
            status = sta_disassociate();

            // Re-enable interrupts
            wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

            // Set return parameters and print info to console
            if (status == 0) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociated from AP\n");
                status = CMD_PARAM_SUCCESS;
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not disassociate from AP\n");
                status = CMD_PARAM_ERROR;
            }


            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// STA Specific Commands
//-----------------------------------------------------------------------------

        //---------------------------------------------------------------------
        case CMDID_NODE_STA_SET_AID: {
            // Set the AID
            //
            // Message format:
            //     cmd_args_32[0]   AID
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            u32 status       = CMD_PARAM_SUCCESS;
            u8  aid          = (Xil_Ntohl(cmd_args_32[0]) & 0xFF);

            // Set STA global AID field
            my_aid = aid;

            // Update the hex display with the new AID
            sta_update_hex_display(my_aid);

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_STA_JOIN: {
            // Join the given BSS
            //
            // Message format:
            //     cmd_args_32[0:1] BSS ID (or CMD_PARAM_RSVD_MAC_ADDR if BSSID not set)
            //     cmd_args_32[2]   Channel
            //     cmd_args_32[3]   SSID Length
            //     cmd_args_32[4:N] SSID (packed array of ascii character values)
            //                          NOTE: The characters are copied using str functions
            //                              and must be correctly processed on the host side
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            u32                             status                   = CMD_PARAM_SUCCESS;
            u8                              bssid[MAC_ADDR_LEN];
            u8                              channel                  = (Xil_Ntohl(cmd_args_32[2]) & 0xFF);
            u32                             ssid_length              = Xil_Ntohl(cmd_args_32[3]);
            char *                          ssid                     = (char *)&cmd_args_32[4];
            volatile join_parameters_t*     join_parameters;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Joining the BSS\n");

            // Get the BSSID
            wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[0], &bssid[0]);

            // Get the current join parameters
            join_parameters = wlan_mac_sta_get_join_parameters();

            // Free the current join SSID, it will be replaced
            if (join_parameters->ssid != NULL) {
                wlan_mac_high_free(join_parameters->ssid);
            }

            // SSID was "None", therefore we need to stop the join process
            if (ssid_length == CMD_PARAM_RSVD) {
                // Set the SSID to NULL
                join_parameters->ssid = NULL;
            } else {
                // Set the SSID
                join_parameters->ssid = strndup(ssid, SSID_LEN_MAX);

                // Set the BSSID (reserved value is all zeros)
                //     - Do not need to check if the value is all zero since that will be done
                //       in the join function itself
                memcpy((void *)join_parameters->bssid, bssid, MAC_ADDR_LEN);

                // Set the channel (reserved value is zero)
                //     - Do not need to check if the value is all zero since that will be done
                //       in the join function itself
                join_parameters->channel = channel;
            }

            // Call join function
            wlan_mac_sta_join();

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_STA_JOIN_STATUS: {
            // Get the current wlan_mac_is_joining() status
            //
            // Message format:
            //
            // Response format:
            //     resp_args_32[0]  Status
            //     resp_args_32[1]  Is Joining?  (0 - No / 1 - Yes)
            //                      Failure (CMD_PARAM_NODE_JOIN_FAILED)
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    is_joining     = wlan_mac_sta_is_joining();

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(is_joining);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        default: {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown node command: 0x%x\n", cmd_id);
        }
        break;
    }

    return resp_sent;
}



#endif
