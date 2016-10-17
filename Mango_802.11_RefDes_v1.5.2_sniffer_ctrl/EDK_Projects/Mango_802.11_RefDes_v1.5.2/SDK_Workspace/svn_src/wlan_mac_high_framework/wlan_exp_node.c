/** @file wlan_exp_node.c
 *  @brief Experiment Framework
 *
 *  This contains the code for WLAN Experiments Framework.
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

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// WARP Includes
#include "w3_iic_eeprom.h"

// WLAN includes
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_sysmon_util.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_scan.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_station_info.h"

// WLAN Exp includes
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_transport.h"
#include "wlan_exp_user.h"


#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

/*************************** Constant Definitions ****************************/

// #define _DEBUG_


// Define Ethernet Header Buffer Constants
//
// The Ethernet header buffer is used when transferring large amounts of data from the node to the
// host in a performance efficient manner.  Since the WARP IP/UDP transport does not block on a
// packet send, if a single command returns many packets with a small processing delay between packets,
// then there must be multiple Ethernet header containers.  Otherwise, the contents of the Ethernet
// header could be changed before it is processed by the transport.
//
//     1)  Each buffer has 128 bytes which is more than needed for an Ethernet header for standard node
//         to host communication in WLAN Exp
//     2)  8 buffers are allocated which is more than the minimum number of buffers needed (ie 5) for 
//         the default transport setting of 10 TX BDs.
//     3)  Use 64 byte alignment for the buffers which is the same as the WARP IP/UDP transport
//         (ie it is the same as WARP_IP_UDP_BUFFER_ALIGNMENT in WARP_ip_udp_config.h)
//
#define WLAN_EXP_ETH_BUFFER_SIZE                           0x80                // Number of bytes per buffer
#define WLAN_EXP_ETH_NUM_BUFFER                            0x08                // Number of buffers allocated
#define WLAN_EXP_ETH_BUFFER_ALIGNMENT                      0x40                // Buffer alignment (64 byte boundary)


/*********************** Global Variable Definitions *************************/

// Declared in wlan_mac_high.c
extern u8                  low_param_rx_ant_mode;
extern u8				   low_param_channel;

extern tx_params_t         default_unicast_mgmt_tx_params;
extern tx_params_t         default_unicast_data_tx_params;
extern tx_params_t         default_multicast_mgmt_tx_params;
extern tx_params_t         default_multicast_data_tx_params;


/*************************** Functions Prototypes ****************************/

int node_init_parameters(u32 *info);
int process_hton_msg(int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, u32 recv_flags, warp_ip_udp_buffer * send_buffer);
void send_early_resp(int socket_index, void * to, cmd_resp_hdr * resp_hdr, void * buffer);
int process_node_cmd(int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len);

void ltg_cleanup(u32 id, void* callback_arg);

int process_tx_power(u32 cmd, u32 aid, int tx_power);
u32 process_tx_rate(u32 cmd, u32 aid, u32 mcs, u32 phy_mode, u32 * ret_mcs, u32 * ret_phy_mode);
u32 process_tx_ant_mode(u32 cmd, u32 aid, u8 ant_mode);

// WLAN Exp buffer functions
void          transfer_log_data(u32 socket_index, void * from,
                                void * resp_buffer_data, u32 eth_dev_num, u32 max_resp_len,
                                u32 id, u32 flags, u32 start_index, u32 size);

u32           process_buffer_cmds(int socket_index, void * from, cmd_resp * command, cmd_resp * response,
                                  cmd_resp_hdr * cmd_hdr, u32 * cmd_args_32,
                                  cmd_resp_hdr * resp_hdr, u32 * resp_args_32,
                                  u32 eth_dev_num, u32 max_resp_len,
                                  char * type, char * description, dl_list * source_list, u32 dest_size,
                                  u32 (*find_id)(u8 *),
                                  dl_entry * (*find_source)(u8 *),
                                  void (*copy_source_to_dest)(void *, void *, u8*),
                                  void (*zero_dest)(void *));

dl_entry *    find_station_info(u8 * mac_addr);
void          zero_station_info(void * dest);
void          copy_station_info_to_dest(void * source, void * dest, u8* mac_addr);
dl_entry *    find_counts_txrx(u8 * mac_addr);
void          zero_counts_txrx(void * dest);
void          copy_counts_txrx_to_dest(void * source, void * dest, u8* mac_addr);

void          zero_bss_info(void * dest);
void          copy_bss_info_to_dest(void * source, void * dest, u8* mac_addr);

// Null callback function declarations
int           null_process_cmd_callback(u32 cmd_id, void* param);



/*************************** Variable Definitions ****************************/

wlan_exp_node_info                node_info;
static wlan_exp_tag_parameter     node_parameters[NODE_PARAM_MAX_PARAMETER];

static wlan_exp_function_ptr_t    wlan_exp_process_node_cmd_callback         = (wlan_exp_function_ptr_t) null_process_cmd_callback;
       wlan_exp_function_ptr_t    wlan_exp_purge_all_data_tx_queue_callback  = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
       wlan_exp_function_ptr_t    wlan_exp_tx_cmd_add_association_callback   = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
       wlan_exp_function_ptr_t    wlan_exp_process_user_cmd_callback         = (wlan_exp_function_ptr_t) null_process_cmd_callback;
       wlan_exp_function_ptr_t    wlan_exp_beacon_ts_update_mode_callback    = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
       wlan_exp_function_ptr_t    wlan_exp_process_config_bss_callback       = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
       wlan_exp_function_ptr_t    wlan_exp_active_bss_info_getter_callback	 = (wlan_exp_function_ptr_t) wlan_exp_null_callback;


// Allocate Ethernet Header buffer
//     NOTE:  The buffer memory must be placed in DMA accessible DDR such that it can be fetched by the AXI DMA
//            attached to the Ethernet module.  Therefore, we will use the same section as other buffers for
//            Ethernet data, ie section ".eth_data".
//
u8     ETH_header_buffer[WLAN_EXP_ETH_NUM_BUFFER * WLAN_EXP_ETH_BUFFER_SIZE] __attribute__ ((aligned(WLAN_EXP_ETH_BUFFER_ALIGNMENT))) __attribute__ ((section (".eth_data")));


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * This will initialize the WLAN Exp node with the appropriate information
 * and set up the node to communicate with a host on the device.
 *
 * @param   serial_number    - Serial number of the node
 * @param   fpga_dna         - FPGA DNA of the node
 * @param   eth_dev_num      - Ethernet device to use for WLAN Exp
 * @param   wlan_exp_hw_addr - WLAN Exp hardware address
 * @param   wlan_hw_addr     - WLAN hardware address
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 * @note    This function will print to the terminal but is not able to control any of the LEDs
 *
 *****************************************************************************/
int wlan_exp_node_init(u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *wlan_exp_hw_addr, u8 *wlan_hw_addr) {

    int  i;
    int  status              = XST_SUCCESS;
    int  link_status;

#if WLAN_SW_CONFIG_ENABLE_LOGGING
    u64  mac_timestamp;
    u64  system_timestamp;
#endif

    u8   default_ip_addr[IP_ADDR_LEN];

    xil_printf("------------------------\n");
    xil_printf("WLAN EXP v%d.%d.%d (compiled %s %s)\n", WLAN_EXP_VER_MAJOR, WLAN_EXP_VER_MINOR, WLAN_EXP_VER_REV, __DATE__, __TIME__);


    // ------------------------------------------
    // Initialize Node information
    //   Node ID / Network information must be set using dynamic node configuration process
    //   Initial IP Address should be NODE_IP_ADDR_BASE for all nodes
    //
    node_info.node_type						= 0; //Field will be overwritten in pieces by the three "wlan_exp_node_set_type_*" setters

    // Set the Design Type fields in wlan_exp's node_type struct
	wlan_exp_node_set_type_design(WLAN_EXP_TYPE_DESIGN_80211);

    node_info.node_id                       = 0xFFFF;
    node_info.hw_generation                 = WLAN_EXP_HW_VERSION;
    node_info.serial_number                 = serial_number;

    // Process both 32 bit arguments of the FPGA DNA
    for(i = 0; i < FPGA_DNA_LEN; i++) {
        node_info.fpga_dna[i]               = fpga_dna[i];
    }

    node_info.wlan_exp_version              = REQ_WLAN_EXP_HW_VER;

    // Set the WLAN MAC address (ie MAC address for Wired to Wireless bridge)
    //     NOTE:  Byte ordering is done so that it is easily processed in the log.
    //
    node_info.wlan_hw_addr[0]               = (wlan_hw_addr[2] << 24) | (wlan_hw_addr[3] << 16) | (wlan_hw_addr[4] << 8) | wlan_hw_addr[5];
    node_info.wlan_hw_addr[1]               = (wlan_hw_addr[0] <<  8) |  wlan_hw_addr[1];

    node_info.wlan_scheduler_resolution     = FAST_TIMER_DUR_US;

    // Set the max/min transmit power
    node_info.wlan_max_tx_power_dbm         = (u32) TX_POWER_MAX_DBM;
    node_info.wlan_min_tx_power_dbm         = (u32) TX_POWER_MIN_DBM;

    // Initialize Ethernet device to NULL; Will be set during transport init
    node_info.eth_dev                       = NULL;


    // ------------------------------------------
    // Initialize Tag parameters
    node_init_parameters((u32*) &node_info);


#if WLAN_SW_CONFIG_ENABLE_LOGGING
    // ------------------------------------------
    // By default, enable all subtype logging
    wlan_exp_log_set_entry_en_mask(ENTRY_EN_MASK_TXRX_CTRL | ENTRY_EN_MASK_TXRX_MPDU);


    // ------------------------------------------
    // Reset the System Time ID
    wlan_exp_log_reset_system_time_id();


    // ------------------------------------------
    // Add a time info entry to the log to record the initial mac time and system time
    mac_timestamp    = get_mac_time_usec();
    system_timestamp = get_system_time_usec();

    add_time_info_entry(mac_timestamp, mac_timestamp, system_timestamp, TIME_INFO_ENTRY_TIME_RSVD_VAL_64, TIME_INFO_ENTRY_SYSTEM, 0, 0);
#endif

    // ------------------------------------------
    // Initialize the System Monitor
    init_sysmon();


    // ------------------------------------------
    // Initialize the default IP address
    //
    //     NOTE:  The default IP address is defined in the "Ethernet controls" section of wlan_exp_common.h
    //
    default_ip_addr[0] = (WLAN_EXP_DEFAULT_IP_ADDR >> 24) & 0xFF;
    default_ip_addr[1] = (WLAN_EXP_DEFAULT_IP_ADDR >> 16) & 0xFF;
    default_ip_addr[2] = (WLAN_EXP_DEFAULT_IP_ADDR >>  8) & 0xFF;
    default_ip_addr[3] = (WLAN_EXP_DEFAULT_IP_ADDR      ) & 0xFF;  // IP ADDR = w.x.y.z


    // ------------------------------------------
    // Transport initialization
    //
    //   NOTE:  These errors are fatal and status error will be displayed on the hex display.
    //
    //   NOTE:  This will initialize all aspects of the transport (unicast receive, broadcast
    //       receive, asynchronous transmit).
    //
    status = transport_init(eth_dev_num,
                            &node_info,
                            default_ip_addr,
                            wlan_exp_hw_addr,
                            WLAN_EXP_DEFAULT_UDP_UNICAST_PORT,
                            WLAN_EXP_DEFAULT_UDP_MULTICAST_PORT);

    if (status == XST_FAILURE) {
        xil_printf("  Error in transport_init()! Exiting...\n");
        return XST_FAILURE;
    }


    // ------------------------------------------
    // Wait for Ethernet to finish initializing the link
    //
    if (WLAN_EXP_WAIT_FOR_ETH) {
        wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "  Waiting for Ethernet link ... \n");

        while(link_status == LINK_NOT_READY) {

            // Check the link status of the Ethernet device
            if (transport_link_status(eth_dev_num) == LINK_NOT_READY) {
                link_status = LINK_NOT_READY;
            } else {
                link_status = LINK_READY;
            }

            // Sleep for a bit before checking the status again
            usleep(100000);
        }

    } else {
        xil_printf("  Not waiting for Ethernet link.  Current status:\n");
        xil_printf("      ETH %c ", warp_conv_eth_dev_num(eth_dev_num));

        if ((transport_link_status(eth_dev_num) == LINK_READY)) {
            xil_printf("ready\n");
        } else {
            xil_printf("not ready\n");
            xil_printf("  !!! Make sure link is ready before using WLAN Exp. !!!\n");
        }
    }


    // ------------------------------------------
    // Assign the transport receive callback (how to process received Ethernet packets)
    //
    //     IMPORTANT: Must be called after transport_init()
    //
    transport_set_process_hton_msg_callback((void *)process_hton_msg);

    wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "WLAN EXP Initialization complete\n");

    return status;
}

/*****************************************************************************/
/**
 * Set Design Type
 *
 * This function sets the design type bits in the node_info.node_type field.
 * It is called by the wlan_exp node initialization function.
 *
 * @param   type_design      - Design Type from wlan_exp.h
 *
 ******************************************************************************/
void wlan_exp_node_set_type_design(u32 type_design){
	node_info.node_type &= ~WLAN_EXP_TYPE_DESIGN_MASK;
	node_info.node_type |= (type_design&WLAN_EXP_TYPE_DESIGN_MASK);
}

/*****************************************************************************/
/**
 * Set CPU_HIGH Type
 *
 * This function sets the CPU_HIGH type bits in the node_info.node_type field.
 * It is typically the responsibility of the high-level application to call
 * this setter.
 *
 * @param   type_high      - CPU_HIGH Type from wlan_exp.h
 *
 ******************************************************************************/
void wlan_exp_node_set_type_high(u32 type_high){
	node_info.node_type &= ~WLAN_EXP_TYPE_DESIGN_80211_CPU_HIGH_MASK;
	node_info.node_type |= (type_high&WLAN_EXP_TYPE_DESIGN_80211_CPU_HIGH_MASK);
}

/*****************************************************************************/
/**
 * Set CPU_LOW Type
 *
 * This function sets the CPU_LOW type bits in the node_info.node_type field.
 * The MAC High Framework will call this function after receiving an IPC message
 * from CPU_LOW indicating its wlan_exp type.
 *
 * @param   type_low      - CPU_LOW Type from wlan_exp.h
 *
 ******************************************************************************/
void wlan_exp_node_set_type_low(u32 type_low){
	node_info.node_type &= ~WLAN_EXP_TYPE_DESIGN_80211_CPU_LOW_MASK;
	node_info.node_type |= (type_low&WLAN_EXP_TYPE_DESIGN_80211_CPU_LOW_MASK);
}

/*****************************************************************************/
/**
 * Null Process Command Callback
 *
 * This function is part of the callback system for processing WLAN Exp commands. If
 * there are no additional node commands, then this will return an appropriate value.
 *
 * To processes additional node commands, please set the process_node_cmd_callback
 *
 * @param   cmd_id           - Command ID from node_process_cmd
 * @param   param            - Generic parameters for the callback
 *
 * @return  int              - Status of the command:
 *                                 NO_RESP_SENT - No response has been sent
 *
 ******************************************************************************/
int null_process_cmd_callback(u32 cmd_id, void * param){

    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown node command: %d\n", cmd_id);

    return NO_RESP_SENT;
};



/*****************************************************************************/
/**
 * Node Transport Processing (Host to Node)
 *
 * This function is how the node processes Ethernet frames from the Transport.  This
 * function will be used as the transport callback for Host-to-Node messages.  Based
 * on the Command Group field in the Command header, this function will call the
 * appropriate sub-system to continue processing the packet.
 *
 * @param   socket_index     - Index of the socket on which message was received
 * @param   from             - Pointer to socket address structure from which message was received
 * @param   recv_buffer      - Pointer to transport buffer with received message
 * @param   send_buffer      - Pointer to transport buffer for a node response to the message
 *
 * @return  None
 *
 * @note    If this packet is a host to node message, then the process_hton_msg_callback
 *          is used to further process the packet.  This method will strip off the
 *          Transport header for future packet processing.
 *
 *****************************************************************************/
int  process_hton_msg(int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, u32 recv_flags, warp_ip_udp_buffer * send_buffer) {

    u8                  cmd_group;
    u32                 resp_sent      = NO_RESP_SENT;
    u32                 resp_length;
    u32                 max_resp_len   = node_info.eth_dev->max_pkt_words;

    cmd_resp_hdr      * cmd_hdr;
    cmd_resp            command;
    cmd_resp_hdr      * resp_hdr;
    cmd_resp            response;


    // Initialize the Command/Response structures
    cmd_hdr             = (cmd_resp_hdr *)(recv_buffer->offset);
    command.flags       = recv_flags;
    command.header      = cmd_hdr;
    command.args        = (u32 *)((recv_buffer->offset) + sizeof(cmd_resp_hdr));
    command.buffer      = (void *)(recv_buffer);

    resp_hdr            = (cmd_resp_hdr *)(send_buffer->offset);
    response.flags      = 0;
    response.header     = resp_hdr;
    response.args       = (u32 *)((send_buffer->offset) + sizeof(cmd_resp_hdr));
    response.buffer     = (void *)(send_buffer);

    // Endian swap the command header so future processing can understand it
    cmd_hdr->cmd        = Xil_Ntohl(cmd_hdr->cmd);
    cmd_hdr->length     = Xil_Ntohs(cmd_hdr->length);
    cmd_hdr->num_args   = Xil_Ntohs(cmd_hdr->num_args);

    // Send command to appropriate processing sub-system
    cmd_group           = CMD_TO_GROUP(cmd_hdr->cmd);

    switch(cmd_group){
        case GROUP_NODE:
            resp_sent = process_node_cmd(socket_index, from, &command, &response, max_resp_len);
        break;
        case GROUP_TRANSPORT:
            resp_sent = process_transport_cmd(socket_index, from, &command, &response, max_resp_len);
        break;
        case GROUP_USER:
            resp_sent = process_user_cmd(socket_index, from, &command, &response, max_resp_len);
        break;
        default:
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command group: %d\n", cmd_group);
        break;
    }

    // Adjust the length of the response to include the response data from the sub-system and the
    // response header
    //
    if(resp_sent == NO_RESP_SENT) {
        resp_length = (resp_hdr->length + sizeof(cmd_resp_hdr));

        // Keep the length and size of the response in sync since we are adding bytes to the buffer
        send_buffer->length += resp_length;
        send_buffer->size   += resp_length;
    }

    // Endian swap the response header before returning
    resp_hdr->cmd       = Xil_Ntohl(resp_hdr->cmd);
    resp_hdr->length    = Xil_Ntohs(resp_hdr->length);
    resp_hdr->num_args  = Xil_Ntohs(resp_hdr->num_args);

    // Return the status
    return resp_sent;
}



