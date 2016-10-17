/** @file wlan_exp_transport.c
 *  @brief Experiment Framework (Transport)
 *
 * Implements the Transport protocol layer for the embedded processor
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
#include <xstatus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_high.h"


// WARP includes
#include "warp_hw_ver.h"


// WLAN EXP includes
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_exp_transport.h"



#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// Transport information
static transport_eth_dev_info     eth_devices[TRANSPORT_NUM_ETH_DEVICES];
static wlan_exp_tag_parameter     transport_parameters[TRANSPORT_NUM_ETH_DEVICES][TRANSPORT_PARAM_MAX_PARAMETER];

// Callbacks
volatile wlan_exp_function_ptr_t  process_hton_msg_callback;



/*************************** Function Prototypes *****************************/

void transport_eth_dev_info_init(u32 eth_dev_num, wlan_exp_node_info * node_info, u8 * ip_addr, u8 * hw_addr, u16 unicast_port, u16 broadcast_port);

void transport_set_eth_phy_auto_negotiation(u32 eth_dev_num, u32 enable);
void transport_set_phy_link_speed(u32 eth_dev_num, u32 speed);

int  transport_check_device(u32 eth_dev_num);

int  transport_init_parameters(u32 eth_dev_num, u32 * values);

/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * @brief Transport subsystem initialization
 *
 * Initializes the transport subsystem
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   node_info        - Pointer to the Node info structure (wlan_exp_node_info)
 * @param   ip_addr          - Pointer to IP address for transport
 * @param   hw_addr          - Pointer to HW address (MAC address) for transport
 * @param   unicast_port     - Unicast port for transport
 * @param   broadcast_port   - Broadcast port for transport
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 ******************************************************************************/
int transport_init(u32 eth_dev_num, void * node_info, u8 * ip_addr, u8 * hw_addr, u16 unicast_port, u16 broadcast_port) {

    int       status = XST_SUCCESS;

    // Print initialization message
    wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "Configuring transport ...\n");

    // Initialize the User callback for processing a packet
    process_hton_msg_callback = wlan_exp_null_callback;

    // Check that we are initializing a valid Ethernet device for the transport
    if (transport_check_device(eth_dev_num) != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Initialize the WARP IP/UDP transport
    warp_ip_udp_init();

    // Print MAC address and IP address
    wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "  ETH %c MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            warp_conv_eth_dev_num(eth_dev_num), hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);
    wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "  ETH %c IP  Address: %d.%d.%d.%d\n",
            warp_conv_eth_dev_num(eth_dev_num), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);

    // Initialize the Ethernet device (use verbose mode)
    status = eth_init(eth_dev_num, hw_addr, ip_addr, WLAN_EXP_TRUE);

    if (status != XST_SUCCESS) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Ethernet %c initialization error\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    // Set interrupt callbacks
    //     NOTE:  The WLAN project uses interrupts so we should make sure to disable
    //         interrupts when sending Ethernet packets.  This step must be done after
    //         the call to eth_init().
    eth_set_interrupt_enable_callback((void *)wlan_mac_high_interrupt_restore_state);
    eth_set_interrupt_disable_callback((void *)wlan_mac_high_interrupt_stop);

    // Initialize the transport_eth_dev_info structure for the Ethernet device
    transport_eth_dev_info_init(eth_dev_num, (wlan_exp_node_info *)node_info, ip_addr, hw_addr, unicast_port, broadcast_port);

    // Set the Ethernet link speed
    if (WLAN_EXP_NEGOTIATE_ETH_LINK_SPEED) {
        // Enable auto-negotiation in the Ethernet PHY
        transport_set_eth_phy_auto_negotiation(eth_dev_num, WLAN_EXP_ENABLE);

        // Update the link speed
        transport_update_link_speed(eth_dev_num, ETH_WAIT_FOR_AUTO_NEGOTIATION);

    } else {
        // Disable auto-negotiation in the Ethernet PHY
        transport_set_eth_phy_auto_negotiation(eth_dev_num, WLAN_EXP_DISABLE);

        // Update the link speed
        transport_update_link_speed(eth_dev_num, ETH_DO_NOT_WAIT_FOR_AUTO_NEGOTIATION);
    }

    // Start Ethernet device
    status = eth_start_device(eth_dev_num);

    if (status != XST_SUCCESS) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Cannot start Ethernet %c\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    // Configure the Sockets for each Ethernet Interface
    status = transport_config_sockets(eth_dev_num, unicast_port, broadcast_port, 1);

    if (status != XST_SUCCESS) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Cannot configure sockets for Ethernet %c\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    // Initialize the tag parameters
    transport_init_parameters(eth_dev_num, (u32 *) &(eth_devices[eth_dev_num].info));

    return status;
}



