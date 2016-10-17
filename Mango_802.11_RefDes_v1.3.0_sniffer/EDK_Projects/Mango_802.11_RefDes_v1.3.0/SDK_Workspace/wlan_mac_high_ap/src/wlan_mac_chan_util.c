/*
 * wlan_mac_chan_util.c
 *
 *  Created on: 12 jul 2016
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
#include "wlan_mac_misc_util.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_high.h"
#include "wlan_mac_ap.h"

//////////// Variables ///////////////

extern volatile u8	ap_mode;
extern volatile u8  sort_mode;
extern          u32 mac_param_chan;
extern			u64 time_elapsed;
extern			mac_chan_state chan_state;

u64 time_timer;

const static u16 mcs_to_n_dbps[WLAN_MAC_NUM_MCS] = {24, 36, 48, 72, 96, 144, 192, 216};
const static u32 wlan_mac_channel[WLAN_CHANNELS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48};

//////////// Initialization Functions ////////////

void wlan_mac_high_init_mpdu_info(rx_mpdu_info* mpdu_info){
	mpdu_info->num_elem = 0;
}

void wlan_mac_high_init_chan_info(mac_chan_info* chan_info){		// This function may be simplified
    chan_info->num_users = 0;
    chan_info->num_elem  = 0;
    u8 counter;
    u8 num_elem;
    for(counter = 0; counter < MAX_NUM_USER; counter++) {
    	chan_info->user_entries[counter].avg_mpdus = 0;
    	chan_info->user_entries[counter].avg_duration = 0;
    	chan_info->user_entries[counter].last_power = 0;
    	chan_info->user_entries[counter].tot_mpdus = 0;
        for(num_elem = 0; num_elem < 3; num_elem++) {
            chan_info->user_entries[counter].num_mpdu[num_elem] = 0;
            chan_info->user_entries[counter].duration[num_elem] = 0;
        }

    }
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

void wlan_mac_high_init_chan_state(mac_chan_state* chan_state){
	chan_state->elem         = 0;
	chan_state->chan_util[0] = 0;
	chan_state->chan_util[1] = 0;
	chan_state->chan_util[2] = 0;
	chan_state->curr_state  = WLAN_CHAN_UTIL_IDLE;
}

void wlan_mac_high_reset_chan_info(mac_chan_info* chan_info){
    u8 counter;
    for(counter = 0; counter < MAX_NUM_USER; counter++){
        chan_info->user_entries[counter].avg_mpdus = 0;
        chan_info->user_entries[counter].avg_duration = 0;
    }
}

void wlan_mac_high_refresh_chan_info(mac_chan_info* chan_info){
    u8 counter;
    u16 num_users = chan_info->num_users;
    for(counter = 0; counter < num_users; counter++) {
        chan_info->user_entries[counter].num_mpdu[chan_info->num_elem] = 0;
        chan_info->user_entries[counter].duration[chan_info->num_elem] = 0;
    }
}

void wlan_mac_high_refresh_chan_data(mac_chan_data* chan_data, u32 mac_param_chan){
	u8 mac_chan_idx = get_mac_channel_index(mac_param_chan);
	chan_data->num_mpdu[mac_chan_idx][0] = 0;
}

///////// Processing Functions ///////////////

void capture_mpdu(void* pkt_buf_addr, rx_mpdu_info* rx_info, mac_chan_info* chan_info, mac_chan_data* chan_data, u8 ap_mode){
	rx_frame_info*      mpdu_info                = (rx_frame_info*)pkt_buf_addr;
	void*               mpdu                     = (u8*)pkt_buf_addr + PHY_RX_PKT_BUF_MPDU_OFFSET;
	u8*                 mpdu_ptr_u8              = (u8*)mpdu;
	mac_header_80211*   rx_80211_header          = (mac_header_80211*)((void *)mpdu_ptr_u8);

	//  The sniff mode must be enabled
	if(ap_mode != ALL_DISABLED) {
		extract_mpdu_info(rx_info, mpdu_info, rx_80211_header);
		if(rx_info->num_elem == MAX_NUM_MPDU){
			switch(ap_mode){
				case SNIFF_ENABLED:
					set_mac_chan_info(rx_info, chan_info);
				break;

				case SCAN_ENABLED:
					set_mac_chan_data(rx_info, chan_data, mac_param_chan);
				break;
			}

		}
	}
}

void extract_mpdu_info(rx_mpdu_info* rx_info, rx_frame_info* mpdu_info, mac_header_80211* rx_header) {

	u32 tx_time;
	u16 length;
	u8  phy_mode;
	u8  mcs;
	u8 ch;
	u8 ch_band;

	// Store modulation and coding scheme
	mcs = mpdu_info->phy_details.mcs;

	// Length of the received mpdu in bytes
	length = mpdu_info->phy_details.length;

	phy_mode = mpdu_info->phy_details.phy_mode;

	// Duration of the data mpdu, in usec
	// get_mpdu_duration may have the field mpdu_info->phy_details.N_DBPS as input instead of mcs
	tx_time = get_mpdu_duration(length, mcs, phy_mode);
	rx_info->mpdu_entries[rx_info->num_elem].tx_time = tx_time;

	ch = mpdu_info->channel;
	ch_band = get_channel_band(ch);

	if(phy_mode != PHY_RX_MODE_DSSS){
		if(ch_band == WLAN_24G_BAND){
			tx_time += T_DIFS_PHY_OFDM_24GHZ;
		}else if(ch_band == WLAN_5G_BAND){
			tx_time += T_DIFS_PHY_OFDM_5GHZ;
		}
	}else{
		tx_time += T_DIFS_PHY_DSSS;
	}

	if((rx_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA){
		if(ch_band == WLAN_24G_BAND){
			tx_time += (T_SIFS_PHY_OFDM_24GHZ + T_ACK);
		} else if (ch_band == WLAN_5G_BAND){
			tx_time += (T_SIFS_PHY_OFDM_5GHZ + T_ACK);
		}
	}

	rx_info->mpdu_entries[rx_info->num_elem].duration = tx_time;

	// Store the power value for the rx mpdu in dBm
	rx_info->mpdu_entries[rx_info->num_elem].power = mpdu_info->rx_power;

	if(rx_header->frame_control_1 == MAC_FRAME_CTRL1_SUBTYPE_BEACON){
		rx_info->mpdu_entries[rx_info->num_elem].is_beacon = 1;
	} else {
		rx_info->mpdu_entries[rx_info->num_elem].is_beacon = 0;
	}

	// Source MAC address to identify the device which transmitted the sensed packet
	memcpy(&((rx_info->mpdu_entries[rx_info->num_elem].src_addr).addr[0]),&(rx_header->address_2[0]), 6 * sizeof(u8));

	// Set the FCS value, Good = 1, Bad = 0
	if(mpdu_info->state == RX_MPDU_STATE_FCS_GOOD){
		rx_info->mpdu_entries[rx_info->num_elem].fcs = 1;
	}else{
		rx_info->mpdu_entries[rx_info->num_elem].fcs = 0;
	}
	rx_info->num_elem++;
}

u32	get_mpdu_duration(u16 length, u8 mcs, u8 phy_mode){
	u16 num_syms;
	u16 n_dbps;
	u16 num_payload_bits;
	u32 tx_time_payload;
	u32 tx_time;

	if(phy_mode == PHY_RX_MODE_11AG){
		num_payload_bits = (16 + 8*length + 6);
		n_dbps = mcs_to_n_dbps[mcs];
		num_syms = num_payload_bits / n_dbps;
		if(num_syms * n_dbps != num_payload_bits) {
			num_syms++;
		}
		tx_time_payload = TXTIME_T_SYM * (u32)num_syms;
		tx_time = (TXTIME_T_PREAMBLE + TXTIME_T_SIGNAL + tx_time_payload + T_SIG_EXT);
	}else{//(phy_mode == PHY_RX_MODE_DSSS && mcs == 0x3F)
		tx_time_payload = ((u32)8*length);
		tx_time = TXTIME_DSSS_PLCP_PREAMBLE + TXTIME_DSSS_PLCP_HEADER + tx_time_payload;
	}
	return 	tx_time;
}

void set_mac_chan_data(rx_mpdu_info* rx_info, mac_chan_data* chan_data, u32 mac_channel){
	u8 mac_chan_idx = get_mac_channel_index(mac_channel);
	chan_data->num_mpdu[mac_chan_idx][0] += rx_info->num_elem;
	chan_data->num_mpdu[mac_chan_idx][1] += rx_info->num_elem;
	//Add here sth else to refresh or to do average.
	//chan_data->num_elem = (chan_data->num_elem + 1) % MAX_NUM_SCAN;
	wlan_mac_high_init_mpdu_info(rx_info);
}

void print_chan_data(mac_chan_data* chan_data, u32 chan_sel){
	u16 idx1;
	u16 idx2;
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

}

void scan_traffic_sweep(rx_mpdu_info* rx_info, mac_chan_data* chan_data, u32 chan_sel){
	ap_mode = ALL_DISABLED;
	if(rx_info->num_elem != 0){
		set_mac_chan_data(rx_info, chan_data, mac_param_chan);
	}
	print_chan_data(chan_data, chan_sel);
	int mac_chan_idx = get_mac_channel_index(mac_param_chan);
	mac_chan_idx = (mac_chan_idx + 1) % WLAN_CHANNELS;
	mac_param_chan = wlan_mac_channel[mac_chan_idx];
	wlan_mac_high_set_channel( mac_param_chan );
	wlan_mac_high_refresh_chan_data(chan_data, mac_param_chan);
	xil_printf("Scanning channel %d...\n", mac_param_chan);
	xil_printf("\n");
	xil_printf("[r] - reset statistics\n");
	xil_printf("[w] - increase channel\n");
	xil_printf("[s] - decrease channel\n");
	xil_printf("[c] - confirm channel selection\n");
	ap_mode = SCAN_ENABLED;
}

void set_mac_chan_info(rx_mpdu_info* mpdu_info, mac_chan_info* chan_info){
    int idx;
    int index;
    u16 counter;
    u16 num_users = chan_info->num_users;
    mac_addr user_mac;
    u8 bad_fcs_mac [6] = {0, 0, 0, 0, 0, 0};

    for(counter = 0; counter < mpdu_info->num_elem; counter++){
    	// Bad FCS MPDUs stored at 00:00:00:00:00:00 MAC address
    	if(mpdu_info->mpdu_entries[counter].fcs == 1){
			memcpy(&(user_mac.addr[0]), &((mpdu_info->mpdu_entries[counter].src_addr).addr[0]), 6 * sizeof(u8));
    	} else {
    		memcpy(&(user_mac.addr[0]), &(bad_fcs_mac[0]), 6 * sizeof(u8));
    	}

    	idx = is_known_mac(&(user_mac.addr[0]), &(chan_info->user_entries[0]), num_users);

		if(idx != -1) {
			index = idx;
		} else {
			index = num_users;
			memcpy(&((chan_info->user_entries[num_users].user_mac).addr[0]), &(user_mac.addr[0]), 6 * sizeof(u8));
			chan_info->user_entries[num_users].is_ap = 0;
			num_users++;
		}
		if(mpdu_info->mpdu_entries[counter].is_beacon == 1){
			chan_info->user_entries[index].is_ap = 1;
		}
		chan_info->user_entries[index].num_mpdu[chan_info->num_elem]++;
		// Cumulative MPDUs for each user
		chan_info->user_entries[index].tot_mpdus++;
		chan_info->user_entries[index].duration[chan_info->num_elem] += mpdu_info->mpdu_entries[counter].duration;
		chan_info->user_entries[index].last_power = mpdu_info->mpdu_entries[counter].power;

    }
    chan_info->num_users = num_users;
    wlan_mac_high_init_mpdu_info(mpdu_info);
}

void set_mac_chan_state(mac_chan_state* chan_state){
	u8  counter;
	u32 chan_util = 0;

	for(counter = 0; counter < WLAN_CHAN_UTIL_NUM_STATES; counter++){
		chan_util += chan_state->chan_util[counter];
	}

	chan_util = chan_util / WLAN_CHAN_UTIL_NUM_STATES;

	if(chan_util <= WLAN_TRAFFIC_THRS_IDLE){
		chan_state->curr_state = WLAN_CHAN_UTIL_IDLE;
	}else if(chan_util <= WLAN_TRAFFIC_THRS_LIGHT){
		chan_state->curr_state = WLAN_CHAN_UTIL_LIGHT;
	}else{
		chan_state->curr_state = WLAN_CHAN_UTIL_HEAVY;
	}
}

int is_known_mac(u8* addr, user_entry* users, u16 num_users){
	u16 idx;
	for(idx = 0; idx < num_users; idx++){
		if(wlan_addr_eq(addr, &((users[idx].user_mac).addr[0]))){
			return idx;
		}
	}
	return -1;
}

void sort_users(mac_chan_info* chan_info, u8 sort_mode) {
    u8 counter1;
    u8 counter2;
    u32 metric1;
    u32 metric2;
    u16 num_user;
    user_entry user_aux;

    if(chan_info->num_users == 0){
    	return;
    }

    num_user = chan_info->num_users - 1;

    for(counter1 = num_user; counter1 > 0; counter1--) {
        for(counter2 = 0; counter2 < counter1; counter2++) {
        	if(sort_mode == SORT_MPDUS_MODE){
        		metric1 = get_tot_mpdu(&(chan_info->user_entries[counter2]));
        		metric2 = get_tot_mpdu(&(chan_info->user_entries[counter2+1]));
        	} else {
        		metric1 = chan_info->user_entries[counter2].last_power;
        		metric2 = chan_info->user_entries[counter2+1].last_power;
        	}
        	if(metric1 < metric2) {
                copy_user_entry(chan_info->user_entries[counter2], &user_aux);
                copy_user_entry(chan_info->user_entries[counter2 + 1], &(chan_info->user_entries[counter2]));
                copy_user_entry(user_aux, &(chan_info->user_entries[counter2 + 1]));
            }
        }
    }
}

void remove_inactive_users(mac_chan_info* chan_info){
	u16 counter;
	for(counter = 0; counter < chan_info->num_users; counter++) {
		if(chan_info->user_entries[counter].num_mpdu[0] == 0 &&
		   chan_info->user_entries[counter].num_mpdu[1] == 0 &&
	       chan_info->user_entries[counter].num_mpdu[2] == 0 ) {
			remove_user_at_index(chan_info, counter);
	        counter -= 1;
	    }
	}
}

void remove_user_at_index(mac_chan_info* chan_info, u16 index) {
	u16 counter;
    for(counter = index + 1; counter < chan_info->num_users; counter++) {
    	copy_user_entry(chan_info->user_entries[counter], &(chan_info->user_entries[counter - 1]));
    }
    if(chan_info->num_users > 0){
        chan_info->user_entries[chan_info->num_users - 1].tot_mpdus = 0;
        chan_info->user_entries[chan_info->num_users - 1].num_mpdu[0] = 0;
        chan_info->user_entries[chan_info->num_users - 1].num_mpdu[1] = 0;
        chan_info->user_entries[chan_info->num_users - 1].num_mpdu[2] = 0;
    }else{
        chan_info->user_entries[0].tot_mpdus = 0;
        chan_info->user_entries[0].num_mpdu[0] = 0;
        chan_info->user_entries[0].num_mpdu[1] = 0;
        chan_info->user_entries[0].num_mpdu[2] = 0;
    }
    chan_info->num_users--;
}

void copy_user_entry(const user_entry in, user_entry* out){
    u8 index;
    memcpy(&(out->user_mac.addr[0]), &(in.user_mac.addr[0]), 6 * sizeof(u8));
    out->avg_mpdus = in.avg_mpdus;
    out->avg_duration = in.avg_duration;
    out->last_power = in.last_power;
    out->tot_mpdus  = in.tot_mpdus;
    out->is_ap = in.is_ap;
    for(index = 0; index < 3; index++) {
        out->num_mpdu[index] = in.num_mpdu[index];
        out->duration[index] = in.duration[index];
    }
}

u32 get_tot_mpdu(user_entry* user){
	u8	index;
	u32 sum = 0;
	for(index = 0; index < 3; index++) {
		sum += user->num_mpdu[index];
	}
	return sum;
}

void get_avg_mpdu(user_entry* user) {
    u8 index;
    u32 sum = 0;
    u32 res;
    for(index = 0; index < 3; index++) {
        sum += user->num_mpdu[index];
    }

    res = (u32) (sum / 3);
    if(res * 3 != sum){
    	res++;
    }
    user->avg_mpdus = res;
}

void get_avg_duration(user_entry* user){
	u8 index;
	u32 sum = 0;
	u32 res;
	for(index = 0; index < 3; index++){
		sum += user->duration[index];
	}

	res = (u32) (sum / 3);
	if(res * 3 != sum){
		res++;
	}
	user->avg_duration = res;
}

void get_mac_chan_util(rx_mpdu_info* mpdu_info, mac_chan_info* chan_info, u32 mac_chan) {


	// Sniff mode is paused until the channel utilization is processed
    ap_mode = ALL_DISABLED;

    // If the structure rx_mpdu_info has elements on it, process them and store
    // in chan_info
    if(mpdu_info->num_elem > 0) {
        set_mac_chan_info(mpdu_info,chan_info);     // Update statistics to chan_info
    }

    u16 counter;
    for(counter = 0; counter < chan_info->num_users; counter++){
        get_avg_mpdu(&(chan_info->user_entries[counter]));
        get_avg_duration(&(chan_info->user_entries[counter]));
    }

    // This line commented for the case of measurement analysis and verification
    remove_inactive_users(chan_info);

    // Sort users either by traffic or rx power level
    sort_users(chan_info, sort_mode);

    // Prints the channel utilization menu with the list of users and the MPDUs per second
    print_chan_util(mac_chan, chan_info, time_elapsed);
    // Reset chan_info to zero average MPDUs per user
    wlan_mac_high_reset_chan_info(chan_info);
    //
    chan_info->num_elem = (chan_info->num_elem + 1) % 3;
    // Reset the set of num_mpus[chan_info->num_elem]
    wlan_mac_high_refresh_chan_info(chan_info);

    // Sniff mode is again enabled
    ap_mode = SNIFF_ENABLED;

}

void print_chan_util(u32 mac_channel, mac_chan_info* chan_info, u64 time_elapsed){
    u16 counter;                                // Counter to loop over the users
    mac_addr user_mac;                          // User MAC address
    u8 bad_fcs_mac [6] = {0, 0, 0, 0, 0, 0};
    u16 user_per_sec = chan_info->num_users; 	// Number of users per second
    u32 tot_mpdu = 0;
    u32 tot_duration = 0;
    int idx;

    idx = is_known_mac(&(bad_fcs_mac[0]), &(chan_info->user_entries[0]), user_per_sec);

    xil_printf("\f");
    xil_printf("-------------------------Channel Utilization-------------------------\n");
    xil_printf("Channel:            %d\n", mac_channel);
    if(idx != -1) {
    	xil_printf("Users/sec:          %d\n", user_per_sec - 1);
    } else {
    	xil_printf("Users/sec:          %d\n", user_per_sec);
    }

    // Prints the list of users and the MPDUs per second
    xil_printf("---------------------------------------------------------------------\n");
    xil_printf("User MAC\t\t MPDUs/sec\t Time(us)\t Power(dBm)\t Tot MPDUs\n");
    for(counter = 0; counter < user_per_sec; counter++) {
        memcpy(&(user_mac.addr[0]), &((chan_info->user_entries[counter].user_mac).addr[0]), 6 * sizeof(u8));
        tot_mpdu += chan_info->user_entries[counter].avg_mpdus;
        tot_duration += chan_info->user_entries[counter].avg_duration;

        if(counter != idx) {

        	xil_printf("%02x:%02x:%02x:%02x:%02x:%02x",
        			user_mac.addr[0], user_mac.addr[1], user_mac.addr[2],
        			user_mac.addr[3], user_mac.addr[4], user_mac.addr[5]);
        	if(chan_info->user_entries[counter].is_ap == 1){
        		xil_printf("(*)");
        	}
        	xil_printf("\t %d\t\t %d\t\t %d\t\t %d\n",
        	           chan_info->user_entries[counter].avg_mpdus,
        	           chan_info->user_entries[counter].avg_duration,
        	           chan_info->user_entries[counter].last_power,
        	           chan_info->user_entries[counter].tot_mpdus);
        }
    }

    if(idx != -1) {
    	xil_printf("Bad FCS\t\t\t %d\t\t %d\t\t %d\t\t %d\n",
    			   chan_info->user_entries[idx].avg_mpdus,
    			   chan_info->user_entries[idx].avg_duration,
    			   chan_info->user_entries[idx].last_power,
    			   chan_info->user_entries[idx].tot_mpdus);
    }

    float chan_util    = (float) (100 * tot_duration) / time_elapsed;
    u32 chan_util_fix  = (u32) chan_util;
    u32 chan_util_frac = (u32) (chan_util * 10000 - ((float)chan_util_fix) * 10000);

    chan_state.chan_util[chan_state.elem] = chan_util_fix;
    chan_state.elem = (chan_state.elem + 1) % WLAN_CHAN_UTIL_NUM_STATES;
    set_mac_chan_state(&chan_state);

    xil_printf("---------------------------------------------------------------------\n");
    xil_printf("Total:\t\t\t %d\t\t %d\n", tot_mpdu, tot_duration);
    xil_printf("---------------------------------------------------------------------\n");
    xil_printf("Time elapsed:\t\t\t\t %d us\n", time_elapsed);
    xil_printf("Channel utilization:\t\t\t %d.%04d%%\n", chan_util_fix, chan_util_frac);
    xil_printf("---------------------------------------------------------------------\n");
    xil_printf("(*) Access Point\n\n");

    xil_printf("[m] - sort users by number of MPDUs per second (by default)\n");
    xil_printf("[p] - sort users by received power level\n");
    xil_printf("[r] - reset channel utilization statistics\n\n");

    if(chan_state.curr_state == WLAN_CHAN_UTIL_IDLE){
    	xil_printf("Ch state: IDLE\n");
    }else if(chan_state.curr_state == WLAN_CHAN_UTIL_LIGHT){
    	xil_printf("Ch state: LIGHT TRAFFIC\n");
    }else{
    	xil_printf("Ch states: HEAVY TRAFFIC\n");
    }
}

u8 get_mac_channel_index(u32 mac_channel){
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

u8 get_channel_band(u32 mac_param_chan){
	u8 chan_idx = get_mac_channel_index(mac_param_chan);
	if(chan_idx >= 0 && chan_idx < 11){ 				// 2.4 GHz band
		return WLAN_24G_BAND;
	} else if(chan_idx >= 11 && chan_idx < 15) {		// 5 GHz band
		return WLAN_5G_BAND;
	}
	return WLAN_INVALID_BAND;
}