/*****************************************************************************/
/**
 * Node Send Early Response
 *
 * Allows a node to send a response back to the host before the command has
 * finished being processed.  This is to minimize the latency between commands
 * since the node is able to finish processing the command during the time
 * it takes to communicate to the host and receive another command.
 *
 * @param   socket_index     - Index of the socket on which message was received
 * @param   to               - Pointer to socket address structure to which message will be sent
 * @param   resp_hdr         - Pointer to Command / Response header for outgoing message
 * @param   buffer           - Pointer to IP/UDP buffer that contains the outgoing message
 *
 * @return  None
 *
 * @note    This function can only send one buffer at a time and will modify both the
 *          response header and buffer length to create an appropriate outgoing message.
 *
 *****************************************************************************/
void send_early_resp(int socket_index, void * to, cmd_resp_hdr * resp_hdr, void * buffer) {
    //
    // This function is used to send a response back to the host outside the normal command processing
    // (ie the response does not complete the steps in node_rx_from_transport() after distribution
    // to the different group processing commands), this method must perform the necessary manipulation
    // of the response header and the buffer size so that the message is ready to be sent and then
    // restore the contents so that everything is ready to be used if additional responses are required.
    //
    u32                      tmp_cmd;
    u16                      tmp_length;
    u16                      tmp_num_args;
    u32                      tmp_buffer_length;
    u32                      tmp_buffer_size;

    warp_ip_udp_buffer     * buffer_ptr;
    u32                      resp_length;

    // Cast the buffer pointer so it is easier to use
    buffer_ptr               = (warp_ip_udp_buffer *) buffer;

    // Get the current values in the buffer so we can restore them after transmission
    tmp_cmd                  = resp_hdr->cmd;
    tmp_length               = resp_hdr->length;
    tmp_num_args             = resp_hdr->num_args;
    tmp_buffer_length        = buffer_ptr->length;
    tmp_buffer_size          = buffer_ptr->size;

    // Adjust the length of the buffer
    resp_length              = resp_hdr->length + sizeof(cmd_resp_hdr);
    buffer_ptr->length      += resp_length;
    buffer_ptr->size        += resp_length;

    // Endian swap the response header before before transport sends it
    resp_hdr->cmd            = Xil_Ntohl(tmp_cmd);
    resp_hdr->length         = Xil_Ntohs(tmp_length);
    resp_hdr->num_args       = Xil_Ntohs(tmp_num_args);

    // Send the packet
    transport_send(socket_index, (struct sockaddr *)to, (warp_ip_udp_buffer **)&buffer, 0x1);

    // Restore the values in the buffer
    resp_hdr->cmd      = tmp_cmd;
    resp_hdr->length   = tmp_length;
    resp_hdr->num_args = tmp_num_args;
    buffer_ptr->length = tmp_buffer_length;
    buffer_ptr->size   = tmp_buffer_size;
}



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
int process_node_cmd(int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len) {

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
    u32                      resp_sent      = NO_RESP_SENT;

    cmd_resp_hdr           * cmd_hdr        = command->header;
    u32                    * cmd_args_32    = command->args;
    u32                      cmd_id         = CMD_TO_CMDID(cmd_hdr->cmd);

    cmd_resp_hdr           * resp_hdr       = response->header;
    u32                    * resp_args_32   = response->args;
    u32                      resp_index     = 0;

    u32                      eth_dev_num    = socket_get_eth_dev_num(socket_index);

    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Finish any CDMA transfers that might be occurring
    wlan_mac_high_cdma_finish_transfer();

    // Process the command
    switch(cmd_id){

//-----------------------------------------------------------------------------
// General Commands
//-----------------------------------------------------------------------------

        //---------------------------------------------------------------------
        case CMDID_NODE_TYPE: {
            // Return the WLAN Exp Type
            resp_args_32[resp_index++] = Xil_Htonl(node_info.node_type);

#ifdef _DEBUG_
            xil_printf("WLAN Exp Type = 0x%08x \n", node_info.node_type);
#endif

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;
        
    
        //---------------------------------------------------------------------
        case CMDID_NODE_INFO: {
            // Return the info about the WLAN_EXP_NODE
            //
            u32    num_params;
            
            // Send node parameters
            num_params    = node_get_parameters(&resp_args_32[resp_index], max_resp_len, WLAN_EXP_TRANSMIT);
            resp_index   += num_params;
            max_resp_len -= num_params;
            if (max_resp_len <= 0) { xil_printf("No more space left in NODE_INFO packet \n"); };
            
            // Send transport parameters
            num_params    = transport_get_parameters(eth_dev_num, &resp_args_32[resp_index], max_resp_len, WLAN_EXP_TRANSMIT);
            resp_index   += num_params;
            max_resp_len -= num_params;
            if (max_resp_len <= 0) { xil_printf("No more space left in NODE_INFO packet \n"); };

#ifdef _DEBUG_
            xil_printf("NODE INFO: \n");
            for ( i = 0; i < resp_index; i++ ) {
                xil_printf("   [%2d] = 0x%8x \n", i, resp_args_32[i]);
            }
            xil_printf("END NODE INFO \n");
#endif

            // --------------------------------
            // Future parameters go here
            // --------------------------------
                        
            // Finalize response
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;
        

        //---------------------------------------------------------------------
        case CMDID_NODE_IDENTIFY: {
            // Blink the HEX display LEDs
            //   - cmd_args_32[0] - Serial Number
            //   - cmd_args_32[1] - Number of blinks
            //   - cmd_args_32[2] - Microseconds per blink (must be an even number)
            //
            u32              serial_number;
            u32              num_blinks;
            u32              time_per_blink;
            transport_info * transport_info;
            u8               ip_addr[IP_ADDR_LEN];

            // Get command parameters
            serial_number  = Xil_Ntohl(cmd_args_32[0]);
            num_blinks     = Xil_Ntohl(cmd_args_32[1]);
            time_per_blink = (Xil_Ntohl(cmd_args_32[2]) >> 1);

            xil_printf("NODE IDENTIFY:  Num blinks = %8d   Time = %8d usec\n", num_blinks, time_per_blink);

            if ((serial_number == CMD_PARAM_NODE_IDENTIFY_ALL) || (serial_number == node_info.serial_number)) {
                transport_info = &(node_info.eth_dev->info);

                // Get IP Address from the node info
                ip_addr[0] = (transport_info->ip_addr >> 24) & 0xFF;
                ip_addr[1] = (transport_info->ip_addr >> 16) & 0xFF;
                ip_addr[2] = (transport_info->ip_addr >>  8) & 0xFF;
                ip_addr[3] = (transport_info->ip_addr      ) & 0xFF;

                // Print Node information
                xil_printf("    Node: %d    IP Address: %d.%d.%d.%d \n", node_info.node_id, ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);

                // Send the response early so that code does not time out while waiting for blinks
                //   The host is responsible for waiting until the LED blinking is done before issuing the
                //   node another command.
                resp_args_32[resp_index++] = Xil_Htonl(CMD_PARAM_SUCCESS);
                resp_hdr->length          += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args         = resp_index;

                send_early_resp(socket_index, from, response->header, response->buffer);

                resp_sent                  = RESP_SENT;

                blink_hex_display(num_blinks, time_per_blink);

            } else {
                resp_args_32[resp_index++] = Xil_Htonl(CMD_PARAM_ERROR);
                resp_hdr->length          += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args         = resp_index;
            }
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_CONFIG_SETUP: {
            // NODE_CONFIG_SETUP Packet Format:
            //   - Note:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmd_args_32[0] - Serial Number
            //   - cmd_args_32[1] - Node ID
            //   - cmd_args_32[2] - IP Address
            //   - cmd_args_32[3] - Unicast Port
            //   - cmd_args_32[4] - Broadcast Port
            // 
            int              status;
            u32              node_id;
            transport_info * transport_info;
            u8               ip_addr[IP_ADDR_LEN];

            // Only update the parameters if the serial numbers match
            if (node_info.serial_number ==  Xil_Ntohl(cmd_args_32[0])) {

                // Only update the node if it has not been configured
                if (node_info.node_id == 0xFFFF) {
                    // Set Node ID
                    //   NOTE:  We need to set the node ID in both the node info and the eth_dev_info
                    //
                    node_id = Xil_Ntohl(cmd_args_32[1]) & 0xFFFF;
                    node_info.node_id          = node_id;
                    node_info.eth_dev->node_id = node_id;

                    // Get New IP Address
                    ip_addr[0]     = (Xil_Ntohl(cmd_args_32[2]) >> 24) & 0xFF;
                    ip_addr[1]     = (Xil_Ntohl(cmd_args_32[2]) >> 16) & 0xFF;
                    ip_addr[2]     = (Xil_Ntohl(cmd_args_32[2]) >>  8) & 0xFF;
                    ip_addr[3]     = (Xil_Ntohl(cmd_args_32[2])      ) & 0xFF;
                    
                    // Get new ports
                    transport_info = &(node_info.eth_dev->info);

                    transport_info->unicast_port   = Xil_Ntohl(cmd_args_32[3]);
                    transport_info->broadcast_port = Xil_Ntohl(cmd_args_32[4]);

                    // Set Transport IP Addresses / Ports
                    transport_set_ip_addr(eth_dev_num, ip_addr);

                    status = transport_config_sockets(eth_dev_num, transport_info->unicast_port, transport_info->broadcast_port, 0);

                    if(status != 0) {
                        xil_printf("Error binding transport...\n");
                    } else {
                        // Print new configuration information
                        xil_printf("NODE_CONFIG_SETUP: Configured wlan_exp with node ID %d, ", node_info.node_id);
                        xil_printf("IP address %d.%d.%d.%d\n", ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);

                        // Set right decimal point to indicate WLAN Exp network is configured
                        set_hex_display_right_dp(1);
                    }
                } else {
                    // Do nothing
                }
            } else {
                // Do nothing
            }
        }
        break;

        
        //---------------------------------------------------------------------
        case CMDID_NODE_CONFIG_RESET: {
            // NODE_CONFIG_RESET Packet Format:
            //   - Note:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmd_args_32[0] - Serial Number
            // 
            u32              serial_number;
            transport_info * transport_info;
            u8               ip_addr[IP_ADDR_LEN];
            
            // If the command was sent directly to the node (ie not a broadcast packet), then the node
            // needs to send a response before the IP address changes.
            //
            if (((command->flags) & 0x00000001) == WLAN_EXP_FALSE) {
                send_early_resp(socket_index, from, response->header, response->buffer);
                resp_sent = RESP_SENT;
            }
            
            serial_number = Xil_Ntohl(cmd_args_32[0]);

            // Only update the parameters if the serial numbers match or this it is "all serial numbers"
            if ((node_info.serial_number ==  serial_number) || (CMD_PARAM_NODE_CONFIG_RESET_ALL == serial_number)) {

                if (node_info.node_id != 0xFFFF){

                    // Reset node to 0xFFFF
                    //   NOTE:  We need to set the node ID in both the node info and the eth_dev_info
                    //
                    node_info.node_id          = 0xFFFF;
                    node_info.eth_dev->node_id = 0xFFFF;

                    // Reset transport;  This will update the IP Address back to default and rebind the sockets
                    //   - See below for default IP address:  NODE_IP_ADDR_BASE + node
                    transport_info = &(node_info.eth_dev->info);

                    ip_addr[0]      = (WLAN_EXP_DEFAULT_IP_ADDR >> 24) & 0xFF;
                    ip_addr[1]      = (WLAN_EXP_DEFAULT_IP_ADDR >> 16) & 0xFF;
                    ip_addr[2]      = (WLAN_EXP_DEFAULT_IP_ADDR >>  8) & 0xFF;
                    ip_addr[3]      = (WLAN_EXP_DEFAULT_IP_ADDR      ) & 0xFF;  // IP ADDR = w.x.y.z

                    transport_info->unicast_port    = WLAN_EXP_DEFAULT_UDP_UNICAST_PORT;
                    transport_info->broadcast_port  = WLAN_EXP_DEFAULT_UDP_MULTICAST_PORT;

                    transport_set_ip_addr(eth_dev_num, ip_addr);
                    transport_config_sockets(eth_dev_num, transport_info->unicast_port, transport_info->broadcast_port, 0);
                    transport_reset_max_pkt_length(eth_dev_num);

                    // Print information
                    xil_printf("NODE_CONFIG_RESET: Reset wlan_exp network config\n");

                    // Clear right decimal point to indicate WLAN Exp network is not configured
                    set_hex_display_right_dp(0);
                } else {
                    // Do nothing
                }
            } else {
                // Do nothing
            }
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_TEMPERATURE: {
            // NODE_TEMPERATURE
            //   - If the system monitor exists, return the current, min and max temperature of the node
            //
            resp_args_32[resp_index++] = Xil_Htonl(get_current_temp());
            resp_args_32[resp_index++] = Xil_Htonl(get_min_temp());
            resp_args_32[resp_index++] = Xil_Htonl(get_max_temp());

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// Log Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_LOG_CONFIG: {
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            // NODE_LOG_CONFIG Packet Format:
            //   - cmd_args_32[0]  - flags
            //                     [ 0] - Logging Enabled = 1; Logging Disabled = 0;
            //                     [ 1] - Wrap = 1; No Wrap = 0;
            //                     [ 2] - Full Payloads Enabled = 1; Full Payloads Disabled = 0;
            //                     [ 3] - Log WN Cmds Enabled = 1; Log WN Cmds Disabled = 0;
            //   - cmd_args_32[1]  - mask for flags
            //
            //   - resp_args_32[0] - CMD_PARAM_SUCCESS
            //                     - CMD_PARAM_ERROR
            //
            int    status         = CMD_PARAM_SUCCESS;
            u8     entry_mask     = wlan_exp_log_get_entry_en_mask();
            u32    flags          = Xil_Ntohl(cmd_args_32[0]);
            u32    mask           = Xil_Ntohl(cmd_args_32[1]);

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_event_log, "Configure flags = 0x%08x  mask = 0x%08x\n", flags, mask);

            // Configure the LOG based on the flag bit / mask
            if (mask & CMD_PARAM_LOG_CONFIG_FLAG_LOGGING) {
                if (flags & CMD_PARAM_LOG_CONFIG_FLAG_LOGGING) {
                    event_log_config_logging(EVENT_LOG_LOGGING_ENABLE);
                } else {
                    event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
                }
            }

            if (mask & CMD_PARAM_LOG_CONFIG_FLAG_WRAP) {
                if (flags & CMD_PARAM_LOG_CONFIG_FLAG_WRAP) {
                    event_log_config_wrap(EVENT_LOG_WRAP_ENABLE);
                } else {
                    event_log_config_wrap(EVENT_LOG_WRAP_DISABLE);
                }
            }

            if (mask & CMD_PARAM_LOG_CONFIG_FLAG_PAYLOADS) {
                if (flags & CMD_PARAM_LOG_CONFIG_FLAG_PAYLOADS) {
                    wlan_exp_log_set_mac_payload_len(MAX_MAC_PAYLOAD_LOG_LEN);
                } else {
                    wlan_exp_log_set_mac_payload_len(MIN_MAC_PAYLOAD_LOG_LEN);
                }
            }

            if (mask & CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU) {
                if (flags & CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU) {
                    entry_mask |= ENTRY_EN_MASK_TXRX_MPDU;
                } else {
                    entry_mask &= ~ENTRY_EN_MASK_TXRX_MPDU;
                }
            }

            if (mask & CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL) {
                if (flags & CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL) {
                    entry_mask |= ENTRY_EN_MASK_TXRX_CTRL;
                } else {
                    entry_mask &= ~ENTRY_EN_MASK_TXRX_CTRL;
                }
            }

            wlan_exp_log_set_entry_en_mask(entry_mask);

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LOG_GET_STATUS: {
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            // NODE_LOG_GET_INFO Packet Format:
            //   - resp_args_32[0] - Next empty entry index
            //   - resp_args_32[1] - Oldest empty entry index
            //   - resp_args_32[2] - Number of wraps
            //   - resp_args_32[3] - Flags
            //                         [0] - Log enabled
            //                         [1] - Log wrapping enabled
            //                         [2] - Log full payloads enabled
            //                         [3] - Log Tx / Rx MPDU frames enabled
            //                         [4] - Log Tx / Rx CTRL frames enabled
            //
            u32 flags         = event_log_get_flags();
            u32 log_length    = wlan_exp_log_get_mac_payload_len();
            u8  entry_en_mask = wlan_exp_log_get_entry_en_mask();

            if (log_length == MAX_MAC_PAYLOAD_LOG_LEN) {
                flags |= CMD_PARAM_LOG_CONFIG_FLAG_PAYLOADS;
            }

            if (entry_en_mask & ENTRY_EN_MASK_TXRX_MPDU) {
                flags |= CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU;
            }

            if (entry_en_mask & ENTRY_EN_MASK_TXRX_CTRL) {
                flags |= CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL;
            }

            // Set response
            resp_args_32[resp_index++] = Xil_Htonl(event_log_get_next_entry_index());
            resp_args_32[resp_index++] = Xil_Htonl(event_log_get_oldest_entry_index());
            resp_args_32[resp_index++] = Xil_Htonl(event_log_get_num_wraps());
            resp_args_32[resp_index++] = Xil_Htonl(flags);

            // Send response of current info
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LOG_GET_CAPACITY: {
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            // NODE_LOG_GET_CAPACITY Packet Format:
            //   - resp_args_32[0] - Max log size
            //   - resp_args_32[1] - Current log size
            //
            resp_args_32[resp_index++] = Xil_Htonl(event_log_get_capacity());
            resp_args_32[resp_index++] = Xil_Htonl(event_log_get_total_size());

            // Send response of current info
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LOG_GET_ENTRIES: {
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            // NODE_LOG_GET_ENTRIES Packet Format:
            //   - Note:  All u32 parameters in cmd_args_32 are byte swapped so use Xil_Ntohl()
            //
            //   - cmd_args_32[0] - buffer id
            //   - cmd_args_32[1] - flags
            //   - cmd_args_32[2] - start_address of transfer
            //   - cmd_args_32[3] - size of transfer (in bytes)
            //                      0xFFFF_FFFF  -> Get everything in the event log
            //
            //   Return Value:
            //     - buffer
            //       - buffer_id       - uint32  - ID of the buffer
            //       - flags           - uint32  - Flags
            //       - bytes_remaining - uint32  - Number of bytes remaining in the transfer
            //       - start_byte      - uint32  - Byte index of the first byte in this packet
            //       - size            - uint32  - Number of payload bytes in this packet
            //       - byte[]          - uint8[] - Array of payload bytes
            //
            // NOTE:  The address passed via the command is the address relative to the current
            //   start of the event log.  It is not an absolute address and should not be treated
            //   as such.
            //
            //     When you transferring "everything" in the event log, the command will take a
            //   snapshot of the size of the log to the "end" at the time the command is received
            //   (ie either the next_entry_index or the end of the log before it wraps).  It will then
            //   only transfer those events.  It will not any new events that are added to the log while
            //   we are transferring the current log as well as transfer any events after a wrap.
            //
            u32    id                  = Xil_Ntohl(cmd_args_32[0]);
            u32    flags               = Xil_Ntohl(cmd_args_32[1]);
            u32    start_index         = Xil_Ntohl(cmd_args_32[2]);
            u32    size                = Xil_Ntohl(cmd_args_32[3]);
            u32    evt_log_size        = event_log_get_size(start_index);

            // Check if we should transfer everything or if the request was larger than the current log
            if ((size == CMD_PARAM_LOG_GET_ALL_ENTRIES) || (size > evt_log_size)) {
                size = evt_log_size;
            }

            // Transfer data to host
            transfer_log_data(socket_index, from,
                              (void *)(((warp_ip_udp_buffer *)(response->buffer))->data),
                              eth_dev_num, max_resp_len,
                              id, flags, start_index, size);

            resp_sent = RESP_SENT;
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LOG_ADD_EXP_INFO_ENTRY: {
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            // Add EXP_INFO entry to the log
            //
            // Message format:
            //     cmd_args_32[0]   info_type (lower 16 bits)
            //     cmd_args_32[1]   info_length (lower 16 bits)
            //     cmd_args_32[2:N] info_payload
            //
            // NOTE:  Entry data will be copied in to the log "as is" (ie it will not
            //     have any network to host order translation performed on it)
            //
            exp_info_entry * exp_info;
            u32              entry_size;
            u32              type           = (Xil_Ntohl(cmd_args_32[0]) & 0xFFFF);
            u32              size           = (Xil_Ntohl(cmd_args_32[1]) & 0xFFFF);

            // Get the entry size
            if (size == 0) {
                entry_size = sizeof(exp_info_entry);
            } else {
                // 32-bit align size; EXP INFO structure already contains 4 bytes of the payload
                entry_size = sizeof(exp_info_entry) + (((size - 1) / 4)*4);
            }

            exp_info = (exp_info_entry *) wlan_exp_log_create_entry(ENTRY_TYPE_EXP_INFO, entry_size);

            if (exp_info != NULL) {
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_event_log,
                                "Adding EXP INFO entry with type %d to log (%d bytes)\n", type, size);

                exp_info->timestamp   = get_mac_time_usec();
                exp_info->info_type   = type;
                exp_info->info_length = size;

                // Copy the data to the log entry
                if (size == 0){
                    bzero((void *)(&exp_info->info_payload[0]), 4);
                } else {
                    memcpy((void *)(&exp_info->info_payload[0]), (void *)(&cmd_args_32[2]), size);
                }
            }
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LOG_ENABLE_ENTRY: {
#if WLAN_SW_CONFIG_ENABLE_LOGGING
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_event_log, "Enable Event not supported\n");
            // TODO:  THIS FUNCTION IS NOT COMPLETE
#endif
        }
        break;


//-----------------------------------------------------------------------------
// Counts Commands
//-----------------------------------------------------------------------------
        //---------------------------------------------------------------------
        case CMDID_COUNTS_GET_TXRX: {
            // NODE_GET_COUNTS Packet Format:
            //   - cmd_args_32[0]   - buffer id
            //   - cmd_args_32[1]   - flags
            //   - cmd_args_32[2]   - start_address of transfer
            //   - cmd_args_32[3]   - size of transfer (in bytes)
            //   - cmd_args_32[4:5] - MAC Address (All 0xFF means all counts)
            //
            // Always returns a valid WLAN Exp Buffer (either 1 or more packets)
            //   - buffer_id       - uint32  - buffer_id
            //   - flags           - uint32  - 0
            //   - bytes_remaining - uint32  - Number of bytes remaining in the transfer
            //   - start_byte      - uint32  - Byte index of the first byte in this packet
            //   - size            - uint32  - Number of payload bytes in this packet
            //   - byte[]          - uint8[] - Array of payload bytes
            //

            resp_sent = process_buffer_cmds(socket_index, from, command, response,
                                            cmd_hdr, cmd_args_32, resp_hdr, resp_args_32, eth_dev_num, max_resp_len,
                                            print_type_counts, "counts",
                                            station_info_get_list(),
                                            sizeof(wlan_exp_station_txrx_counts_t),
                                            &wlan_exp_get_id_in_counts,
                                            &find_counts_txrx,
                                            &copy_counts_txrx_to_dest,
                                            &zero_counts_txrx);

        }
        break;


//-----------------------------------------------------------------------------
// Local Traffic Generator (LTG) Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_LTG_CONFIG: {
#if WLAN_SW_CONFIG_ENABLE_LTG
            // NODE_LTG_START Packet Format:
            //   - cmd_args_32[0]      - Flags
            //                           [0] - Auto-start the LTG flow
            //   - cmd_args_32[1 - N]  - LTG Schedule (packed)
            //                           [0] - [31:16] Type    [15:0] Length
            //   - cmd_args_32[N+1 - M]- LTG Payload (packed)
            //                           [0] - [31:16] Type    [15:0] Length
            //
            //   - resp_args_32[0]     - CMD_PARAM_SUCCESS
            //                         - CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            //
            u32    s1, s2, t1, t2;
            void * ltg_callback_arg;
            void * params;
            u32    status    = CMD_PARAM_SUCCESS;
            u32    id        = LTG_ID_INVALID;
            u32    flags     = Xil_Ntohl(cmd_args_32[0]);

            // Get Schedule & Payload
            // NOTE:  This allocates memory for both the schedule and payload containers.
            //   The payload is freed as part of the ltg_cleanup() callback
            //   The schedule is freed as part of this method
            params           = ltg_sched_deserialize( &(cmd_args_32[1]), &t1, &s1 );
            ltg_callback_arg = ltg_payload_deserialize( &(cmd_args_32[2 + s1]), &t2, &s2);

            if((ltg_callback_arg != NULL) && (params != NULL)) {

                // Configure the LTG
                id = ltg_sched_create(t1, params, ltg_callback_arg, &ltg_cleanup);

                if(id != LTG_ID_INVALID){
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Configured %d\n", id);

                    if (flags & CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Starting %d\n", id);
                        ltg_sched_start( id );
                    }

                    // Free the memory allocated for the params (ltg_callback_arg will be freed later)
                    wlan_mac_high_free(params);
                } else {
                    status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Could not create LTG\n");

                    // Free the memory allocated in the deserialize
                    wlan_mac_high_free(params);
                    wlan_mac_high_free(ltg_callback_arg);
                }
            } else {
                status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;

                // Free the memory allocated in the deserialize
                if (ltg_callback_arg != NULL) { wlan_mac_high_free(ltg_callback_arg); }
                if (params           != NULL) { wlan_mac_high_free(params); }

                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Could not allocate memory for CMDID_LTG_CONFIG\n");
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(id);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif //WLAN_SW_CONFIG_ENABLE_LTG
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LTG_START: {
#if WLAN_SW_CONFIG_ENABLE_LTG
            // NODE_LTG_START Packet Format:
            //   - cmd_args_32[0]      - LTG ID
            //
            //   - resp_args_32[0]     - CMD_PARAM_SUCCESS
            //                         - CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    id             = Xil_Ntohl(cmd_args_32[0]);
            int    ltg_status     = ltg_sched_start(id);

            if (ltg_status == 0) {
                if (id != CMD_PARAM_LTG_ALL_LTGS){
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Starting %d\n", id);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Starting all LTGs\n");
                }
            } else {
                if (id != CMD_PARAM_LTG_ALL_LTGS){
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to start %d\n", id);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to start all LTGs\n");
                }
                status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            }

            // Send response of current rate
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LTG_STOP: {
#if WLAN_SW_CONFIG_ENABLE_LTG
            // NODE_LTG_STOP Packet Format:
            //   - cmd_args_32[0]      - LTG ID
            //
            //   - resp_args_32[0]     - CMD_PARAM_SUCCESS
            //                         - CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    id             = Xil_Ntohl(cmd_args_32[0]);
            int    ltg_status     = ltg_sched_stop(id);

            if (ltg_status == 0) {
                if (id != CMD_PARAM_LTG_ALL_LTGS){
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Stopping %d\n", id);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Stopping all LTGs\n");
                }
            } else {
                if (id != CMD_PARAM_LTG_ALL_LTGS){
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to stop %d\n", id);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to stop all LTGs\n");
                }
                status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            }

            // Send response of current rate
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LTG_REMOVE: {
#if WLAN_SW_CONFIG_ENABLE_LTG
            // NODE_LTG_REMOVE Packet Format:
            //   - cmd_args_32[0]      - LTG ID
            //
            //   - resp_args_32[0]     - CMD_PARAM_SUCCESS
            //                         - CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    id             = Xil_Ntohl(cmd_args_32[0]);
            int    ltg_status     = ltg_sched_remove(id);

            if (ltg_status == 0) {
                if (id != CMD_PARAM_LTG_ALL_LTGS){
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Removing %d\n", id);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_ltg, "Removing all LTGs\n");
                }
            } else {
                if (id != CMD_PARAM_LTG_ALL_LTGS){
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to remove %d\n", id);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_ltg, "Failed to remove all LTGs\n");
                }
                status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif //WLAN_SW_CONFIG_ENABLE_LTG
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_LTG_STATUS: {
#if WLAN_SW_CONFIG_ENABLE_LTG
            // NODE_LTG_STATUS Packet Format:
            //   - cmd_args_32[0]      - LTG ID
            //
            //   - resp_args_32[0]     - CMD_PARAM_SUCCESS
            //                         - CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            //   - resp_args_32[1]     - CMD_PARAM_LTG_RUNNING
            //                         - CMD_PARAM_LTG_STOPPED
            //   - resp_args_32[3:2]   - Last start timestamp
            //   - resp_args_32[5:4]   - Last stop timestamp
            //
            u32         i;
            u32       * state;
            dl_entry  * curr_tg_dl_entry;
            u32        status          = CMD_PARAM_SUCCESS;
            u32        id              = Xil_Ntohl(cmd_args_32[0]);
            u32        max_args        = sizeof(ltg_sched_state_hdr) / 4;      // Maximum number of return args

            curr_tg_dl_entry = ltg_sched_find_tg_schedule(id);

            if(curr_tg_dl_entry != NULL){
                state  = (u32 *)((tg_schedule*)(curr_tg_dl_entry->data))->state;
            } else {
                status = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            if(curr_tg_dl_entry != NULL){
                for (i = 0; i < max_args; i++) {
                    resp_args_32[resp_index++] = Xil_Htonl(state[i]);
                }
            } else {
                for (i = 0; i < max_args; i++) {
                    resp_args_32[resp_index++] = 0xFFFFFFFF;
                }
            }

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
#endif //WLAN_SW_CONFIG_ENABLE_LTG
        }
        break;


//-----------------------------------------------------------------------------
// Node Commands
//-----------------------------------------------------------------------------

        //---------------------------------------------------------------------
	   case CMDID_NODE_CHANNEL: {
		   //   - cmd_args_32[0]      - Command
		   //   - cmd_args_32[1]      - Channel
		   //
		   u32    status         = CMD_PARAM_SUCCESS;
		   u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
		   u32    channel        = Xil_Ntohl(cmd_args_32[1]);

		   if (msg_cmd == CMD_PARAM_WRITE_VAL) {
			   // Set the Channel
			   if (wlan_verify_channel(channel) == 0){

				   wlan_mac_high_set_radio_channel(channel);

				   wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set Channel = %d\n", channel);

			   } else {
				   status  = CMD_PARAM_ERROR;
				   wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node,
								   "Channel %d is not supported by the node.\n", channel);
			   }
		   }

		   // Send response
		   resp_args_32[resp_index++] = Xil_Htonl(status);
		   resp_args_32[resp_index++] = Xil_Htonl(low_param_channel);

		   resp_hdr->length  += (resp_index * sizeof(resp_args_32));
		   resp_hdr->num_args = resp_index;
	   }
	   break;


        //---------------------------------------------------------------------
        // CMDID_NODE_RESET_STATE implemented in child classes
        //


        //---------------------------------------------------------------------
        case CMDID_NODE_CONFIGURE: {
            // CMDID_NODE_CONFIGURE Packet Format:
            //   - cmd_args_32[0]  - Flags
            //                     [0] - NODE_CONFIG_FLAG_DSSS_ENABLE
            //                     [1] - NODE_CONFIG_FLAG_
            //   - cmd_args_32[1]  - Flag mask
            //   - cmd_args_32[2]  - WLAN Exp debug level
            //                     [31]  - Set debug level
            //                     [7:0] - Debug level
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    flags          = Xil_Ntohl(cmd_args_32[0]);
            u32    mask           = Xil_Ntohl(cmd_args_32[1]);
            u32    debug_level    = Xil_Ntohl(cmd_args_32[2]);

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Configure flags = 0x%08x  mask = 0x%08x\n", flags, mask);

            // Set DSS Enable / Disable
            if (mask & CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE) {
                if (flags & CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE) {
                    wlan_mac_high_set_dsss(0x1);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Enabled DSSS\n");
                } else {
                    wlan_mac_high_set_dsss(0x0);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disabled DSSS\n");
                }
            }

            // Set MAC time update from beacon Enable / Disable
            if (mask & CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE) {
                if (flags & CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE) {
                    wlan_exp_beacon_ts_update_mode_callback(1);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Enable MAC time update from beacons\n");
                } else {
                    wlan_exp_beacon_ts_update_mode_callback(0);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Disabled MAC time update from beacons\n");
                }
            }

            // Set debug print level
            if (debug_level & CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL) {
                wlan_exp_set_print_level(debug_level & 0xFF);
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_WLAN_MAC_ADDR: {
            // Get / Set the wireless MAC address
            //
            // Message format:
            //     cmd_args_32[0]    Command:
            //                       - Write       (NODE_WRITE_VAL)
            //                       - Read        (NODE_READ_VAL)
            //     cmd_args_32[1:2]  MAC Address (write-only)
            //
            // Response format:
            //     resp_args_32[0]   Status
            //     resp_args_32[1:2] Current MAC Address
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    // This is dangerous and is not supported
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Setting Wireless MAC Address not supported\n");
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            wlan_exp_put_mac_addr(get_wlan_mac_addr(), &resp_args_32[resp_index]);
            resp_index += 2;

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_TIME: {
            // Set / Get node time
            //
            // Message format:
            //     cmd_args_32[0]   Command:
            //                      - Write                (NODE_WRITE_VAL)
            //                      - Read                 (NODE_READ_VAL)
            //                      - Add to log           (NODE_TIME_ADD_TO_LOG_VAL)
            //                      - Add to log on change (NODE_TIME_ADD_ON_CHANGE)
            //     cmd_args_32[1]   Time ID
            //     cmd_args_32[2]   New MAC Time in microseconds - lower 32 bits (or NODE_TIME_RSVD_VAL)
            //     cmd_args_32[3]   New MAC Time in microseconds - upper 32 bits (or NODE_TIME_RSVD_VAL)
            //     cmd_args_32[4]   Host Time in microseconds    - lower 32 bits (or NODE_TIME_RSVD_VAL)
            //     cmd_args_32[5]   Host Time in microseconds    - upper 32 bits (or NODE_TIME_RSVD_VAL)
            //
            // Response format:
            //     resp_args_32[0]  Status
            //     resp_args_32[1]  MAC Time on node in microseconds    - lower 32 bits
            //     resp_args_32[2]  MAC Time on node in microseconds    - upper 32 bits
            //     resp_args_32[3]  System Time on node in microseconds - lower 32 bits
            //     resp_args_32[4]  System Time on node in microseconds - upper 32 bits
            //
            u32    temp_lo, temp_hi;
            u32    status           = CMD_PARAM_SUCCESS;
            u32    msg_cmd          = Xil_Ntohl(cmd_args_32[0]);
            u32    id               = Xil_Ntohl(cmd_args_32[1]);

            u64    new_mac_time;
            u64    host_timestamp;
            u64    mac_timestamp    = get_mac_time_usec();
            u64    system_timestamp = get_system_time_usec();

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                case CMD_PARAM_NODE_TIME_ADD_TO_LOG_VAL:
                    // Get the new time
                    temp_lo      = Xil_Ntohl(cmd_args_32[2]);
                    temp_hi      = Xil_Ntohl(cmd_args_32[3]);
                    new_mac_time = (((u64)temp_hi) << 32) + ((u64)temp_lo);

                    // If this is a write, then update the time on the node
                    if (msg_cmd == CMD_PARAM_WRITE_VAL){
                        set_mac_time_usec(new_mac_time);
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set time  = 0x%08x 0x%08x\n", temp_hi, temp_lo);
                    }

                    // Get the Host time
                    temp_lo        = Xil_Ntohl(cmd_args_32[4]);
                    temp_hi        = Xil_Ntohl(cmd_args_32[5]);
                    host_timestamp = (((u64)temp_hi) << 32) + ((u64)temp_lo);

                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Host time = 0x%08x 0x%08x\n", temp_hi, temp_lo);
#if WLAN_SW_CONFIG_ENABLE_LOGGING
                    // Add a time info log entry
                    if (msg_cmd == CMD_PARAM_WRITE_VAL) {
                        add_time_info_entry(mac_timestamp, new_mac_time, system_timestamp, host_timestamp, TIME_INFO_ENTRY_WLAN_EXP_SET_TIME, id, WLAN_EXP_TRUE);
                    } else {
                        add_time_info_entry(mac_timestamp, new_mac_time, system_timestamp, host_timestamp, TIME_INFO_ENTRY_WLAN_EXP_ADD_LOG, id, WLAN_EXP_TRUE);
                    }
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING

                    // If this was a write, then update the time value so we can return it to the host
                    //   This is done after the log entry to the fields are correct in the entry.
                    if (msg_cmd == CMD_PARAM_WRITE_VAL){
                        mac_timestamp = new_mac_time;
                    }
                break;

                case CMD_PARAM_READ_VAL:
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            // Add the MAC time to the response
            temp_lo = mac_timestamp & 0xFFFFFFFF;
            temp_hi = (mac_timestamp >> 32) & 0xFFFFFFFF;

            resp_args_32[resp_index++] = Xil_Htonl(temp_lo);
            resp_args_32[resp_index++] = Xil_Htonl(temp_hi);

            // Add the System time to the response
            temp_lo = system_timestamp & 0xFFFFFFFF;
            temp_hi = (system_timestamp >> 32) & 0xFFFFFFFF;

            resp_args_32[resp_index++] = Xil_Htonl(temp_lo);
            resp_args_32[resp_index++] = Xil_Htonl(temp_hi);

            // Complete the response
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_LOW_TO_HIGH_FILTER: {
            // Set node MAC low to high filter
            //
            // Message format:
            //     cmd_args_32[0]   Command
            //     cmd_args_32[1]   RX Filter
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32    filter_mode    = Xil_Ntohl(cmd_args_32[1]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set RX filter = 0x%08x\n", filter_mode);
                    wlan_mac_high_set_rx_filter_mode(filter_mode);
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


        //---------------------------------------------------------------------
        case CMDID_NODE_RANDOM_SEED: {
            // Set the random seed for the random number generator for cpu high / low
            //
            // Message format:
            //     cmd_args_32[0]   Command (only writes are supported
            //     cmd_args_32[1]   CPU High Seed Valid
            //     cmd_args_32[2]   CPU High Seed
            //     cmd_args_32[3]   CPU Low  Seed Valid
            //     cmd_args_32[4]   CPU Low  Seed
            //
            // Response format:
            //     resp_args_32[0]  Status
            //
            u32    seed;
            u32    seed_valid;
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    // Process the seed for CPU high
                    seed_valid = Xil_Ntohl(cmd_args_32[1]);
                    seed       = Xil_Ntohl(cmd_args_32[2]);
                    if (seed_valid == CMD_PARAM_RANDOM_SEED_VALID) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set CPU High random seed = 0x%08x\n", seed);
                        srand(seed);
                    }

                    // Process the seed for CPU low
                    seed_valid = Xil_Ntohl(cmd_args_32[3]);
                    seed       = Xil_Ntohl(cmd_args_32[4]);
                    if (seed_valid == CMD_PARAM_RANDOM_SEED_VALID) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set CPU Low  random seed = 0x%08x\n", seed);
                        wlan_mac_high_set_srand(seed);
                    }
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


        //---------------------------------------------------------------------
        case CMDID_NODE_LOW_PARAM: {
            // Set node MAC low to high filter
            //
            // Message format:
            //     cmd_args_32[0]    Command
            //     cmd_args_32[1]    Size in words of LOW_PARAM_MESSAGE
            //     cmd_args_32[2]    LOW_PARAM_MESSAGE
            //                       [0]   PARAM_ID
            //                         [1:N] ARGS
            //
            // Response format:
            //     resp_args_32[0]    Status
            //
            u32    i;
            u32    id;
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32    size           = Xil_Ntohl(cmd_args_32[1]);

            // Byte swap all the payload words for the LOW_PARAM_MESSAGE
            for (i = 2; i < (size + 2); i++) {
                cmd_args_32[i] = Xil_Ntohl(cmd_args_32[i]);
            }

            id      = cmd_args_32[2];        // Already byte swapped in for loop above

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_mac_high_write_low_param(size, &(cmd_args_32[2]));
                break;

                case CMD_PARAM_READ_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Parameter read not allowed.\n");
                    status = CMD_PARAM_ERROR + id;
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR + id;
                break;
            }

            // Send default response
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_TX_POWER: {
            // CMDID_NODE_TX_POWER Packet Format:
            //   - cmd_args_32[0]      - Command
            //   - cmd_args_32[1]      - Type
            //   - cmd_args_32[2]      - Power (Shifted by TX_POWER_MIN_DBM)
            //   - cmd_args_32[3 - 4]  - MAC Address (All 0xF means all nodes)
            //
            u32    id;
            int    power;
            u8     mac_addr[MAC_ADDR_LEN];
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32    type           = Xil_Ntohl(cmd_args_32[1]);
            u32    power_xmit     = Xil_Ntohl(cmd_args_32[2]);

            // Shift power value from transmission to get the power
            power = power_xmit + TX_POWER_MIN_DBM;

            // Adjust the power so that it falls in an acceptable range
            if(power < TX_POWER_MIN_DBM){ power = TX_POWER_MIN_DBM; }
            if(power > TX_POWER_MAX_DBM){ power = TX_POWER_MAX_DBM; }

            // Process the command
            if (type == CMD_PARAM_UNICAST_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_READ_VAL:
                        // Get MAC Address
                        wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[3], &mac_addr[0]);

                        // If necessary, add an association.  This is primarily for IBSS nodes where
                        //   the association table might not be set up at the time this is called.
                        // NOTE: A multicast mac_addr should *not* be added to the association table.
                        //     Also, an address of zero should not be added to the association table
                        if ((wlan_addr_mcast(mac_addr) == 0) && (wlan_addr_eq(mac_addr, zero_addr) == 0)) {
                            wlan_exp_tx_cmd_add_association_callback(&mac_addr[0]);
                        }

                        id = wlan_exp_get_id_in_associated_stations(&mac_addr[0]);

                        status = process_tx_power(msg_cmd, id, power);
                    break;

                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default unicast data & management parameter
                        default_unicast_data_tx_params.phy.power = power;
                        default_unicast_mgmt_tx_params.phy.power = power;
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set default unicast TX power = %d dBm\n", power);
                    break;

                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default unicast data parameter
                        power = default_unicast_data_tx_params.phy.power;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_MULTICAST_DATA_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default multicast data parameter
                        default_multicast_data_tx_params.phy.power = power;
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set default multicast data TX power = %d dBm\n", power);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default multicast data parameter
                        power = default_multicast_data_tx_params.phy.power;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_MULTICAST_MGMT_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default multicast management parameter
                        default_multicast_mgmt_tx_params.phy.power = power;
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set default multicast mgmt TX power = %d dBm\n", power);

                        // Update beacon tx params
                        wlan_mac_high_update_beacon_tx_params(&default_multicast_mgmt_tx_params);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default multicast management parameter
                        power = default_multicast_mgmt_tx_params.phy.power;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_NODE_TX_POWER_LOW) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Send IPC to CPU low to set the Tx power for control frames
                        wlan_mac_high_set_tx_ctrl_pow(power);
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set control packet TX power = %d dBm\n", power);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Reading control packet power not currently supported\n");
                        status = CMD_PARAM_ERROR;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_NODE_TX_POWER_ALL) {
                // Set all power values:
                //     * Default Unicast Management Packet Tx Power for new associations
                //     * Default Unicast Data Packet Tx Power for new associations
                //     * Default Multicast Management Packet Tx Power for new associations
                //     * Default Multicast Data Packet Tx Power for new associations
                //     * Control Packet Tx Power
                //     * Update the transmit power of all current associations on the node.
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set all TX power = %d dBm\n", power);

                // Set the default unicast power for new associations
                default_unicast_mgmt_tx_params.phy.power   = power;
                default_unicast_data_tx_params.phy.power   = power;

                // Set the default multicast power for new associations
                default_multicast_mgmt_tx_params.phy.power = power;
                default_multicast_data_tx_params.phy.power = power;

                // Send IPC to CPU low to set the Tx power for control frames
                wlan_mac_high_set_tx_ctrl_pow(power);

                // Update the Tx power in each current association
                status = process_tx_power(CMD_PARAM_WRITE_VAL, WLAN_EXP_AID_ALL, power);

                // Update beacon tx params
                wlan_mac_high_update_beacon_tx_params(&default_multicast_mgmt_tx_params);
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown type for CMDID_NODE_TX_POWER: %d\n", type);
                status = CMD_PARAM_ERROR;
            }

            // Shift power for transmission
            power_xmit = power - TX_POWER_MIN_DBM;

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(power_xmit);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_TX_RATE: {
            // NODE_TX_RATE Packet Format:
            //   - cmd_args_32[0]      - Command
            //   - cmd_args_32[1]      - Type
            //   - cmd_args_32[2]      - MCS
            //   - cmd_args_32[3]      - PHY Mode
            //   - cmd_args_32[4 - 5]  - MAC Address (All 0xF means all nodes)
            //
            u32 id;
            u8  mac_addr[MAC_ADDR_LEN];
            u32 status         = CMD_PARAM_SUCCESS;
            u32 msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32 type           = Xil_Ntohl(cmd_args_32[1]);
            u32 mcs            = Xil_Ntohl(cmd_args_32[2]) & 0xFF;
            u32 phy_mode       = Xil_Ntohl(cmd_args_32[3]) & 0xFF;
            u32 ret_mcs;
            u32 ret_phy_mode;

            // Force invalid mcs / phy_mode values to sane defaults
            if (mcs > 7) {
                mcs = 7;
            }

            if ((phy_mode & (PHY_MODE_NONHT | PHY_MODE_HTMF)) == 0) {
                phy_mode = PHY_MODE_NONHT;
            }

            // Set default return values
            ret_mcs      = mcs;
            ret_phy_mode = phy_mode;

            // Process the command
            if (type == CMD_PARAM_UNICAST_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_READ_VAL:
                        // Get MAC Address
                        wlan_exp_get_mac_addr(&cmd_args_32[4], &mac_addr[0]);

                        // If necessary, add an association.  This is primarily for IBSS nodes where
                        //   the association table might not be set up at the time this is called.
                        // NOTE: A multicast mac_addr should *not* be added to the association table.
                        //     Also, an address of zero should not be added to the association table
                        if ((wlan_addr_mcast(mac_addr) == 0) && (wlan_addr_eq(mac_addr, zero_addr) == 0)) {
                            wlan_exp_tx_cmd_add_association_callback(&mac_addr[0]);
                        }

                        id = wlan_exp_get_id_in_associated_stations(&mac_addr[0]);

                        if (id != WLAN_EXP_AID_NONE) {
                            status = process_tx_rate(msg_cmd, id, mcs, phy_mode, &ret_mcs, &ret_phy_mode);
                        } else {
                            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Station not found\n");
                            status = CMD_PARAM_ERROR;
                        }
                    break;

                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default unicast data & management parameter
                        default_unicast_data_tx_params.phy.mcs      = mcs;
                        default_unicast_data_tx_params.phy.phy_mode = phy_mode;

                        default_unicast_mgmt_tx_params.phy.mcs      = mcs;
                        default_unicast_mgmt_tx_params.phy.phy_mode = phy_mode;

                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                        "Set default unicast Tx rate to MCS %d, PHY mode %d\n", mcs, phy_mode);
                    break;

                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default unicast data parameter
                        ret_mcs      = default_unicast_data_tx_params.phy.mcs;
                        ret_phy_mode = default_unicast_data_tx_params.phy.phy_mode;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_MULTICAST_DATA_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default multicast data parameter
                        default_multicast_data_tx_params.phy.mcs      = mcs;
                        default_multicast_data_tx_params.phy.phy_mode = phy_mode;

                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                        "Set default multicast data Tx rate to MCS %d, PHY mode %d\n", mcs, phy_mode);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default multicast data parameter
                        ret_mcs      = default_multicast_data_tx_params.phy.mcs;
                        ret_phy_mode = default_multicast_data_tx_params.phy.phy_mode;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_MULTICAST_MGMT_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default multicast management parameter
                        default_multicast_mgmt_tx_params.phy.mcs      = mcs;
                        default_multicast_mgmt_tx_params.phy.phy_mode = phy_mode;

                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                        "Set default multicast mgmt Tx rate to MCS %d, PHY mode %d\n", mcs, phy_mode);

                        // Update beacon tx params
                        wlan_mac_high_update_beacon_tx_params(&default_multicast_mgmt_tx_params);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default multicast management parameter
                        ret_mcs      = default_multicast_mgmt_tx_params.phy.mcs;
                        ret_phy_mode = default_multicast_mgmt_tx_params.phy.phy_mode;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown type for NODE_TX_RATE: %d\n", type);
                status = CMD_PARAM_ERROR;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(ret_mcs);
            resp_args_32[resp_index++] = Xil_Htonl(ret_phy_mode);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_TX_ANT_MODE: {
            // NODE_TX_ANT_MODE Packet Format:
            //   - cmd_args_32[0]      - Command
            //   - cmd_args_32[1]      - Type
            //   - cmd_args_32[2]      - Antenna Mode
            //   - cmd_args_32[3 - 4]  - MAC Address (All 0xF means all nodes)
            //
            u32    id;
            u8     mac_addr[MAC_ADDR_LEN];
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32    type           = Xil_Ntohl(cmd_args_32[1]);
            u32    ant_mode       = Xil_Ntohl(cmd_args_32[2]);

            // NOTE:  This method assumes that the Antenna mode received is valid.
            // The checking will be done on either the host, in CPU Low or both.

            // Need to convert antenna mode from:   Python       C
            //     - TX_ANTMODE_SISO_ANTA:            0x0   to  0x10
            //     - TX_ANTMODE_SISO_ANTB:            0x1   to  0x20
            //     - TX_ANTMODE_SISO_ANTC:            0x2   to  0x30
            //     - TX_ANTMODE_SISO_ANTD:            0x3   to  0x40
            //
            // Formula:  y = (x + 1) << 4;
            //
            ant_mode = (ant_mode + 1) << 4;

            // Process command
            if (type == CMD_PARAM_UNICAST_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_READ_VAL:
                        // Get MAC Address
                        wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[3], &mac_addr[0]);

                        // If necessary, add an association.  This is primarily for IBSS nodes where
                        //   the association table might not be set up at the time this is called.
                        // NOTE: A multicast mac_addr should *not* be added to the association table.
                        //     Also, an address of zero should not be added to the association table
                        if ((wlan_addr_mcast(mac_addr) == 0) && (wlan_addr_eq(mac_addr, zero_addr) == 0)) {
                            wlan_exp_tx_cmd_add_association_callback(&mac_addr[0]);
                        }

                        id = wlan_exp_get_id_in_associated_stations(&mac_addr[0]);

                        ant_mode = process_tx_ant_mode(msg_cmd, id, (ant_mode & 0xFF));

                        if (ant_mode == CMD_PARAM_ERROR) {
                            status = CMD_PARAM_ERROR;
                        }
                    break;

                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default unicast data & management parameter
                        default_unicast_data_tx_params.phy.antenna_mode = ant_mode;
                        default_unicast_mgmt_tx_params.phy.antenna_mode = ant_mode;
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set default unicast TX antenna mode = %d\n", ant_mode);
                    break;

                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default unicast data parameter
                        ant_mode = default_unicast_data_tx_params.phy.antenna_mode;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_MULTICAST_DATA_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default multicast data paramter
                        default_multicast_data_tx_params.phy.antenna_mode = ant_mode;
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set default multicast data TX antenna mode = %d\n", ant_mode);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default multicast data parameter
                        ant_mode = default_multicast_data_tx_params.phy.antenna_mode;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_MULTICAST_MGMT_VAL) {
                switch (msg_cmd) {
                    case CMD_PARAM_WRITE_VAL:
                    case CMD_PARAM_WRITE_DEFAULT_VAL:
                        // Set the default multicast management parameter
                        default_multicast_mgmt_tx_params.phy.antenna_mode = ant_mode;
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set default multicast mgmt TX antenna mode = %d\n", ant_mode);

                        // Update beacon tx params
                        wlan_mac_high_update_beacon_tx_params(&default_multicast_mgmt_tx_params);
                    break;

                    case CMD_PARAM_READ_VAL:
                    case CMD_PARAM_READ_DEFAULT_VAL:
                        // Get the default multicast management parameter
                        ant_mode = default_multicast_mgmt_tx_params.phy.antenna_mode;
                    break;

                    default:
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                        status = CMD_PARAM_ERROR;
                    break;
                }
            } else if (type == CMD_PARAM_NODE_TX_ANT_ALL) {
                // Set all power values:
                //     * Default Unicast Management Packet Tx antenna mode for new associations
                //     * Default Unicast Data Packet Tx antenna mode for new associations
                //     * Default Multicast Management Packet Tx antenna mode for new associations
                //     * Default Multicast Data Packet Tx antenna mode for new associations
                //     * Update the transmit antenna mode of all current associations on the node.
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set all TX ant mode = %d\n", ant_mode);

                // Set the default unicast antenna mode for new associations
                default_unicast_data_tx_params.phy.antenna_mode = ant_mode;
                default_unicast_mgmt_tx_params.phy.antenna_mode = ant_mode;

                // Set the default multicast antenna mode for new associations
                default_multicast_data_tx_params.phy.antenna_mode = ant_mode;
                default_multicast_mgmt_tx_params.phy.antenna_mode = ant_mode;

                // Update the Tx antenna mode in each current association
                ant_mode = process_tx_ant_mode(CMD_PARAM_WRITE_VAL, WLAN_EXP_AID_ALL, (ant_mode & 0xFF));

                if (ant_mode == CMD_PARAM_ERROR) {
                    status = CMD_PARAM_ERROR;
                }

                // Update beacon tx params
                wlan_mac_high_update_beacon_tx_params(&default_multicast_mgmt_tx_params);
            } else {
                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown type for NODE_TX_ANT_MODE: %d\n", type);
                status = CMD_PARAM_ERROR;
            }

            // Need to convert antenna mode from:      C         Python
            //     - TX_ANTMODE_SISO_ANTA:            0x10   to   0x0
            //     - TX_ANTMODE_SISO_ANTB:            0x20   to   0x1
            //     - TX_ANTMODE_SISO_ANTC:            0x30   to   0x2
            //     - TX_ANTMODE_SISO_ANTD:            0x40   to   0x3
            //
            // Formula:  y = (x >> 4) - 1;
            //
            ant_mode = (ant_mode >> 4) - 1;

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(ant_mode & 0xFF);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_RX_ANT_MODE: {
            // NODE_RX_ANT_MODE Packet Format:
            //   - cmd_args_32[0]      - Command
            //   - cmd_args_32[1]      - Antenna Mode
            //
            // NOTE:  This method assumes that the Antenna mode received is valid.
            // The checking will be done on either the host, in CPU Low or both.
            //
            u32    status         = CMD_PARAM_SUCCESS;
            u32    msg_cmd        = Xil_Ntohl(cmd_args_32[0]);
            u32    ant_mode       = Xil_Ntohl(cmd_args_32[1]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                case CMD_PARAM_WRITE_DEFAULT_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set RX antenna mode = %d\n", ant_mode);
                    wlan_mac_high_set_rx_ant_mode(ant_mode);
                break;

                case CMD_PARAM_READ_VAL:
                case CMD_PARAM_READ_DEFAULT_VAL:
                    ant_mode = low_param_rx_ant_mode;
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(ant_mode);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// Scan Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_SCAN_PARAM: {
            // Set the active scan parameters
            //
            // Message format:
            //     cmd_args_32[0]    Command:
            //                           - Write       (NODE_WRITE_VAL)
            //     cmd_args_32[1]    Time per channel (in microseconds)
            //                         (or CMD_PARAM_NODE_TIME_RSVD_VAL if not setting the parameter)
            //     cmd_args_32[2]    Number of probe request Tx per channel
            //                         (or CMD_PARAM_RSVD if not setting the parameter)
            //     cmd_args_32[3]    Length of channel list
            //                         (or CMD_PARAM_RSVD if not setting channel list)
            //     cmd_args_32[4:N]  Channel
            //     cmd_args_32[N+1]  Length of SSID
            //                         (or CMD_PARAM_RSVD if not setting SSID)
            //     cmd_args_32[N+2]  SSID
            //
            // Response format:
            //     resp_args_32[0]   Status
            //
            u32                             i;
            volatile scan_parameters_t    * scan_params;
            u32                             time_per_channel;
            u32                             num_probe_tx;
            u32                             channel_list_len;
            u8                            * channel_list;
            u32                             is_scanning;
            u32                             ssid_len;
            char                          * ssid;
            u32                             update_probe_interval    = 0;
            u32                             curr_num_probe_tx        = 0;
            u32                             status         = CMD_PARAM_SUCCESS;
            u32                             msg_cmd        = Xil_Ntohl(cmd_args_32[0]);

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Set Scan Parameters\n");

                    // Check if node is currently in a scan
                    is_scanning = wlan_mac_scan_is_scanning();

                    // Stop the current scan to update the scan parameters
                    //     - Because the underlying channel list can be updated, the scan is stopped
                    //       vs being paused.  This will reduce any corner cases.
                    if (is_scanning) {
                        wlan_mac_scan_stop();
                    }

                    // Get current scan parameters
                    scan_params = wlan_mac_scan_get_parameters();

                    // Set the time per channel
                    time_per_channel   = Xil_Ntohl(cmd_args_32[1]);

                    if (time_per_channel != CMD_PARAM_NODE_TIME_RSVD_VAL) {
                        // Compute current num_probe_tx
                        if (scan_params->probe_tx_interval_usec == 0) {
                            curr_num_probe_tx = 0;
                        } else {
                            curr_num_probe_tx = scan_params->time_per_channel_usec / scan_params->probe_tx_interval_usec;
                        }

                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Time per channel   = %d us\n", time_per_channel);
                        scan_params->time_per_channel_usec = time_per_channel;
                        update_probe_interval = 1;
                    }

                    // Set Probe request interval
                    num_probe_tx = Xil_Ntohl(cmd_args_32[2]);

                    if (num_probe_tx != CMD_PARAM_RSVD) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Num Probe Req      = %d \n", num_probe_tx);
                        curr_num_probe_tx = num_probe_tx;
                        update_probe_interval = 1;
                    }

                    // Set the probe_tx_interval
                    if (update_probe_interval) {
                        if (curr_num_probe_tx == 0) {
                            scan_params->probe_tx_interval_usec = 0;
                        } else {
                            scan_params->probe_tx_interval_usec = scan_params->time_per_channel_usec / curr_num_probe_tx;
                        }
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Probe Req interval = %d us\n", scan_params->probe_tx_interval_usec);
                    }


                    // Set the scan channels
                    channel_list_len = Xil_Ntohl(cmd_args_32[3]);

                    if (channel_list_len != CMD_PARAM_RSVD){
                        // Free the current channel list in the scan parameters
                        wlan_mac_high_free(scan_params->channel_vec);

                        // Update new channel list
                        channel_list = wlan_mac_high_malloc(channel_list_len);

                        for (i = 0; i < channel_list_len; i++) {
                            channel_list[i] = Xil_Ntohl(cmd_args_32[4 + i]);
                        }

                        // Set scan parameters
                        scan_params->channel_vec_len = channel_list_len;
                        scan_params->channel_vec     = channel_list;

                        // Print information about the new channels
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Channels = ");
                        for (i = 0; i < channel_list_len; i++) {
                            wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "%d ",channel_list[i]);
                        }
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");
                    } else {
                        // No channel list to process
                        channel_list_len = 0;
                    }

                    // Set the SSID
                    ssid_len = Xil_Ntohl(cmd_args_32[4 + channel_list_len]);

                    if (ssid_len != CMD_PARAM_RSVD){
                        // Get pointer to new SSID
                        ssid = (char *) &cmd_args_32[5 + channel_list_len];

                        // Free the current ssid in the scan parameters
                        wlan_mac_high_free(scan_params->ssid);

                        // Update new ssid
                        scan_params->ssid = strndup(ssid, SSID_LEN_MAX);

                        // Print information about the new channels
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  SSID = %s\n", scan_params->ssid);
                    }

                    // If the node was scanning, re-start the scan
                    if (is_scanning) {
                        wlan_mac_scan_start();
                    }
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_SCAN: {
            // Enable / Disable active scan
            //
            //   Scans initiated by WLAN Exp will use the current scan parameters.  To
            // update the scan parameters use the CMDID_NODE_SCAN_PARAM command.
            //
            // Message format:
            //     cmd_args_32[0]   Enable / Disable scan
            //                          - CMD_PARAM_NODE_SCAN_ENABLE  - Enable scan
            //                          - CMD_PARAM_NODE_SCAN_DISABLE - Disable scan
            //                          - CMD_PARAM_RSVD              - Do nothing
            //
            // Response format:
            //     resp_args_32[0]  Status
            //     resp_args_32[1]  Is Scanning?
            //
            u32    		status         = CMD_PARAM_SUCCESS;
            u32    		enable         = Xil_Ntohl(cmd_args_32[0]);
            bss_info_t* 	active_bss_info = ((bss_info_t*)wlan_exp_active_bss_info_getter_callback());

            switch (enable) {
                case CMD_PARAM_NODE_SCAN_ENABLE:
                    // Enable scan
                    if (active_bss_info == NULL) {
                        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Scan enabled.\n");
                        wlan_mac_scan_start();
                    } else {
                        // "my_bss_info" must be NULL to start a scan
                        //     - This will avoid any corner cases with scanning
                        status = CMD_PARAM_ERROR;
                    }
                break;

                case CMD_PARAM_NODE_SCAN_DISABLE:
                    // Disable scan
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Scan disabled.\n");
                    wlan_mac_scan_stop();
                break;
            }

            // Send response of status
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_args_32[resp_index++] = Xil_Htonl(wlan_mac_scan_is_scanning());

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


//-----------------------------------------------------------------------------
// Association Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_NODE_CONFIG_BSS: {
            // Configure the BSS
            //
            // Message format:
            //     cmd_args_32[0]      - Data length
            //     cmd_args_32[1:N]    - BSS config structure
            //
            // Response format:
            //     resp_args_32[0]     - Status
            //
            u32              status         = CMD_PARAM_SUCCESS;
            bss_config_t   * bss_config     = (bss_config_t *)(&cmd_args_32[1]);

            // Each MAC implementation is responsible for the implementation of this command.
            status = wlan_exp_process_config_bss_callback(bss_config);

            // If there was an error, add CMD_PARAM_ERROR bits on return value
            if (status != CMD_PARAM_SUCCESS) {
                status |= CMD_PARAM_ERROR;
            }

            // Send response
            resp_args_32[resp_index++] = Xil_Htonl(status);
            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = resp_index;
        }
        break;


        //---------------------------------------------------------------------
        // Case NODE_DISASSOCIATE      is implemented in the child classes


        //---------------------------------------------------------------------
        case CMDID_NODE_GET_BSS_MEMBERS: {
            // NODE_GET_STATION_INFO Packet Format:
            //   - cmd_args_32[0]   - buffer id
            //   - cmd_args_32[1]   - flags
            //   - cmd_args_32[2]   - start_address of transfer
            //   - cmd_args_32[3]   - size of transfer (in bytes)
            //   - cmd_args_32[4:5] - MAC Address (All 0xFF means all entries)
            //
            // Always returns a valid WLAN Exp Buffer (either 1 or more packets)
            //   - buffer_id       - uint32  - buffer_id
            //   - flags           - uint32  - 0
            //   - bytes_remaining - uint32  - Number of bytes remaining in the transfer
            //   - start_byte      - uint32  - Byte index of the first byte in this packet
            //   - size            - uint32  - Number of payload bytes in this packet
            //   - byte[]          - uint8[] - Array of payload bytes
            //
            resp_sent = process_buffer_cmds(socket_index, from, command, response,
                                            cmd_hdr, cmd_args_32, resp_hdr, resp_args_32, eth_dev_num, max_resp_len,
                                            print_type_node, "station info",
                                            get_bss_member_list(),
                                            sizeof(wlan_exp_station_info_t),
                                            &wlan_exp_get_id_in_associated_stations,
                                            &find_station_info,
                                            &copy_station_info_to_dest,
                                            &zero_station_info);
        }
        break;

        //---------------------------------------------------------------------
        case CMDID_NODE_GET_STATION_INFO_LIST: {
            // NODE_GET_STATION_INFO Packet Format:
            //   - cmd_args_32[0]   - buffer id
            //   - cmd_args_32[1]   - flags
            //   - cmd_args_32[2]   - start_address of transfer
            //   - cmd_args_32[3]   - size of transfer (in bytes)
            //   - cmd_args_32[4:5] - MAC Address (All 0xFF means all entries)
            //
            // Always returns a valid WLAN Exp Buffer (either 1 or more packets)
            //   - buffer_id       - uint32  - buffer_id
            //   - flags           - uint32  - 0
            //   - bytes_remaining - uint32  - Number of bytes remaining in the transfer
            //   - start_byte      - uint32  - Byte index of the first byte in this packet
            //   - size            - uint32  - Number of payload bytes in this packet
            //   - byte[]          - uint8[] - Array of payload bytes
            //
            resp_sent = process_buffer_cmds(socket_index, from, command, response,
                                            cmd_hdr, cmd_args_32, resp_hdr, resp_args_32, eth_dev_num, max_resp_len,
                                            print_type_node, "station info",
                                            station_info_get_list(),
                                            sizeof(wlan_exp_station_info_t),
                                            &wlan_exp_get_id_in_associated_stations,
                                            &find_station_info,
                                            &copy_station_info_to_dest,
                                            &zero_station_info);
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_NODE_GET_BSS_INFO: {
            // NODE_GET_BSS_INFO Packet Format:
            //   - cmd_args_32[0]   - buffer id
            //   - cmd_args_32[1]   - flags
            //   - cmd_args_32[2]   - start_address of transfer
            //   - cmd_args_32[3]   - size of transfer (in bytes)
            //   - cmd_args_32[4:5] - MAC Address (All 0x00 means all entries)
            //
            // Always returns a valid WLAN Exp Buffer (either 1 or more packets)
            //   - buffer_id       - uint32  - buffer_id
            //   - flags           - uint32  - 0
            //   - bytes_remaining - uint32  - Number of bytes remaining in the transfer
            //   - start_byte      - uint32  - Byte index of the first byte in this packet
            //   - size            - uint32  - Number of payload bytes in this packet
            //   - byte[]          - uint8[] - Array of payload bytes
            //
            u8   			process_buffer = 1;
            bss_info_t* 	active_bss_info = ((bss_info_t*)wlan_exp_active_bss_info_getter_callback());

            // If MAC address is all zeros, then return my_bss_info
            if ((cmd_args_32[4] == CMD_PARAM_RSVD) && (cmd_args_32[5] == CMD_PARAM_RSVD)) {
                if (active_bss_info != NULL) {
                    // Replace MAC address of command with my_bss_info BSSID
                    wlan_exp_put_mac_addr(active_bss_info->bssid, &cmd_args_32[4]);
                } else {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Return NULL BSS info\n");

                    // Do not use process buffer command and return Null buffer
                    //     - This will prevent a collision with the broadcast BSSID of all 0xFFs
                    process_buffer = 0;

                    // Set return values
                    resp_args_32[0] = cmd_args_32[0];
                    resp_args_32[1] = cmd_args_32[1];
                    resp_args_32[2] = 0;
                    resp_args_32[3] = 0;
                    resp_args_32[4] = 0;

                    // Set the length and number of response args
                    resp_hdr->length  += (5 * sizeof(resp_args_32));
                    resp_hdr->num_args = resp_index;
                }
            }

            if (process_buffer) {
                resp_sent = process_buffer_cmds(socket_index, from, command, response,
                                                cmd_hdr, cmd_args_32, resp_hdr, resp_args_32, eth_dev_num, max_resp_len,
                                                print_type_node, "bss info",
                                                wlan_mac_high_get_bss_info_list(),
                                                sizeof(wlan_exp_bss_info_t),
                                                &wlan_exp_get_id_in_bss_info,
                                                &wlan_mac_high_find_bss_info_BSSID,
                                                &copy_bss_info_to_dest,
                                                &zero_bss_info);
            }
        }
        break;


//-----------------------------------------------------------------------------
// Queue Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_QUEUE_TX_DATA_PURGE_ALL: {
            wlan_exp_purge_all_data_tx_queue_callback();
        }
        break;


//-----------------------------------------------------------------------------
// Memory Access Commands - For developer use only
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        case CMDID_DEV_MEM_HIGH: {
            // Read/write memory in CPU High
            //
            // Write Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_WRITE_VAL
            //     cmd_args_32[1]      Address
            //     cmd_args_32[2]      Length (number of u32 words to write)
            //     cmd_args_32[3:]     Values to write (integral number of u32 words)
            // Response format:
            //     resp_args_32[0]     Status
            //
            // Read Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_READ_VAL
            //     cmd_args_32[1]      Address
            //     cmd_args_32[2]      Length (number of u32 words to read)
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Length (number of u32 values)
            //     resp_args_32[2:]    Memory values (length u32 values)
            //
            u32    mem_idx;
            u32    status              = CMD_PARAM_SUCCESS;
            u32    msg_cmd             = Xil_Ntohl(cmd_args_32[0]);
            u32    mem_addr            = Xil_Ntohl(cmd_args_32[1]);
            u32    mem_length          = Xil_Ntohl(cmd_args_32[2]);
            u32    use_default_resp    = WLAN_EXP_TRUE;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Write CPU High Mem\n");
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", mem_addr);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", mem_length);

                    // Don't bother if length is clearly bogus
                    if(mem_length < max_resp_len) {
                        for (mem_idx = 0; mem_idx < mem_length; mem_idx++) {
                            wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  W[%2d]: 0x%08x\n", mem_idx, Xil_Ntohl(cmd_args_32[3 + mem_idx]));
                            Xil_Out32((mem_addr + mem_idx*sizeof(u32)), Xil_Ntohl(cmd_args_32[3 + mem_idx]));
                        }
                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_MEM_HIGH write longer than 1400 bytes\n");
                        status = CMD_PARAM_ERROR;
                    }
                break;

                case CMD_PARAM_READ_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Read CPU High Mem:\n");
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", mem_addr);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", mem_length);

                    // Add payload to response
                    if(mem_length < max_resp_len) {

                        // Don't set the default response
                        use_default_resp = WLAN_EXP_FALSE;

                        // Add length argument to response
                        resp_args_32[resp_index++] = Xil_Htonl(status);
                        resp_args_32[resp_index++] = Xil_Htonl(mem_length);
                        resp_hdr->length += (resp_index * sizeof(resp_args_32));
                        resp_hdr->num_args = resp_index;

                        for (mem_idx = 0; mem_idx < mem_length; mem_idx++) {
                            resp_args_32[resp_index + mem_idx] = Xil_Ntohl(Xil_In32((void*)(mem_addr) + mem_idx*sizeof(u32)));
                        }

                        // Update response header with payload length
                        resp_hdr->length   += (mem_length * sizeof(u32));
                        resp_hdr->num_args += mem_length;
                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_MEM_HIGH read longer than 1400 bytes\n");
                        status = CMD_PARAM_ERROR;
                    }
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            if (use_default_resp) {
                // Send default response
                resp_args_32[resp_index++] = Xil_Htonl(status);
                resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args = resp_index;
            }
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_DEV_MEM_LOW: {
            // Read/write memory in CPU Low via IPC message
            //
            // Write Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_WRITE_VAL
            //     cmd_args_32[1]      Address
            //     cmd_args_32[2]      Length (number of u32 words to write)
            //     cmd_args_32[3:]     Values to write (integral number of u32 words)
            // Response format:
            //     resp_args_32[0]     Status
            //
            // Read Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_READ_VAL
            //     cmd_args_32[1]      Address
            //     cmd_args_32[2]      Length (number of u32 words to read)
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Length (number of u32 values)
            //     resp_args_32[2:]    Memory values (length u32 values)
            //
            u32    mem_idx;
            int    mem_status;
            u32    status              = CMD_PARAM_SUCCESS;
            u32    msg_cmd             = Xil_Ntohl(cmd_args_32[0]);
            u32    mem_addr            = Xil_Ntohl(cmd_args_32[1]);
            u32    mem_length          = Xil_Ntohl(cmd_args_32[2]);
            u32    use_default_resp    = WLAN_EXP_TRUE;

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Write CPU Low Mem:\n");
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", mem_addr);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", mem_length);

                    // Don't bother if length is clearly bogus
                    if(mem_length < max_resp_len) {
                        // Endian swap payload here - CPU Low requires payload that is ready to use as-is
                        for (mem_idx = 0; mem_idx < mem_length+2; mem_idx++) {
                            cmd_args_32[1 + mem_idx] = Xil_Ntohl(cmd_args_32[1 + mem_idx]);
                        }

                        mem_status = wlan_mac_high_write_low_mem(mem_length + 2, &(cmd_args_32[1]));

                        if (mem_status == -1) {
                            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_MEM_LOW write failed\n");
                            status = CMD_PARAM_ERROR;
                        }
                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_MEM_LOW write longer than 1400 bytes\n");
                        status = CMD_PARAM_ERROR;
                    }
                break;

                case CMD_PARAM_READ_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Read CPU Low Mem:\n");
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", mem_addr);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", mem_length);

                    if(mem_length < max_resp_len) {
                        mem_status = wlan_mac_high_read_low_mem(mem_length, mem_addr, &(resp_args_32[2]));

                        if(mem_status == 0) { //Success
                            // Don't set the default response
                            use_default_resp = WLAN_EXP_FALSE;

                            // Add length argument to response
                            resp_args_32[resp_index++] = Xil_Htonl(status);
                            resp_args_32[resp_index++] = Xil_Htonl(mem_length);
                            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                            resp_hdr->num_args = resp_index;

                            // Endian swap payload returned by CPU Low
                            for (mem_idx = 0; mem_idx < mem_length; mem_idx++) {
                                resp_args_32[2 + mem_idx] = Xil_Htonl(resp_args_32[2 + mem_idx]);
                            }

                            resp_hdr->length   += (mem_length * sizeof(u32));
                            resp_hdr->num_args += mem_length;

                        } else { //failed
                            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_MEM_LOW read failed\n");
                            status = CMD_PARAM_ERROR;
                        }
                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_MEM_LOW read longer than 1400 bytes\n");
                        status = CMD_PARAM_ERROR;
                    }
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            if (use_default_resp) {
                // Send default response
                resp_args_32[resp_index++] = Xil_Htonl(status);
                resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args = resp_index;
            }
        }
        break;


        //---------------------------------------------------------------------
        case CMDID_DEV_EEPROM: {
            // Read / Write values from / to EEPROM
            //
            // Write Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_WRITE_VAL
            //     cmd_args_32[1]      EEPROM (0 = ON_BOARD / 1 = FMC)
            //     cmd_args_32[2]      Address
            //     cmd_args_32[3]      Length (Number of u8 bytes to write)
            //     cmd_args_32[4:]     Values to write (Length u32 values each containing a single byte to write)
            // Response format:
            //     resp_args_32[0]     Status
            //
            // Read Message format:
            //     cmd_args_32[0]      Command == CMD_PARAM_READ_VAL
            //     cmd_args_32[1]      EEPROM Device (1 = ON_BOARD / 0 = FMC)
            //     cmd_args_32[2]      Address
            //     cmd_args_32[3]      Length (number of u8 bytes to read)
            // Response format:
            //     resp_args_32[0]     Status
            //     resp_args_32[1]     Length (Number of u8 bytes read)
            //     resp_args_32[2:]    EEPROM values (Length u32 values each containing a single byte read)
            //
            #define EEPROM_BASEADDR                        XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR
            #define FMC_EEPROM_BASEADDR                    XPAR_W3_IIC_EEPROM_FMC_BASEADDR

            u32    eeprom_idx;
            int    eeprom_status;
            u8     byte_to_write;
            u32    status              = CMD_PARAM_SUCCESS;
            u32    msg_cmd             = Xil_Ntohl(cmd_args_32[0]);
            u32    eeprom_device       = Xil_Ntohl(cmd_args_32[1]);
            u32    eeprom_addr         = (Xil_Ntohl(cmd_args_32[2]) & 0xFFFF);
            u32    eeprom_length       = Xil_Ntohl(cmd_args_32[3]);
            u32    use_default_resp    = WLAN_EXP_TRUE;
            u32    eeprom_ba           = EEPROM_BASEADDR;

            // Select EEPROM device
            if (eeprom_device) {
                eeprom_ba = EEPROM_BASEADDR;
            } else {
                #if FMC_EEPROM_BASEADDR
                    eeprom_ba = FMC_EEPROM_BASEADDR;
                #else
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "FMC EEPROM not supported\n");
                    msg_cmd = CMD_PARAM_RSVD;
                #endif
            }

            switch (msg_cmd) {
                case CMD_PARAM_WRITE_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Write EEPROM:\n");
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", eeprom_addr);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", eeprom_length);

                    // Don't bother if length is clearly bogus
                    if(eeprom_length < max_resp_len) {
                        for (eeprom_idx = 0; eeprom_idx < eeprom_length; eeprom_idx++) {
                            // Endian swap payload and extract the byte to write
                            byte_to_write = (Xil_Ntohl(cmd_args_32[eeprom_idx + 4]) & 0xFF);

                            // Write the byte and break if there was an EEPROM failure
                            eeprom_status = iic_eeprom_write_byte(eeprom_ba, (eeprom_addr + eeprom_idx), byte_to_write, XPAR_CPU_ID);

                            if (eeprom_status == IIC_EEPROM_FAILURE) {
                                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM write failed at byte %d\n", eeprom_idx);
                                status = CMD_PARAM_ERROR;
                                break;
                            }
                        }
                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM write longer than %d bytes\n", max_resp_len);
                        status = CMD_PARAM_ERROR;
                    }
                break;

                case CMD_PARAM_READ_VAL:
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Read EEPROM:\n");
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Addr: 0x%08x\n", eeprom_addr);
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "  Len:  %d\n", eeprom_length);

                    if (eeprom_length < max_resp_len) {
                        // Don't set the default response
                        use_default_resp = WLAN_EXP_FALSE;

                        for (eeprom_idx = 0; eeprom_idx < eeprom_length; eeprom_idx++) {
                            // Read the byte and break if there was an EEPROM failure
                            eeprom_status = iic_eeprom_read_byte(EEPROM_BASEADDR, (eeprom_addr + eeprom_idx), XPAR_CPU_ID);

                            if (eeprom_status == IIC_EEPROM_FAILURE) {
                                wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM write failed at byte %d\n", eeprom_idx);
                                status = CMD_PARAM_ERROR;
                                break;
                            }

                            // Add the byte read and Endian swap the payload
                            //     - This modified the output Ethernet packet but does not update the resp_index variable
                            resp_args_32[resp_index + eeprom_idx + 2] = Xil_Htonl(eeprom_status & 0xFF);
                        }

                        // Add length argument to response
                        resp_args_32[resp_index++] = Xil_Htonl(status);
                        resp_args_32[resp_index++] = Xil_Htonl(eeprom_idx);
                        resp_index        += eeprom_idx;                       // Update response index for all EEPROM bytes
                        resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                        resp_hdr->num_args = resp_index;

                    } else {
                        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "CMDID_DEV_EEPROM read longer than %d bytes\n", max_resp_len);
                        status = CMD_PARAM_ERROR;
                    }
                break;

                case CMD_PARAM_RSVD:
                    status = CMD_PARAM_ERROR;
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_node, "Unknown command for 0x%6x: %d\n", cmd_id, msg_cmd);
                    status = CMD_PARAM_ERROR;
                break;
            }

            if (use_default_resp) {
                // Send default response
                resp_args_32[resp_index++] = Xil_Htonl(status);
                resp_hdr->length  += (resp_index * sizeof(resp_args_32));
                resp_hdr->num_args = resp_index;
            }
        }
        break;



//-----------------------------------------------------------------------------
// Child Commands
//-----------------------------------------------------------------------------


        //---------------------------------------------------------------------
        default: {
            // Call standard function in child class to parse parameters implemented there
            resp_sent = wlan_exp_process_node_cmd_callback(cmd_id, socket_index, from, command, response, max_resp_len);
        }
        break;
    }

    return resp_sent;
}




/*****************************************************************************/
/**
 * Process buffer commands and return a valid buffer with the requested information.
 *
 * Terminology:
 *    "source" - the data to be transferred
 *    "dest"   - destination within Ethernet packet for the data
 *    "entry"  - Element of a dl_list
 *
 * @param    None
 *
 * @return   NO_RESP_SENT
 *           RESP_SENT
 *
 *****************************************************************************/
u32 process_buffer_cmds(int socket_index, void * from, cmd_resp * command, cmd_resp * response,
                        cmd_resp_hdr * cmd_hdr, u32 * cmd_args_32,
                        cmd_resp_hdr * resp_hdr, u32 * resp_args_32,
                        u32 eth_dev_num, u32 max_resp_len,
                        char * type, char * description, dl_list * source_list, u32 dest_size,
                        u32 (*find_id)(u8 *),
                        dl_entry * (*find_source)(u8 *),
                        void (*copy_source_to_dest)(void *, void *, u8*),
                        void (*zero_dest)(void *)) {

    u32            resp_index           = 5;                // There will always be 5 return args for a buffer
    u32            resp_sent            = NO_RESP_SENT;

    u32            i, j;

    u32            id;
    u8             mac_addr[MAC_ADDR_LEN];

    u32            size;
    u32            transfer_size;
    u32            curr_index;
    u32            next_index;
    u32            num_pkts;
    u32            bytes_per_pkt;
    u32            bytes_remaining;

    u32            total_entries;
    u32            entry_remaining;
    u32            transfer_entry_num;
    u32            entry_per_pkt;

    dl_entry     * curr_entry;
    void         * curr_dest;

    // Get MAC Address
    wlan_exp_get_mac_addr(&((u32 *)cmd_args_32)[4], &mac_addr[0]);
    id = find_id(&mac_addr[0]);

    // Initialize return values
    resp_args_32[0] = cmd_args_32[0];
    resp_args_32[1] = cmd_args_32[1];
    resp_args_32[2] = 0;
    resp_args_32[3] = 0;
    resp_args_32[4] = 0;

    if (id == WLAN_EXP_AID_NONE) {
        if ((Xil_Ntohl(cmd_args_32[1]) & CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE) == CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE) {

            // Copy routine will zero out destination if the source is NULL
            copy_source_to_dest(NULL, &resp_args_32[resp_index], &mac_addr[0]);

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, type, "Returning zeroed %s entry for node: ", description);
            wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

            // Set the return args and increment the size
            resp_args_32[2]   = Xil_Htonl(dest_size);
            resp_args_32[3]   = 0;
            resp_args_32[4]   = Xil_Htonl(dest_size);
            resp_hdr->length += dest_size;
        } else {
            // Cannot find the MAC address, return an empty buffer
            wlan_exp_printf(WLAN_EXP_PRINT_INFO, type, "Could not find %s for specified node: ", description);
            wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");
        }
    } else {
        // If parameter is not the magic number to return all structures
        if (id != WLAN_EXP_AID_ALL) {
            // Find the source information dl_entry
            curr_entry = find_source(&mac_addr[0]);

            if (curr_entry != NULL) {
                // Copy the info to the destination
                copy_source_to_dest(curr_entry->data, &resp_args_32[resp_index], &mac_addr[0]);

                wlan_exp_printf(WLAN_EXP_PRINT_INFO, type, "Get %s entry for node: ", description);
                wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");

                // Set the return args and increment the size
                resp_args_32[2]   = Xil_Htonl(dest_size);
                resp_args_32[3]   = 0;
                resp_args_32[4]   = Xil_Htonl(dest_size);
                resp_hdr->length += dest_size;

            } else {
                // If we cannot find the MAC address, print a warning and return an empty buffer
                wlan_exp_printf(WLAN_EXP_PRINT_INFO, type, "Could not find %s for specified node: ", description);
                wlan_exp_print_mac_address(WLAN_EXP_PRINT_INFO, &mac_addr[0]); wlan_exp_printf(WLAN_EXP_PRINT_INFO, NULL, "\n");
            }

        } else {
            // Create a WLAN Exp buffer response to send all entries
            if(source_list != NULL){
                total_entries     = source_list->length;
            } else {
                total_entries     = 0;
            }

            size = dest_size * total_entries;

            wlan_exp_printf(WLAN_EXP_PRINT_INFO, type, "Getting %d entries (%d bytes)\n", total_entries, size);

            if (size != 0) {
                // Send the dl_list entries as a series of WLAN Exp Buffers

                // Set loop variables
                entry_per_pkt     = (max_resp_len * 4) / dest_size;
                bytes_per_pkt     = entry_per_pkt * dest_size;
                num_pkts          = size / bytes_per_pkt + 1;

                if ((size % bytes_per_pkt) == 0){ num_pkts--; }      // Subtract the extra pkt if the division had no remainder

                entry_remaining   = total_entries;
                bytes_remaining   = size;
                curr_index        = 0;
                curr_entry        = source_list->first;

                // Set response header arguments that do not change per packet
                resp_hdr->num_args = 5;

                // Iterate through all the packets
                for (i = 0; i < num_pkts; i++) {

                    // Get the next index
                    next_index  = curr_index + bytes_per_pkt;

                    // Compute the transfer size (use the full buffer unless you run out of space)
                    if(next_index > size) {
                        transfer_size = size - curr_index;
                    } else {
                        transfer_size = bytes_per_pkt;
                    }

                    if(entry_remaining < entry_per_pkt) {
                        transfer_entry_num = entry_remaining;
                    } else {
                        transfer_entry_num = entry_per_pkt;
                    }

                    // Set response args that change per packet
                    resp_args_32[2]    = Xil_Htonl(bytes_remaining);
                    resp_args_32[3]    = Xil_Htonl(curr_index);
                    resp_args_32[4]    = Xil_Htonl(transfer_size);

                    // Set the response header fields that change per packet
                    resp_hdr->length   = 20 + transfer_size;

                    // Transfer data
                    curr_dest = (void *) &resp_args_32[resp_index];

                    for (j = 0; j < transfer_entry_num; j++) {
                        // Since this method is interruptable, we need to protect ourselves from list elements being
                        // removed (we will not handle the case that list elements are added and just ignore the new
                        // elements).
                        if (curr_entry != NULL) {
                            // Copy the info to the Ethernet packet
                            //   NOTE:  This assumes that the info entry in wlan_mac_entries.h has a contiguous piece of memory
                            //          similar to the info structures in wlan_mac_high.h
                            copy_source_to_dest(curr_entry->data, curr_dest, &mac_addr[0]);

                            // Increment the entry pointers
                            curr_entry = dl_entry_next(curr_entry);

                        } else {
                            // Instead of transferring the information, zero out the destination
                            //   NOTE:  The destination will still potentially have a timestamp
                            zero_dest(curr_dest);

                            // Do not do anything to the station info pointers since we are already at the end of the list
                        }

                        // Increment the ethernet packet pointer
                        curr_dest = (void *)(((void *)curr_dest) + dest_size);
                    }

                    // Send the packet
                    send_early_resp(socket_index, from, response->header, response->buffer);

                    // Update our current address and bytes remaining
                    curr_index       = next_index;
                    bytes_remaining -= transfer_size;
                    entry_remaining -= entry_per_pkt;
                }

                resp_sent = RESP_SENT;
            }
        }
    }

    // Set the length and number of response args
    resp_hdr->length  += (5 * sizeof(resp_args_32));
    resp_hdr->num_args = resp_index;

    return resp_sent;
}