/*****************************************************************************/
/**
 * Initialize the information about the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 ******************************************************************************/
void transport_eth_dev_info_init(u32 eth_dev_num, wlan_exp_node_info * node_info, u8 * ip_addr, u8 * hw_addr, u16 unicast_port, u16 broadcast_port) {

    // Initialize the fields for the given Ethernet device
    eth_devices[eth_dev_num].node_id             = node_info->node_id;
    eth_devices[eth_dev_num].initialized         = TRANSPORT_ETH_DEV_INITIALIZED;
    eth_devices[eth_dev_num].default_speed       = WLAN_EXP_DEFAULT_SPEED;
    eth_devices[eth_dev_num].max_pkt_words       = WLAN_EXP_DEFAULT_MAX_PACKET_WORDS;


    // Initialize fields that depend on the Ethernet device
    switch (eth_dev_num) {
        case TRANSPORT_ETH_A:
            eth_devices[eth_dev_num].phy_addr    = TRANSPORT_ETH_A_MDIO_PHYADDR;
        break;

        case TRANSPORT_ETH_B:
            eth_devices[eth_dev_num].phy_addr    = TRANSPORT_ETH_B_MDIO_PHYADDR;
        break;

        default:
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Ethernet device %c not configured in hardware.\n", warp_conv_eth_dev_num(eth_dev_num));
        break;
    }


    // Initialize all sockets as invalid
    eth_devices[eth_dev_num].socket_unicast      = SOCKET_INVALID_SOCKET;
    eth_devices[eth_dev_num].socket_broadcast    = SOCKET_INVALID_SOCKET;
    eth_devices[eth_dev_num].socket_async        = SOCKET_INVALID_SOCKET;


    // Initialize the Asynchronous send socket address structure
    //
    //     NOTE:  Socket IP address and port will be filled in when async socket is initialized
    //
    bzero((void *) &eth_devices[eth_dev_num].async_sockaddr, sizeof(struct sockaddr));

    ((struct sockaddr_in *)&eth_devices[eth_dev_num].async_sockaddr)->sin_family = AF_INET;

    // Initialize the Asynchronous cmd/resp structure
    eth_devices[eth_dev_num].async_cmd_resp.args      = NULL;
    eth_devices[eth_dev_num].async_cmd_resp.header    = NULL;
    eth_devices[eth_dev_num].async_cmd_resp.buffer    = NULL;

    // Initialize the transport_info structure
    eth_devices[eth_dev_num].info.type           = (eth_dev_num << 16) | TRANSPORT_PARAM_TYPE_UDP;;
    eth_devices[eth_dev_num].info.hw_addr[0]     = (hw_addr[0]<<8)  |  hw_addr[1];
    eth_devices[eth_dev_num].info.hw_addr[1]     = (hw_addr[2] << 24) | (hw_addr[3] << 16) | (hw_addr[4] << 8) | hw_addr[5];
    eth_devices[eth_dev_num].info.ip_addr        = (ip_addr[0] << 24) | (ip_addr[1] << 16) | (ip_addr[2] << 8) | ip_addr[3];
    eth_devices[eth_dev_num].info.group_id       = 0;
    eth_devices[eth_dev_num].info.unicast_port   = unicast_port;
    eth_devices[eth_dev_num].info.broadcast_port = broadcast_port;


    // Set the Ethernet device in the node_info structure
    node_info->eth_dev                           = & eth_devices[eth_dev_num];

}



/*****************************************************************************/
/**
 * Close all sockets associated with a given Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 *****************************************************************************/
void transport_close(u32 eth_dev_num) {

    if (transport_check_device(eth_dev_num) == XST_SUCCESS) {
        socket_close(eth_devices[eth_dev_num].socket_unicast);
        socket_close(eth_devices[eth_dev_num].socket_broadcast);
        socket_close(eth_devices[eth_dev_num].socket_async);

        // Free async send buffer
        if (eth_devices[eth_dev_num].async_cmd_resp.buffer != NULL) {
            socket_free_send_buffer(eth_devices[eth_dev_num].async_cmd_resp.buffer);
        }
    }
}



/*****************************************************************************/
/**
 * This function will poll the given Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 * @note    Buffers are managed by the WARP UDP transport driver
 *
 *****************************************************************************/
void transport_poll(u32 eth_dev_num) {

    int                     recv_bytes;
    int                     socket_index;
    warp_ip_udp_buffer      recv_buffer;
    warp_ip_udp_buffer    * send_buffer;
    struct sockaddr         from;

    // Check the socket to see if there is data
    recv_bytes = socket_recvfrom_eth(eth_dev_num, &socket_index, &from, &recv_buffer);

    // If we have received data, then we need to process it
    if (recv_bytes > 0) {
        // Allocate a send buffer from the transport driver
        send_buffer = socket_alloc_send_buffer();

        // Process the received packet
        transport_receive(eth_dev_num, socket_index, &from, &recv_buffer, send_buffer);

        // Need to communicate to the transport driver that the buffers can now be reused
        socket_free_recv_buffer(socket_index, &recv_buffer);
        socket_free_send_buffer(send_buffer);
    }
}



