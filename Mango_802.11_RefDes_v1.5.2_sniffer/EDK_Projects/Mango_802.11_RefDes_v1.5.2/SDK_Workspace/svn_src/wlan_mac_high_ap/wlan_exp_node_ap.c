/** @file wlan_exp_node_ap.c
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
#include "wlan_exp.h"
#include "wlan_mac_high.h"
#include "wlan_mac_entries.h"
#include "wlan_exp_node.h"
#include "wlan_exp_node_ap.h"

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
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_addr_filter.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_ap.h"
#include "wlan_mac_bss_info.h"
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
#endif //WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
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
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Resetting BSS \n");

                // Note: Interrupts are currently disabled

                // Deauthenticate all stations
                //     - This will send deauthentication packets to each Station
                deauthenticate_all_stations();

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


        //---------------------------------------------------------------------
        case CMDID_NODE_DISASSOCIATE: {
            // Disassociate device from node
            //
            // Message format:
            //     cmd_args_32[0:1]      MAC Address (All 0xFF means all station info)
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            u32                   id;
            u8                    mac_addr[MAC_ADDR_LEN];
            dl_entry            * curr_entry;
            station_info_t      * curr_station_info;
            interrupt_state_t     prev_interrupt_state;
            u32                   status         = CMD_PARAM_SUCCESS;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociate\n");

            // Get MAC Address
            wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[0], &mac_addr[0]);
            id = wlan_exp_get_id_in_associated_stations(&mac_addr[0]);

            if (id == WLAN_EXP_AID_NONE) {
                // If we cannot find the MAC address, return status error
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not find specified node: ");
                wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                status = CMD_PARAM_ERROR;

            } else {
                // If parameter is not the magic number to disassociate all stations
                if (id != WLAN_EXP_AID_ALL) {
                    // Find the station_info entry
                    curr_entry = station_info_find_by_addr( &mac_addr[0], get_bss_member_list() );

                    if (curr_entry != NULL) {
                        curr_station_info = (station_info_t*)(curr_entry->data);

                        // Disable interrupts so no packets interrupt the disassociate
                        prev_interrupt_state = wlan_mac_high_interrupt_stop();

                        // Deauthenticate station
                        deauthenticate_station(curr_station_info);

                        // Re-enable interrupts
                        wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

                        // Set return parameters and print info to console
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociated node: ");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                    } else {
                        // If we cannot find the MAC address, return status error
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not find specified node: ");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                        status = CMD_PARAM_ERROR;
                    }
                } else {
                    // Disable interrupts so no packets interrupt the disassociate
                    prev_interrupt_state = wlan_mac_high_interrupt_stop();

                    // Deauthenticate all stations
                    deauthenticate_all_stations();

                    // Re-enable interrupts
                    wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

                    // Set return parameters and print info to console
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disassociated node: ");
                    wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");
                }
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// AP Specific Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_CONFIG: {
            // Set AP configuration flags
            //
            // Message format:
            //     cmd_args_32[0]   Flags
            //                          [ 0] - NODE_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFERING
            //     cmd_args_32[1]   Mask for flags
            //
            // Response format:
            //     resp_args_32[0]  Status (CMD_PARAM_SUCCESS/CMD_PARAM_ERROR)
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    flags          = Xil_Ntohl(cmd_args_32[0]);
            u32    mask           = Xil_Ntohl(cmd_args_32[1]);

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "AP: Configure flags = 0x%08x  mask = 0x%08x\n", flags, mask);

            // Configure based on the flag bit / mask
            if (mask & CMD_PARAM_NODE_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER) {
                volatile ps_conf * current_params;
                ps_conf   new_params;

                // Get the current PS config
                current_params = get_power_save_configuration();

                // Copy PS config
                memcpy((void *)&new_params, (void *)current_params, sizeof(ps_conf));

                // Update enable based on command
                if (flags & CMD_PARAM_NODE_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER) {
                    new_params.dtim_mcast_buffer_enable = 1;
                } else {
                    new_params.dtim_mcast_buffer_enable = 0;
                }

                // Update PS config
                set_power_save_configuration(new_params);
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER: {
            // Allow / Disallow wireless authentications
            //
            // Message format:
            //     cmd_args_32[0]   Command:
            //                          - Write       (CMD_PARAM_WRITE_VAL)
            //     cmd_args_32[1]   Number of address filters
            //     cmd_args_32[2:N] [Compare address (u64), (Mask (u64)]
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            u32                   i;
            u8                    mac_addr[MAC_ADDR_LEN];
            u8                    mask[MAC_ADDR_LEN];
            interrupt_state_t     prev_interrupt_state;
            u32                   status         = CMD_PARAM_SUCCESS;
            u32                   msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32                   num_ranges     = Xil_Ntohl(cmd_args_32[1]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    // Need to disable interrupts during this operation so the filter does not have any holes
                    prev_interrupt_state = wlan_mac_high_interrupt_stop();

                    // Reset the current address filter
                    wlan_mac_addr_filter_reset();

                    // Add all the address ranges to the filter
                    for (i = 0; i < num_ranges; i++) {
                        // Extract the address and the mask
                        wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[2 + (4*i)], &mac_addr[0]);
                        wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[4 + (4*i)], &mask[0]);

                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Adding Address filter: (");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, mac_addr); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, ", ");
                        wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, mask); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                        if (wlan_mac_addr_filter_add(mask, mac_addr) == -1) {
                            status = CMD_PARAM_ERROR;
                        }
                    }

                    wlan_mac_high_interrupt_restore_state(prev_interrupt_state);
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// Association Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_ASSOCIATE: {
            // Associate with the device
            //
            // Message format:
            //     cmd_args_32[0]        Association flags
            //                               CMD_PARAM_AP_ASSOCIATE_FLAG_ALLOW_TIMEOUT
            //                               CMD_PARAM_AP_ASSOCIATE_FLAG_STATION_INFO_DO_NOT_REMOVE
            //     cmd_args_32[1]        Association flags mask
            //     cmd_args_32[2:3]      Association MAC Address
            //
            // Response format:
            //     resp_args_32[0]       Status
            //
            u32                   flags;
            u32                   mask;
            u8                    mac_addr[MAC_ADDR_LEN];
            interrupt_state_t     prev_interrupt_state;
            u32                   status              = CMD_PARAM_SUCCESS;
            station_info_t      * curr_station_info   = NULL;
            u32                   station_flags       = 0;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "AP: Associate\n");

            if ((active_bss_info != NULL) &&(active_bss_info->members.length < MAX_NUM_ASSOC)) {

                // Get MAC Address
                wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[2], &mac_addr[0]);

                // Get flags
                flags = Xil_Ntohl(cmd_args_32[0]);
                mask  = Xil_Ntohl(cmd_args_32[1]);

                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Associate flags = 0x%08x  mask = 0x%08x\n", flags, mask);

                // Disable interrupts to avoid race conditions between wlan_exp and wireless Tx/Rx when
                //  modifying the AP's association table
                prev_interrupt_state = wlan_mac_high_interrupt_stop();

                // Add association
                //     - Set ht_capable argument to zero.  This will be set correctly by the code below based on the
                //       flags of the command.
                curr_station_info = station_info_add(&active_bss_info->members, &mac_addr[0], ADD_STATION_INFO_ANY_ID, &default_unicast_data_tx_params, 0);

                // Update the new station_info flags field
                //  Only override the defaults set by the framework add_station_info if the wlan_exp command explicitly included a flag
                station_flags = curr_station_info->flags;

                // Raise the KEEP flag to prevent the MAC High Framework from removing the struct
                station_flags |= STATION_INFO_FLAG_KEEP;

                if (mask & CMD_PARAM_AP_ASSOCIATE_FLAG_DISABLE_INACTIVITY_TIMEOUT) {
                    if (flags & CMD_PARAM_AP_ASSOCIATE_FLAG_DISABLE_INACTIVITY_TIMEOUT) {
                        station_flags |= STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
                    } else {
                        station_flags &= ~STATION_INFO_FLAG_DISABLE_ASSOC_CHECK;
                    }
                }

                if (mask & CMD_PARAM_AP_ASSOCIATE_FLAG_HT_CAPABLE_STA) {
                    if (flags & CMD_PARAM_AP_ASSOCIATE_FLAG_HT_CAPABLE_STA) {
                        station_flags |= STATION_INFO_FLAG_HT_CAPABLE;
                    } else {
                        station_flags &= ~STATION_INFO_FLAG_HT_CAPABLE;
                    }
                }

                // Update the station_info flags
                curr_station_info->flags = station_flags;

                // Update the rate based on the flags that were set
                station_info_update_rate(curr_station_info, default_unicast_data_tx_params.phy.mcs, default_unicast_data_tx_params.phy.phy_mode);

                // Re-enable interrupts
                wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

                // Set return parameters and print info to console
                if (curr_station_info != NULL) {

                    //
                    // TODO:  (Optional) Log association state change
                    //

                    // Update the hex display
                    ap_update_hex_display(active_bss_info->members.length);

                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Associated with node: ");
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not associate with node: ");
                    status = CMD_PARAM_ERROR;
                }
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not associate with node: ");
                status = CMD_PARAM_ERROR;
            }

            wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            if (curr_station_info != NULL) {
                resp_args_32[resp_index++] = Xil_Htonl(curr_station_info->ID);
            } else {
                resp_args_32[resp_index++] = Xil_Htonl(0);
            }

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

