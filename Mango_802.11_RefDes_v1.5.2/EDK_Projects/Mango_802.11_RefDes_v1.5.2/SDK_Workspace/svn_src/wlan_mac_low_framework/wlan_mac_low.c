/** @file wlan_mac_low.c
 *  @brief Low-level WLAN MAC High Framework
 *
 *  This contains the low-level code for accessing the WLAN MAC Low Framework.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

// Xilinx / Standard library includes
#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>
#include <stdlib.h>
#include <string.h>

// WARP Includes
#include "w3_ad_controller.h"
#include "w3_clock_controller.h"
#include "w3_iic_eeprom.h"
#include "radio_controller.h"

// WLAN includes
#include "wlan_mac_userio_util.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_phy_util.h"
#include "wlan_mac_low.h"

// WLAN Exp includes
#include "wlan_exp.h"


/*************************** Constant Definitions ****************************/

#define DBG_PRINT  0

// Power / RSSI conversion
#define POW_LOOKUP_SHIFT  3                   // Shift from 10 bit RSSI to 7 bit for lookup


/*********************** Global Variable Definitions *************************/

/*************************** Functions Prototypes ****************************/


/*************************** Variable Definitions ****************************/
volatile static phy_samp_rate_t   gl_phy_samp_rate;                                 ///< Current PHY sampling rate
volatile static u32               mac_param_chan;                                   ///< Current channel of the lower-level MAC
volatile static u8                mac_param_band;                                   ///< Current band of the lower-level MAC
volatile static u8                mac_param_dsss_en;                                ///< Enable / Disable DSSS when possible
volatile static s8                mac_param_ctrl_tx_pow;                            ///< Current transmit power (dBm) for control packets
volatile static u32               mac_param_rx_filter;                              ///< Current filter applied to packet receptions
volatile static u8                rx_pkt_buf;                                       ///< Current receive buffer of the lower-level MAC

static u32                        cpu_low_status;                                   ///< Status flags that are reported to upper-level MAC
static u32						  cpu_low_type;										///< wlan_exp CPU_LOW type that is reported to upperp-level MAC

static wlan_ipc_msg_t        	  ipc_msg_from_high;                                          ///< Buffer for incoming IPC messages
static u32                   	  ipc_msg_from_high_payload[MAILBOX_BUFFER_MAX_NUM_WORDS];    ///< Buffer for payload of incoming IPC messages

volatile static u8           allow_new_mpdu_tx;                                     ///< Toggle for allowing new MPDU Tx requests to be processed
volatile static s8           pkt_buf_pending_tx;                                    ///< Internal state variable for knowing if an MPDU Tx request is pending

// Callback function pointers
static function_ptr_t        frame_rx_callback;                                     ///< User callback frame receptions
static function_ptr_t        frame_tx_callback;                                     ///< User callback frame transmissions

static function_ptr_t		 beacon_txrx_config_callback;
static function_ptr_t		 mactime_change_callback;
static function_ptr_t		 sample_rate_change_callback;

static function_ptr_t        ipc_low_param_callback;                                ///< User callback for IPC_MBOX_LOW_PARAM ipc calls

// Unique transmit sequence number
volatile static u64	         unique_seq;

// NOTE: this statically allocated space should be larger than the maximum number of attempts
//     dot11ShortRetryLimit+dot11LongRetryLimit-1
static wlan_mac_low_tx_details_t  low_tx_details[50]; //TODO make a #define

// Constant LUTs for MCS
const static u16 mcs_to_n_dbps_nonht_lut[WLAN_MAC_NUM_MCS] = {24, 36, 48, 72, 96, 144, 192, 216};
const static u16 mcs_to_n_dbps_htmf_lut[WLAN_MAC_NUM_MCS] = {26, 52, 78, 104, 156, 208, 234, 260};

/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * @brief Initialize MAC Low Framework
 *
 * This function initializes the MAC Low Framework by setting
 * up the hardware and other subsystems in the framework.
 *
 * @param   type             - Lower-level MAC type
 * @return  int              - Initialization status (0 = success)
 */
int wlan_mac_low_init(u32 type){
    u32 		     status;
    rx_frame_info_t* rx_frame_info;
	tx_frame_info_t* tx_frame_info;
    u32			     i;

    mac_param_dsss_en        = 1;
    mac_param_band           = RC_24GHZ;
    mac_param_ctrl_tx_pow    = 10;
    cpu_low_status           = 0;
    cpu_low_type			 = type;

    unique_seq = 0;

    mac_param_rx_filter      = (RX_FILTER_FCS_ALL | RX_FILTER_HDR_ALL);

    frame_rx_callback           = (function_ptr_t) wlan_null_callback;
    frame_tx_callback           = (function_ptr_t) wlan_null_callback;
    ipc_low_param_callback      = (function_ptr_t) wlan_null_callback;
    beacon_txrx_config_callback = (function_ptr_t) wlan_null_callback;
    mactime_change_callback		= (function_ptr_t) wlan_null_callback;
    sample_rate_change_callback = (function_ptr_t) wlan_null_callback;
    allow_new_mpdu_tx        = 1;
    pkt_buf_pending_tx       = -1; // -1 is an invalid pkt_buf index

    status = w3_node_init();

    if(status != 0) {
        xil_printf("Error in w3_node_init()! Exiting\n");
        return -1;
    }

	// Initialize mailbox
	init_mailbox();

    // Initialize packet buffers
	init_pkt_buf();

	// ***************************************************
	// Initialize Transmit Packet Buffers
	// ***************************************************
	for(i = 0; i < NUM_TX_PKT_BUFS; i++){
		tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(i);
		switch(i){
			case TX_PKT_BUF_MPDU_1:
			case TX_PKT_BUF_MPDU_2:
			case TX_PKT_BUF_MPDU_3:
				switch(tx_frame_info->tx_pkt_buf_state){
					case TX_PKT_BUF_UNINITIALIZED:
					case TX_PKT_BUF_HIGH_CTRL:
						// CPU High will initialize
						break;
					break;
					case TX_PKT_BUF_READY:
					case TX_PKT_BUF_DONE:
			            // CPU Low rebooted after finishing old Tx
			            // No way to know if CPU Low sent TX_DONE(p) message - must reset p.state here
			            //  Two potential races:
			            //   -CPU High just rebooted and will also attempt setting p.state=TX_PKT_BUF_HIGH_CTRL
			            //      No problem if both CPUs set state to TX_PKT_BUF_HIGH_CTRL
			            //   -CPU High did not reboot and will attempt tx_done_handler(p)
			            //      If p.state=TX_PKT_BUF_HIGH_CTRL when tx_done_handler(p) runs, CPU High will fail gracefully
			            //      If p.state set to TX_PKT_BUF_HIGH_CTRL during tx_done_handler(p), CPU High will succeed normally
					case TX_PKT_BUF_LOW_CTRL:
			            // CPU Low rebooted after CPU High submitted packet for Tx
			            //  Release lock and reset state
			            //  CPU High will find this TX_PKT_BUF_HIGH_CTRL buffer in next ping/pong update
					default:
						// Something went wrong if tx_pkt_buf_state is something
						// other than one of the tx_pkt_buf_state_t enums. We'll
						// attempt to resolve the problem by explicitly setting
						// the state.
						tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
						unlock_tx_pkt_buf(i);
					break;
				}
			break;
			case TX_PKT_BUF_BEACON:
				unlock_tx_pkt_buf(i);
			break;
			case TX_PKT_BUF_RTS:
			case TX_PKT_BUF_ACK_CTS:
				force_lock_tx_pkt_buf(i);
				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_LOW_CTRL;
			default:
			break;
		}
	}
	// ***************************************************
	// Initialize Receive Packet Buffers
	// ***************************************************
	for(i = 0; i < NUM_RX_PKT_BUFS; i++){
		rx_frame_info = (rx_frame_info_t*)RX_PKT_BUF_TO_ADDR(i);
		switch(rx_frame_info->rx_pkt_buf_state){
		   case RX_PKT_BUF_UNINITIALIZED:
		   case RX_PKT_BUF_LOW_CTRL:
		   default:
				// Something went wrong if rx_pkt_buf_state is something
				// other than one of the rx_pkt_buf_state_t enums. We'll
				// attempt to resolve the problem by explicitly setting
				// the state.
			   force_lock_rx_pkt_buf(i);
			   rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_LOW_CTRL;
		   break;
		   case RX_PKT_BUF_HIGH_CTRL:
		   case RX_PKT_BUF_READY:
	           // CPU Low rebooted after submitting packet for de-encapsulation/logging
	           //  Will be handled by CPU High, either because CPU High is about
	           //  to de-encapsulate/log p or just rebooted and will clean up
		   break;
		}
	}

    // Create IPC message to receive into
    ipc_msg_from_high.payload_ptr = &(ipc_msg_from_high_payload[0]);

    // Point the PHY to an empty Rx Pkt Buffer
    wlan_mac_low_lock_empty_rx_pkt_buf();

    // Move the PHY's starting address into the packet buffers by PHY_XX_PKT_BUF_PHY_HDR_OFFSET.
    // This accounts for the metadata located at the front of every packet buffer (Xx_mpdu_info)
    wlan_phy_rx_pkt_buf_phy_hdr_offset(PHY_RX_PKT_BUF_PHY_HDR_OFFSET);
    wlan_phy_tx_pkt_buf_phy_hdr_offset(PHY_TX_PKT_BUF_PHY_HDR_OFFSET);

    wlan_mac_reset(1);
    wlan_radio_init();
    wlan_phy_init();
    wlan_mac_hw_init();
    wlan_mac_reset(0);

    // Initialize the HW info structure
    init_mac_hw_info();

    // Set the NAV ignore addr to this HW address
    wlan_mac_low_set_nav_check_addr(get_mac_hw_addr_wlan());

    return 0;
}



