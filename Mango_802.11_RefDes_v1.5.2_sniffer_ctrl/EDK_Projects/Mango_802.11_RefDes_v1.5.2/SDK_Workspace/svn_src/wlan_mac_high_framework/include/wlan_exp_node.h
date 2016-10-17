/** @file wlan_exp_node.h
 *  @brief Experiment Framework
 *
 *  This contains the code for WLAN Experimental Framework.
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

#include "wlan_exp_common.h"
#include "wlan_exp_transport.h"

#include "wlan_mac_bss_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_mac_high.h"

/*************************** Constant Definitions ****************************/
#ifndef WLAN_EXP_NODE_H_
#define WLAN_EXP_NODE_H_



// ****************************************************************************
// Define Node Commands
//
// NOTE:  All Command IDs (CMDID_*) must be a 24 bit unique number
//

//-----------------------------------------------
// Node Commands
//
#define CMDID_NODE_TYPE                                    0x000000
#define CMDID_NODE_INFO                                    0x000001
#define CMDID_NODE_IDENTIFY                                0x000002

#define CMD_PARAM_NODE_IDENTIFY_ALL                        0xFFFFFFFF

#define CMDID_NODE_CONFIG_SETUP                            0x000003
#define CMDID_NODE_CONFIG_RESET                            0x000004

#define CMD_PARAM_NODE_CONFIG_RESET_ALL                    0xFFFFFFFF

#define CMDID_NODE_TEMPERATURE                             0x000005


//-----------------------------------------------
// WLAN Exp Node Commands
//
#define CMDID_NODE_RESET_STATE                             0x001000
#define CMDID_NODE_CONFIGURE                               0x001001
#define CMDID_NODE_CONFIG_BSS                              0x001002
#define CMDID_NODE_TIME                                    0x001010
#define CMDID_NODE_CHANNEL                                 0x001011
#define CMDID_NODE_TX_POWER                                0x001012
#define CMDID_NODE_TX_RATE                                 0x001013
#define CMDID_NODE_TX_ANT_MODE                             0x001014
#define CMDID_NODE_RX_ANT_MODE                             0x001015
#define CMDID_NODE_LOW_TO_HIGH_FILTER                      0x001016
#define CMDID_NODE_RANDOM_SEED                             0x001017
#define CMDID_NODE_WLAN_MAC_ADDR                           0x001018
#define CMDID_NODE_LOW_PARAM                               0x001020

#define CMD_PARAM_WRITE_VAL                                0x00000000
#define CMD_PARAM_READ_VAL                                 0x00000001
#define CMD_PARAM_WRITE_DEFAULT_VAL                        0x00000002
#define CMD_PARAM_READ_DEFAULT_VAL                         0x00000004
#define CMD_PARAM_RSVD                                     0xFFFFFFFF

#define CMD_PARAM_SUCCESS                                  0x00000000
#define CMD_PARAM_WARNING                                  0xF0000000
#define CMD_PARAM_ERROR                                    0xFF000000

#define CMD_PARAM_UNICAST_VAL                              0x00000000
#define CMD_PARAM_MULTICAST_DATA_VAL                       0x00000001
#define CMD_PARAM_MULTICAST_MGMT_VAL                       0x00000002

#define CMD_PARAM_NODE_CONFIG_ALL                          0xFFFFFFFF

#define CMD_PARAM_NODE_RESET_FLAG_LOG                      0x00000001
#define CMD_PARAM_NODE_RESET_FLAG_TXRX_COUNTS              0x00000002
#define CMD_PARAM_NODE_RESET_FLAG_LTG                      0x00000004
#define CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE            0x00000008
#define CMD_PARAM_NODE_RESET_FLAG_BSS                      0x00000010
#define CMD_PARAM_NODE_RESET_FLAG_NETWORK_LIST             0x00000020

#define CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE             0x00000001
#define CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE      0x00000002
#define CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL     0x80000000

#define CMD_PARAM_NODE_TIME_ADD_TO_LOG_VAL                 0x00000002
#define CMD_PARAM_NODE_TIME_RSVD_VAL                       0xFFFFFFFF
#define CMD_PARAM_NODE_TIME_RSVD_VAL_64                    0xFFFFFFFFFFFFFFFF

#define CMD_PARAM_NODE_TX_POWER_LOW                        0x00000010
#define CMD_PARAM_NODE_TX_POWER_ALL                        0x00000020

#define CMD_PARAM_NODE_TX_ANT_ALL                          0x00000010

#define CMD_PARAM_RSVD_CHANNEL                             0x00000000
#define CMD_PARAM_RSVD_MAC_ADDR                            0x00000000

#define CMD_PARAM_RANDOM_SEED_VALID                        0x00000001
#define CMD_PARAM_RANDOM_SEED_RSVD                         0xFFFFFFFF


//-----------------------------------------------
// LTG Commands
//
#define CMDID_LTG_CONFIG                                   0x002000
#define CMDID_LTG_START                                    0x002001
#define CMDID_LTG_STOP                                     0x002002
#define CMDID_LTG_REMOVE                                   0x002003
#define CMDID_LTG_STATUS                                   0x002004