/*****************************************************************************/
/**
 * Process the received UDP packet by the transport
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   socket_index     - Index of the socket on which message was received
 * @param   from             - Pointer to socket address structure from which message was received
 * @param   recv_buffer      - Pointer to transport buffer with received message
 * @param   send_buffer      - Pointer to transport buffer for a node response to the message
 *
 * @return  None
 *
 * @note    If this packet is a host to node message, then the  process_hton_msg_callback
 *          is used to further process the packet.  This method will strip off the
 *          Transport header for future packet processing.
 *
 *****************************************************************************/
void transport_receive(u32 eth_dev_num, int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, warp_ip_udp_buffer * send_buffer) {

    int                           status;
    u16                           dest_id;
    u16                           src_id;
    u16                           seq_num;
    u16                           flags;
    u32                           node_id;
    u32                           group_id;
    u32                           recv_flags = 0;

    transport_header            * transport_header_rx = (transport_header*)(recv_buffer->offset);   // Contains entire Ethernet frame; offset points to UDP payload
    transport_header            * transport_header_tx = (transport_header*)(send_buffer->offset);   // New buffer for UDP payload

    // Get the transport headers for the send / receive buffers
    //     NOTE:  For the receive buffer, offset points to UDP payload of the Ethernet frame
    //     NOTE:  For the send buffer, the offset points to the start of the buffer but since we will use the
    //            UDP header of the socket to transmit the frame, this is effectively the start of the UDP payload
    //
    transport_header_rx      = (transport_header*)(recv_buffer->offset);
    transport_header_tx      = (transport_header*)(send_buffer->offset);

    // Update the buffers to account for the transport headers
    recv_buffer->offset     += sizeof(transport_header);
    recv_buffer->length     -= sizeof(transport_header);                    // Remaining bytes in receive buffer

    send_buffer->offset     += sizeof(transport_header);
    send_buffer->length     += sizeof(transport_header);                    // Adding bytes to the send buffer
    send_buffer->size       += sizeof(transport_header);                    // Keep size in sync


    // Process the data based on the packet type
    //     NOTE:  The pkt_type does not need to be endian swapped because it is a u8
    //
    switch(transport_header_rx->pkt_type){

        //-------------------------------
        // Message from the Host to the Node
        //
        case PKT_TYPE_HTON_MSG:
            // Extract values from the received transport header
            //
            dest_id  = Xil_Ntohs(transport_header_rx->dest_id);
            src_id   = Xil_Ntohs(transport_header_rx->src_id);
            seq_num  = Xil_Ntohs(transport_header_rx->seq_num);
            flags    = Xil_Ntohs(transport_header_rx->flags);

            node_id  = eth_devices[eth_dev_num].node_id;
            group_id = eth_devices[eth_dev_num].info.group_id;

            // If this message is not for the given node, then ignore it
            if((dest_id != node_id) && (dest_id != TRANSPORT_BROADCAST_DEST_ID) && ((dest_id & (0xFF00 | group_id)) == 0)) { return; }

            // Set the receive flags
            //     [0] - Is the packet broadcast?  WLAN_EXP_TRUE / WLAN_EXP_FALSE
            //
            if (dest_id == TRANSPORT_BROADCAST_DEST_ID) {
                recv_flags |= 0x00000001;
            }

            // Form outgoing Transport header for any outgoing packet in response to this message
            //     NOTE:  The u16/u32 fields here will be endian swapped in transport_send
            //     NOTE:  The length field of the header will be set in transport_send
            //
            transport_header_tx->dest_id  = src_id;
            transport_header_tx->src_id   = node_id;
            transport_header_tx->pkt_type = PKT_TYPE_NTOH_MSG;
            transport_header_tx->seq_num  = seq_num;
            transport_header_tx->flags    = 0;
            transport_header_tx->reserved = 0;

            // Call the callback to further process the recv_buffer
            status = process_hton_msg_callback(socket_index, from, recv_buffer, recv_flags, send_buffer);

            if (send_buffer->size != send_buffer->length) {
                wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_transport, "Send buffer length (%d) does not match size (%d)\n", send_buffer->length, send_buffer->size);
            }

            // Based on the status, return a message to the host
            switch(status) {

                //-------------------------------
                // No response has been sent by the node
                //
                case NO_RESP_SENT:
                    // Check if the host requires a response from the node
                    if (flags & TRANSPORT_HDR_ROBUST_FLAG) {

                        // Check that the node has something to send to the host
                        if ((send_buffer->length) > sizeof(transport_header)) {
                            transport_send(socket_index, from, &send_buffer, 1);                        // Send the buffer of data
                        } else {
                            wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_transport, "Host requires response but node has nothing to send.\n");
                        }
                    }
                break;

                   //-------------------------------
                // A response has already been sent by the node
                //
                case RESP_SENT:
                    // The transport does not need to do anything else
                break;

                default:
                    wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Received unknown status for message: %d\n", status);
                break;
               }
        break;

        default:
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Received packet with unknown packet type: %d\n", (transport_header_rx->pkt_type));
        break;
    }
}



