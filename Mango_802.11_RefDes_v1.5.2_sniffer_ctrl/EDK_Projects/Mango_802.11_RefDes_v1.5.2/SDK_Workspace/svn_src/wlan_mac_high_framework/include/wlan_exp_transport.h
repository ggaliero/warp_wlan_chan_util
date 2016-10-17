/** @file wlan_exp_transport.h
 *  @brief Experiment Framework (Transport)
 *
 * Implements the Transport protocol layer for the embedded processor.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

// WLAN Exp includes
#include "wlan_exp_common.h"

// WARP UDP transport includes
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
#include <WARP_ip_udp.h>
#include <WARP_ip_udp_device.h>
#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP


/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_TRANSPORT_H_
#define WLAN_EXP_TRANSPORT_H_


// ****************************************************************************
// Define Transport Commands
//
#define CMDID_TRANSPORT_PING                               0x001
#define CMDID_TRANSPORT_PAYLOAD_SIZE_TEST                  0x002
#define CMDID_TRANSPORT_NODE_GROUP_ID_ADD                  0x100
#define CMDID_TRANSPORT_NODE_GROUP_ID_CLEAR                0x101


// ****************************************************************************
// Define Transport Tag Parameters
//
//     NOTE:  To add another parameter, add the define before "TRANSPORT_PARAM_MAX_PARAMETER"
//         and then change the value of "TRANSPORT_PARAM_MAX_PARAMETER" to be the largest value
//         in the list so it is easy to iterate over all parameters
//
#define TRANSPORT_PARAM_TYPE                               0
#define TRANSPORT_PARAM_HW_ADDR                            1
#define TRANSPORT_PARAM_IP_ADDR                            2
#define TRANSPORT_PARAM_GROUP_ID                           3
#define TRANSPORT_PARAM_UNICAST_PORT                       4
#define TRANSPORT_PARAM_BROADCAST_PORT                     5

//
// ADD NEW TAG PARAMETERS HERE
//


//
// END ADD NEW TAG PARAMETERS HERE
//
//     NOTE:  Make sure that TRANSPORT_PARAM_MAX_PARAMETER is adjusted accordingly
//

#define TRANSPORT_PARAM_MAX_PARAMETER                      6


// TRANSPORT_PARAM_TYPE values
#define TRANSPORT_PARAM_TYPE_UDP                           0


// ****************************************************************************
// Define Node Tag Parameter Field Lengths
//
//     NOTE:  Tag Parameters must be 32 bit aligned.  The array below represents the number
//         of 32 bit unsigned integers required for each field.  If another field is added
//         to the Tag Parameters, then the TRANSPORT_PARAM_FIELD_LENGTHS array must be updated
//         to represent the appropriate length of each new field.
//
#define TRANSPORT_PARAM_FIELD_LENGTHS                      {1, 2, 1, 1, 1, 1}


// ****************************************************************************
// Define Transport Ethernet Information
//
#define TRANSPORT_NUM_ETH_DEVICES                          WARP_IP_UDP_NUM_ETH_DEVICES
#define TRANSPORT_ETH_DEV_INITIALIZED                      1

// Ethernet A constants
#define TRANSPORT_ETH_A                                    ETH_A_MAC
#define TRANSPORT_ETH_A_MDIO_PHYADDR                       0x6

// Ethernet B constants
#define TRANSPORT_ETH_B                                    ETH_B_MAC
#define TRANSPORT_ETH_B_MDIO_PHYADDR                       0x7

// Ethernet constants
#define ETH_DO_NOT_WAIT_FOR_AUTO_NEGOTIATION               0
#define ETH_WAIT_FOR_AUTO_NEGOTIATION                      1

// Ethernet PHY constants
#define ETH_PHY_CONTROL_REG                                0
#define ETH_PHY_STATUS_REG                                 17

#define ETH_PHY_REG_0_RESET                                0x8000
#define ETH_PHY_REG_0_SPEED_LSB                            0x2000
#define ETH_PHY_REG_0_AUTO_NEGOTIATION                     0x1000
#define ETH_PHY_REG_0_SPEED_MSB                            0x0040

#define ETH_PHY_REG_17_0_SPEED                             0xC000
#define ETH_PHY_REG_17_0_SPEED_RESOLVED                    0x0800
#define ETH_PHY_REG_17_0_LINKUP                            0x0400


// NOTE:  These constants are for in place computation based on ETH_PHY_REG_17_0_SPEED (ie no bit shifting required)
#define ETH_PHY_REG_17_0_SPEED_10_MBPS                     0x0000
#define ETH_PHY_REG_17_0_SPEED_100_MBPS                    0x4000
#define ETH_PHY_REG_17_0_SPEED_1000_MBPS                   0x8000
#define ETH_PHY_REG_17_0_SPEED_RSVD                        0xC000


// Ethernet PHY macros
//     NOTE:  Speed is defined by the Ethernet hardware (xaxiethernet_hw.h):
//         #define XAE_SPEED_10_MBPS        10      /**< Speed of 10 Mbps */
//         #define XAE_SPEED_100_MBPS       100     /**< Speed of 100 Mbps */
//         #define XAE_SPEED_1000_MBPS      1000    /**< Speed of 1000 Mbps */
//
#define ETH_PHY_SPEED_10_MBPS                              10
#define ETH_PHY_SPEED_100_MBPS                             100
#define ETH_PHY_SPEED_1000_MBPS                            1000