#define CMD_PARAM_LTG_ERROR                                0x000001

#define CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART                0x00000001

#define CMD_PARAM_LTG_ALL_LTGS                             LTG_ID_INVALID

#define CMD_PARAM_LTG_RUNNING                              0x00000001
#define CMD_PARAM_LTG_STOPPED                              0x00000000


//-----------------------------------------------
// Log Commands
//
#define CMDID_LOG_CONFIG                                   0x003000
#define CMDID_LOG_GET_STATUS                               0x003001
#define CMDID_LOG_GET_CAPACITY                             0x003002
#define CMDID_LOG_GET_ENTRIES                              0x003003
#define CMDID_LOG_ADD_EXP_INFO_ENTRY                       0x003004

#define CMDID_LOG_ENABLE_ENTRY                             0x003006

#define CMD_PARAM_LOG_GET_ALL_ENTRIES                      0xFFFFFFFF

#define CMD_PARAM_LOG_CONFIG_FLAG_LOGGING                  0x00000001
#define CMD_PARAM_LOG_CONFIG_FLAG_WRAP                     0x00000002
#define CMD_PARAM_LOG_CONFIG_FLAG_PAYLOADS                 0x00000004
#define CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU                0x00000008
#define CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL                0x00000010


//-----------------------------------------------
// Counts Commands
//
#define CMDID_COUNTS_GET_TXRX                              0x004001

#define CMD_PARAM_COUNTS_CONFIG_FLAG_PROMISC               0x00000001
#define CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE             0x80000000


//-----------------------------------------------
// Queue Commands
//
#define CMDID_QUEUE_TX_DATA_PURGE_ALL                      0x005000


//-----------------------------------------------
// Scan Commands
//
#define CMDID_NODE_SCAN_PARAM                              0x006000
#define CMDID_NODE_SCAN                                    0x006001

#define CMD_PARAM_NODE_SCAN_ENABLE                         0x00000001
#define CMD_PARAM_NODE_SCAN_DISABLE                        0x00000000


//-----------------------------------------------
// Association Commands
//
#define CMDID_NODE_GET_BSS_MEMBERS                         0x007001
#define CMDID_NODE_GET_BSS_INFO                            0x007002
#define CMDID_NODE_GET_STATION_INFO_LIST                   0x007003

#define CMDID_NODE_DISASSOCIATE                            0x007010
#define CMDID_NODE_ASSOCIATE                               0x007011


//-----------------------------------------------
// Development Commands
//
#define CMDID_DEV_MEM_HIGH                                 0xFFF000
#define CMDID_DEV_MEM_LOW                                  0xFFF001
#define CMDID_DEV_EEPROM                                   0xFFF002


// ****************************************************************************
// WLAN Exp Defines
//
// TODO - What are these? If they are needed, we should at least remove "AID" and make them
// more generic.
#define WLAN_EXP_AID_NONE                                  0x00000000
#define WLAN_EXP_AID_ALL                                   0xFFFFFFFF
#define WLAN_EXP_AID_ME                                    0xFFFFFFFE
#define WLAN_EXP_AID_DEFAULT                               0x00000001



// ****************************************************************************
// Define Node Tag Parameters
//
//     NOTE:  To add another parameter, add the define before "NODE_PARAM_MAX_PARAMETER"
//         and then change the value of "NODE_PARAM_MAX_PARAMETER" to be the largest value
//         in the list so it is easy to iterate over all parameters
//
#define NODE_PARAM_NODE_TYPE                               0
#define NODE_PARAM_NODE_ID                                 1
#define NODE_PARAM_HW_GENERATION                           2
#define NODE_PARAM_SERIAL_NUM                              3
#define NODE_PARAM_FPGA_DNA                                4
#define NODE_PARAM_WLAN_EXP_VERSION                        5
#define NODE_PARAM_WLAN_SCHEDULER_RESOLUTION               6
#define NODE_PARAM_WLAN_MAC_ADDR                           7
#define NODE_PARAM_WLAN_MAX_TX_POWER_DBM                   8
#define NODE_PARAM_WLAN_MIN_TX_POWER_DBM                   9

//
// ADD NEW TAG PARAMETERS HERE
//


//
// END ADD NEW TAG PARAMETERS HERE
//
//     NOTE:  Make sure that NODE_PARAM_MAX_PARAMETER is adjusted accordingly
//

#define NODE_PARAM_MAX_PARAMETER                           10


// ****************************************************************************
// Define Node Tag Parameter Field Lengths
//
//     NOTE:  Tag Parameters must be 32 bit aligned.  The array below represents the number
//         of 32 bit unsigned integers required for each field.  If another field is added
//         to the Tag Parameters, then the NODE_PARAM_FIELD_LENGTHS array must be updated
//         to represent the appropriate length of each new field.
//
#define NODE_PARAM_FIELD_LENGTHS                           {1, 1, 1, 1, 2, 1, 1, 2, 1, 1}



