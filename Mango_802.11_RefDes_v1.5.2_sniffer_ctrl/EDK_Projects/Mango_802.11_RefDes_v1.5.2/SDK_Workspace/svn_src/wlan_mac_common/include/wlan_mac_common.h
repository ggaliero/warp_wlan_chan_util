/** @file wlan_mac_common.h
 *  @brief Common Definitions
 *
 *  This contains common definitions of required by both the upper and
 *  lower-level CPUs.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "xil_io.h"

#include "wlan_mac_802_11_defs.h"


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_COMMON_H_
#define WLAN_MAC_COMMON_H_


//-----------------------------------------------
// CPU defines
//
#ifdef XPAR_MB_HIGH_FREQ
#define WLAN_COMPILE_FOR_CPU_HIGH                          1
#define WLAN_COMPILE_FOR_CPU_LOW                           0
#endif

#ifdef XPAR_MB_LOW_FREQ
#define WLAN_COMPILE_FOR_CPU_HIGH                          0
#define WLAN_COMPILE_FOR_CPU_LOW                           1
#endif


//-----------------------------------------------
// Common function defines
//
#define max(a, b)                                       (((a) > (b)) ? (a) : (b))
#define min(a, b)                                       (((a) < (b)) ? (a) : (b))

#define abs_64(a)                                       (((a) < 0) ? (-1 * a) : (a))

#define sat_add16(a, b)                                 (((a) > (0xFFFF - (b))) ? 0xFFFF : ((a) + (b)))
#define sat_add32(a, b)                                 (((a) > (0xFFFFFFFF - (b))) ? 0xFFFFFFFF : ((a) + (b)))

#define sat_sub(a, b)                                   (((a) > (b)) ? ((a) - (b)) : 0)

#define wlan_addr_eq(addr1, addr2)                        (memcmp((void*)(addr1), (void*)(addr2), 6)==0)
#define wlan_addr_mcast(addr)                          ((((u8*)(addr))[0] & 1) == 1)


//-----------------------------------------------
// Compile-time assertion defines
//
#define CASSERT(predicate, file)                           _impl_CASSERT_LINE(predicate, __LINE__, file)

#define _impl_PASTE(a, b)                                  a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
    typedef char _impl_PASTE(assertion_failed_##file##_, line)[2*!!(predicate)-1];


//-----------------------------------------------
// Level Print function defines
//
#define PRINT_LEVEL                                        PL_ERROR

#define PL_NONE                                            0
#define PL_ERROR                                           1
#define PL_WARNING                                         2
#define PL_VERBOSE                                         3

#define wlan_printf(severity, format, args...) \
    do { \
        if (PRINT_LEVEL >= severity){ xil_printf(format, ##args); } \
    } while(0)


//-----------------------------------------------
// PHY defines
//
#define WLAN_PHY_FCS_NBYTES                                4

#define PHY_MODE_DSSS                                      0x0
#define PHY_MODE_NONHT                                     0x1       // 11a OFDM
#define PHY_MODE_HTMF                                      0x2       // 11n OFDM, HT mixed format


//-----------------------------------------------
// Unique sequence number defines
//
#define UNIQUE_SEQ_INVALID                                 0xFFFFFFFFFFFFFFFF


//-----------------------------------------------
// WLAN defines
//

// Reference: http://standards.ieee.org/develop/regauth/tut/macgrp.pdf
#define MAC_ADDR_MSB_MASK_MCAST                            0x01
#define MAC_ADDR_MSB_MASK_LOCAL                            0x02


//-----------------------------------------------
// CPU Status defines
//
#define CPU_STATUS_INITIALIZED                             0x00000001
#define CPU_STATUS_EXCEPTION                               0x80000000


//-----------------------------------------------
// Antenna mode defines
//     NOTE:  These values are enumerated and are *not* written to PHY registers
//
#define RX_ANTMODE_SISO_ANTA                               0x0
#define RX_ANTMODE_SISO_ANTB                               0x1
#define RX_ANTMODE_SISO_ANTC                               0x2
#define RX_ANTMODE_SISO_ANTD                               0x3
#define RX_ANTMODE_SISO_SELDIV_2ANT                        0x4
#define RX_ANTMODE_SISO_SELDIV_4ANT                        0x5

#define TX_ANTMODE_SISO_ANTA                               0x10
#define TX_ANTMODE_SISO_ANTB                               0x20
#define TX_ANTMODE_SISO_ANTC                               0x30
#define TX_ANTMODE_SISO_ANTD                               0x40


//-----------------------------------------------
// Receive filter defines
//     NOTE:  These filters allow the user to select the types of received packets to process
//
#define RX_FILTER_FCS_GOOD                                 0x1000    ///< Pass only packets with good checksum result
#define RX_FILTER_FCS_ALL                                  0x2000    ///< Pass packets with any checksum result
#define RX_FILTER_FCS_MASK                                 0xF000
#define RX_FILTER_FCS_NOCHANGE                             RX_FILTER_FCS_MASK

#define RX_FILTER_HDR_ADDR_MATCH_MPDU                      0x0001    ///< Pass any unicast-to-me or multicast data or management packet
#define RX_FILTER_HDR_ALL_MPDU                             0x0002    ///< Pass any data or management packet (no address filter)
#define RX_FILTER_HDR_ALL                                  0x0003    ///< Pass any packet (no type or address filters)
#define RX_FILTER_HDR_MASK                                 0x0FFF
#define RX_FILTER_HDR_NOCHANGE                             RX_FILTER_HDR_MASK


//-----------------------------------------------
// Error defines
//     Currently, the framework supports error values of 0 - 0xF.  These will
//     be displayed on the hex display as Ex, where x is the error value.
//
#define WLAN_ERROR_CODE_RIGHT_SHIFT                        0
#define WLAN_ERROR_CODE_INSUFFICIENT_BD_SIZE               1
#define WLAN_ERROR_CODE_DRAM_NOT_PRESENT                   2
#define WLAN_ERROR_CODE_CPU_LOW_TX_MUTEX                   3
#define WLAN_ERROR_CODE_CPU_LOW_RX_MUTEX                   4


#define WLAN_ERROR_CPU_STOP                                0x80000000



//-----------------------------------------------
// Debug / Monitor defines
//
#define ISR_PERF_MON_GPIO_MASK                             0x01
// #define _ISR_PERF_MON_EN_                                         ///< ISR Performance Monitor Toggle


/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// Generic function pointer
//
typedef int (*function_ptr_t)();


//-----------------------------------------------
// PHY Bandwidth Configuration
//
typedef enum {
    PHY_10M   = 10, 
    PHY_20M   = 20, 
    PHY_40M   = 40
} phy_samp_rate_t;


//-----------------------------------------------
// LLC Header
//
typedef struct{
    u8   dsap;
    u8   ssap;
    u8   control_field;
    u8   org_code[3];
    u16  type;
} llc_header_t;


//-----------------------------------------------
// LTG Payload Contents
//
typedef struct {
    llc_header_t   llc_hdr;
    u64            unique_seq;
    u32            ltg_id;
} ltg_packet_id_t;


//-----------------------------------------------
// Beacon Tx/Rx Configuration Struct
//
typedef enum {
    NEVER_UPDATE,
    ALWAYS_UPDATE,
    FUTURE_ONLY_UPDATE,
} mactime_update_mode_t;

typedef enum {
    NO_BEACON_TX,
    AP_BEACON_TX,
    IBSS_BEACON_TX,
} beacon_tx_mode_t;

typedef struct{
    // Beacon Rx Configuration Parameters
    mactime_update_mode_t    ts_update_mode;               // Determines how MAC time is updated on reception of beacons
    u8                       bssid_match[MAC_ADDR_LEN];    // BSSID of current association for Rx matching

    // Beacon Tx Configuration Parameters
    u8                       beacon_template_pkt_buf;      // Packet Buffer that contains the the beacon template to transmit
    beacon_tx_mode_t         beacon_tx_mode;               // Tx Beacon Mode
    u32                      beacon_interval_tu;           // Beacon Interval (in TU)
} beacon_txrx_configure_t;


//-----------------------------------------------
// Hardware information struct to share data between the
//   low and high CPUs

#define WLAN_MAC_FPGA_DNA_LEN         2


typedef struct {
    u32  serial_number;
    u32  fpga_dna[WLAN_MAC_FPGA_DNA_LEN];
    u8   hw_addr_wlan[MAC_ADDR_LEN];
    u8   hw_addr_wlan_exp[MAC_ADDR_LEN];
} wlan_mac_hw_info_t;


/*************************** Function Prototypes *****************************/

int                     wlan_null_callback(void * param);

int                     wlan_verify_channel(u32 channel);

void                    cpu_error_halt(u32 error_code);

void                    init_mac_hw_info();
wlan_mac_hw_info_t    * get_mac_hw_info();
u8                    * get_mac_hw_addr_wlan();
u8                    * get_mac_hw_addr_wlan_exp();

#endif   // END WLAN_MAC_COMMON_H_