/*****************************************************************************/
/**
 * This function is used to send a message over Ethernet
 *
 * @param   socket_index     - Index of the socket on which to send message
 * @param   to               - Pointer to socket address structure to send message
 * @param   buffers          - Array of transport buffers to send
 * @param   num_buffers      - Number of transport buffers in 'buffers' array
 *
 * @return  None
 *
 * @note    This function requires that the first transport buffer in the 'buffers'
 *          array contain the Transport header.
 *
 *****************************************************************************/
void transport_send(int socket_index, struct sockaddr * to, warp_ip_udp_buffer ** buffers, u32 num_buffers) {

    u32                      i;
    int                      status;
    transport_header       * transport_header_tx;
    u16                      buffer_length = 0;
	// interrupt_state_t     prev_interrupt_state;

    // Check that we have a valid socket to send a message on
    if (socket_index == SOCKET_INVALID_SOCKET) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Invalid socket.\n");
        return;
    }

    // Initialize the header
    //     NOTE:  We require that the first warp_ip_udp_buffer always contain the wl_transport_header
    //
    transport_header_tx = (transport_header *)(buffers[0]->data);

    // Compute the length
    for (i = 0; i < num_buffers; i++) {
        buffer_length += buffers[i]->size;
    }

    //
    // NOTE:  Through performance testing, we found that it was faster to just manipulate the header
    //   in place vs creating a copy, updating the header and then restoring the copy.
    //

    // Make the outgoing transport header endian safe for sending on the network
    //     NOTE:  Set the 'length' to the computed value above
    //
    transport_header_tx->dest_id = Xil_Htons(transport_header_tx->dest_id);
    transport_header_tx->src_id  = Xil_Htons(transport_header_tx->src_id);
    transport_header_tx->length  = Xil_Htons(buffer_length + WARP_IP_UDP_DELIM_LEN);
    transport_header_tx->seq_num = Xil_Htons(transport_header_tx->seq_num);
    transport_header_tx->flags   = Xil_Htons(transport_header_tx->flags);

	// Check the interrupt status; Disable interrupts if enabled
    //     NOTE:  This is done inside the Eth send function
	// prev_interrupt_state = wlan_mac_high_interrupt_stop();

    // Send the Ethernet packet
    status = socket_sendto(socket_index, to, buffers, num_buffers);

	// Restore interrupts
    //     NOTE:  This is done inside the Eth send function
	// wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

    // Restore wl_header_tx
	transport_header_tx->dest_id = Xil_Ntohs(transport_header_tx->dest_id);
	transport_header_tx->src_id  = Xil_Ntohs(transport_header_tx->src_id);
	transport_header_tx->length  = 0;
	transport_header_tx->seq_num = Xil_Ntohs(transport_header_tx->seq_num);
	transport_header_tx->flags   = Xil_Ntohs(transport_header_tx->flags);

    // Check that the packet was sent correctly
    if (status == WARP_IP_UDP_FAILURE) {
        wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_transport, "Issue sending packet %d to host.\n", transport_header_tx->seq_num);
    }
}



/*****************************************************************************/
/**
 * This function is used to send a message over Ethernet
 *
 * @param   buffers          - Array of transport buffers to send
 * @param   num_buffers      - Number of transport buffers in 'buffers' array
 *
 * @return  None
 *
 * @note    This function requires that the first transport buffer in the 'buffers'
 *          array contain the Transport header.
 *
 *****************************************************************************/