#if WLAN_SW_CONFIG_ENABLE_LOGGING
/*****************************************************************************/
/**
 * Transfer Log Data
 *
 * Transfers the requested log data to the host
 *
 * @param   socket_index     -- Index of socket to send data
 * @param   from             -- Socket address structure of host from which command was received
 * @param   resp_buffer_data -- Address of the response data buffer (ie address of response transport header)
 * @param   eth_dev_num      -- Ethernet device number to send response
 * @param   max_resp_len     -- Maximum number of u32 words allowed in response
 * @param   id               -- Buffer ID for transfer
 * @param   flags            -- Buffer flags for transfer
 * @param   start_index      -- Start index for transfer
 * @param   size             -- Size of transfer
 *
 * @return  None
 *
 * @note    The WARP IP/UDP Ethernet send function only blocks when it runs out of transmit
 *     buffer descriptors.  If all header modifications are performed in place, this will
 *     cause problems when trying to get all log entries when WARP_IP_UDP_TXBD_CNT (ie the
 *     number of TX BDs) is greater than 5 because the Ethernet DMA will not have transfered
 *     the header before the next round of processing that modifies the header.  Therefore,
 *     the function will create multiple copies of the packet header in the buffer allocated
 *     above.  The contents of the packet header are:
 *
 *     Packet Header (84 bytes total):
 *            Eth header       = 14 bytes
 *            IP header        = 20 bytes
 *            UDP header       =  8 bytes
 *            Delimiter        =  2 bytes
 *            Transport header = 12 bytes
 *            Command header   =  8 bytes
 *            Buffer header    = 20 bytes
 *
 *     In order to minimize the impact to performance, the header will be pulled in from
 *     DDR into LMB memory for processing and then copy the completed header back to
 *     DDR so the Ethernet DMA can fetch it.  This way the header can be maintained for
 *     the current packet and then copy it to multiple locations to avoid overwriting the
 *     header before the DMA can transfer its contents.
 *
 *
 * @note    While this method is specifically for transferring log data, it can easily be
 *     abstracted to transfer generic data by including an input argument that is a
 *     function pointer to get the data.  Instead of:
 *
 *         num_bytes = event_log_get_data(curr_index, transfer_size, &data_buffer, 0);
 *
 *     use a function pointer with the same contract.
 *
 *****************************************************************************/