/*****************************************************************************/
/**
 * @brief Finish Initializing MAC Low Framework
 *
 * This function finishes the initialization and notifies the upper-level
 * MAC that it has finished booting.
 *
 * @param   None
 * @return  None
 */
void wlan_mac_low_init_finish(){

    //Set the default PHY sample rate to 20 MSps
	set_phy_samp_rate(PHY_20M);

    // Update the CPU Low status
    cpu_low_status |= CPU_STATUS_INITIALIZED;

    wlan_mac_low_send_status(CPU_STATUS_REASON_BOOTED);

}

void wlan_mac_low_send_status(u8 cpu_status_reason){
	wlan_ipc_msg_t ipc_msg_to_high;
	u32            ipc_msg_to_high_payload[2];

	// Send a message to other processor to say that this processor is initialized and ready
	ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
	ipc_msg_to_high.arg0			  = cpu_status_reason;
	ipc_msg_to_high.num_payload_words = 2;
	ipc_msg_to_high.payload_ptr       = &(ipc_msg_to_high_payload[0]);
	ipc_msg_to_high_payload[0]        = cpu_low_status;
	ipc_msg_to_high_payload[1]        = cpu_low_type;

	write_mailbox_msg(&ipc_msg_to_high);
}


/*****************************************************************************/
/**
 * @brief Set the PHY Sampling Rate
 *
 * This function should be called to switch the PHY sampling rate between
 * 10/20/40 MSps.
 *
 * @param   phy_samp_rate_t phy_samp_rate
 * @return  None
 */
void set_phy_samp_rate(phy_samp_rate_t phy_samp_rate){

    // Check sample rate argument
    //     - Must be in [PHY_10M, PHY_20M, PHY_40M]
    if (!((phy_samp_rate == PHY_10M) || (phy_samp_rate == PHY_20M) || (phy_samp_rate == PHY_40M))) {
        return;
    }

    // Set global sample rate variable
	gl_phy_samp_rate = phy_samp_rate;

    // Assert PHY Tx/Rx and MAC Resets
    REG_SET_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);
    wlan_mac_reset(1);

    // DSSS Rx only supported at 20Msps
    switch(phy_samp_rate){
		case PHY_10M:
    	case PHY_40M:
    		// Always disable DSSS when PHY sample rate is not 20 MSps
    		wlan_phy_DSSS_rx_disable();
    	break;
    	case PHY_20M:
			// Enable DSSS if global variable indicates it should be enabled and RF band allows it
    		if ((mac_param_dsss_en) && (mac_param_band == RC_24GHZ)) {
    			wlan_phy_DSSS_rx_enable();
			}
    	break;
    }

    // Configure auto-correlation packet detection
    //  wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(corr_thresh, energy_thresh, min_dur, post_wait)
    switch(phy_samp_rate){
    	case PHY_40M:
    		//TODO: The 2 value is suspiciously low
    		wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(200, 2, 15, 0x3F);
		break;
    	case PHY_10M:
    	case PHY_20M:
    		wlan_phy_rx_pktDet_autoCorr_ofdm_cfg(200, 9, 4, 0x3F);
    	break;
    }

    // Set post Rx extension
    //  Number of sample periods post-Rx the PHY waits before asserting Rx END. The additional
    //   -3 usec accounts for latency through the Rx RF chain.
    switch(phy_samp_rate){
    	case PHY_40M:
    		// 6us Extension
    		wlan_phy_rx_set_extension((6*40) - 128);
		break;
    	case PHY_20M:
    		// 6us Extension
    		wlan_phy_rx_set_extension((6*20) - 64);
    	break;
    	case PHY_10M:
    		// 6us Extension
    		wlan_phy_rx_set_extension((6*10) - 32);
    	break;
    }

    // Set Tx duration extension, in units of sample periods
	switch(phy_samp_rate){
		case PHY_40M:
			// 224 40MHz sample periods -- aligns TX_END to RX_END
			wlan_phy_tx_set_extension(224);

			// Set extension from last samp output to RF Tx -> Rx transition
			//     This delay allows the Tx pipeline to finish driving samples into DACs
			//     and for DAC->RF frontend to finish output Tx waveform
			wlan_phy_tx_set_txen_extension(100);

			// Set extension from RF Rx -> Tx to un-blocking Rx samples
			wlan_phy_tx_set_rx_invalid_extension(300);
		break;
		case PHY_20M:
			// 112 20MHz sample periods -- aligns TX_END to RX_END
			wlan_phy_tx_set_extension(112);

			// Set extension from last samp output to RF Tx -> Rx transition
			//     This delay allows the Tx pipeline to finish driving samples into DACs
			//     and for DAC->RF frontend to finish output Tx waveform
			wlan_phy_tx_set_txen_extension(50);

			// Set extension from RF Rx -> Tx to un-blocking Rx samples
			wlan_phy_tx_set_rx_invalid_extension(150);
		break;
		case PHY_10M:
			// 56 10MHz sample periods -- aligns TX_END to RX_END
			wlan_phy_tx_set_extension(56);

			// Set extension from last samp output to RF Tx -> Rx transition
			//     This delay allows the Tx pipeline to finish driving samples into DACs
			//     and for DAC->RF frontend to finish output Tx waveform
			wlan_phy_tx_set_txen_extension(25);

			// Set extension from RF Rx -> Tx to un-blocking Rx samples
			wlan_phy_tx_set_rx_invalid_extension(75);
		break;
	}

	// Set RF interface clocking and interp/decimation filters
	switch(phy_samp_rate){
		case PHY_40M:
			// Set ADC_CLK=DAC_CLK=40MHz, interp_rate=decim_rate=1
			clk_config_dividers(CLK_BASEADDR, 2, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
			ad_config_filters(AD_BASEADDR, AD_ALL_RF, 1, 1);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x2F);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x00);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);
		break;
		case PHY_20M:
			// Set ADC_CLK=DAC_CLK=40MHz, interp_rate=decim_rate=2
			clk_config_dividers(CLK_BASEADDR, 2, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
			ad_config_filters(AD_BASEADDR, AD_ALL_RF, 2, 2);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x27);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x00);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);
		break;
		case PHY_10M:
			// Set ADC_CLK=DAC_CLK=20MHz, interp_rate=decim_rate=2
			clk_config_dividers(CLK_BASEADDR, 4, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
			ad_config_filters(AD_BASEADDR, AD_ALL_RF, 2, 2);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x27);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x00);
			ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);
		break;
	}

    switch(phy_samp_rate){
    	case PHY_40M:
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXLPF_BW, 3);
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLPF_BW, 3);
		break;
    	case PHY_10M:
    	case PHY_20M:
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXLPF_BW, 1);
    	    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLPF_BW, 1);
    	break;
    }

    // AGC timing: capt_rssi_1, capt_rssi_2, capt_v_db, agc_done
    switch(phy_samp_rate){
    	case PHY_40M:
    		wlan_agc_set_AGC_timing(10, 30, 90, 96);
		break;
    	case PHY_10M:
    	case PHY_20M:
    		wlan_agc_set_AGC_timing(1, 30, 90, 96);
    	break;
    }

    // Call user callback so it can deal with any changes that need to happen due to a change in sampling rate
	sample_rate_change_callback(gl_phy_samp_rate);

    // Deassert PHY Tx/Rx and MAC Resets
    REG_CLEAR_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);
    wlan_mac_reset(0);

    // Let PHY Tx take control of radio TXEN/RXEN
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);
}


