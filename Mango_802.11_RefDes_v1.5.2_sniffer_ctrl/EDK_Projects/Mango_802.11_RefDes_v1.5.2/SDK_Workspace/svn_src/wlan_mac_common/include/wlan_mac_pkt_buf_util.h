/** @file wlan_mac_pkt_buf_util.h
 *  @brief Packet Buffer Definitions
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to use the packet buffers.
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

#include "wlan_mac_common.h"


/*************************** Constant Definitions ****************************/

#ifndef WLAN_MAC_PKT_BUF_UTIL_H_
#define WLAN_MAC_PKT_BUF_UTIL_H_


//-----------------------------------------------
// Hardware defines
//
#define PKT_BUF_MUTEX_DEVICE_ID                            XPAR_MUTEX_0_DEVICE_ID


// Base address of mutex for Tx/Rx packet buffers
#define PKT_BUF_MUTEX_TX_BASE                              0
#define PKT_BUF_MUTEX_RX_BASE                              16


// Mutex status values
#define PKT_BUF_MUTEX_SUCCESS                              0
#define PKT_BUF_MUTEX_FAIL_INVALID_BUF                    -1
#define PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED                 -2
#define PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER                 -3
#define	PKT_BUF_MUTEX_ALREADY_UNLOCKED				  	  -4


//-----------------------------------------------
// Packet buffer defines
//
#define NUM_TX_PKT_BUFS                                    8
#define NUM_RX_PKT_BUFS                                    8


// Packet buffer size (in bytes)
#define PKT_BUF_SIZE                                       4096


// Tx packet buffer assignments
//     The definitions for the MPDU packet buffers are not directly
//     used. Instead, the actual packet buffer values are used to
//     make iteration easier.
//
// Packet Buffers owned by CPU_HIGH at boot
#define TX_PKT_BUF_MPDU_1                                  0
#define TX_PKT_BUF_MPDU_2                                  1
#define TX_PKT_BUF_MPDU_3                                  2
// #define TX_PKT_BUF_                                        3
// #define TX_PKT_BUF_                                        4
#define TX_PKT_BUF_BEACON                                  5
// Packet Buffers owned by CPU_LOW at boot
#define TX_PKT_BUF_RTS                                     6
#define TX_PKT_BUF_ACK_CTS                                 7


// Tx / Rx packet buffer macros
//
// Packet buffer memory format:
//   - [(M -  1):       0] - Frame info structure (M is the size of the frame info for Tx / Rx)
//                           - See definitions below for rx_frame_info / tx_frame_info
//
//   - [(M + 15):       M] - PHY Header           (16 bytes)
//                           - 11a:
//                                 [ 2: 0] SIGNAL
//                                 [ 4: 3] SERVICE (must be 0)
//                                 [15: 5] Reserved (should be 0)
//                                 [ N:16] MAC payload - first header byte at [16]
//                           - 11n:
//                                 [ 2: 0] L-SIG (same format as 11a SIGNAL)
//                                 [ 8: 3] HT-SIG
//                                 [10: 9] SERVICE (must be 0)
//                                 [15:11] Reserved (should be 0)
//                                 [ N:16] MAC payload - first header byte at [16]
//
//   - [(M +  N):(M + 16)] - MAC payload          (N is the size of the MAC payload)
//                           - Standard 802.11 MAC payload
//
#define TX_PKT_BUF_TO_ADDR(n)                             (XPAR_PKT_BUFF_TX_BRAM_CTRL_S_AXI_BASEADDR + ((n) & 0x7) * PKT_BUF_SIZE)
#define RX_PKT_BUF_TO_ADDR(n)                             (XPAR_PKT_BUFF_RX_BRAM_CTRL_S_AXI_BASEADDR + ((n) & 0x7) * PKT_BUF_SIZE)

#define PHY_RX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(rx_frame_info_t))
#define PHY_TX_PKT_BUF_PHY_HDR_OFFSET                     (sizeof(tx_frame_info_t))

#define PHY_RX_PKT_BUF_PHY_HDR_SIZE                        0x10
#define PHY_TX_PKT_BUF_PHY_HDR_SIZE                        0x10

#define PHY_RX_PKT_BUF_MPDU_OFFSET                        (PHY_RX_PKT_BUF_PHY_HDR_SIZE + PHY_RX_PKT_BUF_PHY_HDR_OFFSET)
#define PHY_TX_PKT_BUF_MPDU_OFFSET                        (PHY_TX_PKT_BUF_PHY_HDR_SIZE + PHY_TX_PKT_BUF_PHY_HDR_OFFSET)



/*********************** Global Structure Definitions ************************/

//-----------------------------------------------
// TX parameters
//     - Be careful when modifying these structures, there are alignment concerns
//       for many of the structures that contain these structures.  In general,
//       tx_params_t should be 8-byte aligned.
//
typedef struct{
    u8                       mcs;                          ///< MCS index
    u8                       phy_mode;                     ///< PHY mode selection and flags
    u8                       antenna_mode;                 ///< Tx antenna selection
    s8                       power;                        ///< Tx power (in dBm)
} phy_tx_params_t;