void transfer_log_data(u32 socket_index, void * from,
                       void * resp_buffer_data, u32 eth_dev_num, u32 max_resp_len,
                       u32 id, u32 flags, u32 start_index, u32 size) {

    u32                      i;
    int                      status;

    u32                      end_index;
    u32                      curr_index;
    u32                      next_index;
    u32                      bytes_remaining;

    u32                      bytes_per_pkt;
    u32                      num_bytes;
    u32                      num_pkts;

    warp_ip_udp_buffer       header_buffer;
    warp_ip_udp_buffer       data_buffer;
    warp_ip_udp_buffer     * resp_array[2];
    u8                       tmp_header[100];              // NOTE:  Size must be larger than entire header.

    warp_ip_udp_header     * tx_eth_ip_udp_header;
    transport_header       * tx_transport_header;
    cmd_resp_hdr           * tx_resp_header;
    u32                    * tx_resp_args;

    u32                      header_length;
    u16                      ip_length;
    u16                      udp_length;
    u32                      transfer_length;
    u16                      data_length;
    u32                      total_hdr_length;

    u8                       dest_hw_addr[MAC_ADDR_LEN];
    u32                      dest_ip_addr;
    u16                      dest_port;

    u8                     * header_base_addr;
    u32                      header_offset;
    u8                     * header_addr;
    u32                      header_buffer_size;

    // interrupt_state_t        prev_interrupt_state;

    // Set up control variables
    bytes_per_pkt         = ((max_resp_len) * 4) - WLAN_EXP_BUFFER_HEADER_SIZE;     // Subtract the bytes for the buffer header
    num_pkts              = (size / bytes_per_pkt) + 1;

    if ((size % bytes_per_pkt) == 0){ num_pkts--; }                  // Subtract the extra pkt if the division had no remainder

    end_index             = start_index + size;
    curr_index            = start_index;
    bytes_remaining       = size;

    // Initialize the response buffer array
    resp_array[0]         = (warp_ip_udp_buffer *)&header_buffer;    // Contains all header information
    resp_array[1]         = (warp_ip_udp_buffer *)&data_buffer;      // Contains log entry data

    // Set up temporary pointers to the header data
    //     NOTE:  The memory space for the temporary header must be large enough for the entire header.
    //
    tx_eth_ip_udp_header  = (warp_ip_udp_header *)(&tmp_header[0]);
    tx_transport_header   = (transport_header   *)(&tmp_header[sizeof(warp_ip_udp_header)]);
    tx_resp_header        = (cmd_resp_hdr       *)(&tmp_header[sizeof(warp_ip_udp_header) + sizeof(transport_header)]);
    tx_resp_args          = (u32                *)(&tmp_header[sizeof(warp_ip_udp_header) + sizeof(transport_header) + sizeof(cmd_resp_hdr)]);

    // Set up temporary variables with the length values of the header
    ip_length             = WARP_IP_UDP_DELIM_LEN + UDP_HEADER_LEN + IP_HEADER_LEN_BYTES;
    udp_length            = WARP_IP_UDP_DELIM_LEN + UDP_HEADER_LEN;
    header_length         = sizeof(transport_header) + sizeof(cmd_resp_hdr) + WLAN_EXP_BUFFER_HEADER_SIZE;
    total_hdr_length      = sizeof(warp_ip_udp_header) + header_length;

    // Get values out of the socket address structure
    dest_ip_addr          = ((struct sockaddr_in*)from)->sin_addr.s_addr;    // NOTE:  Value big endian
    dest_port             = ((struct sockaddr_in*)from)->sin_port;

    // Get hardware address of the destination
    arp_get_hw_addr(eth_dev_num, dest_hw_addr, (u8 *)(&dest_ip_addr));

    // Pull in header information into local LMB memory:
    //   - Copy the header information from the socket
    //   - Copy the information from the response
    //
    memcpy((void *)tx_eth_ip_udp_header, (void *)socket_get_warp_ip_udp_header(socket_index), sizeof(warp_ip_udp_header));
    memcpy((void *)tx_transport_header, resp_buffer_data, header_length);

    // Initialize header buffer size/length (see above for description)
    header_buffer.length = total_hdr_length;
    header_buffer.size   = total_hdr_length;

    //
    // NOTE:  In order to make large transfers more efficient, most of the response packet can be
    //   pre-processed such that the WARP IP/UDP library has to do only the minimal amount of
    //   processing per packet.  This should not cause any additional overhead for a single packet
    //   but will have have reduced overhead for all other packets.
    //

    // Initialize constant header parameters
    tx_resp_args[0]                 = Xil_Htonl(id);
    tx_resp_args[1]                 = Xil_Htonl(flags);

    // Populate response header fields with static data
    tx_resp_header->cmd             = Xil_Ntohl(tx_resp_header->cmd);
    tx_resp_header->num_args        = Xil_Ntohs(WLAN_EXP_BUFFER_NUM_ARGS);

    // Populate transport header fields with static data
    tx_transport_header->dest_id    = Xil_Htons(tx_transport_header->dest_id);
    tx_transport_header->src_id     = Xil_Htons(tx_transport_header->src_id);
    tx_transport_header->seq_num    = Xil_Htons(tx_transport_header->seq_num);
    tx_transport_header->flags      = Xil_Htons(tx_transport_header->flags);

    // Update the Ethernet header
    //     NOTE:  dest_hw_addr must be big-endian; ethertype must be little-endian
    //     NOTE:  Adapted from the function:
    //                eth_update_header(&(eth_ip_udp_header->eth_hdr), dest_hw_addr, ETHERTYPE_IP_V4);
    //
    memcpy((void *)tx_eth_ip_udp_header->eth_hdr.dest_mac_addr, (void *)dest_hw_addr, MAC_ADDR_LEN);
    tx_eth_ip_udp_header->eth_hdr.ethertype  = Xil_Htons(ETHERTYPE_IP_V4);

    // Update the UDP header
    //     NOTE:  Requires dest_port to be big-endian; udp_length to be little-endian
    //     NOTE:  Adapted from the function:
    //                udp_update_header(&(eth_ip_udp_header->udp_hdr), dest_port, (udp_length + data_length));
    //
    tx_eth_ip_udp_header->udp_hdr.dest_port  = dest_port;
    tx_eth_ip_udp_header->udp_hdr.checksum   = UDP_NO_CHECKSUM;

    // Set address for the Ethernet header in DMA accessible memory
    header_base_addr       = ETH_header_buffer;             // Use the buffer allocated above
    header_offset          = 0;
    header_buffer_size     = WLAN_EXP_ETH_BUFFER_SIZE * WLAN_EXP_ETH_NUM_BUFFER;


#ifdef _DEBUG_
    xil_printf("EVENT LOG: Get Log Data \n");
    xil_printf("    start_index      = 0x%8x\n", start_index);
    xil_printf("    size             = %10d\n",  size);
    xil_printf("    num_pkts         = %10d\n",  num_pkts);
#endif


    // Iterate through all the packets
    for (i = 0; i < num_pkts; i++) {

        // Get the next address
        header_addr = (u8 *)(((u32)header_base_addr) + header_offset);
        next_index  = curr_index + bytes_per_pkt;

        // Compute the transfer size (use the full buffer unless you run out of space)
        if (next_index > end_index) {
            transfer_length = end_index - curr_index;
        } else {
            transfer_length = bytes_per_pkt;
        }

        data_length     = transfer_length + header_length;

        // Set response args that change per packet
        tx_resp_args[2] = Xil_Htonl(bytes_remaining);
        tx_resp_args[3] = Xil_Htonl(curr_index);
        tx_resp_args[4] = Xil_Htonl(transfer_length);

        // Set the response header fields that change per packet
        tx_resp_header->length = Xil_Ntohs(transfer_length + WLAN_EXP_BUFFER_HEADER_SIZE);

        // Populate transport header fields with per packet data
        tx_transport_header->length = Xil_Htons(data_length + WARP_IP_UDP_DELIM_LEN);

        // Update the UDP header
        //     NOTE:  Requires dest_port to be big-endian; udp_length to be little-endian
        //     NOTE:  Adapted from the function:
        //                udp_update_header(&(eth_ip_udp_header->udp_hdr), dest_port, (udp_length + data_length));
        //
        tx_eth_ip_udp_header->udp_hdr.length = Xil_Htons(udp_length + data_length);

        // Update the IPv4 header
        //     NOTE:  Requires dest_ip_addr to be big-endian; ip_length to be little-endian
        //     NOTE:  We did not break this function apart like the other header updates b/c the IP ID counter is
        //            maintained in the library and we did not want to violate that.
        //
        ipv4_update_header(&(tx_eth_ip_udp_header->ip_hdr), dest_ip_addr, (ip_length + data_length), IP_PROTOCOL_UDP);

        // Copy the completed header to DMA accessible BRAM
        memcpy((void *)header_addr, (void *)tmp_header, total_hdr_length);

        // Set the header buffer data / offset
        header_buffer.data   = (u8 *)header_addr;
        header_buffer.offset = (u8 *)header_addr;

        // Transfer data
        //     NOTE:  This selects the "do not copy data" option and instead provides
        //         a WARP IP/UDP buffer to transfer the data.
        //
        num_bytes = event_log_get_data(curr_index, transfer_length, &data_buffer, 0);

        // Check that we copied everything
        if (num_bytes == transfer_length) {

            // Check the interrupt status; Disable interrupts if enabled
            //     NOTE:  This is done inside the Eth send function
            // prev_interrupt_state = wlan_mac_high_interrupt_stop();

            // Send the Ethernet packet
            //   NOTE:  In an effort to reduce overhead (ie improve performance), the "raw"
            //       socket_sendto method is used which transmits the provided buffers "as is"
            //       (ie there are no header updates or other modifications to the buffer data).
            //       Also, we have consolidated all the headers into a single buffer so that a
            //       Ethernet packet only requires two Transmit Buffer Descriptors (TX BDs).
            //
            status = socket_sendto_raw(socket_index, resp_array, 0x2);

            // Restore interrupts
            //     NOTE:  This is done inside the Eth send function
            // wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

            // Check that the packet was sent correctly
            if (status == WARP_IP_UDP_FAILURE) {
                wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_event_log,
                            "Issue sending log entry packet to host.\n");
            }
        } else {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_event_log,
                            "Tried to get %d bytes, but only received %d @ 0x%x \n", transfer_length, num_bytes, curr_index );
        }

        // Update our current address and bytes remaining
        curr_index       = next_index;
        bytes_remaining -= transfer_length;
        header_offset    = (header_offset + WLAN_EXP_ETH_BUFFER_SIZE) % header_buffer_size;
    }
}
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING



/*****************************************************************************/
/**
 * Helper functions for node_process_buffer_cmds
 *
 * For each type of structure (ie <> in the notation below) to be transferred
 * using a buffer, the following commands are needed:
 *
 *     dl_entry * find_<>(u8 * mac_addr)
 *     void       zero_<>(void * dest);
 *     void       copy_<>_to_dest(void * source, void * dest);
 *
 * @param   See description
 *
 *****************************************************************************/
dl_entry * find_station_info(u8 * mac_addr) {
    dl_list * source_list = get_bss_member_list();

    if (source_list != NULL) {
        return station_info_find_by_addr(mac_addr, source_list);
    } else {
        return NULL;
    }
}



void zero_station_info(void * dest) {
    bzero(dest, sizeof(wlan_exp_station_info_t));
}



void copy_station_info_to_dest(void * source, void * dest, u8* mac_addr) {

    station_info_t          * curr_source = (station_info_t *)(source);
    wlan_exp_station_info_t * curr_dest   = (wlan_exp_station_info_t *)(dest);

    // Fill in zeroed entry if source is NULL
    if (source == NULL) {
        curr_source = wlan_mac_high_malloc(sizeof(station_info_t));

        if (curr_source != NULL) {
            bzero(curr_source, sizeof(station_info_t));

            // Add in MAC address
            memcpy(curr_source->addr, mac_addr, MAC_ADDR_LEN);
        }
    }

    // Copy the source information to the destination
    if (curr_source != NULL) {
        memcpy((void *)(curr_dest), (void *)(curr_source), sizeof(wlan_exp_station_info_t));
    } else {
        wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_node, "Could not copy station_info to entry\n");
    }

    // Free curr_source if source was NULL
    if (source == NULL) {
        wlan_mac_high_free(curr_source);
    }
}