/*****************************************************************************/
/**
 * @brief Initialize the DCF Hardware Core
 *
 * This function initializes the DCF hardware core.
 *
 * @param   None
 * @return  None
 */
void wlan_mac_hw_init(){
    // Enable blocking of the Rx PHY following good-FCS receptions and bad-FCS receptions
    //     BLOCK_RX_ON_VALID_RXEND will block the Rx PHY on all RX_END events following valid RX_START events
    //     This allows the wlan_exp framework to count and log bad FCS receptions
    //
    REG_SET_BITS(WLAN_MAC_REG_CONTROL, WLAN_MAC_CTRL_MASK_BLOCK_RX_ON_TX);

    // Enable the NAV counter
    REG_CLEAR_BITS(WLAN_MAC_REG_CONTROL, (WLAN_MAC_CTRL_MASK_DISABLE_NAV));

    // Set sane defaults for MAC timing values. These will be overwritten by
    // low-level applications that need to specify these times (e.g. the DCF)
    wlan_mac_set_slot(9*10);
	wlan_mac_set_DIFS(28*10);
	wlan_mac_set_TxDIFS((28*10) - (TX_PHY_DLY_100NSEC));
	wlan_mac_postTx_timer1_en(0);
	wlan_mac_postRx_timer2_en(0);
	wlan_mac_set_NAV_adj(0*10);
	wlan_mac_set_EIFS(88*10);

    // Set the TU target to 2^32-1 (max value) and hold TU_LATCH in reset
    //  MAC Low application should re-enabled if needed
    wlan_mac_set_tu_target(0xFFFFFFFF);
    wlan_mac_reset_tu_target_latch(1);

    // Clear any stale Rx events
    wlan_mac_hw_clear_rx_started();
}



/*****************************************************************************/
/**
 * @brief Send Exception to Upper-Level MAC
 *
 * This function generates an IPC message for the upper-level MAC
 * to tell it that something has gone wrong
 *
 * @param u32 reason
 *  - reason code for the exception
 * @return None
 */
inline void wlan_mac_low_send_exception(u32 reason){
    wlan_ipc_msg_t ipc_msg_to_high;
    u32            ipc_msg_to_high_payload[2];

    // Update CPU Low status
    cpu_low_status |= CPU_STATUS_EXCEPTION;

    // Send an exception to CPU_HIGH along with a reason
    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
    ipc_msg_to_high.arg0			  = (u8)CPU_STATUS_REASON_EXCEPTION;
    ipc_msg_to_high.num_payload_words = 2;
    ipc_msg_to_high.payload_ptr       = &(ipc_msg_to_high_payload[0]);
    ipc_msg_to_high_payload[0]        = cpu_low_status;
    ipc_msg_to_high_payload[1]        = reason;

    write_mailbox_msg(&ipc_msg_to_high);

    // Set the Hex display with the reason code and flash the LEDs
    cpu_error_halt(reason);
}

/*****************************************************************************/
/**
 * @brief Poll the Receive Frame Start
 *
 * This function will poll the hardware to see if the PHY is currently receiving or
 * has finished receiving a packet.  This will then dispatch the current RX packet
 * buffer and PHY details to the frame_rx_callback(). The callback is responsible for
 * updating the current Rx packet buffer, typically required if the received packet
 * is passed to CPU High for further processing.
 *
 * @param   None
 * @return  u32              - Status (See MAC Polling defines in wlan_mac_low.h)
 */
inline u32 wlan_mac_low_poll_frame_rx(){
    phy_rx_details_t phy_details;

    volatile u32 mac_hw_status;
    volatile u32 phy_hdr_params;

    int i = 0;

    u32 return_status = 0;

    // Read the MAC/PHY status
    mac_hw_status = wlan_mac_get_status();

    // Check if PHY has started a new reception
    if(mac_hw_status & WLAN_MAC_STATUS_MASK_RX_PHY_STARTED) {

    	// Check whether this is an OFDM or DSSS Rx
        if(wlan_mac_get_rx_phy_sel() == WLAN_MAC_PHY_RX_PHY_HDR_PHY_SEL_DSSS) {
            // DSSS Rx - PHY Rx length is already valid, other params unused for DSSS
            phy_details.phy_mode = PHY_MODE_DSSS;
            phy_details.N_DBPS   = 0;

            // Strip off extra pre-MAC-header bytes used in DSSS frames; this adjustment allows the next
            //     function to treat OFDM and DSSS payloads the same
            phy_details.length   = wlan_mac_get_rx_phy_length() - 5;
            phy_details.mcs      = 0;

            // Call the user callback to handle this Rx, capture return value
        	return_status |= POLL_MAC_STATUS_RECEIVED_PKT;
        	return_status |= frame_rx_callback(rx_pkt_buf, &phy_details);

        } else {
        	// OFDM Rx - must wait for valid PHY header
        	// Order of operations is critical here
			//  1) Read status first
			//  2) Read PHY header register second
			//  3) Check for complete PHY header - continue if complete
			//  4) Else check for early PHY reset - quit if reset

        	while (1) {
				mac_hw_status = wlan_mac_get_status();
				phy_hdr_params = wlan_mac_get_rx_phy_hdr_params();

				if(i++ > 1000000) {xil_printf("Stuck in OFDM Rx PHY hdr check: 0x%08x 0x%08x\n", mac_hw_status, phy_hdr_params);}

				if(phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_READY) {
					// Rx PHY received enough bytes to decode PHY header
					//  Exit loop and check PHY header params
					break;
				}
				if((mac_hw_status & WLAN_MAC_STATUS_MASK_RX_PHY_ACTIVE) == 0) {
					// Rx PHY went idle before asserting RX_PHY_HDR_READY
					//  This only happens if the PHY is reset externally, possible if MAC starts a Tx during Rx
					//  Only option is to reset RX_STARTED and wait for next Rx

					// There is a 1-cycle race in this case, because RX_END asserts 1 cycle before RX_PHY_HDR_READY in the
					//  case of an invalid HT-SIG. The invalid HT-SIG generates an RX_END_ERROR which causes
					//  RX_END to assert. The simple workaround used below is to re-read phy_hdr_params one last time
					//  before concluding that the Rx PHY was reset unexpectedly
					break;
				}
			}

        	//  Re-read phy_hdr_params to resolve 1-cycle ambiguity in case of HT-SIG error
			phy_hdr_params = wlan_mac_get_rx_phy_hdr_params();

			// Decide how to handle this waveform
			if(phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_READY) {
                // Received PHY header - decide whether to call MAC callback
                if( (phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_MASK_UNSUPPORTED) ||
                	(wlan_mac_get_rx_phy_mode() > 0x2) ) {
                    // Valid HT-SIG but unsupported waveform
                    //  Rx PHY will hold ACTIVE until last samp but will not write payload
                	//  HT-SIG fields (MCS, length) can be safely read here if desired
                	// Or detected VHT waveform (not supported), did not attempt decoding VHT-SIG
        			//xil_printf("Quitting - WLAN_MAC_PHY_RX_PHY_HDR_MASK_UNSUPPORTED (MCS = %d, Length = %d)", wlan_mac_get_rx_phy_mcs(), wlan_mac_get_rx_phy_length());

                } else if(phy_hdr_params & WLAN_MAC_PHY_RX_PHY_HDR_MASK_RX_ERROR) {
                    // Invalid HT-SIG (CRC error, invalid RESERVED or TAIL bits, invalid LENGTH, etc)
                    //  Rx PHY has already released ACTIVE and will not write payload
                	//  HT-SIG fields (MCS, length) should not be trusted in this case
        			//xil_printf("Quitting - WLAN_MAC_PHY_RX_PHY_HDR_MASK_RX_ERROR");

                } else {
                    // NONHT waveform or HTMF waveform with supported HT-SIG - PHY will write payload
                    //  Call lower MAC Rx callback
                    //  Callback can safely return anytime (before or after RX_END)

                    phy_details.phy_mode = wlan_mac_get_rx_phy_mode();
                    phy_details.length   = wlan_mac_get_rx_phy_length();
                    phy_details.mcs      = wlan_mac_get_rx_phy_mcs();
                    phy_details.N_DBPS   = wlan_mac_low_mcs_to_n_dbps(phy_details.mcs, phy_details.phy_mode);

                	return_status |= POLL_MAC_STATUS_RECEIVED_PKT;
                	return_status |= frame_rx_callback(rx_pkt_buf, &phy_details);
                }
            } else {
            	// PHY went idle before PHY_HDR_DONE, probably due to external reset
            	//  The Rx PHY can be reset from software (only used in wlan_phy_init()) or hardware.
            	//  The hardware reset is asserted by the MAC core during Tx. Asserting Tx during Rx is
            	//  impossible with the normal DCF code, as packet det is treated as a busy medium. With a
            	//  custom MAC implementation that allows Tx during Rx this code block will catch the
            	//  unexpected reset events.

            	// PHY header cannot be trusted in this case - do nothing and return

            }//END if(PHY_HDR_DONE)
        } //END if(OFDM Rx)

    	// Clear the MAC status register RX_STARTED bit
    	//  By this point the framework and MAC are done with the current waveform, however it was handled.
    	//   The RX_STARTED bit is cleared in every case. It is important to only call clear_rx_started() after
        //   the RX_STARTED latch has asserted. Calling it any other time creates a race that can miss Rx events
    	wlan_mac_hw_clear_rx_started();

    } //END if(PHY_RX_STARTED)

    return return_status;
}