void transport_send_async(u32 eth_dev_num, u8 * payload, u32 length) {

    u32                      initial_length;
    u32                      packet_length;

    // Get variables from the async command / response structure
    warp_ip_udp_buffer     * buffer       = (warp_ip_udp_buffer *) eth_devices[eth_dev_num].async_cmd_resp.buffer;
    cmd_resp_hdr           * cmd_header   = eth_devices[eth_dev_num].async_cmd_resp.header;
    void                   * payload_dest = (void *) eth_devices[eth_dev_num].async_cmd_resp.args;
    transport_header       * tmp_header   = (transport_header *) (buffer->offset);

    // Set length fields
    initial_length = buffer->length;
    packet_length  = WARP_IP_UDP_HEADER_LEN + initial_length + length;

    // Makes sure packet stays under the maximum packet size
    if (packet_length < WLAN_EXP_TX_ASYNC_PACKET_BUFFER_SIZE) {

        // Update the command header size
        cmd_header->length = length;

        // Update the buffer length / size
        buffer->length    += length;
        buffer->size      += length;

        // Populate the buffer with the payload
        wlan_mac_high_cdma_start_transfer((void *) payload, payload_dest, length);

        // Send the command
        transport_send(eth_devices[eth_dev_num].socket_async, &eth_devices[eth_dev_num].async_sockaddr, &buffer, 0x1);

        // Increment the sequence number in the header
        tmp_header->seq_num++;

        // Set the length fields back to the original values
        buffer->length     = initial_length;
        buffer->size       = initial_length;
    }
}



/*****************************************************************************/
/**
 * Process Transport Commands
 *
 * This function is part of the Ethernet processing system and will process the
 * various transport related commands.
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
int process_transport_cmd(int socket_index, void * from, cmd_resp * command, cmd_resp * response, u32 max_resp_len) {

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

    cmd_resp_hdr      * cmd_hdr        = command->header;
    u32               * cmd_args_32    = command->args;
    u32                 cmd_id         = CMD_TO_CMDID(cmd_hdr->cmd);

    cmd_resp_hdr      * resp_hdr       = response->header;
    u32               * resp_args_32   = response->args;
    u32                 resp_index     = 0;

    u32                 eth_dev_num    = socket_get_eth_dev_num(socket_index);


    // Set up the response header
    resp_hdr->cmd       = cmd_hdr->cmd;
    resp_hdr->length    = 0;
    resp_hdr->num_args  = 0;

    // Check Ethernet device number
    if (eth_dev_num == WARP_IP_UDP_INVALID_ETH_DEVICE) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Invalid socket index: %d\n", socket_index);
        return resp_sent;
    }

    // Process the command
    switch(cmd_id){

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_PING: {
            //
            // Nothing actually needs to be done when receiving the ping command. The framework is going
            // to respond regardless, which is all the host wants.
            //
        }
        break;

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_PAYLOAD_SIZE_TEST: {
            //
            // Due to packet fragmentation, it is not safe to just return the packet length.  We have seen
            // an issue where the host will send a 1514 byte fragment which results in a payload size of
            // 1472 and causes the transport to not behave correctly.  Therefore, we need to find the
            // last valid command argument and check that against the packet length.
            //
            u32              temp;
            u32              payload_index;
            u32              payload_size;
            u32              payload_num_words;
            u32              header_size;

            header_size        = (sizeof(transport_header) + sizeof(cmd_resp_hdr));                               // Transport / Command headers
            payload_index      = (((warp_ip_udp_buffer *)(command->buffer))->length - sizeof(cmd_resp_hdr)) / 4;  // Final index into command args (/4 truncates)

            // Check the value in the command args to make sure it matches the size_index
            payload_num_words  = Xil_Htonl(cmd_args_32[payload_index - 1]) + 1;     // NOTE:  Add 1 since the payload is zero indexed
            payload_size       = (payload_num_words * 4) + header_size;
            temp               = ((payload_index * 4) + header_size);

            // Update the max_pkt_words field
            if (payload_num_words > eth_devices[eth_dev_num].max_pkt_words) {
                eth_devices[eth_dev_num].max_pkt_words = payload_num_words;
            }

            if (payload_size != temp) {
                wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_transport, "Payload size mismatch.  Value in command args does not match index:  %d != %d\n", payload_size, temp);
            }

            resp_args_32[resp_index++] = Xil_Ntohl(payload_size);

            resp_hdr->length  += (resp_index * sizeof(resp_args_32));
            resp_hdr->num_args = 1;
        }
        break;

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_NODE_GROUP_ID_ADD: {
            eth_devices[eth_dev_num].info.group_id = (eth_devices[eth_dev_num].info.group_id | Xil_Htonl(cmd_args_32[0]));
        }
        break;

        //---------------------------------------------------------------------
        case CMDID_TRANSPORT_NODE_GROUP_ID_CLEAR: {
            eth_devices[eth_dev_num].info.group_id = (eth_devices[eth_dev_num].info.group_id & ~Xil_Htonl(cmd_args_32[0]));
        }
        break;

        //---------------------------------------------------------------------
        default: {
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Unknown user command ID: %d\n", cmd_id);
        }
        break;
    }

    return resp_sent;
}



/*****************************************************************************/
/**
 * This function is the Transport callback that allows framework to process
 * a received Ethernet packet
 *
 * @param   hander           - Pointer to the transport receive callback function
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *
 *****************************************************************************/