dl_entry * find_counts_txrx(u8 * mac_addr) {

	return station_info_find_by_addr(mac_addr, NULL);

}

void zero_counts_txrx(void * dest) {

	wlan_exp_station_txrx_counts_t* counts = (wlan_exp_station_txrx_counts_t *)(dest);

    // Do not zero out timestamp
    bzero((void *)(&counts->counts), sizeof(wlan_exp_station_txrx_counts_lite_t));
}



void copy_counts_txrx_to_dest(void* source, void* dest, u8* mac_addr) {

	station_info_t* 					curr_source    = (station_info_t*)(source);
    wlan_exp_station_txrx_counts_t* 	curr_dest      = (wlan_exp_station_txrx_counts_t*)(dest);

    // Set the timestamp using system time
    curr_dest->timestamp = get_system_time_usec();

    // Fill in zeroed entry if source is NULL
    //   - All fields are zero except last_txrx_timestamp which is CMD_PARAM_NODE_TIME_RSVD_VAL_64
    if (source == NULL) {
        curr_source = wlan_mac_high_malloc(sizeof(station_info_t));

        if (curr_source != NULL) {
            bzero(curr_source, sizeof(station_info_t));

            // Add in MAC address
            memcpy(curr_source->addr, mac_addr, MAC_ADDR_LEN);
        }
    }

    // Copy the source information to the destination
    if (curr_source != NULL) {

    	//Copy the address out of the station_info_t
    	memcpy(curr_dest->addr, curr_source->addr, 6);

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
    	//Copy the counts out of the station_info_t
        memcpy((void *)(&curr_dest->counts), (void *)(&curr_source->txrx_counts), sizeof(wlan_exp_station_txrx_counts_lite_t));
#else
        //There are no counts anywhere in the station_info_t struct, so we will return all zeroes.
        bzero((void *)(&curr_dest->counts), sizeof(station_txrx_counts_t));
#endif
    } else {
        wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_counts, "Could not copy counts_txrx to entry\n");
    }

    // Free curr_source if source was NULL
    if (source == NULL) {
        wlan_mac_high_free(curr_source);
    }
}