/*****************************************************************************/
/**
 * @brief Poll for IPC Receptions
 *
 * This function is a non-blocking poll for IPC receptions from the upper-level MAC.
 *
 * @param   None
 * @return  None
 */
inline void wlan_mac_low_poll_ipc_rx(){
    // Poll mailbox read msg
    if (read_mailbox_msg(&ipc_msg_from_high) == IPC_MBOX_SUCCESS) {
        wlan_mac_low_process_ipc_msg(&ipc_msg_from_high);
    }
}



/*****************************************************************************/
/**
 * @brief Process IPC Reception
 *
 * This is an internal function to the WLAN MAC Low framework to process
 * received IPC messages and call the appropriate callback.
 *
 * @param   None
 * @return  None
 */
void wlan_mac_low_process_ipc_msg(wlan_ipc_msg_t * msg){
    wlan_ipc_msg_t           ipc_msg_to_high;

    switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)){

    	//---------------------------------------------------------------------
    	case IPC_MBOX_SET_MAC_TIME:
    		switch(msg->arg0){
				default:
				case 0:
					//Payload is an absolute MAC time that must be applied
					set_mac_time_usec( *(u64*)(msg->payload_ptr) );
					mactime_change_callback( (*(s64*)(msg->payload_ptr))-((s64)get_mac_time_usec()) );
    			break;
				case 1:
					//Payload is a MAC time delta that must be applied
					apply_mac_time_delta_usec( *(s64*)(msg->payload_ptr));
					mactime_change_callback( *(s64*)(msg->payload_ptr));
				break;
    		}
    	break;

    	//---------------------------------------------------------------------
    	case IPC_MBOX_TXRX_BEACON_CONFIGURE: {
    		beacon_txrx_config_callback(msg->payload_ptr);
    	}
    	break;

		//---------------------------------------------------------------------
        case IPC_MBOX_CPU_STATUS: {
        	if(msg->arg0 == (u8)CPU_STATUS_REASON_BOOTED){
        		// If CPU_HIGH just booted, we should re-inform it of our CPU status
        		wlan_mac_low_send_status(CPU_STATUS_REASON_RESPONSE);
        	}
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_MEM_READ_WRITE: {
            switch(msg->arg0){
                case IPC_REG_WRITE_MODE: {
                    u32    * payload_to_write = (u32*)((u8*)ipc_msg_from_high_payload + sizeof(ipc_reg_read_write_t));

                    // IMPORTANT: this memcpy assumes the payload provided by CPU high is ready as-is
                    //     Any byte swapping (i.e. for payloads that arrive over Ethernet) *must* be performed
                    //     before the payload is passed to this function
                    memcpy((u8*)(((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->baseaddr),
                           (u8*)payload_to_write,
                           (sizeof(u32) * ((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->num_words));
                }
                break;

                case IPC_REG_READ_MODE: {
                    /*
                    xil_printf("\nCPU Low Read:\n");
                    xil_printf(" Addr: 0x%08x\n", (u32*)((ipc_reg_read_write*)ipc_msg_from_high_payload)->baseaddr);
                    xil_printf(" N Wrds: %d\n", ((ipc_reg_read_write*)ipc_msg_from_high_payload)->num_words);

                    xil_printf("Mem[0x%08x] = 0x%08x\n",
                            (u32*)((ipc_reg_read_write*)ipc_msg_from_high_payload)->baseaddr,
                            Xil_In32((u32*)((ipc_reg_read_write*)ipc_msg_from_high_payload)->baseaddr));
                     */
                    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_MEM_READ_WRITE);
                    ipc_msg_to_high.num_payload_words = ((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->num_words;
                    ipc_msg_to_high.payload_ptr       = (u32*)((ipc_reg_read_write_t*)ipc_msg_from_high_payload)->baseaddr;

                    write_mailbox_msg(&ipc_msg_to_high);
                }
                break;
            }
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_LOW_PARAM: {
            switch(msg->arg0){
                case IPC_REG_WRITE_MODE: {
                    switch(ipc_msg_from_high_payload[0]){
                        case LOW_PARAM_BB_GAIN: {
                            if(ipc_msg_from_high_payload[1] <= 3){
                                radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXGAIN_BB, ipc_msg_from_high_payload[1]);
                            }
                        }
                        break;

                        case LOW_PARAM_LINEARITY_PA: {
                            if(ipc_msg_from_high_payload[1] <= 3){
                                radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_PADRIVER, ipc_msg_from_high_payload[1]);
                            }
                        }
                        break;

                        case LOW_PARAM_LINEARITY_VGA: {
                            if(ipc_msg_from_high_payload[1] <= 3){
                                radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_VGA, ipc_msg_from_high_payload[1]);
                            }
                        }
                        break;

                        case LOW_PARAM_LINEARITY_UPCONV: {
                            if(ipc_msg_from_high_payload[1] <= 3){
                                radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_UPCONV, ipc_msg_from_high_payload[1]);
                            }
                        }
                        break;

                        case LOW_PARAM_AD_SCALING: {
                            ad_spi_write(AD_BASEADDR, AD_ALL_RF, 0x36, (0x1F & ipc_msg_from_high_payload[1]));
                            ad_spi_write(AD_BASEADDR, AD_ALL_RF, 0x37, (0x1F & ipc_msg_from_high_payload[2]));
                            ad_spi_write(AD_BASEADDR, AD_ALL_RF, 0x35, (0x1F & ipc_msg_from_high_payload[3]));
                        }
                        break;

                        case LOW_PARAM_PKT_DET_MIN_POWER: {
                            if( ipc_msg_from_high_payload[1]&0xFF000000 ){
                                wlan_phy_enable_req_both_pkt_det();
                                //The value sent from wlan_exp will be unsigned with 0 representing PKT_DET_MIN_POWER_MIN
                                wlan_mac_low_set_pkt_det_min_power((ipc_msg_from_high_payload[1]&0x000000FF) - PKT_DET_MIN_POWER_MIN);

                            } else {
                                wlan_phy_disable_req_both_pkt_det();
                            }
                        }
                        break;

                        case LOW_PARAM_PHY_SAMPLE_RATE: {
                            set_phy_samp_rate(ipc_msg_from_high_payload[1]);
                        }
                        break;

                        default: {
                            ipc_low_param_callback(IPC_REG_WRITE_MODE, ipc_msg_from_high_payload);
                        }
                        break;
                    }
                }
                break;

                case IPC_REG_READ_MODE: {
                    // Read Mode is not supported
                    //
                    // NOTE:  This is due to the fact that IPC messages in CPU low can take an infinitely long amount of
                    //     to return given that the sending and receiving of wireless data takes precedent.  Therefore,
                    //     it is not good to try to return values from CPU low since there is no guarantee when the values
                    //     will be available.
                    //
                    u32      ret_val                  = 0;

                    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_LOW_PARAM);
                    ipc_msg_to_high.num_payload_words = 0;
                    ipc_msg_to_high.payload_ptr       = (u32 *)&ret_val;

                    write_mailbox_msg(&ipc_msg_to_high);
                }
                break;
            }
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_CHANNEL: {
            wlan_mac_low_set_radio_channel(ipc_msg_from_high_payload[0]);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_LOW_RANDOM_SEED: {
            srand(ipc_msg_from_high_payload[0]);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_TX_CTRL_POW: {
            mac_param_ctrl_tx_pow = (s8)ipc_msg_from_high_payload[0];
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_RX_FILTER: {
            u32    filter_mode_hi = (u32)ipc_msg_from_high_payload[0];
            u32    filter_mode_lo = 0;

            if((filter_mode_hi & RX_FILTER_FCS_MASK) == RX_FILTER_FCS_NOCHANGE){
                filter_mode_lo |= (mac_param_rx_filter & RX_FILTER_FCS_MASK);
            } else {
                filter_mode_lo |= (filter_mode_hi & RX_FILTER_FCS_MASK);
            }

            if((filter_mode_hi & RX_FILTER_HDR_NOCHANGE) == RX_FILTER_HDR_NOCHANGE){
                filter_mode_lo |= (mac_param_rx_filter & RX_FILTER_HDR_NOCHANGE);
            } else {
                filter_mode_lo |= (filter_mode_hi & RX_FILTER_HDR_NOCHANGE);
            }

            mac_param_rx_filter = filter_mode_lo;
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_RX_ANT_MODE: {
            wlan_rx_config_ant_mode(ipc_msg_from_high_payload[0]);
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_CONFIG_DSSS_EN: {
			if (ipc_msg_from_high_payload[0] == 1) {
				// xil_printf("Enabling DSSS\n");
				wlan_mac_low_DSSS_rx_enable();
			} else {
				// xil_printf("Disabling DSSS\n");
				wlan_mac_low_DSSS_rx_disable();
			}
        }
        break;

        //---------------------------------------------------------------------
        case IPC_MBOX_TX_MPDU_READY: {
        	u8 tx_pkt_buf;
        	tx_pkt_buf = msg->arg0;
        	if(tx_pkt_buf < NUM_TX_PKT_BUFS){
				// Message is an indication that a Tx Pkt Buf needs processing
				if(allow_new_mpdu_tx){
					wlan_mac_low_proc_pkt_buf( tx_pkt_buf );
				} else {
					pkt_buf_pending_tx = tx_pkt_buf;
				}
        	}
        }
        break;
    }
}



/*****************************************************************************/
/**
 * @brief Disable New MPDU Transmissions
 *
 * This function will prevent any future MPDU transmission requests from
 * being accepted.
 *
 * @param   None
 * @return  None
 *
 */
void wlan_mac_low_disable_new_mpdu_tx(){
    allow_new_mpdu_tx = 0;
}



/*****************************************************************************/
/**
 * @brief Enable New MPDU Transmissions
 *
 * This function will allow future MPDU transmission requests from
 * being accepted.
 *
 * @param   None
 * @return  None
 *
 */
void wlan_mac_low_enable_new_mpdu_tx(){
    if(allow_new_mpdu_tx == 0){
        allow_new_mpdu_tx = 1;
        if(pkt_buf_pending_tx != -1){
            wlan_mac_low_proc_pkt_buf(pkt_buf_pending_tx);
            pkt_buf_pending_tx = -1;
        }
    }
}



/*****************************************************************************/
/**
 * @brief Set the radio channel
 *
 * This function will set the radio channel for CPU LOW
 *
 * @param   channel     - Radio channel
 * @return  None
 *
 */
void wlan_mac_low_set_radio_channel(u32 channel){

	mac_param_chan = channel;

	if (wlan_verify_channel(mac_param_chan) == XST_SUCCESS) {
		if(mac_param_chan <= 14){
			mac_param_band = RC_24GHZ;

			// Enable DSSS if global variable indicates it should be enabled and PHY sample rate allows it
			if ((mac_param_dsss_en) && (gl_phy_samp_rate == PHY_20M)) {
				wlan_phy_DSSS_rx_enable();
			}
		} else {
			mac_param_band = RC_5GHZ;

			// Always disable DSSS when in the 5 GHZ band
			wlan_phy_DSSS_rx_disable();
		}

	    if(channel >= 36){
	        // Adjust Tx baseband gain when switching to 5GHz channels; this adjustment makes
	    	//  the actual Tx power set via the Tx VGA more accurate
	        radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXGAIN_BB, 3);
	    } else {
	        radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXGAIN_BB, 1);
	    }

		radio_controller_setCenterFrequency(RC_BASEADDR, (RC_ALL_RF), mac_param_band, wlan_mac_low_wlan_chan_to_rc_chan(mac_param_chan));
		wlan_mac_reset_NAV_counter();

	} else {
		xil_printf("Invalid channel selection %d\n", mac_param_chan);
	}
}



/*****************************************************************************/
/**
 * @brief Enable / Disable DSSS RX
 *
 * DSSS RX must be disabled when in the 5 GHz band or when the PHY sample rate
 * is not 20 MSps.  However, the low framework will maintain what the state
 * should be when in the 2.4 GHz band and the PHY sample rate is 20 MSps
 *
 * @param   None
 * @return  None
 *
 */
void wlan_mac_low_DSSS_rx_enable() {
	mac_param_dsss_en = 1;

	// Only enable DSSS if in 2.4 GHz band and phy sample rate is 20
	if ((mac_param_band == RC_24GHZ) && (gl_phy_samp_rate == PHY_20M)) {
		wlan_phy_DSSS_rx_enable();
	}
}


void wlan_mac_low_DSSS_rx_disable() {
	mac_param_dsss_en = 0;
	wlan_phy_DSSS_rx_disable();
}



/*****************************************************************************/
/**
 * @brief Process a Tx Packet Buffer for Transmission
 *
 * This function can be called directly via the IPC_MBOX_TX_MPDU_READY message
 * or asynchronously if allow_new_mpdu_tx is enabled.
 *
 * @param   tx_pkt_buf         - u16 packet buffer index
 * @return  None
 *
 */
void wlan_mac_low_proc_pkt_buf(u16 tx_pkt_buf){
    u32                      status;
    tx_frame_info_t        * tx_frame_info;
    mac_header_80211       * tx_80211_header;
    u32                      is_locked, owner;
    u32                      low_tx_details_size;
    wlan_ipc_msg_t           ipc_msg_to_high;
    ltg_packet_id_t*         pkt_id;

    if(tx_pkt_buf >= NUM_TX_PKT_BUFS){
    	xil_printf("Error: Tx Pkt Buf index exceeds NUM_TX_PKT_BUFS\n");
    	return;
    }

    tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);

	switch(tx_frame_info->tx_pkt_buf_state){
		// ---- Normal Tx process - buffer contains packet ready for Tx ----
		case TX_PKT_BUF_READY:
			if(lock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
				wlan_printf(PL_ERROR, "Error: unable to lock TX pkt_buf %d\n", tx_pkt_buf);

				get_tx_pkt_buf_status(tx_pkt_buf, &is_locked, &owner);

				wlan_printf(PL_ERROR, "    TX pkt_buf %d status: isLocked = %d, owner = %d\n", tx_pkt_buf, is_locked, owner);
				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;

			} else {

				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_LOW_CTRL;

				tx_frame_info->delay_accept = (u32)(get_mac_time_usec() - tx_frame_info->timestamp_create);

				// Get pointer to start of MAC header in packet buffer
				tx_80211_header = (mac_header_80211*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)+PHY_TX_PKT_BUF_MPDU_OFFSET);

				// Insert sequence number here
				tx_80211_header->sequence_control = ((tx_80211_header->sequence_control) & 0xF) | ( (unique_seq&0xFFF)<<4 );

				// Insert unique sequence into tx_frame_info
				tx_frame_info->unique_seq = unique_seq;

				if((tx_frame_info->flags) & TX_FRAME_INFO_FLAGS_FILL_UNIQ_SEQ){
					// Fill unique sequence number into LTG payload
					pkt_id             = (ltg_packet_id_t*)((u8*)tx_80211_header + sizeof(mac_header_80211));
					pkt_id->unique_seq = unique_seq;
				}

				//Increment the global unique sequence number
				unique_seq++;

				// Submit the MPDU for transmission - this callback will return only when the MPDU Tx is
				//     complete (after all re-transmissions, ACK Rx, timeouts, etc.)
				//
				// If a frame_tx_callback is not provided, the wlan_null_callback will always
				// return 0 (ie TX_MPDU_RESULT_SUCCESS).
				//

				status = frame_tx_callback(tx_pkt_buf, low_tx_details);

				//Record the total time this MPDU spent in the Tx state machine
				tx_frame_info->delay_done = (u32)(get_mac_time_usec() - (tx_frame_info->timestamp_create + (u64)(tx_frame_info->delay_accept)));

				low_tx_details_size = (tx_frame_info->num_tx_attempts)*sizeof(wlan_mac_low_tx_details_t);

				if(status == TX_FRAME_INFO_RESULT_SUCCESS){
					tx_frame_info->tx_result = TX_FRAME_INFO_RESULT_SUCCESS;
				} else {
					tx_frame_info->tx_result = TX_FRAME_INFO_RESULT_FAILURE;
				}

				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_DONE;

				//Revert the state of the packet buffer and return control to CPU High
				if(unlock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
					wlan_printf(PL_ERROR, "Error: unable to unlock TX pkt_buf %d\n", tx_pkt_buf);
					wlan_mac_low_send_exception(WLAN_ERROR_CODE_CPU_LOW_TX_MUTEX);
					tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
				} else {
					ipc_msg_to_high.msg_id =  IPC_MBOX_MSG_ID(IPC_MBOX_TX_MPDU_DONE);

					//Add the per-Tx-event details to the IPC message so CPU High can add them to the log as TX_LOW entries
					if(low_tx_details != NULL){
						ipc_msg_to_high.payload_ptr = (u32*)low_tx_details;

						//Make sure we don't overfill the IPC mailbox with TX_LOW data; truncate the Tx details if necessary
						if(low_tx_details_size < (MAILBOX_BUFFER_MAX_NUM_WORDS << 2)){
							ipc_msg_to_high.num_payload_words = ( low_tx_details_size ) >> 2; // # of u32 words
						} else {
							ipc_msg_to_high.num_payload_words = (((MAILBOX_BUFFER_MAX_NUM_WORDS << 2) / sizeof(wlan_mac_low_tx_details_t)) * sizeof(wlan_mac_low_tx_details_t)) >> 2; // # of u32 words
						}
					} else {
						ipc_msg_to_high.num_payload_words = 0;
						ipc_msg_to_high.payload_ptr = NULL;
					}
					ipc_msg_to_high.arg0 = tx_pkt_buf;
					write_mailbox_msg(&ipc_msg_to_high);
				}
			}
		break;
		// ---- Something went wrong - TX_READY message didn't match state of pkt buf ----
		default:
		case TX_PKT_BUF_LOW_CTRL:
            // CPU Low responsible for any LOW_CTRL buffers
            //  Don't transmit - just clean up and return
			tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
		case TX_PKT_BUF_UNINITIALIZED:
		case TX_PKT_BUF_DONE:
		case TX_PKT_BUF_HIGH_CTRL:
            //  CPU High will handle it eventually
            //  Ensure CPU Low doesn't own lock then return
            unlock_tx_pkt_buf(tx_pkt_buf);
		break;
	}
}



/*****************************************************************************/
/**
 * @brief Notify upper-level MAC of frame reception
 *
 * Sends an IPC message to the upper-level MAC to notify it that a frame has been
 * received and is ready to be processed
 *
 * @param   None
 * @return  None
 *
 * @note This function assumes it is called in the same context where rx_pkt_buf is still valid.
 */
void wlan_mac_low_frame_ipc_send(){
    wlan_ipc_msg_t ipc_msg_to_high;

    ipc_msg_to_high.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_RX_MPDU_READY);
    ipc_msg_to_high.num_payload_words = 0;
    ipc_msg_to_high.arg0              = rx_pkt_buf;

    write_mailbox_msg(&ipc_msg_to_high);
}



/*****************************************************************************/
/**
 * @brief Set Frame Reception Callback
 *
 * Tells the framework which function should be called when the PHY begins processing a frame reception
 *
 * @param   callback         - Pointer to callback function
 * @return  None
 */
inline void wlan_mac_low_set_frame_rx_callback(function_ptr_t callback){
    frame_rx_callback = callback;
}

inline void wlan_mac_low_set_sample_rate_change_callback(function_ptr_t callback){
	sample_rate_change_callback = callback;
}

inline void wlan_mac_low_set_beacon_txrx_config_callback(function_ptr_t callback){
	beacon_txrx_config_callback = callback;
}

inline void wlan_mac_low_set_mactime_change_callback(function_ptr_t callback){
	mactime_change_callback = callback;
}


/*****************************************************************************/
/**
 * @brief Set Frame Transmission Callback
 *
 * Tells the framework which function should be called when an MPDU is passed down from the
 * upper-level MAC for wireless transmission
 *
 * @param   callback         - Pointer to callback function
 * @return  None
 */
inline void wlan_mac_low_set_frame_tx_callback(function_ptr_t callback){
    frame_tx_callback = callback;
}



/*****************************************************************************/
/**
 * @brief Set IPC_MBOX_LOW_PARAM Callback
 *
 * Tells the framework which function should be called when an ipc message is received for
 * the IPC_MBOX_LOW_PARAM command.
 *
 * @param   callback         - Pointer to callback function
 * @return  None
 */
void wlan_mac_low_set_ipc_low_param_callback(function_ptr_t callback){
    ipc_low_param_callback = callback;
}



/*****************************************************************************/
/**
 * @brief Various Getter Methods
 *
 * These functions will get parameters from the low framework.
 *
 * @param   None
 * @return  (see individual function)
 */
inline u32 wlan_mac_low_get_active_channel(){
    return mac_param_chan;
}


inline s8 wlan_mac_low_get_current_ctrl_tx_pow(){
    return mac_param_ctrl_tx_pow;
}


inline u32 wlan_mac_low_get_current_rx_filter(){
    return mac_param_rx_filter;
}


inline phy_samp_rate_t wlan_mac_low_get_phy_samp_rate() {
    return gl_phy_samp_rate;
}



/*****************************************************************************/
/**
 * @brief Get the Rx Start Microsecond Timestamp
 *
 * This function returns the Rx start timestamp of the system
 *
 * @param   None
 * @return  u64              - microsecond timestamp
 */
inline u64 wlan_mac_low_get_rx_start_timestamp() {
    u32 timestamp_high_u32;
    u32 timestamp_low_u32;
    u64 timestamp_u64;

    // RX_START timestamp is captured once per reception - no race condition between 32-bit reads
    timestamp_high_u32 = Xil_In32(WLAN_MAC_REG_RX_TIMESTAMP_MSB);
    timestamp_low_u32 = Xil_In32(WLAN_MAC_REG_RX_TIMESTAMP_LSB);
    timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);

    return timestamp_u64;
}



/*****************************************************************************/
/**
 * @brief Get the Tx Start Microsecond Timestamp
 *
 * This function returns the Tx start timestamp of the system
 *
 * @param   None
 * @return  u64              - microsecond timestamp
 */
inline u64 wlan_mac_low_get_tx_start_timestamp() {

    u32 timestamp_high_u32;
    u32 timestamp_low_u32;
    u64 timestamp_u64;

    // TX_START timestamp is captured once per transmission - no race condition between 32-bit reads
    timestamp_high_u32 = Xil_In32(WLAN_MAC_REG_TX_TIMESTAMP_MSB);
    timestamp_low_u32 = Xil_In32(WLAN_MAC_REG_TX_TIMESTAMP_LSB);
    timestamp_u64 = (((u64)timestamp_high_u32)<<32) + ((u64)timestamp_low_u32);

    return timestamp_u64;
}



/*****************************************************************************/
/**
 * @brief Various Setter Methods
 *
 * These functions will set parameters in the low framework.
 *
 * @param   (see individual function)
 * @return  None
 */
void wlan_mac_low_set_nav_check_addr(u8* addr) {
    Xil_Out32(WLAN_MAC_REG_NAV_CHECK_ADDR_1, *((u32*)&(addr[0])) );
    Xil_Out32(WLAN_MAC_REG_NAV_CHECK_ADDR_2, *((u32*)&(addr[4])) );
}



/*****************************************************************************/
/**
 * @brief Calculates Rx Power (in dBm)
 *
 * This function calculates receive power for a given band, RSSI and LNA gain. This
 * provides a reasonable estimate of Rx power, accurate to a few dB for standard waveforms.
 *
 * This function does not use the VGA gain setting or I/Q magnitudes. The PHY should use these
 * to refine its own power measurement if needed.
 *
 * NOTE:  These lookup tables were developed as part of the RF characterization.  See:
 *     http://warpproject.org/trac/wiki/802.11/Benchmarks/Rx_Char
 *
 *
 * @param   rssi             - RSSI value from RF frontend
 * @param   lna_gain         - Value of LNA gain stage in RF frontend
 * @return  int              - Power in dBm
 */
const s8 pow_lookup_B24[128]  = {-90, -90, -89, -88, -88, -87, -87, -86, -86, -85, -84, -84, -83, -83, -82, -82,
                                 -81, -81, -80, -79, -79, -78, -78, -77, -77, -76, -75, -75, -74, -74, -73, -73,
                                 -72, -72, -71, -70, -70, -69, -69, -68, -68, -67, -66, -66, -65, -65, -64, -64,
                                 -63, -63, -62, -61, -61, -60, -60, -59, -59, -58, -58, -57, -56, -56, -55, -55,
                                 -54, -54, -53, -52, -52, -51, -51, -50, -50, -49, -49, -48, -47, -47, -46, -46,
                                 -45, -45, -44, -43, -43, -42, -42, -41, -41, -40, -40, -39, -38, -38, -37, -37,
                                 -36, -36, -35, -34, -34, -33, -33, -32, -32, -31, -31, -30, -29, -29, -28, -28,
                                 -27, -27, -26, -26, -25, -24, -24, -23, -23, -22, -22, -21, -20, -20, -19, -19};

const s8 pow_lookup_B5[128]   = {-97, -97, -96, -96, -95, -94, -94, -93, -93, -92, -92, -91, -90, -90, -89, -89,
                                 -88, -88, -87, -87, -86, -85, -85, -84, -84, -83, -83, -82, -81, -81, -80, -80,
                                 -79, -79, -78, -78, -77, -76, -76, -75, -75, -74, -74, -73, -72, -72, -71, -71,
                                 -70, -70, -69, -69, -68, -67, -67, -66, -66, -65, -65, -64, -63, -63, -62, -62,
                                 -61, -61, -60, -60, -59, -58, -58, -57, -57, -56, -56, -55, -54, -54, -53, -53,
                                 -52, -52, -51, -51, -50, -49, -49, -48, -48, -47, -47, -46, -45, -45, -44, -44,
                                 -43, -43, -42, -42, -41, -40, -40, -39, -39, -38, -38, -37, -36, -36, -35, -35,
                                 -34, -34, -33, -32, -32, -31, -31, -30, -30, -29, -29, -28, -27, -27, -26, -26};


inline int wlan_mac_low_calculate_rx_power(u16 rssi, u8 lna_gain){

    u8             band;
    int            power     = -100;
    u16            adj_rssi  = 0;

    band = mac_param_band;

    if(band == RC_24GHZ){
        switch(lna_gain){
            case 0:
            case 1:
                // Low LNA Gain State
                adj_rssi = rssi + (440 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 2:
                // Medium LNA Gain State
                adj_rssi = rssi + (220 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 3:
                // High LNA Gain State
                adj_rssi = rssi;
            break;
        }

        power = pow_lookup_B24[(adj_rssi >> (PHY_RX_RSSI_SUM_LEN_BITS+POW_LOOKUP_SHIFT))];

    } else if(band == RC_5GHZ){
        switch(lna_gain){
            case 0:
            case 1:
                // Low LNA Gain State
                adj_rssi = rssi + (540 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 2:
                // Medium LNA Gain State
                adj_rssi = rssi + (280 << PHY_RX_RSSI_SUM_LEN_BITS);
            break;

            case 3:
                // High LNA Gain State
                adj_rssi = rssi;
            break;
        }

        power = pow_lookup_B5[(adj_rssi >> (PHY_RX_RSSI_SUM_LEN_BITS+POW_LOOKUP_SHIFT))];
    }

    return power;
}



/*****************************************************************************/
/**
 * @brief Calculates RSSI from Rx power (in dBm)
 *
 * This function calculates receive power for a given band, RSSI and LNA gain. This
 * provides a reasonable estimate of Rx power, accurate to a few dB for standard waveforms.
 *
 * This function does not use the VGA gain setting or I/Q magnitudes. The PHY should use these
 * to refine its own power measurement if needed.
 *
 * NOTE:  These lookup tables were developed as part of the RF characterization.  See:
 *     http://warpproject.org/trac/wiki/802.11/Benchmarks/Rx_Char
 *
 *
 * @param   rx_pow           - Receive power in dBm
 * @return  u16              - RSSI value
 *
 * @note    rx_pow must be in the range [PKT_DET_MIN_POWER_MIN, PKT_DET_MIN_POWER_MAX] inclusive
 */
const u16 rssi_lookup_B24[61] = {  1,  16,  24,  40,  56,  72,  80,  96, 112, 128, 144, 152, 168, 184, 200, 208,
                                 224, 240, 256, 272, 280, 296, 312, 328, 336, 352, 368, 384, 400, 408, 424, 440,
                                 456, 472, 480, 496, 512, 528, 536, 552, 568, 584, 600, 608, 624, 640, 656, 664,
                                 680, 696, 712, 728, 736, 752, 768, 784, 792, 808, 824, 840, 856};

const u16 rssi_lookup_B5[61]  = { 96, 112, 128, 144, 160, 168, 184, 200, 216, 224, 240, 256, 272, 288, 296, 312,
                                 328, 344, 352, 368, 384, 400, 416, 424, 440, 456, 472, 480, 496, 512, 528, 544,
                                 552, 568, 584, 600, 608, 624, 640, 656, 672, 680, 696, 712, 728, 736, 752, 768,
                                 784, 800, 808, 824, 840, 856, 864, 880, 896, 912, 920, 936, 952};


int wlan_mac_low_rx_power_to_rssi(s8 rx_pow){
    u8 band;
    u16 rssi_val = 0;

    band = mac_param_band;

    if ((rx_pow <= PKT_DET_MIN_POWER_MAX) && (rx_pow >= PKT_DET_MIN_POWER_MIN)) {
        if(band == RC_24GHZ){
            rssi_val = rssi_lookup_B24[rx_pow-PKT_DET_MIN_POWER_MIN];
        } else if(band == RC_5GHZ){
            rssi_val = rssi_lookup_B5[rx_pow-PKT_DET_MIN_POWER_MIN];
        }

        return rssi_val;

    } else {
        return -1;
    }
}



/*****************************************************************************/
/**
 * @brief Set the minimum power for packet detection
 *
 * @param   rx_pow           - Receive power in dBm
 * @return  int              - Status:  0 - Success; -1 - Failure
 */
int wlan_mac_low_set_pkt_det_min_power(s8 rx_pow){
    int rssi_val;

    rssi_val = wlan_mac_low_rx_power_to_rssi(rx_pow);

    if(rssi_val != -1){
        wlan_phy_rx_pktDet_RSSI_cfg( (PHY_RX_RSSI_SUM_LEN-1), (rssi_val << PHY_RX_RSSI_SUM_LEN_BITS), 1);

        return  0;
    } else {
        return -1;
    }
}



/*****************************************************************************/
/**
 * @brief Search for and Lock Empty Packet Buffer (Blocking)
 *
 * This is a blocking function for finding and locking an empty rx packet buffer. The low framework
 * calls this function after passing a new wireless reception up to CPU High for processing. CPU High
 * must unlock Rx packet buffers after processing the received packet. This function loops over all Rx
 * packet buffers until it finds one that has been unlocked by CPU High.
 *
 * By design this function prints a message if it fails to unlock the oldest packet buffer. When this
 * occurs it indicates that CPU Low has outrun CPU High, a situation that leads to dropped wireless
 * receptions with high probability. The node recovers gracefully from this condition and will
 * continue processing new Rx events after CPU High catches up. But seeing this message in the UART
 * for CPU Low is a strong indicator the CPU High code is not keeping up with wireless receptions.
 *
 * @param   None
 * @return  None
 *
 * @note    This function assumes it is called in the same context where rx_pkt_buf is still valid.
 */
inline void wlan_mac_low_lock_empty_rx_pkt_buf(){
    // This function blocks until it safely finds a packet buffer for the PHY RX to store a future reception
    rx_frame_info_t* rx_frame_info;
    u32 i = 1;

    while(1) {
    	//rx_pkt_buf is the global shared by all contexts which deal with wireless Rx
    	// Rx packet buffers are used in order. Thus incrementing rx_pkt_buf should
    	// select the "oldest" packet buffer, the one that is most likely to have already
    	// been processed and released by CPU High
    	rx_pkt_buf = (rx_pkt_buf+1) % NUM_RX_PKT_BUFS;
    	rx_frame_info    = (rx_frame_info_t*) RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

        if((rx_frame_info->rx_pkt_buf_state) == RX_PKT_BUF_LOW_CTRL){
			// By default Rx pkt buffers are not zeroed out, to save the performance penalty of bzero'ing 2KB
			//     However zeroing out the pkt buffer can be helpful when debugging Rx MAC/PHY behaviors
			// bzero((void *)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), 2048);

			// Set the OFDM and DSSS PHYs to use the same Rx pkt buffer
			wlan_phy_rx_pkt_buf_ofdm(rx_pkt_buf);
			wlan_phy_rx_pkt_buf_dsss(rx_pkt_buf);

			if (i > 1) { xil_printf("found in %d iterations.\n", i); }

			return;

        }

        if (i == 1) { xil_printf("Searching for empty packet buff ... "); }
        i++;
    }
}


/*****************************************************************************/
/**
 * @brief Finish PHY Reception
 *
 * This function polls the MAC status register until the Rx PHY goes idle. The
 * return value indicates whether the just-completed reception was good
 * (no Rx errors and matching checksum) or bad
 *
 * @param   None
 * @return  u32              - FCS status (RX_MPDU_STATE_FCS_GOOD or RX_MPDU_STATE_FCS_BAD)
 */
inline u32 wlan_mac_hw_rx_finish() {
    u32 mac_hw_status;
    int i = 0;

    // Wait for the packet to finish - Rx PHY is "active" if the demod/decoding pipeline is
    //  still writing bytes to the packet buffer. The FCS result is not known until the last
    //  payload byte has been written. The RX_PHY_ACTIVE bit might de-assert before this happens
    //  for some lengths/rates/bandwidths. This allows RX_END to start the SIFS timer at the right
    //  time independent of payload-specific PHY latencies.
    do{
        mac_hw_status = wlan_mac_get_status();
        if(i++>1000000) {xil_printf("Stuck in wlan_mac_hw_rx_finish! 0x%08x\n", mac_hw_status);}
    } while(mac_hw_status & (WLAN_MAC_STATUS_MASK_RX_PHY_ACTIVE | WLAN_MAC_STATUS_MASK_RX_PHY_WRITING_PAYLOAD));

    // Check RX_END_ERROR and FCS
    if( (mac_hw_status & WLAN_MAC_STATUS_MASK_RX_FCS_GOOD) &&
       ((mac_hw_status & WLAN_MAC_STATUS_MASK_RX_END_ERROR) == 0)) {
        return 1;

    } else {
        return 0;

    }
}



/*****************************************************************************/
/**
 * @brief Force reset backoff counter in MAC hardware
 */
inline void wlan_mac_reset_backoff_counter() {
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) | WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF);
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_A_BACKOFF);
}



/*****************************************************************************/
/**
 * @brief Force reset NAV counter in MAC hardware
 */
inline void wlan_mac_reset_NAV_counter() {
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) | WLAN_MAC_CTRL_MASK_RESET_NAV);
    Xil_Out32(WLAN_MAC_REG_CONTROL, Xil_In32(WLAN_MAC_REG_CONTROL) & ~WLAN_MAC_CTRL_MASK_RESET_NAV);
}



