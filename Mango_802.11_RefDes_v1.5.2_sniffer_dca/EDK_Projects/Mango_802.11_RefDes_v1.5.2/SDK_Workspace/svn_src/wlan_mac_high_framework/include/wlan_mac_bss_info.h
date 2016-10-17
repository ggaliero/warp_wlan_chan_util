/** @file wlan_mac_bss_info.h
 *  @brief BSS Info Subsystem
 *
 *  This contains code tracking BSS information. It also contains code for managing
 *  the active scan state machine.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_BSS_INFO_H_
#define WLAN_MAC_BSS_INFO_H_

/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_common.h"


/*************************** Constant Definitions ****************************/

//-----------------------------------------------
// Timeout used to remove inactive BSS infos
//     - Part of bss_info_timestamp_check()
//
#define BSS_INFO_TIMEOUT_USEC                              600000000


//-----------------------------------------------
// Field size defines
//
#define NUM_BASIC_RATES_MAX                                10


//-----------------------------------------------
// BSS Beacon Interval defines
//
#define BSS_MICROSECONDS_IN_A_TU                           1024
#define BEACON_INTERVAL_NO_BEACON_TX                       0x0
#define BEACON_INTERVAL_UNKNOWN                            0xFFFF

//-----------------------------------------------
// BSS Capabilities defines
//     - Uses some values from wlan_mac_802_11_defs.h for to map BSS
//       capabilities to the capabilities transmitted in a beacon.
//
#define BSS_CAPABILITIES_BEACON_MASK                      (CAPABILITIES_ESS | CAPABILITIES_IBSS | CAPABILITIES_PRIVACY)

#define BSS_CAPABILITIES_ESS                               CAPABILITIES_ESS
#define BSS_CAPABILITIES_IBSS                              CAPABILITIES_IBSS
#define BSS_CAPABILITIES_HT_CAPABLE                        0x0004
#define BSS_CAPABILITIES_PRIVACY                           CAPABILITIES_PRIVACY


//-----------------------------------------------
// BSS Flags defines
//
#define BSS_FLAGS_KEEP                                     0x0001


//-----------------------------------------------
// BSS Configuration Bit Masks
//
#define BSS_FIELD_MASK_BSSID                               0x00000001
#define BSS_FIELD_MASK_CHAN                                0x00000002
#define BSS_FIELD_MASK_SSID                                0x00000004
#define BSS_FIELD_MASK_BEACON_INTERVAL                     0x00000008
#define BSS_FIELD_MASK_HT_CAPABLE                          0x00000010


//-----------------------------------------------
// configure_bss() return error flags
//
#define BSS_CONFIG_FAILURE_BSSID_INVALID                   0x00000001
#define BSS_CONFIG_FAILURE_BSSID_INSUFFICIENT_ARGUMENTS    0x00000002
#define BSS_CONFIG_FAILURE_CHANNEL_INVALID                 0x00000004
#define BSS_CONFIG_FAILURE_BEACON_INTERVAL_INVALID         0x00000008
#define BSS_CONFIG_FAILURE_HT_CAPABLE_INVALID              0x00000010


//-----------------------------------------------
// Define common BSS info fields
//   Note: These fields have been 32 bit aligned using padding bytes

#define BSS_INFO_COMMON_FIELDS                                                                               \
        u8              bssid[MAC_ADDR_LEN];               /* BSS ID - 48 bit HW address */                  \
        chan_spec_t     chan_spec;                         /* Channel Specification */                       \
        u64             latest_beacon_rx_time;	           /* Timestamp - Last interaction with BSS */       \
        char            ssid[SSID_LEN_MAX + 1];            /* SSID of the BSS - 33 bytes */                  \
        s8              latest_beacon_rx_power;            /* Last observed Rx Power (dBm) */                \
        u16             padding0;                                                                            \
        u16             capabilities;                      /* Supported capabilities */                      \
        u16             beacon_interval;                   /* Beacon interval - In time units of 1024 us */


/*********************** Global Structure Definitions ************************/

/********************************************************************
 * @brief Channel Type Enum
 *
 * This enum described the type of channel that is specified in the
 * chan_spec_t struct. The size of an enum is compiler dependent. Because
 * this enum will be used in structs whose contents must be aligned with
 * wlan_exp in Python, we use a compile time assertion to at least create
 * a compilation error if the size winds up being different than wlan_exp
 * expects.
 *
 ********************************************************************/
typedef enum __attribute__((__packed__)) {
	CHAN_TYPE_BW20 = 0,
	CHAN_TYPE_BW40_SEC_BELOW = 1,
	CHAN_TYPE_BW40_SEC_ABOVE = 2
} chan_type_t;

CASSERT(sizeof(chan_type_t) == 1, chan_type_t_alignment_check);



/********************************************************************
 * @brief Channel Specifications Struct
 *
 * This struct contains a primary channel number and a chan_type_t.
 * Together, this tuple of information can be used to calculate the
 * center frequency and bandwidth of the radio.
 ********************************************************************/
typedef struct __attribute__((__packed__)){
	u8             chan_pri;
	chan_type_t    chan_type;
} chan_spec_t;

CASSERT(sizeof(chan_spec_t) == 2, chan_spec_t_alignment_check);

/********************************************************************
 * @brief Basic Service Set (BSS) Information Structure
 *
 * This struct contains information about the basic service set for the node.
 ********************************************************************/
typedef struct{
    BSS_INFO_COMMON_FIELDS

    u32     flags;
    dl_list members;
} bss_info_t;

CASSERT(sizeof(bss_info_t) == 72, bss_info_t_alignment_check);



/********************************************************************
 * @brief BSS Configuration Structure
 *
 * This struct contains all the BSS info fields that can be modified.
 ********************************************************************/
typedef struct{
    u32            update_mask;                       /* Mask of fields that were updated */
    u8             bssid[MAC_ADDR_LEN];               /* BSS ID */
    u16            beacon_interval;                   /* Beacon interval - In time units of 1024 us */
    char           ssid[SSID_LEN_MAX + 1];            /* SSID of the BSS - 33 bytes */
    chan_spec_t    chan_spec;                         /* Channel Specification*/
    u8             ht_capable;                        /* Support HTMF Tx/Rx */
} bss_config_t;



/*************************** Function Prototypes *****************************/

void             bss_info_init(u8 dram_present);
void             bss_info_init_finish();

dl_entry       * bss_info_checkout();
void             bss_info_checkin(dl_entry* bsi);

inline void      bss_info_rx_process(void* pkt_buf_addr);

void             print_bss_info();
void             bss_info_timestamp_check();

dl_list        * wlan_mac_high_find_bss_info_SSID(char* ssid);
dl_entry       * wlan_mac_high_find_bss_info_BSSID(u8* bssid);

bss_info_t     * wlan_mac_high_create_bss_info(u8* bssid, char* ssid, u8 chan);
void             wlan_mac_high_reset_network_list();
void 			 wlan_mac_high_delete_bss_from_list(char* ssid);	//Gabri
void             wlan_mac_high_clear_bss_info(bss_info_t * info);

inline dl_list * wlan_mac_high_get_bss_info_list();

#endif