int transport_set_process_hton_msg_callback(void(*handler)) {
    process_hton_msg_callback = handler;

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Create and bind a socket for the Ethernet device
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   socket_index     - Socket index (return value)
 * @param   udp_port         - UDP port number
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int transport_config_socket(u32 eth_dev_num, int * socket_index, u32 udp_port) {

    int status;
    int tmp_socket  = *socket_index;

    // Release socket if it is already bound
    if (tmp_socket != SOCKET_INVALID_SOCKET) {
        socket_close(tmp_socket);
    }

    // Create a new socket
    tmp_socket = socket_socket(AF_INET, SOCK_DGRAM, 0);

    if (tmp_socket == SOCKET_INVALID_SOCKET) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Could not create socket\n");

        * socket_index = SOCKET_INVALID_SOCKET;

        return XST_FAILURE;
    }

    // Bind the socket
    status = socket_bind_eth(tmp_socket, eth_dev_num, udp_port);

    if (status == WARP_IP_UDP_FAILURE) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Unable to bind socket on port: %d\n", udp_port);

        socket_close(tmp_socket);

        * socket_index = SOCKET_INVALID_SOCKET;

        return XST_FAILURE;
    }

    * socket_index = tmp_socket;

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * This function will configure the unicast and broadcast sockets to be used
 * by the transport using the default transport_receive_callback() function.
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   unicast_port     - Unicast port for the node
 * @param   broadcast_port   - Broadcast port for the node
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int transport_config_sockets(u32 eth_dev_num, u32 unicast_port, u32 broadcast_port, u8 verbose) {
    int status = XST_SUCCESS;

    status = transport_config_socket(eth_dev_num, &(eth_devices[eth_dev_num].socket_unicast), unicast_port);
    if (status == XST_FAILURE) { return status; }

    status = transport_config_socket(eth_dev_num, &(eth_devices[eth_dev_num].socket_broadcast), broadcast_port);
    if (status == XST_FAILURE) { return status; }

    // Update the Tag Parameters
    eth_devices[eth_dev_num].info.unicast_port   = unicast_port;
    eth_devices[eth_dev_num].info.broadcast_port = broadcast_port;

    if (verbose) {
        wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "  Listening on UDP ports %d (unicast) and %d (broadcast)\n", unicast_port, broadcast_port);
    }

    return status;
}



/*****************************************************************************/
/**
 * This function will read the status of a Ethernet controller
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  u16              - Bits are defined for the Ethernet chip.  See wl_transport.h
 *                             for defines for relevant fields
 *
 *****************************************************************************/
u16 transport_get_ethernet_status(u32 eth_dev_num) {

    u16 reg_val = 0;

    // Check that we are initializing a valid Ethernet device for the transport
    if (transport_check_device(eth_dev_num) != XST_SUCCESS) {
        return LINK_NOT_READY;
    }

    if (eth_devices[eth_dev_num].initialized == TRANSPORT_ETH_DEV_INITIALIZED) {

        // Check if the Ethernet PHY reports a valid link
        eth_read_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_STATUS_REG, &reg_val);
    }

    return reg_val;
}



/*****************************************************************************/
/**
 * This function will check the link status of a Ethernet controller
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  int              - Status of the command:
 *                                 LINK_READY     - Ethernet controller is ready to be used
 *                                 LINK_NOT_READY - Ethernet controller is not ready to be used
 *
 *****************************************************************************/
int transport_link_status(u32 eth_dev_num) {

    int status  = LINK_READY;
    u16 reg_val = transport_get_ethernet_status(eth_dev_num);

    if(reg_val & ETH_PHY_REG_17_0_LINKUP) {
        status = LINK_READY;
    } else {
        status = LINK_NOT_READY;
    }

    return status;
}



/*****************************************************************************/
/**
 * This function will update the link speed of a Ethernet controller
 *
 * @param   eth_dev_num           - Ethernet device number
 * @param   wait_for_negotiation  - Flag to wait for auto-negotiation of Ethernet link speed
 *
 * @return  speed            - Ethernet link speed that was chosen
 *
 *****************************************************************************/
