/*
 * wlan_mac_chan_util.c
 *
 *  Created on: 7 jul 2016
 *      Author: egagali
 */

//Xilinx SDK includes
#include <math.h>
#include "stdlib.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xuartlite.h"
#include "xaxicdma.h"
#include "malloc.h"

#include "wlan_mac_chan_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ap.h"


/*********************** Global Variable Definitions *************************/

volatile	u8	 					ap_mode;
volatile	u8  					chan_mode;

			u64 					time_elapsed;

			mac_chan_data			chan_data;
			mac_chan_state_t        chan_state;

extern   	u8 						mac_param_chan;

/*************************** Variable Definitions ****************************/

static volatile u32 ch_busy_time;
static volatile u32 num_mpdu;

// Parameters for OFDM PHY, HT and NONHT MPDUs
const static u16 mcs_to_n_dbps_nonht[WLAN_MAC_NUM_MCS] = {24, 36, 48, 72, 96, 144, 192, 216};
const static u16 mcs_to_n_dbps_htmf[WLAN_MAC_NUM_MCS] = {26, 52, 78, 104, 156, 208, 234, 260};

// Channels

const static u8 wlan_mac_channel[WLAN_CHANNELS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48};
const static u8	wlan_chan_dual_mode[2] = {36, 48};

	  static u8 wlan_chan_dual_idx = 0;

/*************************** Functions Prototypes ****************************/

u32  wlan_calc_dsss_mpdu_txtime(u16 length, u8 data_rate, dsss_plcp_format format);
u16  wlan_calc_num_payload_syms(u16 length, u16 n_dbps, u8 mcs, u8 phy_mode);
u32  wlan_calc_ofdm_mpdu_txtime(u16 length, u16 n_dbps, u8 mcs, u8 phy_mode, u8 phy_samp_rate);

/******************************** Functions **********************************/

void wlan_mac_high_init_chan_util(){

	// The AP is initialized to its normal operation:
	// - It does not capture any MPDU
	// - It does not change its operation channel based on the channel state
	ap_mode   = ALL_DISABLED;
	chan_mode = WLAN_CHAN_ALLOC_DISABLED;

	// Initializes the structure of captured MPDUs per channel
	wlan_mac_high_init_chan_data(&chan_data);

	// Initializes the channel state to IDLE traffic
	wlan_mac_high_init_chan_state(&chan_state);
	wlan_mac_high_init_ch_busy_time();
	wlan_mac_high_init_num_mpdu();

}

void wlan_mac_high_init_ch_busy_time(){
	ch_busy_time = 0;
}

void wlan_mac_high_init_num_mpdu(){
	num_mpdu = 0;
}

void wlan_mac_high_init_chan_data(mac_chan_data* chan_data){
	u8 i;
	u8 j;
	chan_data->num_elem = 0;
	for(i = 0; i < WLAN_CHANNELS; i++){
		for(j = 0; j < MAX_NUM_SCAN; j++){
			chan_data->num_mpdu[i][j] = 0;
		}
	}
}

void wlan_mac_high_init_chan_state(mac_chan_state_t* chan_state){
	chan_state->elem         = 0;
	chan_state->chan_util[0] = 0;
	chan_state->chan_util[1] = 0;
	chan_state->chan_util[2] = 0;
	chan_state->curr_state  = CH_TRAFFIC_IDLE;
}

u16  wlan_calc_num_payload_syms(u16 length, u16 ndbps, u8 mcs, u8 phy_mode){

	u16 num_payload_syms;
	u32 num_payload_bits;
	u16 n_dbps;

	// Payload is composed by:
	// 16-bit SERVICE field
	// 'length' byte MAC payload
	// 6-bit TAIL field
	num_payload_bits = 16 + (8 * length) + 6;

	if(mcs >= 0 || mcs < WLAN_MAC_NUM_MCS){
		// Number of data bits per symbol from MCS
		if(phy_mode == PHY_MODE_NONHT){
			n_dbps = mcs_to_n_dbps_nonht[mcs];
	//	}else if(phy_mode == PHY_MODE_HTMF){
		}else{
			n_dbps = mcs_to_n_dbps_htmf[mcs];
		}
	}else{
		n_dbps = ndbps;
	}

	// Number of payload ofdm symbols is calculated by ceil(payload_length_bits / num_bits_per_ofdm_sym)
	num_payload_syms = num_payload_bits / n_dbps;
	if((num_payload_syms * n_dbps) != num_payload_bits){
		num_payload_syms++;
	}

	return num_payload_syms;
}