/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Node Info Structure for Tag Parameter Information
//
//     NOTE:  This structure has to have the same fields in the same order as the Node Tag Parameters
//         defined above (except for the eth_dev field).  This structure will be used as storage for
//         the Tag Parameter values.
//
typedef struct {

    u32                      node_type;                    // Type of node
    u32                      node_id;                      // Node ID (Only bits [15:0] are valid)
    u32                      hw_generation;                // Node Hardware generation

    u32                      serial_number;                // Node serial number
    u32                      fpga_dna[FPGA_DNA_LEN];       // Node FPGA DNA number

    u32                      wlan_exp_version;             // WLAN Exp Version
    u32                      wlan_scheduler_resolution;    // WLAN Exp - Minimum Scheduler resolution
    u32                      wlan_hw_addr[2];              // WLAN Exp - Wireless MAC address (ie ETH A MAC address)

    u32                      wlan_max_tx_power_dbm;        // WLAN maximum transmit power
    u32                      wlan_min_tx_power_dbm;        // WLAN minimum transmit power

    //
    // ADD NEW TAG PARAMETERS HERE
    //
    //     NOTE:  The #defines above, both the field name and the field length, must be adjusted in order
    //         for the new Tag Parameter to be populated.    //
    //



    //
    // END ADD NEW TAG PARAMETERS HERE
    //

    transport_eth_dev_info  * eth_dev;                     // Information on Ethernet device

} wlan_exp_node_info;



//-----------------------------------------------
// wlan_exp Station Info
//
//     Only used to communicate with WLAN Exp Host.
//
typedef struct __attribute__((__packed__)){
    // All station_info_t common fields
    STATION_INFO_COMMON_FIELDS
} wlan_exp_station_info_t;

CASSERT(sizeof(wlan_exp_station_info_t) == 64, wlan_exp_station_info_alignment_check);


#define STATION_INFO_ENTRY_NO_CHANGE             0
#define STATION_INFO_ENTRY_ZERO_AID              1



//-----------------------------------------------
// wlan_exp Basic Service Set (BSS) Info
//
//     Only used to communicate with WLAN Exp Host.
//
typedef struct __attribute__((__packed__)){
    // All bss_info_t common fields
    BSS_INFO_COMMON_FIELDS
} wlan_exp_bss_info_t;

CASSERT(sizeof(wlan_exp_bss_info_t) == 56, wlan_exp_bss_info_alignment_check);



//-----------------------------------------------
// wlan_exp Tx/Rx Counts
//
//     Only used to communicate with WLAN Exp Host.
//

typedef struct{
	STATION_TXRX_COUNTS_COMMON_FIELDS
} wlan_exp_station_txrx_counts_lite_t;

typedef struct{
    u64                 				timestamp;                 // Timestamp of the log entry
    u8									addr[6];				   // MAC address associated with this counts struct
    u16									reserved;
    wlan_exp_station_txrx_counts_lite_t counts;                    // Framework's counts struct
} wlan_exp_station_txrx_counts_t;
CASSERT(sizeof(wlan_exp_station_txrx_counts_t) == 128, wlan_exp_station_txrx_counts_alignment_check);


/*************************** Function Prototypes *****************************/
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

// Initialization Commands
int  wlan_exp_node_init           (u32 serial_number, u32 *fpga_dna, u32 eth_dev_num, u8 *wlan_exp_hw_addr, u8 *wlan_hw_addr);
void wlan_exp_node_set_type_design(u32 type_design);
void wlan_exp_node_set_type_high  (u32 type_high);
void wlan_exp_node_set_type_low   (u32 type_low);

// Callbacks
void wlan_exp_reset_all_callbacks                     ();
void wlan_exp_set_process_node_cmd_callback           (void(*callback)());
void wlan_exp_set_purge_all_data_tx_queue_callback    (void(*callback)());
void wlan_exp_set_tx_cmd_add_association_callback     (void(*callback)());
void wlan_exp_set_process_user_cmd_callback           (void(*callback)());
void wlan_exp_set_beacon_ts_update_mode_callback      (void(*callback)());
void wlan_exp_set_process_config_bss_callback         (void(*callback)());
void wlan_exp_set_active_bss_info_getter_callback	  (void(*callback)());


// WLAN Exp commands
u32  wlan_exp_get_id_in_associated_stations(u8 * mac_addr);
u32  wlan_exp_get_id_in_counts(u8 * mac_addr);
u32  wlan_exp_get_id_in_bss_info(u8 * bssid);

// Node commands
int  node_get_parameters(u32 * buffer, u32 max_resp_len, u8 transmit);
int  node_get_parameter_values    (u32 * buffer, u32 max_resp_len);

void node_info_set_wlan_hw_addr   (u8 * hw_addr  );
void node_info_set_max_assn       (u32 max_assn  );
void node_info_set_event_log_size (u32 log_size  );
void node_info_set_max_counts     (u32 max_counts);

u32  node_get_node_id             (void);
u32  node_get_serial_number       (void);

#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP


#endif /* WLAN_EXP_NODE_H_ */