#define ETH_PHY_SPEED_TO_MBPS(speed)                       (((speed) == ETH_PHY_REG_17_0_SPEED_1000_MBPS) ? ETH_PHY_SPEED_1000_MBPS : \
                                                            ((speed) == ETH_PHY_REG_17_0_SPEED_100_MBPS)  ? ETH_PHY_SPEED_100_MBPS  : \
                                                            ((speed) == ETH_PHY_REG_17_0_SPEED_10_MBPS)   ? ETH_PHY_SPEED_10_MBPS   : 0 )


// Message types
//     NOTE:
//         HTON = Host TO Node  (ie a packet sent from the host to the node)
//         NTOH = Node TO Host  (ie a packet sent from the node to the host)
//
#define PKT_TYPE_TRIGGER                                   0
#define PKT_TYPE_HTON_MSG                                  1
#define PKT_TYPE_NTOH_MSG                                  2
#define PKT_TPYE_NTOH_MSG_ASYNC                            3

// Transport status types
#define LINK_READY                                         0
#define LINK_NOT_READY                                    -1

// Transport header flags (16 bits)
#define TRANSPORT_HDR_ROBUST_FLAG                          0x0001
#define TRANSPORT_HDR_NODE_NOT_READY_FLAG                  0x8000

// Transport header dest_id values
#define TRANSPORT_BROADCAST_DEST_ID                        0xFFFF



/*********************** Global Structure Definitions ************************/

// Transport header
//     NOTE:  This conforms to the Transport Header Wire Format:
//            http://warpproject.org/trac/wiki/WARPLab/Reference/Architecture/WireFormat
//
typedef struct {
    u16                      dest_id;                      // Destination ID
    u16                      src_id;                       // Source ID
    u8                       reserved;                     // Reserved
    u8                       pkt_type;                     // Packet Type (see Message types above)
    u16                      length;                       // Length of the Packet
    u16                      seq_num;                      // Sequence Number
    u16                      flags;                        // Transport flags
} transport_header;


// Transport info structure for Tag parameter information
//
//     NOTE:  This structure has to have the same fields in the same order as the Transp    rt Parameters
//         defined above.  Th    s structure will be used a     storage for the     ag Parameter values.
//
typedef struct {
    u32                      type;                         // Transport Type
    u32                      hw_addr[2];                   // HW Address (big endian as 2 u32 values with 16 bit padding)
    u32                      ip_addr;                      // IP Address (big endian)
    u32                      group_id;                     // Group ID
    u32                      unicast_port;                 // Unicast port
    u32                      broadcast_port;               // Broadcast port

    //
    // ADD NEW TAG PARAMETERS HERE
    //
    //     NOTE:  The #defines above, both the field name and the field length, must be adjusted in order
    //         for the new Tag Parameter to be populated.
    //
    //



    //
    // END ADD NEW TAG PARAMETERS HERE
    //

} transport_info;


// Ethernet Device Information
//
//     NOTE:  This structure exists so that differences between Ethernet devices can be consolidated
//
typedef struct {
    u32                      node_id;                      // Node ID (Only bits [15:0] are valid)
                                                           //     NOTE:  This is replicated from node_id in wlan_exp_node_info
    u32                      initialized;                  // Ethernet device initialized
    u32                      default_speed;                // Default Ethernet speed
    u32                      max_pkt_words;                // Largest supported packet size
    u32                      phy_addr;                     // Address of the Ethernet PHY

    int                      socket_unicast;               // Unicast receive socket index
    int                      socket_broadcast;             // Broadcast receive socket index
    int                      socket_async;                 // Asynchronous send socket index
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
    struct sockaddr          async_sockaddr;               // Address structure for Asynchronous send socket
#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP
    cmd_resp                 async_cmd_resp;               // Command / Response structure for Asynchronous send socket

    transport_info           info;                         // Transport info structure

} transport_eth_dev_info;


/*************************** Function Prototypes *****************************/
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

// Transport functions
int  transport_init(u32 eth_dev_num, void * node_info, u8 * ip_addr, u8 * hw_addr, u16 unicast_port, u16 broadcast_port);

int  transport_set_process_hton_msg_callback(void(*handler));
int  process_transport_cmd(int socket_index, void* from, cmd_resp* command, cmd_resp* response, u32 max_resp_len);

void transport_poll(u32 eth_dev_num);
void transport_receive(u32 eth_dev_num, int socket_index, struct sockaddr * from, warp_ip_udp_buffer * recv_buffer, warp_ip_udp_buffer * send_buffer);
void transport_send(int socket_index, struct sockaddr* to, warp_ip_udp_buffer** buffers, u32 num_buffers);
void transport_send_async(u32 eth_dev_num, u8 * payload, u32 length);
void transport_close(u32 eth_dev_num);

int  transport_set_hw_addr(u32 eth_dev_num, u8* hw_addr);
int  transport_get_hw_addr(u32 eth_dev_num, u8* hw_addr);
int  transport_set_ip_addr(u32 eth_dev_num, u8* ip_addr);
int  transport_get_ip_addr(u32 eth_dev_num, u8* ip_addr);

void transport_reset_max_pkt_length(u32 eth_dev_num);

int  transport_config_sockets(u32 eth_dev_num, u32 unicast_port, u32 broadcast_port, u8 verbose);
int  transport_config_socket(u32 eth_dev_num, int* socket_index, u32 udp_port);

int  transport_link_status(u32 eth_dev_num);
u32  transport_update_link_speed(u32 eth_dev_num, u32 wait_for_negotiation);
u16  transport_get_ethernet_status(u32 eth_dev_num);

int  transport_get_parameters(u32 eth_dev_num, u32* buffer, u32 max_resp_len, u8 network);

#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP

#endif /* TRANSPORT_H_ */