u32  wlan_calc_ofdm_mpdu_txtime(u16 length, u16 n_dbps, u8 mcs, u8 phy_mode, u8 phy_samp_rate){

	u16 num_ht_preamble_syms, num_payload_syms;

	u16 t_preamble;
	u16 t_sym;			// OFDM symbol interval

	u16 t_ext = 6;			// Signal extention

	// Set OFDM symbol duration in microseconds; only depends on PHY sampling rate
	switch(phy_samp_rate) {
		case PHY_RATE_40M:
			t_sym = 2;
	    break;

	    default:
	    case PHY_RATE_20M:
	    	t_sym = 4;
	    break;

	    case PHY_RATE_10M:
	        t_sym = 8;
	    break;
	}

	// PHY preamble common to NONHT and HTMF waveforms consists of 5 OFDM symbols
	//  4 symbols for STF/LTF
	//  1 symbol for SIGNAL/L-SIG
	t_preamble = 5 * t_sym;

	// Only HTMF waveforms have HT-SIG, HT-STF and HT-LTF symbols
	if(phy_mode == PHY_MODE_HTMF) {
		num_ht_preamble_syms = 4;
	} else {
	    num_ht_preamble_syms = 0;
	}

	num_payload_syms = wlan_calc_num_payload_syms(length, n_dbps, mcs, phy_mode);

	return (t_preamble + (t_sym * (num_ht_preamble_syms + num_payload_syms)) + t_ext);
}

u32  wlan_calc_dsss_mpdu_txtime(u16 length, u8 data_rate, dsss_plcp_format format){

	u16 tx_time_plcp_preamble;
	u16 tx_time_plcp_header;
	u16 tx_time_payload;

	// First tx time for plcp preamble
	if(format == DSSS_LONG) {
		tx_time_plcp_preamble = 144;
		tx_time_plcp_header   = 48;	// Fixed value for long format (usec)
	} else {
		tx_time_plcp_preamble = 72;
		tx_time_plcp_header   = 24;			// Fixed value for short format (usec)
	}

	//tx_time_payload = wlan_calc_dsss_payload_tx_time(len, data_rate);
	tx_time_payload = length * 8;

	return (tx_time_plcp_preamble + tx_time_plcp_header + tx_time_payload);

}

void capture_mpdu(void* pkt_buf_addr, u8 ap_mode){
	rx_frame_info_t*    mpdu_info                = (rx_frame_info_t*)pkt_buf_addr;
	void*               mpdu                     = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mpdu_ptr_u8              = (u8*)mpdu;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mpdu_ptr_u8);

	if(ap_mode != ALL_DISABLED) {
		extract_mpdu_info_reduced(mpdu_info, rx_80211_header);
		num_mpdu++;
	}
}