//
// Do not need separate find_bss_info function.  Currently exists in wlan_mac_bss_info.c
//



void zero_bss_info(void * dest) {
    bzero(dest, sizeof(wlan_exp_bss_info_t));
}



void copy_bss_info_to_dest(void * source, void * dest, u8* mac_addr) {

    bss_info_t          * curr_source  = (bss_info_t *)(source);
    wlan_exp_bss_info_t * curr_dest    = (wlan_exp_bss_info_t *)(dest);

    // Fill in zeroed entry if source is NULL
    if (source == NULL) {
        curr_source = wlan_mac_high_malloc(sizeof(bss_info_t));

        if (curr_source != NULL) {
            bzero(curr_source, sizeof(bss_info_t));

            // Add in MAC address
            memcpy(curr_source->bssid, mac_addr, MAC_ADDR_LEN);
        }
    }

    // Copy the source information to the destination log entry
    if (curr_source != NULL) {
        memcpy((void *)(curr_dest), (void *)(curr_source), sizeof(wlan_exp_bss_info_t));
    } else {
        wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "Could not copy bss_info to entry\n");
    }

    // Free curr_source if source was NULL
    if (source == NULL) {
        wlan_mac_high_free(curr_source);
    }
}



/*****************************************************************************/
/**
 * Set the WLAN Exp callbacks
 *
 * @param   callback         - Pointer to the callback function
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_exp_reset_all_callbacks(){
    wlan_exp_process_node_cmd_callback         = (wlan_exp_function_ptr_t) null_process_cmd_callback;
    wlan_exp_purge_all_data_tx_queue_callback  = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
    wlan_exp_tx_cmd_add_association_callback   = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
    wlan_exp_process_user_cmd_callback         = (wlan_exp_function_ptr_t) null_process_cmd_callback;
    wlan_exp_beacon_ts_update_mode_callback    = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
    wlan_exp_process_config_bss_callback       = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
    wlan_exp_active_bss_info_getter_callback   = (wlan_exp_function_ptr_t) wlan_exp_null_callback;
}

void wlan_exp_set_process_node_cmd_callback(void(*callback)()){
	wlan_exp_process_node_cmd_callback = (wlan_exp_function_ptr_t) callback;
}


void wlan_exp_set_purge_all_data_tx_queue_callback(void(*callback)()){
    wlan_exp_purge_all_data_tx_queue_callback = (wlan_exp_function_ptr_t) callback;
}


void wlan_exp_set_tx_cmd_add_association_callback(void(*callback)()){
    wlan_exp_tx_cmd_add_association_callback = (wlan_exp_function_ptr_t) callback;
}


void wlan_exp_set_process_user_cmd_callback(void(*callback)()){
    wlan_exp_process_user_cmd_callback = (wlan_exp_function_ptr_t) callback;
}


void wlan_exp_set_beacon_ts_update_mode_callback(void(*callback)()){
    wlan_exp_beacon_ts_update_mode_callback = (wlan_exp_function_ptr_t) callback;
}


void wlan_exp_set_process_config_bss_callback(void(*callback)()){
    wlan_exp_process_config_bss_callback = (wlan_exp_function_ptr_t) callback;
}

void wlan_exp_set_active_bss_info_getter_callback(void(*callback)()){
	wlan_exp_active_bss_info_getter_callback = (wlan_exp_function_ptr_t) callback;
}


/*****************************************************************************/
/**
 * Initialize the node TAG parameters structure
 *
 * @param   values           - Pointer to structure from which to get all the tag parameter values
 *
 * @return  int              - Total number of bytes of the TAG parameter structure
 *
 * @note    Please make sure that the parameters structure and the parameter values
 *          maintain the same order
 *
 *****************************************************************************/