typedef struct{
    u8                       flags;                        ///< Flags affecting waveform construction
    u8                       reserved[3];                  ///< Reserved for 32-bit alignment
} mac_tx_params_t;


typedef struct{
    phy_tx_params_t          phy;                          ///< PHY Tx params
    mac_tx_params_t          mac;                          ///< Lower-level MAC Tx params
} tx_params_t;


//-----------------------------------------------
// Packet buffer state
//
typedef enum __attribute__ ((__packed__)) {
   TX_PKT_BUF_UNINITIALIZED   = 0,
   TX_PKT_BUF_HIGH_CTRL       = 1,
   TX_PKT_BUF_READY           = 2,
   TX_PKT_BUF_LOW_CTRL        = 3,
   TX_PKT_BUF_DONE            = 4
} tx_pkt_buf_state_t;

typedef enum __attribute__ ((__packed__)) {
   RX_PKT_BUF_UNINITIALIZED   = 0,
   RX_PKT_BUF_HIGH_CTRL       = 1,
   RX_PKT_BUF_READY           = 2,
   RX_PKT_BUF_LOW_CTRL        = 3
} rx_pkt_buf_state_t;



//-----------------------------------------------
// Meta-data about transmissions from CPU Low
//     - This struct must be padded to be an integer number of u32 words.
//
typedef struct __attribute__ ((__packed__)){
    u64                      tx_start_timestamp_mpdu;
    u64                      tx_start_timestamp_ctrl;
    phy_tx_params_t          phy_params_mpdu;
    phy_tx_params_t          phy_params_ctrl;

    u8                       tx_details_type;
    u8                       chan_num;
    u16                      duration;

    s16                      num_slots;
    u16                      cw;

    u8                       tx_start_timestamp_frac_mpdu;
    u8                       tx_start_timestamp_frac_ctrl;
    u8                       src;
    u8                       lrc;

    u16                      ssrc;
    u16                      slrc;

    u8						 flags;
    u8						 reserved[3];

} wlan_mac_low_tx_details_t;
CASSERT(sizeof(wlan_mac_low_tx_details_t) == 44, wlan_mac_low_tx_details_t_alignment_check);

#define TX_DETAILS_FLAGS_RECEIVED_RESPONSE				   1


// tx_details_type defines
#define TX_DETAILS_MPDU                                    0
#define TX_DETAILS_RTS_ONLY                                1
#define TX_DETAILS_RTS_MPDU                                2
#define TX_DETAILS_CTS                                     3
#define TX_DETAILS_ACK                                     4


//-----------------------------------------------
// RX PHY details
//     - Information recorded from the RX PHY when receiving a packet
//     - While N_DBPS can be calculated from (mcs, phy_mode), it is easier to calculate
//       the value once in CPU Low and pass it up vs re-calculating it in CPU High.
//     - This structure must be 32-bit aligned
//
typedef struct {
    u8                       mcs;
    u8                       phy_mode;
    u8                       reserved[2];
    u16                      length;
    u16                      N_DBPS;                       ///< Number of data bits per OFDM symbol
} phy_rx_details_t;


//-----------------------------------------------
// TX queue information
//     - Information about the TX queue that contained the packet while in CPU High.
//     - This structure must be 32-bit aligned.
//
typedef struct {
    u16                      QID;                          ///< ID of the Queue
    u16                      occupancy;                    ///< Number of elements in the queue when the
                                                           ///<   packet was enqueued (including itself)
} tx_queue_details_t;


//-----------------------------------------------
// TX frame information
//     - Defines the information passed in the packet buffer between CPU High and
//       CPU Low as part of transmitting packets.
//
//     IMPORTANT:  This structure must be 8-byte aligned.
//
typedef struct{
    u64                      	timestamp_create;             ///< MAC timestamp of packet creation
    //----- 8-byte boundary ------
    u32                      	delay_accept;                 ///< Time in microseconds between timestamp_create and packet acceptance by CPU Low
    u32                      	delay_done;                   ///< Time in microseconds between acceptance and transmit completion
    //----- 8-byte boundary ------
    u64                      	unique_seq;                   ///< Unique sequence number for this packet (12 LSB used as 802.11 MAC sequence number)
    //----- 8-byte boundary ------
    tx_queue_details_t       	queue_info;                   ///< Information about the TX queue used for the packet (4 bytes)
    u8                       	tx_result;                    ///< Result of transmission attempt - TX_MPDU_RESULT_SUCCESS or TX_MPDU_RESULT_FAILURE
    u8                       	reserved;
    u16                       	num_tx_attempts;              ///< Number of transmission attempts for this frame
    //----- 8-byte boundary ------
    volatile tx_pkt_buf_state_t tx_pkt_buf_state;             ///< State of the Tx Packet Buffer
    u8                       	flags;                        ///< Bit flags en/disabling certain operations by the lower-level MAC
    u8                       	phy_samp_rate;                ///< PHY Sampling Rate
    u8                       	padding0;                     ///< Used for alignment of fields (can be appropriated for any future use)

    u16                      	length;                       ///< Number of bytes in MAC packet, including MAC header and FCS
    u16                      	ID;                           ///< Station ID of the node to which this packet is addressed
    //----- 8-byte boundary ------

    //
    // Place additional fields here.  Make sure the new fields keep the structure 8-byte aligned
    //

    //----- 8-byte boundary ------
    tx_params_t              params;                       ///< Additional lower-level MAC and PHY parameters (8 bytes)
} tx_frame_info_t;