/*****************************************************************************/
/**
 * @brief Convert dBm to Tx Gain Target
 *
 * This function maps a transmit power (in dBm) to a radio gain target.
 *
 * @param   s8 power         - Power in dBm
 * @return  u8 gain_target   - Gain target in range of [0,63]
 */
inline u8 wlan_mac_low_dbm_to_gain_target(s8 power){
    s8 power_railed;
    u8 return_value;

    if(power > TX_POWER_MAX_DBM){
        power_railed = TX_POWER_MAX_DBM;
    } else if( power < TX_POWER_MIN_DBM){
        power_railed = TX_POWER_MIN_DBM;
    } else {
        power_railed = power;
    }

    // This is only save because 'power' is constrained to less than half the dynamic range of an s8 type
    return_value = (u8)((power_railed << 1) + 20);

    return return_value;
}



/*****************************************************************************/
/**
 * @brief Map the WLAN channel frequencies onto the convention used by the radio controller
 */
inline u32 wlan_mac_low_wlan_chan_to_rc_chan(u32 mac_channel) {
    int return_value = 0;

    switch(mac_channel){
        // 2.4GHz channels
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            return_value = mac_channel;
        break;

        // 5GHz channels
        case 36: // 5180 MHz
            return_value = 1;
        break;
        case 38: // 5190 MHz
            return_value = 2;
        break;
        case 40: // 5200 MHz
            return_value = 3;
        break;
        case 44: // 5220 MHz
            return_value = 4;
        break;
        case 46: // 5230 MHz
            return_value = 5;
        break;
        case 48: // 5240 MHz
            return_value = 6;
        break;
#if 0 // Disable these channels by default
        case 52: // 5260 MHz
            return_value = 7;
        break;
        case 54: // 5270 MHz
            return_value = 8;
        break;
        case 56: // 5280 MHz
            return_value = 9;
        break;
        case 60: // 5300 MHz
            return_value = 10;
        break;
        case 62: // 5310 MHz
            return_value = 11;
        break;
        case 64: // 5320 MHz
            return_value = 12;
        break;
        case 100: // 5500 MHz
            return_value = 13;
        break;
        case 102: // 5510 MHz
            return_value = 14;
        break;
        case 104: // 5520 MHz
            return_value = 15;
        break;
        case 108: // 5540 MHz
            return_value = 16;
        break;
        case 110: // 5550 MHz
            return_value = 17;
        break;
        case 112: // 5560 MHz
            return_value = 18;
        break;
        case 116: // 5580 MHz
            return_value = 19;
        break;
        case 118: // 5590 MHz
            return_value = 20;
        break;
        case 120: // 5600 MHz
            return_value = 21;
        break;
        case 124: // 5620 MHz
            return_value = 22;
        break;
        case 126: // 5630 MHz
            return_value = 23;
        break;
        case 128: // 5640 MHz
            return_value = 24;
        break;
        case 132: // 5660 MHz
            return_value = 25;
        break;
        case 134: // 5670 MHz
            return_value = 26;
        break;
        case 136: // 5680 MHz
            return_value = 27;
        break;
        case 140: // 5700 MHz
            return_value = 28;
        break;
        case 142: // 5710 MHz
            return_value = 29;
        break;
        case 144: // 5720 MHz
            return_value = 30;
        break;
        case 149: // 5745 MHz
            return_value = 31;
        break;
        case 151: // 5755 MHz
            return_value = 32;
        break;
        case 153: // 5765 MHz
            return_value = 33;
        break;
        case 157: // 5785 MHz
            return_value = 34;
        break;
        case 159: // 5795 MHz
            return_value = 35;
        break;
        case 161: // 5805 MHz
            return_value = 36;
        break;
        case 165: // 5825 MHz
            return_value = 37;
        break;
        case 172: // 5860 MHz
            return_value = 38;
        break;
        case 174: // 5870 MHz
        	return_value = 39;
		break;
        case 175: // 5875 MHz
        	return_value = 40;
		break;
        case 176: // 5880 MHz
        	return_value = 41;
		break;
        case 177: // 5885 MHz
        	return_value = 42;
		break;
        case 178: // 5890 MHz
        	return_value = 43;
		break;
#endif
    }

    return return_value;
}



