/** @file wlan_mac_dcf.h
 *  @brief Distributed Coordination Function
 *
 *  This contains code to implement the 802.11 DCF.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_DCF_H_
#define WLAN_MAC_DCF_H_

#define PKT_BUF_INVALID                                    0xFF


//-----------------------------------------------
// MAC Timing Structure
typedef struct{
	u16 t_slot;
	u16 t_sifs;
	u16 t_difs;
	u16	t_eifs;
	u16 t_phy_rx_start_dly;
	u16 t_timeout;
} mac_timing;

//-----------------------------------------------
// CW Update Reasons
#define DCF_CW_UPDATE_MPDU_TX_ERR                          0
#define DCF_CW_UPDATE_MPDU_RX_ACK                          1
#define DCF_CW_UPDATE_BCAST_TX                             2


//-----------------------------------------------
// Reason codes for generating a random number of slots
//     See:  rand_num_slots()
//
#define RAND_SLOT_REASON_STANDARD_ACCESS                   0
#define RAND_SLOT_REASON_IBSS_BEACON                       1


//-----------------------------------------------
// These are hardcoded OFDM TX times for CTS frames of various rates
//     Since CTS is a fixed size, we can precompute these to save time
//
#define TX_TIME_CTS_R6                                     50
#define TX_TIME_CTS_R12                                    38
#define TX_TIME_CTS_R24                                    34


//-----------------------------------------------
// WLAN Exp low parameter defines (DCF)
//     NOTE:  Need to make sure that these values do not conflict with any of the LOW PARAM
//     callback defines
//
#define LOW_PARAM_DCF_RTS_THRESH                           0x10000001
#define LOW_PARAM_DCF_DOT11SHORTRETRY                      0x10000002
#define LOW_PARAM_DCF_DOT11LONGRETRY                       0x10000003
#define LOW_PARAM_DCF_PHYSICAL_CS_THRESH                   0x10000004
#define LOW_PARAM_DCF_CW_EXP_MIN                           0x10000005
#define LOW_PARAM_DCF_CW_EXP_MAX                           0x10000006



/*********************** Global Structure Definitions ************************/

typedef enum {
    RX_FINISH_SEND_NONE,
    RX_FINISH_SEND_A,
    RX_FINISH_SEND_B
} rx_finish_state_t;


typedef enum {
    TX_PENDING_NONE,
    TX_PENDING_A,
    TX_PENDING_B
} tx_pending_state_t;


typedef enum {
    TX_WAIT_NONE,
    TX_WAIT_ACK,
    TX_WAIT_CTS
} tx_wait_state_t;


typedef enum {
    TX_MODE_SHORT,
    TX_MODE_LONG
} tx_mode_t;

typedef enum {
	BEACON_SENT,
	BEACON_DEFERRED,
	TBTT_NOT_ACHIEVED,
} poll_tbtt_return_t;



/*************************** Function Prototypes *****************************/
int                main();

u32                frame_receive(u8 rx_pkt_buf, phy_rx_details_t* phy_details);
void 			   handle_sample_rate_change(phy_samp_rate_t phy_samp_rate);
void 			   handle_mactime_change(s64 time_delta_usec);
void 			   configure_beacon_txrx(beacon_txrx_configure_t* beacon_txrx_configure);
int 			   frame_transmit(u8 pkt_buf, wlan_mac_low_tx_details_t* low_tx_details);

inline void        increment_src(u16* src_ptr);
inline void        increment_lrc(u16* lrc_ptr);

inline poll_tbtt_return_t poll_tbtt();
inline int 		   		  send_beacon(u8 tx_pkt_buf);

inline void        reset_cw();
inline void        reset_ssrc();
inline void        reset_slrc();

inline u32         rand_num_slots(u8 reason);

void               wlan_mac_dcf_hw_start_backoff(u16 num_slots);

int                wlan_create_ack_frame(void* pkt_buf_addr, u8* address_ra);
int                wlan_create_cts_frame(void* pkt_buf_addr, u8* address_ra, u16 duration);
int                wlan_create_rts_frame(void* pkt_buf_addr, u8* address_ra, u8* address_ta, u16 duration);

#endif /* WLAN_MAC_DCF_H_ */