// The above structure assumes that pkt_buf_state_t is a u8.  However, that is architecture dependent.
// Therefore, the code will check the size of the structure using a compile-time assertion.  This check
// will need to be updated if fields are added to the structure
//
CASSERT(sizeof(tx_frame_info_t) == 48, tx_frame_info_alignment_check);


// Defines for power field in phy_tx_params_t in tx_params_t
#define TX_POWER_MAX_DBM                                  (21)
#define TX_POWER_MIN_DBM                                  (-9)

// Defines for tx_result field
#define TX_FRAME_INFO_RESULT_SUCCESS                             0
#define TX_FRAME_INFO_RESULT_FAILURE                             1

// Defines for flags field
#define TX_FRAME_INFO_FLAGS_REQ_TO                               0x01
#define TX_FRAME_INFO_FLAGS_FILL_TIMESTAMP                       0x02
#define TX_FRAME_INFO_FLAGS_FILL_DURATION                        0x04
#define TX_FRAME_INFO_FLAGS_FILL_UNIQ_SEQ                        0x20


//-----------------------------------------------
// RX frame information
//     - Defines the information passed in the packet buffer between CPU High and
//       CPU Low as part of receiving packets.
//     - The struct is padded to give space for the PHY to fill in channel estimates.
//
//     IMPORTANT:  This structure must be 8-byte aligned.
//
typedef struct __attribute__ ((__packed__)){
    u8                       	  	flags;                        ///< Bit flags
    u8                       	  	ant_mode;                     ///< Rx antenna selection
    s8                       	 	rx_power;                     ///< Rx power, in dBm
    u8                       	 	rf_gain;                      ///< Gain setting of radio Rx LNA, in [0,1,2]
    u8                       	  	bb_gain;                      ///< Gain setting of radio Rx VGA, in [0,1,...31]
    u8                       	  	channel;                      ///< Channel index
    volatile rx_pkt_buf_state_t     rx_pkt_buf_state;             ///< State of the Rx Packet Buffer
    u8                       	  	reserved0;
    //----- 8-byte boundary ------
    u32                      	  	cfo_est;                      ///< Carrier Frequency Offset Estimate
    u32							  	reserved1;
    //----- 8-byte boundary ------
    phy_rx_details_t         	  	phy_details;                  ///< Details from PHY used in this reception
    //----- 8-byte boundary ------
    u8                       	  	timestamp_frac;               ///< Fractional timestamp beyond usec timestamp for time of reception
    u8                       	  	phy_samp_rate;                ///< PHY Sampling Rate
    u8                       	  	reserved2[2];                 ///< Reserved bytes for alignment
    u32                      	  	additional_info;              ///< Field to hold MAC-specific info, such as a pointer to a station_info
    //----- 8-byte boundary ------
    wlan_mac_low_tx_details_t     	resp_low_tx_details;			///< Tx Low Details for a control resonse (e.g. ACK or CTS)
    u32								reserved3;
    //----- 8-byte boundary ------
    u64                      	  	timestamp;                    ///< MAC timestamp at time of reception
    //----- 8-byte boundary ------
    u32                      	  	channel_est[64];              ///< Rx PHY channel estimates
} rx_frame_info_t;
// The above structure assumes that pkt_buf_state_t is a u8.  However, that is architecture dependent.
// Therefore, the code will check the size of the structure using a compile-time assertion.  This check
// will need to be updated if fields are added to the structure
//
CASSERT(sizeof(rx_frame_info_t) == 344, rx_frame_info_alignment_check);


// Defines for flags field
#define RX_FRAME_INFO_FLAGS_CTRL_RESP_TX                         0x1
#define RX_FRAME_INFO_FLAGS_RETRY                                0x2
#define RX_FRAME_INFO_FLAGS_FCS_GOOD                             0x4
#define RX_FRAME_INFO_UNEXPECTED_RESPONSE						 0x8


/*************************** Function Prototypes *****************************/

int init_pkt_buf();

// TX packet buffer functions
int lock_tx_pkt_buf(u8 pkt_buf_ind);
int force_lock_tx_pkt_buf(u8 pkt_buf_ind);
int unlock_tx_pkt_buf(u8 pkt_buf_ind);
int get_tx_pkt_buf_status(u8 pkt_buf_ind, u32* locked, u32 *owner);


// RX packet buffer functions
int lock_rx_pkt_buf(u8 pkt_buf_ind);
int force_lock_rx_pkt_buf(u8 pkt_buf_ind);
int unlock_rx_pkt_buf(u8 pkt_buf_ind);
int get_rx_pkt_buf_status(u8 pkt_buf_ind, u32* locked, u32 *owner);


#endif /* WLAN_MAC_IPC_UTIL_H_ */