int  node_init_parameters(u32 * values) {

    u16    lengths[NODE_PARAM_MAX_PARAMETER] = NODE_PARAM_FIELD_LENGTHS;

    return wlan_exp_init_parameters((wlan_exp_tag_parameter *) &node_parameters,
                                    GROUP_NODE,
                                    NODE_PARAM_MAX_PARAMETER,
                                    values,
                                    lengths);
}



/*****************************************************************************/
/**
 * @ brief Get all tag parameter information from the node tag parameters
 *
 * This function will populate a buffer with the all the information from the
 * node tag parameter structure.
 *
 * @param   buffer           - u32 array to place tag parameter information
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 * @param   transmit         - Flag to adjust the values for network transmission (WLAN_EXP_TRANSMIT) or
 *                                 leave them alone (WLAN_EXP_NO_TRANSMIT)
 *
 * @return  int              - Total number of words placed in the buffer
 *
 * @note    The tag parameters must be initialized before this function is called.
 *
 *****************************************************************************/
int node_get_parameters(u32 * buffer, u32 max_resp_len, u8 transmit) {

    return wlan_exp_get_parameters((wlan_exp_tag_parameter *) &node_parameters,
                                   NODE_PARAM_MAX_PARAMETER,
                                   buffer,
                                   max_resp_len,
                                   WLAN_EXP_FALSE,
                                   transmit);
}



/*****************************************************************************/
/**
 * @ brief Get the tag parameter values from the node tag parameters
 *
 * This function will populate a buffer with just the tag parameter values from the
 * node tag parameter structure.
 *
 * @param   buffer           - u32 array to place tag parameter information
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 *
 * @return  int              - Total number of words placed in the buffer
 *
 * @note    The tag parameters must be initialized before this function is called.
 *
 *****************************************************************************/
int node_get_parameter_values(u32 * buffer, u32 max_resp_len) {

    return wlan_exp_get_parameters((wlan_exp_tag_parameter *) &node_parameters,
                                   NODE_PARAM_MAX_PARAMETER,
                                   buffer,
                                   max_resp_len,
                                   WLAN_EXP_TRUE,
                                   WLAN_EXP_NO_TRANSMIT);
}



/*****************************************************************************/
/**
 * Helper functions to get field values
 *
 * @param   None
 *
 * @return  u32              - Field value
 *
 *****************************************************************************/
u32  node_get_node_id       ( void ) { return node_info.node_id; }
u32  node_get_serial_number ( void ) { return node_info.serial_number; }



/*****************************************************************************/
/**
 * This is a helper function to clean up the LTGs owned by WLAN Exp
 *
 * @param   id               - LTG id
 *          callback_arg     - Callback argument for LTG
 *
 * @return  None
 *
 *****************************************************************************/
void ltg_cleanup(u32 id, void* callback_arg){
    wlan_mac_high_free(callback_arg);
}



/*****************************************************************************/
/**
 * WLAN Mapping of MAC Addr to ID
 *
 * This function contains the mapping of MAC address to ID within a node for
 *   - associated stations
 *   - counts
 *   - bss infos
 *
 * @param   mac_addr         - MAC Address
 *
 * @return  u32              - ID associated with that MAC address
 *
 *****************************************************************************/
u32  wlan_exp_get_id_in_associated_stations(u8 * mac_addr) {
    u32            		id;
    dl_entry     	* 	entry;
    station_info_t  * 	station_info;
    bss_info_t* 		active_bss_info = ((bss_info_t*)wlan_exp_active_bss_info_getter_callback());

    if (wlan_addr_eq(mac_addr, zero_addr)) {
        id = WLAN_EXP_AID_ALL;
    } else {
        if(active_bss_info != NULL){
            if (wlan_addr_eq(mac_addr, active_bss_info->bssid)) {
                id = WLAN_EXP_AID_ME;
            } else {
                entry = station_info_find_by_addr(mac_addr, &(active_bss_info->members));

                if (entry != NULL) {
                	station_info = (station_info_t*)(entry->data);
                    id = station_info->ID;
                } else {
                    id = WLAN_EXP_AID_NONE;
                }
            }
        } else {
            id = WLAN_EXP_AID_NONE;
        }
    }

    return id;
}


u32  wlan_exp_get_id_in_counts(u8 * mac_addr) {
    u32         	id;
    dl_entry* 		entry;
    bss_info_t* 	active_bss_info = ((bss_info_t*)wlan_exp_active_bss_info_getter_callback());

    if (wlan_addr_eq(mac_addr, zero_addr)) {
        id = WLAN_EXP_AID_ALL;
    } else {
		entry = station_info_find_by_addr(mac_addr, &(active_bss_info->members));

		if (entry != NULL) {
			id = WLAN_EXP_AID_DEFAULT;            // Only returns the default AID if found
		} else {
			id = WLAN_EXP_AID_NONE;
		}
    }

    return id;
}


u32  wlan_exp_get_id_in_bss_info(u8 * bssid) {
    u32            id;
    dl_entry*       entry;

    if (wlan_addr_eq(bssid, zero_addr)) {
        id = WLAN_EXP_AID_ALL;
    } else {
        entry = wlan_mac_high_find_bss_info_BSSID(bssid);

        if (entry != NULL) {
            id = WLAN_EXP_AID_DEFAULT;
        } else {
            id = WLAN_EXP_AID_NONE;
        }
    }

    return id;
}



/*****************************************************************************/
/**
 * Process TX Power
 *
 * @param   cmd              - Command:  NODE_WRITE_VAL or NODE_READ_VAL
 * @param   aid              - AID of the station or NODE_CONFIG_ALL_ASSOCIATED
 * @param   tx_power         - Power to set the node (function assumes power is valid)
 *
 * @return  int              - Power
 *                             - CMD_PARAM_ERROR on ERROR
 *
 *****************************************************************************/
int process_tx_power(u32 cmd, u32 aid, int tx_power) {

    int           iter;
    int           power;
    dl_list     	* curr_list;
    dl_entry    	* curr_entry;
    station_info_t	* curr_station_info;

    power = CMD_PARAM_ERROR;

    // For Writes
    if (cmd == CMD_PARAM_WRITE_VAL) {
        curr_list  = get_bss_member_list();

        if (curr_list != NULL) {
            if (curr_list->length == 0) { return tx_power; }

            iter       = curr_list->length;
            curr_entry = curr_list->first;

            while ((curr_entry != NULL) && (iter-- > 0)) {
                curr_station_info = (station_info_t*)(curr_entry->data);

                if (aid == WLAN_EXP_AID_ALL) {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                    "Set TX power on ID %d = %d dBm\n", curr_station_info->ID, tx_power);
                    curr_station_info->tx.phy.power = tx_power;
                    power                           = tx_power;

                } else if (aid == curr_station_info->ID) {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                    "Set TX power on ID %d = %d dBm\n", aid, tx_power);
                    curr_station_info->tx.phy.power = tx_power;
                    power                           = tx_power;
                    break;
                }
                curr_entry = dl_entry_next(curr_entry);
            }
        } else {
            if (aid == WLAN_EXP_AID_ALL) {
                // This is not an error because we are trying to set the rate for all
                // associations and there currently are none.
                power = tx_power;
            }
        }

    // For Reads
    } else {
        if (aid != WLAN_EXP_AID_ALL) {
            curr_list  = get_bss_member_list();

            if (curr_list != NULL) {
                iter       = curr_list->length;
                curr_entry = curr_list->first;

                while ((curr_entry != NULL) && (iter-- > 0)) {
                    curr_station_info = (station_info_t*)(curr_entry->data);
                    if (aid == curr_station_info->ID) {
                        power = curr_station_info->tx.phy.power;
                        break;
                    }
                    curr_entry = dl_entry_next(curr_entry);
                }
            }
        }

        // NOTE:  Trying to read the rate for all associations returns an error.
    }

    return power;
}



/*****************************************************************************/
/**
 * Process TX Rate
 *
 * @param   cmd          - Command:  NODE_WRITE_VAL or NODE_READ_VAL
 * @param   aid          - AID of the station or NODE_CONFIG_ALL_ASSOCIATED
 * @param   mcs          - MCS
 * @param   phy_mode     - PHY mode (PHY_MODE_NONHT or PHY_MODE_HTMF)
 * @param   ret_mcs      - Pointer to return value for MCS
 * @param   ret_phy_mode - Pointer to return value for PHY mode
 *
 * @return  u32          - Status
 *                         - CMD_PARAM_SUCCESS
 *                         - CMD_PARAM_WARNING
 *                         - CMD_PARAM_ERROR
 *
 *****************************************************************************/
u32 process_tx_rate(u32 cmd, u32 aid, u32 mcs, u32 phy_mode, u32 * ret_mcs, u32 * ret_phy_mode) {

    int             iter;
    dl_list       * curr_list;
    dl_entry      * curr_entry;
    station_info_t* curr_station_info;
    u32             status               = CMD_PARAM_ERROR;

    // For Writes
    if (cmd == CMD_PARAM_WRITE_VAL) {
        curr_list  = get_bss_member_list();

        if (curr_list != NULL) {
            if (curr_list->length == 0) { return CMD_PARAM_SUCCESS; }

            iter       = curr_list->length;
            curr_entry = curr_list->first;
            status     = CMD_PARAM_SUCCESS;

            while ((curr_entry != NULL) && (iter-- > 0)) {
                curr_station_info = (station_info_t*)(curr_entry->data);

                if (aid == WLAN_EXP_AID_ALL) {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                            "Set Tx rate on AID %d to MCS %d , PHY Mode %d\n", curr_station_info->ID, mcs, phy_mode);

                    if (station_info_update_rate(curr_station_info, mcs, phy_mode) != 0) {
                        status = CMD_PARAM_WARNING;
                    }

                } else if (aid == curr_station_info->ID) {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                            "Set Tx rate on AID %d to MCS %d , PHY Mode %d\n", curr_station_info->ID, mcs, phy_mode);

                    if (station_info_update_rate(curr_station_info, mcs, phy_mode) != 0) {
                        status = CMD_PARAM_WARNING;
                    }

                    break;
                }
                curr_entry = dl_entry_next(curr_entry);
            }
        } else {
            if (aid == WLAN_EXP_AID_ALL) {
                // This is not an error because we are trying to set the rate for all
                // associations and there currently are none.
                status = CMD_PARAM_SUCCESS;
            }
        }

        // Set return values
        if (status == CMD_PARAM_SUCCESS) {
            *ret_mcs      = mcs;
            *ret_phy_mode = phy_mode;
        }

    // For Reads
    } else {
        if (aid != WLAN_EXP_AID_ALL) {
            curr_list  = get_bss_member_list();

            if (curr_list != NULL) {
                iter       = curr_list->length;
                curr_entry = curr_list->first;

                while ((curr_entry != NULL) && (iter-- > 0)) {
                    curr_station_info = (station_info_t*)(curr_entry->data);
                    if (aid == curr_station_info->ID) {
                        *ret_mcs      = (curr_station_info->tx.phy.mcs      & 0xFF);
                        *ret_phy_mode = (curr_station_info->tx.phy.phy_mode & 0xFF);
                        status       = CMD_PARAM_SUCCESS;
                        break;
                    }
                    curr_entry = dl_entry_next(curr_entry);
                }
            }
        }

        // NOTE:  Trying to read the rate for all associations returns an error.
    }

    return status;
}



/*****************************************************************************/
/**
 * Process TX Antenna Mode
 *
 * @param   cmd              - Command:  NODE_WRITE_VAL or NODE_READ_VAL
 * @param   aid              - AID of the station or NODE_CONFIG_ALL_ASSOCIATED
 * @param   ant_mode         - Antenna mode (function assumes antenna mode is valid)
 *
 * @return  u32              - Antenna mode
 *                             - CMD_PARAM_ERROR on ERROR
 *
 *****************************************************************************/
u32 process_tx_ant_mode(u32 cmd, u32 aid, u8 ant_mode) {

    int             iter;
    u32             mode;
    dl_list       * curr_list;
    dl_entry      * curr_entry;
    station_info_t* curr_station_info;

    mode = CMD_PARAM_ERROR;

    // For Writes
    if (cmd == CMD_PARAM_WRITE_VAL) {
        curr_list  = get_bss_member_list();

        if (curr_list != NULL) {
            if (curr_list->length == 0) { return ant_mode; }

            iter       = curr_list->length;
            curr_entry = curr_list->first;

            while ((curr_entry != NULL) && (iter-- > 0)) {
                curr_station_info = (station_info_t*)(curr_entry->data);

                if (aid == WLAN_EXP_AID_ALL) {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                    "Set TX ant mode on AID %d = %d \n", curr_station_info->ID, ant_mode);
                    curr_station_info->tx.phy.antenna_mode = ant_mode;
                    mode                                   = ant_mode;

                } else if (aid == curr_station_info->ID) {
                    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node,
                                    "Set TX ant mode on AID %d = %d \n", curr_station_info->ID, ant_mode);
                    curr_station_info->tx.phy.antenna_mode = ant_mode;
                    mode                                   = ant_mode;
                    break;
                }
                curr_entry = dl_entry_next(curr_entry);
            }
        } else {
            if (aid == WLAN_EXP_AID_ALL) {
                // This is not an error because we are trying to set the mode for all
                // associations and there currently are none.
                mode = ant_mode;
            }
        }

    // For Reads
    } else {
        if (aid != WLAN_EXP_AID_ALL) {
            curr_list  = get_bss_member_list();

            if (curr_list != NULL) {
                iter       = curr_list->length;
                curr_entry = curr_list->first;

                while ((curr_entry != NULL) && (iter-- > 0)) {
                    curr_station_info = (station_info_t*)(curr_entry->data);
                    if (aid == curr_station_info->ID) {
                        mode = curr_station_info->tx.phy.antenna_mode;
                        break;
                    }
                    curr_entry = dl_entry_next(curr_entry);
                }
            }
        }

        // NOTE:  Trying to read the mode for all associations returns an error.
    }

    return mode;
}




#ifdef _DEBUG_

/*****************************************************************************/
/**
 * Print Node Info
 *
 * @param   info             - Pointer to Node info structure
 *
 * @return  None.
 *
 *****************************************************************************/
void print_wlan_exp_node_info(wlan_exp_node_info * info) {
    int i;

    xil_printf("Node Information: \n");
    xil_printf("  WLAN Exp Type:      0x%08x\n", info->wlan_exp_type);
    xil_printf("  Node ID:            %d\n",     info->node_id);
    xil_printf("  Version:            %d\n",     info->version);
    xil_printf("  Serial Number:      0x%x\n",   info->serial_number);

    xil_printf("  FPGA DNA:           ");
    for( i = 0; i < FPGA_DNA_LEN; i++ ) {
        xil_printf("0x%8x  ", info->fpga_dna[i]);
    }
    xil_printf("\n");

    xil_printf("  WLAN HW Address:    "); wlan_exp_print_mac_address(&info->wlan_hw_addr[0]); xil_printf("\n");
    xil_printf("  Scheduler Res:      %d\n",     info->wlan_scheduler_resolution);

    xil_printf("  Transport Information:\n");
    xil_printf("    Type:             0x%8x\n",  info->eth_dev->info.type);
    xil_printf("    HW Address:       "); wlan_exp_print_mac_address(&info->eth_dev->info.hw_addr[0]); xil_printf("\n");
    
    xil_printf("  IP Address 0:       %d",       info->eth_dev->info.ip_addr[0]);
    for( i = 1; i < IP_VERSION; i++ ) {
        xil_printf(".%d", info->eth_dev->info.ip_addr[i]);
    }
    xil_printf("\n");

    xil_printf("    Group ID:         0x%8x\n",  info->eth_dev->info.group_id);
    xil_printf("    Unicast Port:     %d\n",     info->eth_dev->info.unicast_port);
    xil_printf("    Broadcast Port:   %d\n",     info->eth_dev->info.broadcast_port);
    xil_printf("\n");
    
}

#endif

#endif        // End WLAN_SW_CONFIG_ENABLE_WLAN_EXP
