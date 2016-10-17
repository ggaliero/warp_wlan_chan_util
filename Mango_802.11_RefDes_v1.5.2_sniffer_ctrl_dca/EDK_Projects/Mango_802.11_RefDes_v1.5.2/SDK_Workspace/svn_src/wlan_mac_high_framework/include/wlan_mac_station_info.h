/** @file wlan_mac_station_info.c
 *  @brief Station Information Metadata Subsystem
 *
 *  This contains code tracking metadata about stations.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_STATION_INFO_H_
#define WLAN_MAC_STATION_INFO_H_

/***************************** Include Files *********************************/

 #include "wlan_mac_high_sw_config.h"

#include "wlan_mac_high.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_common.h"
#include "wlan_mac_pkt_buf_util.h"


/*************************** Constant Definitions ****************************/


#define ADD_STATION_INFO_ANY_ID                            0                                  ///< Special argument to function that adds station_info structs

//-----------------------------------------------
// Timeout used to remove inactive BSS infos
//     - Part of bss_info_timestamp_check()
//
#define STATION_INFO_TIMEOUT_USEC                           600000000

/********************************************************************
 * @brief Tx/Rx Counts Sub-structure
 *
 * This struct contains counts about the communications link. It is intended to
 * be instantiated multiple times in the broader txrx_counts_t struct so that
 * different packet types can be individually tracked.
 *
 ********************************************************************/
typedef struct{
    u64        rx_num_bytes;                ///< # of successfully received bytes (de-duplicated)
    u64        rx_num_bytes_total;          ///< # of successfully received bytes (including duplicates)
    u64        tx_num_bytes_success;        ///< # of successfully transmitted bytes (high-level MPDUs)
    u64        tx_num_bytes_total;          ///< Total # of transmitted bytes (high-level MPDUs)
    u32        rx_num_packets;              ///< # of successfully received packets (de-duplicated)
    u32        rx_num_packets_total;        ///< # of successfully received packets (including duplicates)
    u32        tx_num_packets_success;      ///< # of successfully transmitted packets (high-level MPDUs)
    u32        tx_num_packets_total;        ///< Total # of transmitted packets (high-level MPDUs)
    u64        tx_num_attempts;             ///< # of low-level attempts (including retransmissions)
} txrx_counts_sub_t;
CASSERT(sizeof(txrx_counts_sub_t) == 56,txrx_counts_sub_alignment_check);


/********************************************************************
 * @brief Station Counts Structure
 *
 * This struct contains counts about the communications link.  Additionally,
 * counting different parameters can be decoupled from station_info structs
 * entirely to enable promiscuous counts about unassociated devices seen in
 * the network.
 *
 ********************************************************************/
#define STATION_TXRX_COUNTS_COMMON_FIELDS                                                                  						\
		txrx_counts_sub_t   data;                          /* Counts about data types	*/							\
		 /*----- 8-byte boundary ------*/																				\
		txrx_counts_sub_t   mgmt;                          /* Counts about management types */						\
		 /*----- 8-byte boundary ------*/																				\


typedef struct{
	STATION_TXRX_COUNTS_COMMON_FIELDS
} station_txrx_counts_t;

CASSERT(sizeof(station_txrx_counts_t) == 112, station_txrx_counts_alignment_check);


/********************************************************************
 * @brief Rate Selection Information
 *
 * This structure contains information about the rate selection scheme.
 *
 ********************************************************************/
typedef struct{
    u16 rate_selection_scheme;
    u8	reserved[6];
} rate_selection_info_t;

#define RATE_SELECTION_SCHEME_STATIC                       0

/********************************************************************
 * @brief Station Information Structure
 *
 * This struct contains information about associated stations (or, in the
 * case of a station, information about the associated access point).
 *
 * NOTE:  The reason that the reference design uses a #define for fields in
 *     two different structs is so that fields that must be in two different
 *     structs stay in sync and so there is not another level of indirection
 *     by using nested structs.
 *
 ********************************************************************/
#define STATION_INFO_HOSTNAME_MAXLEN                       19

#define STATION_INFO_COMMON_FIELDS                                                                                          \
        u8                 addr[MAC_ADDR_LEN];                         /* HW Address */                                     \
        u16                ID;                                         /* Identification Index for this station */          \
        char               hostname[STATION_INFO_HOSTNAME_MAXLEN+1];   /* Hostname from DHCP requests */                    \
        u32                flags;                                      /* 1-bit flags */                                    \
        u64                latest_rx_timestamp;               		   /* Timestamp of most recent reception */   		    \
		u64                latest_txrx_timestamp;               	   /* Timestamp of most recent reception or transmission */    \
        u16                latest_rx_seq;                              /* Sequence number of the last MPDU reception */     \
        u8                 reserved0[6];                                                                                    \
        tx_params_t        tx;                                         /* Transmission Parameters Structure */


typedef struct{
    STATION_INFO_COMMON_FIELDS
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
    station_txrx_counts_t		txrx_counts;                        			/* Tx/Rx Counts */
#endif
    rate_selection_info_t		rate_info;

} station_info_t;
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
CASSERT(sizeof(station_info_t) == 184, station_info_alignment_check);
#else
CASSERT(sizeof(station_info_t) == 72, station_info_alignment_check);
#endif

#define STATION_INFO_FLAG_KEEP                             0x00000001			   ///< Prevent MAC High Framework from deleting this station_infO
#define STATION_INFO_FLAG_DISABLE_ASSOC_CHECK              0x00000002              ///< Mask for flag in station_info -- disable association check
#define STATION_INFO_FLAG_DOZE                             0x00000004              ///< Mask to sleeping stations (if STA supports PS)
#define STATION_INFO_FLAG_HT_CAPABLE                       0x00000008              ///< Station is capable of HT Tx and Rx

#define RX_PROCESS_COUNTS_OPTION_FLAG_IS_DUPLICATE		   0x00000001

#define STATION_INFO_PRINT_OPTION_FLAG_INCLUDE_COUNTS	   0x00000001

/*************************** Function Prototypes *****************************/

void             station_info_init(u8 dram_present);
void             station_info_init_finish();

dl_entry* 		 station_info_checkout();
void             station_info_checkin(dl_entry* entry);

inline station_info_t*  station_info_tx_process(void* pkt_buf_addr);
inline station_info_t*	station_info_rx_process(void* pkt_buf_addr);
#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
void   					station_info_rx_process_counts(void* pkt_buf_addr, station_info_t* station_info, u32 option_flags);
#endif

void             station_info_print(dl_list* list, u32 option_flags);

#if WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
void             txrx_counts_zero_all();
void 			 station_info_clear_txrx_counts(station_txrx_counts_t* txrx_counts);
#endif

void             station_info_timestamp_check();

station_info_t*  station_info_create(u8* addr);
void             station_info_reset_all();
void 			 station_info_clear(station_info_t* station_info);

inline dl_list*  station_info_get_list();

dl_entry*        station_info_find_by_id(u32 id, dl_list* list);
dl_entry*        station_info_find_by_addr(u8* addr, dl_list* list);

station_info_t*  station_info_add(dl_list* app_station_info_list, u8* addr, u16 requested_ID, tx_params_t* tx_params, u8 ht_capable);
int              station_info_remove(dl_list* app_station_info_list, u8* addr);

u8               station_info_is_member(dl_list* app_station_info_list, station_info_t* station_info);
int              station_info_update_rate(station_info_t* station_info, u8 mcs, u8 phy_mode);

#endif