/*****************************************************************************/
/**
 * @brief Convert MCS to number of data bits per symbol
 */
inline u16 wlan_mac_low_mcs_to_n_dbps(u8 mcs, u8 phy_mode) {

	if(phy_mode == PHY_MODE_NONHT && mcs < (sizeof(mcs_to_n_dbps_nonht_lut)/sizeof(mcs_to_n_dbps_nonht_lut[0]))) {
		return mcs_to_n_dbps_nonht_lut[mcs];
	} else if(phy_mode == PHY_MODE_HTMF && mcs < (sizeof(mcs_to_n_dbps_htmf_lut)/sizeof(mcs_to_n_dbps_htmf_lut[0]))) {
		return mcs_to_n_dbps_htmf_lut[mcs];
	} else {
		xil_printf("ERROR (wlan_mac_low_mcs_to_n_dbps): Invalid PHY_MODE (%d) or MCS (%d)\n", phy_mode, mcs);
		return 1; // N_DBPS used as denominator, so better not return 0
	}
}



/*****************************************************************************/
/**
 * @brief Convert MCS to Control Response MCS
 */
inline u8 wlan_mac_low_mcs_to_ctrl_resp_mcs(u8 mcs, u8 phy_mode){
    // Returns the fastest NON-HT half-rate MCS lower than the provided MCS and no larger that 24Mbps.
	//  Valid return values are [0, 2, 4]
    u8 return_value = 0;

    if(phy_mode == PHY_MODE_NONHT){
    	return_value = mcs;
		if(return_value > 4){ return_value = 4; }
		if(return_value % 2){ return_value--;   }
    } else if(phy_mode == PHY_MODE_HTMF) {
    	switch(mcs){
    		default:
    		case 0:
    			return_value = 0;
			break;
    		case 1:
    			return_value = 2;
			break;
    		case 2:
    			return_value = 2;
			break;
    		case 3:
    		case 4:
    		case 5:
    		case 6:
    		case 7:
    			return_value = 4;
			break;
    	}
    }
    return return_value;
}

inline u64 wlan_mac_low_get_unique_seq(){
	return unique_seq;
}

inline void wlan_mac_low_set_unique_seq(u64 curr_unique_seq){
	unique_seq = curr_unique_seq;
}

inline void wlan_mac_hw_clear_rx_started() {
	wlan_mac_reset_rx_started(1);
	wlan_mac_reset_rx_started(0);

	return;
}