u32 transport_update_link_speed(u32 eth_dev_num, u32 wait_for_negotiation) {

    volatile u16 reg_val          = 0;
    u32          negotiated       = 1;
    u16          speed            = 0;

    u32          start_timestamp  = get_system_time_usec();
    u32          end_timestamp    = start_timestamp;

    // Make sure the Ethernet device is initialized
    if (eth_devices[eth_dev_num].initialized == TRANSPORT_ETH_DEV_INITIALIZED) {

        xil_printf("  ETH %c speed ", warp_conv_eth_dev_num(eth_dev_num));

        reg_val = transport_get_ethernet_status(eth_dev_num);

        if (wait_for_negotiation == ETH_WAIT_FOR_AUTO_NEGOTIATION) {

            while((reg_val & ETH_PHY_REG_17_0_SPEED_RESOLVED) == 0) {
                usleep(1000);
                reg_val = transport_get_ethernet_status(eth_dev_num);
            }

            speed = ETH_PHY_SPEED_TO_MBPS((reg_val & ETH_PHY_REG_17_0_SPEED));
            end_timestamp = get_system_time_usec();

        } else {
            // Check to see if the Ethernet controller has auto-negotiated a speed
            if (reg_val & ETH_PHY_REG_17_0_SPEED_RESOLVED) {
                speed      = ETH_PHY_SPEED_TO_MBPS((reg_val & ETH_PHY_REG_17_0_SPEED));
            } else {
                speed      = eth_devices[eth_dev_num].default_speed;
                negotiated = 0;
            }
        }

        // Set the operating speed of the Ethernet controller
        eth_set_mac_operating_speed(eth_dev_num, speed);

        // Set the operating speed of the Ethernet PHY
        transport_set_phy_link_speed(eth_dev_num, speed);

        // Sleep for a short period of time to let everything settle
        usleep(1 * 10000);

    } else {
        wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "  ETH %c not initialized.  Link speed not updated.\n", warp_conv_eth_dev_num(eth_dev_num));
    }

    if (negotiated) {
        xil_printf("%d Mbps (auto-negotiated", speed);

        if (start_timestamp != end_timestamp) {
            xil_printf(" in %d usec)\n", (end_timestamp - start_timestamp));
        } else {
            xil_printf(")\n");
        }
    } else {
        xil_printf("%d Mbps (default)\n", speed);
    }

    return speed;
}



/*****************************************************************************/
/**
 * This function set the speed of a Ethernet PHY
 *
 * @param   eth_dev_num           - Ethernet device number
 * @param   speed                 - Speed
 *
 * @return  None
 *
 *****************************************************************************/
void transport_set_phy_link_speed(u32 eth_dev_num, u32 speed) {

    // See Ethernet PHY specification for documentation on the values used for PHY commands
    u16          phy_ctrl_reg_val;

    // Read the PHY Control register
    eth_read_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, &phy_ctrl_reg_val);

    // Based on the argument configure the appropriate bits to set the desired speed
    switch (speed) {
        case ETH_PHY_SPEED_1000_MBPS:
            // Set speed to 1000 Mbps (MSB = 1; LSB = 0)
            phy_ctrl_reg_val = (phy_ctrl_reg_val & ~ETH_PHY_REG_0_SPEED_LSB) | ETH_PHY_REG_0_SPEED_MSB;
        break;
        case ETH_PHY_SPEED_100_MBPS:
            // Set speed to 100 Mbps (MSB = 0; LSB = 1)
            phy_ctrl_reg_val = (phy_ctrl_reg_val & ~ETH_PHY_REG_0_SPEED_MSB) | ETH_PHY_REG_0_SPEED_LSB;
        break;
        case ETH_PHY_SPEED_10_MBPS:
            // Set speed to 10 Mbps (MSB = 0; LSB = 0)
            phy_ctrl_reg_val = phy_ctrl_reg_val & ~(ETH_PHY_REG_0_SPEED_MSB | ETH_PHY_REG_0_SPEED_LSB);
        break;
        default:
            wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Ethernet %c invalid speed: %d.\n", warp_conv_eth_dev_num(eth_dev_num), speed);
        break;
    }

    // Write the value to the PHY and trigger a reset to update PHY internal state
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, phy_ctrl_reg_val);
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, (ETH_PHY_REG_0_RESET | phy_ctrl_reg_val));
}



/*****************************************************************************/
/**
 * This function set the auto-negotiation state of a Ethernet controller
 *
 * @param   eth_dev_num           - Ethernet device number
 * @param   enable                - Enable / Disable auto-negotiation
 *
 * @return  None
 *
 *****************************************************************************/
void transport_set_eth_phy_auto_negotiation(u32 eth_dev_num, u32 enable) {

    // See Ethernet PHY specification for documentation on the values used for PHY commands
    u16          phy_ctrl_reg_val;

    // Read the PHY Control register
    eth_read_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, &phy_ctrl_reg_val);

    // Based on the argument enable or disable auto-negotiation
    if (enable) {
        // Enable auto-negotiation
        phy_ctrl_reg_val = phy_ctrl_reg_val | ETH_PHY_REG_0_AUTO_NEGOTIATION;
    } else {
        // Disable auto-negotiation
        phy_ctrl_reg_val = phy_ctrl_reg_val & ~ETH_PHY_REG_0_AUTO_NEGOTIATION;
    }

    // Write the value to the PHY and trigger a reset to update PHY internal state
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, phy_ctrl_reg_val);
    eth_write_phy_reg(eth_dev_num, eth_devices[eth_dev_num].phy_addr, ETH_PHY_CONTROL_REG, (ETH_PHY_REG_0_RESET | phy_ctrl_reg_val));
}