void extract_mpdu_info_reduced(rx_frame_info_t* mpdu_info, mac_header_80211* rx_header){

	u16 length;
	u16 n_dbps;
	u8	mcs;
	u8	phy_mode;
	u8	phy_samp_rate;
	u8  wlan_chan;
	u8  wlan_ch_band;
	u32 tx_time;
	u32 t_ack;

	length 	 	  = mpdu_info->phy_details.length;
	n_dbps 		  = mpdu_info->phy_details.N_DBPS;
	mcs	   		  = mpdu_info->phy_details.mcs;
	phy_mode 	  = mpdu_info->phy_details.phy_mode;
	phy_samp_rate = mpdu_info->phy_samp_rate;
	wlan_chan	  = mpdu_info->channel;

	wlan_ch_band  = get_channel_band(wlan_chan);

	// Transmission time calculation
	switch(phy_mode){
		default:
		case PHY_MODE_HTMF:
		case PHY_MODE_NONHT:
			tx_time = wlan_calc_ofdm_mpdu_txtime(length, n_dbps, mcs, phy_mode, phy_samp_rate);
		break;
		case PHY_MODE_DSSS:
			tx_time = wlan_calc_dsss_mpdu_txtime(length, WLAN_MAC_DSSS_1M, DSSS_LONG);
		break;
	}

	if(phy_mode != PHY_MODE_DSSS){
		if(phy_mode == PHY_MODE_HTMF){
			t_ack = T_ACK_HT;
		}else{
			t_ack = T_ACK_NONHT;
		}
		if(wlan_ch_band == WLAN_24G_BAND){
			tx_time += T_DIFS_PHY_OFDM_24GHZ;
		}else{
			tx_time += T_DIFS_PHY_OFDM_5GHZ;
		}
	}else{
		tx_time += T_DIFS_PHY_DSSS;
		t_ack = 0;
	}

	if((rx_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA){
		if(wlan_ch_band == WLAN_24G_BAND){
			tx_time += (T_SIFS_PHY_OFDM_24GHZ + t_ack);
		} else if (wlan_ch_band == WLAN_5G_BAND){
			tx_time += (T_SIFS_PHY_OFDM_5GHZ + t_ack);
		}
	}

	ch_busy_time += tx_time;

}

void set_mac_chan_state(mac_chan_state_t* chan_state){

	if((chan_state->chan_util[0] <= WLAN_TRAFFIC_THRS_IDLE) &&
	   (chan_state->chan_util[1] <= WLAN_TRAFFIC_THRS_IDLE) &&
	   (chan_state->chan_util[2] <= WLAN_TRAFFIC_THRS_IDLE)){
		chan_state->curr_state = CH_TRAFFIC_IDLE;
	}else if((chan_state->chan_util[0] <= WLAN_TRAFFIC_THRS_LIGHT) &&
			 (chan_state->chan_util[1] <= WLAN_TRAFFIC_THRS_LIGHT) &&
			 (chan_state->chan_util[2] <= WLAN_TRAFFIC_THRS_LIGHT)){
		chan_state->curr_state = CH_TRAFFIC_LIGHT;
	}else if((chan_state->chan_util[0] > WLAN_TRAFFIC_THRS_LIGHT) &&
			 (chan_state->chan_util[1] > WLAN_TRAFFIC_THRS_LIGHT) &&
			 (chan_state->chan_util[2] > WLAN_TRAFFIC_THRS_LIGHT)){
		chan_state->curr_state = CH_TRAFFIC_HEAVY;
	}
}

void get_mac_chan_util_reduced(){

	bss_config_t bss_config;

	ap_mode = ALL_DISABLED;

	float chan_util    = (float) (100 * ch_busy_time) / time_elapsed;
	u32 chan_util_fix  = (u32) chan_util;
	u32 chan_util_frac = (u32) (chan_util * 10000 - ((float)chan_util_fix) * 10000);

	xil_printf("\f");
	xil_printf("-------------------------------------------------------\n");
	xil_printf("Time elapsed:\t\t\t\t %d us\n", time_elapsed);
	xil_printf("Channel utilization:\t\t\t %d.%04d%%\n", chan_util_fix, chan_util_frac);
	xil_printf("Channel:\t\t\t\t %d\n", mac_param_chan);
	xil_printf("-------------------------------------------------------\n");

	chan_state.chan_util[chan_state.elem] = chan_util_fix;
	chan_state.elem = (chan_state.elem + 1) % WLAN_CHAN_UTIL_STATES;
	set_mac_chan_state(&chan_state);

	wlan_mac_high_init_ch_busy_time();

	// Debug
	xil_printf("CH UTIL: %d, %d, %d\n", chan_state.chan_util[0], chan_state.chan_util[1], chan_state.chan_util[2]);
	xil_printf("Channel Status: ");
	if(chan_state.curr_state == CH_TRAFFIC_HEAVY){
		xil_printf("CH_TRAFFIC_HEAVY\n");
	}else if(chan_state.curr_state == CH_TRAFFIC_LIGHT){
		xil_printf("CH_TRAFFIC_LIGHT\n");
	}else{
		xil_printf("CH_TRAFFIC_IDLE\n");
	}

	if(chan_mode == WLAN_CHAN_ALLOC_ENABLED && chan_state.curr_state == CH_TRAFFIC_HEAVY){
		wlan_chan_dual_idx = (wlan_chan_dual_idx + 1) % 2;
		mac_param_chan = wlan_chan_dual_mode[wlan_chan_dual_idx];
		bss_config.chan_spec.chan_pri = mac_param_chan;
		bss_config.chan_spec.chan_type = CHAN_TYPE_BW20;
		bss_config.update_mask = BSS_FIELD_MASK_CHAN;
		configure_bss(&bss_config);
		wlan_mac_high_init_chan_state(&chan_state);
	}
	ap_mode = SNIFF_ENABLED;
}

void scan_traffic_sweep(mac_chan_data* chan_data, u8 chan_sel){

//	bss_config_t				bss_config; // Only if we advertise the BSS

	u8 mac_chan_idx;

	ap_mode = ALL_DISABLED;

	mac_chan_idx = get_mac_channel_index(mac_param_chan);
	chan_data->num_mpdu[mac_chan_idx][0] += num_mpdu;
	chan_data->num_mpdu[mac_chan_idx][1] += num_mpdu;

	print_chan_data(chan_data, chan_sel);

	mac_chan_idx = (mac_chan_idx + 1) % WLAN_CHANNELS;
	mac_param_chan = wlan_mac_channel[mac_chan_idx];
	wlan_mac_high_set_radio_channel( mac_param_chan );

	// Instead line above, we can update the channel by there following lines
	// Attention, the AP new update will be advertised
//	bss_config.chan_spec.chan_pri = *mac_param_chan;
//	bss_config.chan_spec.chan_type = CHAN_TYPE_BW20;
//	bss_config.update_mask = BSS_FIELD_MASK_CHAN;
//	configure_bss(&bss_config);

	chan_data->num_mpdu[mac_chan_idx][0] = 0;
	wlan_mac_high_init_num_mpdu();

	ap_mode = SCAN_ENABLED;
}

void print_chan_data(mac_chan_data* chan_data, u8 chan_sel){
	u8 chan_idx;
	u8 idx1;
	u8 idx2;
	xil_printf("\f");
	xil_printf("---------------TRAFFIC PER CHANNEL--------------\n");
	xil_printf("-----------(MPDUs/sec)------(Total MPDUs)-------\n");
	for(idx1 = 0; idx1 < WLAN_CHANNELS; idx1++){
		if(wlan_mac_channel[idx1] == chan_sel){
			xil_printf("CH %d*\t|", wlan_mac_channel[idx1]);
		}else{
			xil_printf("CH %d\t|", wlan_mac_channel[idx1]);
		}

		for(idx2 = 0; idx2 < MAX_NUM_SCAN; idx2++){
			xil_printf("\t%d\t|", chan_data->num_mpdu[idx1][idx2]);
		}
		xil_printf("\n------------------------------------------\n");
	}

	chan_idx = get_mac_channel_index(mac_param_chan);
	chan_idx = (chan_idx + 1) % WLAN_CHANNELS;

	xil_printf("Scanning channel %d...\n", wlan_mac_channel[chan_idx]);
	xil_printf("\n");
	xil_printf("[r] - reset statistics\n");
	xil_printf("[w] - increase channel\n");
	xil_printf("[s] - decrease channel\n");
	xil_printf("[c] - confirm channel selection\n");
}

u8 get_channel_band(u8 mac_param_chan){
	u8 chan_idx = get_mac_channel_index(mac_param_chan);
	if(chan_idx >= 0 && chan_idx < 11){ 				// 2.4 GHz band
		return WLAN_24G_BAND;
	} else if(chan_idx >= 11 && chan_idx < 15) {		// 5 GHz band
		return WLAN_5G_BAND;
	}
	return WLAN_INVALID_BAND;
}

u8 get_mac_channel_index(u8 mac_channel){
	u8 returned_value;
	if(mac_channel > 0 && mac_channel < 12){	//2.4 GHz
		returned_value = mac_channel - 1;
	}else{										//5 GHz
		switch(mac_channel){
			case 36:
				returned_value = 11;
			break;
			case 40:
				returned_value = 12;
			break;
			case 44:
				returned_value = 13;
			break;
			case 48:
				returned_value = 14;
			break;
			default:
				returned_value = 0;
			break;
		}
	}
	return returned_value;
}