/*****************************************************************************/
/**
 * Check the Ethernet device of the transport
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 ******************************************************************************/

int transport_check_device(u32 eth_dev_num) {

    // Check that we have a valid Ethernet device for the transport
    if (eth_dev_num >= TRANSPORT_NUM_ETH_DEVICES) {
        wlan_exp_printf(WLAN_EXP_PRINT_ERROR, print_type_transport, "Ethernet %c is not available on WARP HW.\n", warp_conv_eth_dev_num(eth_dev_num));
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Set / Get the HW address (MAC address)
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   hw_addr          - u8 pointer of MAC address to set / get
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int transport_set_hw_addr(u32 eth_dev_num, u8 * hw_addr) {

    // Update the device information
    eth_devices[eth_dev_num].info.hw_addr[0]     = (hw_addr[0]<<8)  |  hw_addr[1];
    eth_devices[eth_dev_num].info.hw_addr[1]     = (hw_addr[2] << 24) | (hw_addr[3] << 16) | (hw_addr[4] << 8) | hw_addr[5];

    return eth_set_hw_addr(eth_dev_num, hw_addr);
}


int transport_get_hw_addr(u32 eth_dev_num, u8 * hw_addr) {

    return eth_get_hw_addr(eth_dev_num, hw_addr);
}



/*****************************************************************************/
/**
 * Set / Get the IP address
 *
 * @param   eth_dev_num      - Ethernet device number
 * @param   ip_addr          - u8 pointer of IP address to set / get
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int transport_set_ip_addr(u32 eth_dev_num, u8 * ip_addr) {

    // Update the device information
    eth_devices[eth_dev_num].info.ip_addr        = (ip_addr[0] << 24) | (ip_addr[1] << 16) | (ip_addr[2] << 8) | ip_addr[3];

    return eth_set_ip_addr(eth_dev_num, ip_addr);
}


int transport_get_ip_addr(u32 eth_dev_num, u8 * ip_addr) {

    return eth_get_ip_addr(eth_dev_num, ip_addr);
}



/*****************************************************************************/
/**
 * Reset the maximum packet length
 *
 * This function resets the maximum packet length.  This will be performed when a
 * node receives a CONFIG_RESET so that a new maximum length can be established
 * during the next initialization.
 *
 * @param   eth_dev_num      - Ethernet device number
 *
 * @return  None
 *
 *****************************************************************************/
void transport_reset_max_pkt_length(u32 eth_dev_num) {

    // Update the device information
    eth_devices[eth_dev_num].max_pkt_words       = WLAN_EXP_DEFAULT_MAX_PACKET_WORDS;
}



/*****************************************************************************/
/**
 * Initialize the transport TAG parameters structure
 *
 * @param   eth_dev_num      - Ethernet device to use
 * @param   values           - Pointer to structure from which to get all the tag parameter values
 *
 * @return  int              - Total number of bytes of the TAG parameter structure
 *
 * @note    Please make sure that the parameters structure and the parameter values
 *          maintain the same order
 *
 *****************************************************************************/
int  transport_init_parameters(u32 eth_dev_num, u32 * values) {

    u16    lengths[TRANSPORT_PARAM_MAX_PARAMETER] = TRANSPORT_PARAM_FIELD_LENGTHS;

    return wlan_exp_init_parameters((wlan_exp_tag_parameter *) &transport_parameters[eth_dev_num][0],
                                    GROUP_TRANSPORT,
                                    TRANSPORT_PARAM_MAX_PARAMETER,
                                    values,
                                    lengths);
}



/*****************************************************************************/
/**
 * @ brief Get all tag parameter information from the transport tag parameters
 *
 * This function will populate a buffer with the all the information from the
 * transport tag parameter structure.
 *
 * @param   eth_dev_num      - Ethernet device to use
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
int transport_get_parameters(u32 eth_dev_num, u32 * buffer, u32 max_resp_len, u8 transmit) {

    return wlan_exp_get_parameters((wlan_exp_tag_parameter *) &transport_parameters[eth_dev_num][0],
                                   TRANSPORT_PARAM_MAX_PARAMETER,
                                   buffer,
                                   max_resp_len,
                                   WLAN_EXP_FALSE,
                                   transmit);

}



#endif
