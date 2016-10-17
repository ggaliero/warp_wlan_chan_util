/** @file wlan_mac_ap_uart_menu.c
 *  @brief Access Point UART Menu
 *
 *  This contains code for the 802.11 Access Point's UART menu.
 *
 *  @copyright Copyright 2013-2015, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *				See LICENSE.txt included in the design archive or
 *				at http://mangocomm.com/802.11/license
 *
 *  @author Chris Hunter (chunter [at] mangocomm.com)
 *  @author Patrick Murphy (murphpo [at] mangocomm.com)
 *  @author Erik Welsh (welsh [at] mangocomm.com)
 */

//Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdlib.h"
#include "xtmrctr.h"
#include "xio.h"
#include "string.h"
#include "xintc.h"

//WARP includes
#include "wlan_mac_ipc_util.h"
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ap.h"
#include "ascii_characters.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_chan_util.h"

#ifndef WLAN_USE_UART_MENU

void uart_rx(u8 rxByte){ };

#else

#define MAX_NUM_CHARS                       31


extern tx_params                            default_unicast_data_tx_params;

extern bss_info*                            my_bss_info;
extern dl_list                              statistics_table;

extern rx_mpdu_info							mpdu_data;
extern mac_chan_info						chan_info;
extern mac_chan_data						chan_data;
extern mac_chan_state						chan_state;
extern volatile u8							ap_mode;
extern volatile u8							sort_mode;
extern u64									time_elapsed;

extern u32                                  mac_param_chan;

static volatile u32							mac_chan_sel;
static volatile u32							mac_chan_prev;

static volatile u8                          uart_mode            = UART_MODE_MAIN;
static volatile u32                         schedule_ID;
static volatile u8                          print_scheduled      = 0;
static volatile u8							chan_util_scheduled  = 0;
static volatile u8							sweep_scheduled      = 0;

static          ltg_pyld_all_assoc_fixed    traffic_blast_pyld;
static          ltg_sched_periodic_params   traffic_blast_sched;
static volatile u32                         traffic_blast_ltg_id = LTG_ID_INVALID;

static u64									time_start;
static u32								    channels[WLAN_CHANNELS] = 	 {1, 2, 3, 4,
																		  5, 6, 7, 8,
																		  9, 10, 11, 36,
																		  40, 44, 48};

void uart_rx(u8 rxByte){

	dl_entry* 	                curr_station_info_entry;
	station_info*               curr_station_info;

	void*                       ltg_state;

	char                        text_entry[MAX_NUM_CHARS+1];
	u8                          curr_char = 0;

	u8							chan_idx;

	if(rxByte == ASCII_ESC){
		uart_mode = UART_MODE_MAIN;
		ap_mode   = ALL_DISABLED;
		sort_mode = SORT_MPDUS_MODE;
		stop_periodic_print();
		stop_periodic_util();
		stop_periodic_channel_sweep();
		print_menu();
		ltg_sched_remove(LTG_REMOVE_ALL);
		traffic_blast_ltg_id = LTG_ID_INVALID;
		return;
	}

	switch(uart_mode){
		case UART_MODE_MAIN:
			switch(rxByte){
				case ASCII_1:
					uart_mode = UART_MODE_INTERACTIVE;
					start_periodic_print();
				break;

				case ASCII_2:
					print_queue_status();
				break;

				case ASCII_3:
					print_all_observed_statistics();
				break;

				case ASCII_4:
					mac_chan_prev = mac_param_chan;
					mac_chan_sel  = channels[0];
					uart_mode = UART_MODE_INTERACTIVE_SCAN;
					wlan_mac_high_init_chan_data(&chan_data);
					mac_param_chan = channels[0];
					wlan_mac_high_set_channel( mac_param_chan );
					start_periodic_channel_sweep();
				break;

				case ASCII_5:
					if(ap_mode == ALL_DISABLED){
						// Structures are initialized inside the if, so if key 4 is pushed
						// twice, nothing happens
						wlan_mac_high_init_mpdu_info(&mpdu_data);
						wlan_mac_high_init_chan_info(&chan_info);
						wlan_mac_high_init_chan_state(&chan_state);
						//wlan_mac_high_init_chan_util(&chan_util);
						uart_mode = UART_MODE_INTERACTIVE_SORT;
						ap_mode = SNIFF_ENABLED;
					}
					start_periodic_util();
				break;

				case ASCII_e:
			        event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
					print_event_log_size();
#ifdef _DEBUG_
			        print_event_log( 0xFFFF );
					print_event_log_size();
#endif
			        event_log_config_logging(EVENT_LOG_LOGGING_ENABLE);
				break;

				case ASCII_c:
					if(mac_param_chan > channels[0] && ap_mode == ALL_DISABLED){
						deauthenticate_stations();
						chan_idx = get_mac_channel_index(mac_param_chan);
						(chan_idx--);
						mac_param_chan = channels[chan_idx];

						if(my_bss_info != NULL){
							my_bss_info->chan = mac_param_chan;
						}

						//Send a message to other processor to tell it to switch channels
						wlan_mac_high_set_channel( mac_param_chan );
					} else {

					}
					xil_printf("(-) Channel: %d\n", mac_param_chan);


				break;
				case ASCII_C:
					if(mac_param_chan < channels[14] && ap_mode == ALL_DISABLED){
						deauthenticate_stations();
						chan_idx = get_mac_channel_index(mac_param_chan);
						(chan_idx++);
						mac_param_chan = channels[chan_idx];

						if(my_bss_info != NULL){
							my_bss_info->chan = mac_param_chan;
						}

						//Send a message to other processor to tell it to switch channels
						wlan_mac_high_set_channel( mac_param_chan );
					} else {

					}
					xil_printf("(+) Channel: %d\n", mac_param_chan);

				break;
				case ASCII_g:
					if((default_unicast_data_tx_params.phy.power) > TX_POWER_MIN_DBM){
						(default_unicast_data_tx_params.phy.power)--;
					} else {
						(default_unicast_data_tx_params.phy.power) = TX_POWER_MIN_DBM;
					}

					xil_printf("(-) Default Tx Power: %d dBm\n", (default_unicast_data_tx_params.phy.power));

				break;
				case ASCII_G:
					if((default_unicast_data_tx_params.phy.power) < TX_POWER_MAX_DBM){
						(default_unicast_data_tx_params.phy.power)++;
					} else {
						(default_unicast_data_tx_params.phy.power) = TX_POWER_MAX_DBM;
					}

					xil_printf("(+) Default Tx Power: %d dBm\n", (default_unicast_data_tx_params.phy.power));

				break;
				case ASCII_r:
					if((default_unicast_data_tx_params.phy.rate) > WLAN_MAC_MCS_6M){
						(default_unicast_data_tx_params.phy.rate)--;
					} else {
						(default_unicast_data_tx_params.phy.rate) = WLAN_MAC_MCS_6M;
					}

					curr_station_info_entry = my_bss_info->associated_stations.first;
					while(curr_station_info_entry != NULL){
						curr_station_info = (station_info*)(curr_station_info_entry->data);
						curr_station_info->tx.phy.rate = (default_unicast_data_tx_params.phy.rate);
						curr_station_info_entry = dl_entry_next(curr_station_info_entry);
					}

					xil_printf("(-) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps((default_unicast_data_tx_params.phy.rate)));
				break;
				case ASCII_R:
					if((default_unicast_data_tx_params.phy.rate) < WLAN_MAC_MCS_54M){
						(default_unicast_data_tx_params.phy.rate)++;
					} else {
						(default_unicast_data_tx_params.phy.rate) = WLAN_MAC_MCS_54M;
					}

					curr_station_info_entry = my_bss_info->associated_stations.first;
					while(curr_station_info_entry != NULL){
						curr_station_info = (station_info*)(curr_station_info_entry->data);
						curr_station_info->tx.phy.rate = (default_unicast_data_tx_params.phy.rate);
						curr_station_info_entry = dl_entry_next(curr_station_info_entry);
					}
					xil_printf("(+) Default Unicast Rate: %d Mbps\n", wlan_lib_mac_rate_to_mbps((default_unicast_data_tx_params.phy.rate)));
				break;
				case ASCII_s:
					uart_mode = UART_MODE_SSID_CHANGE;
					deauthenticate_stations();
					curr_char = 0;
					print_ssid_menu();
				break;
				case ASCII_m:
					wlan_mac_high_display_mallinfo();
				break;
			}
		break;

		case UART_MODE_INTERACTIVE_SCAN:
			switch(rxByte){
				case ASCII_r:
					wlan_mac_high_init_chan_data(&chan_data);
					mac_param_chan = channels[0];
				break;
				case ASCII_w:
					chan_idx = get_mac_channel_index(mac_chan_sel);
					chan_idx = (chan_idx + 1) % WLAN_CHANNELS;
					mac_chan_sel = channels[chan_idx];
				break;

				case ASCII_s:
					chan_idx = get_mac_channel_index(mac_chan_sel);
					chan_idx = (chan_idx + WLAN_CHANNELS - 1) % WLAN_CHANNELS;
					mac_chan_sel = channels[chan_idx];
				break;

				case ASCII_c:
					ap_mode = ALL_DISABLED;
					uart_mode = UART_MODE_INTERACTIVE_SORT;
					stop_periodic_channel_sweep();
					mac_param_chan = mac_chan_sel;
					mac_chan_sel = channels[0];
					wlan_mac_high_set_channel( mac_param_chan );
					// Advertise BSS
					if(my_bss_info != NULL){
						my_bss_info->chan = mac_param_chan;
					}
					wlan_mac_high_init_mpdu_info(&mpdu_data);
					wlan_mac_high_init_chan_info(&chan_info);
					ap_mode = SNIFF_ENABLED;
					start_periodic_util();
				break;
			}
		break;

		case UART_MODE_INTERACTIVE_SORT:
			switch(rxByte){
				case ASCII_m:
					sort_mode = SORT_MPDUS_MODE;
					xil_printf("Users sorted by number of MPDUs\n");
				break;
				case ASCII_p:
					sort_mode = SORT_POWER_MODE;
					xil_printf("Users sorted by power level\n");
				break;
				case ASCII_r:
					wlan_mac_high_init_chan_info(&chan_info);
				break;
			}
		break;

		case UART_MODE_INTERACTIVE:
			switch(rxByte){
				case ASCII_r:
					//Reset statistics
					reset_station_statistics();
				break;
				case ASCII_d:
					//Deauthenticate all stations
					deauthenticate_stations();
				break;

				case ASCII_b:
					//Toggle Traffic Blaster

					//First, we'll see if an LTG has been created and create a new one if not.

					if(traffic_blast_ltg_id == LTG_ID_INVALID){
						//Set up and start
						traffic_blast_pyld.hdr.type = LTG_PYLD_TYPE_ALL_ASSOC_FIXED;
						traffic_blast_pyld.length = 1400;
						traffic_blast_sched.duration_count = LTG_DURATION_FOREVER;
						traffic_blast_sched.interval_count = 0;

						traffic_blast_ltg_id = ltg_sched_create(LTG_SCHED_TYPE_PERIODIC, &traffic_blast_sched, &traffic_blast_pyld, NULL);

						if(traffic_blast_ltg_id == LTG_ID_INVALID){
							xil_printf("Error in creating LTG\n");
							break;
						}

					}

					//Second, we'll check to see if this LTG ID is currently running. If we just created it in the above,
					//this check isn't necessary (we know it's not running). We'll ask anyway so that we have fewer cases
					//to deal with.

					ltg_sched_get_state(traffic_blast_ltg_id, NULL, &ltg_state);

					//Note: Here I have the luxury of knowing what type ltg_state is. In general, the second argument
					//to ltg_sched_get_state can be used to figure out what type to cast ltg_state to

					switch(((ltg_sched_periodic_state*)ltg_state)->hdr.enabled){
						case 0:
							//LTG is not running, so let's start it
							ltg_sched_start(traffic_blast_ltg_id);
						break;

						case 1:
							//LTG is running, so let's stop it
							ltg_sched_stop(traffic_blast_ltg_id);
						break;
					}
				break;

			}
		break;

		case UART_MODE_SSID_CHANGE:
			switch(rxByte){
				case ASCII_CR:


					text_entry[curr_char] = 0;
					curr_char = 0;
					uart_mode = UART_MODE_MAIN;

					strcpy(my_bss_info->ssid,text_entry);
					xil_printf("\nSetting new SSID: %s\n", my_bss_info->ssid);

				break;
				case ASCII_DEL:
					if(curr_char > 0){
						curr_char--;
						xil_printf("\b \b");
					}

				break;
				default:
					if( (rxByte <= ASCII_z) && (rxByte >= ASCII_A) ){
						//the user entered a character

						if(curr_char < MAX_NUM_CHARS){
							xil_printf("%c", rxByte);
							text_entry[curr_char] = rxByte;
							curr_char++;
						}
					}
				break;
			}
		break;
	}
}

void print_ssid_menu(){
	xil_printf("\f");
	xil_printf("Current SSID: %s\n", my_bss_info->ssid);
	xil_printf("To change the current SSID, please type a new string and press enter\n");
	xil_printf(": ");
}


void print_queue_status(){
	dl_entry* curr_entry;
	station_info* curr_station_info;
	xil_printf("\nQueue Status:\n");
	xil_printf(" FREE || MCAST|");

	curr_entry = my_bss_info->associated_stations.first;
	while(curr_entry != NULL){
		curr_station_info = (station_info*)(curr_entry->data);
		xil_printf("%6d|", curr_station_info->AID);
		curr_entry = dl_entry_next(curr_entry);
	}
	xil_printf("\n");

	xil_printf("%6d||%6d|",queue_num_free(),queue_num_queued(MCAST_QID));

	curr_entry = my_bss_info->associated_stations.first;
	while(curr_entry != NULL){
		curr_station_info = (station_info*)(curr_entry->data);
		xil_printf("%6d|", queue_num_queued(AID_TO_QID(curr_station_info->AID)));
		curr_entry = dl_entry_next(curr_entry);
	}
	xil_printf("\n");

}

void print_menu(){
	xil_printf("\f");
	xil_printf("****************** AP Menu/Sniffer ******************\n");
	xil_printf("[1] - Interactive AP Status\n");
	xil_printf("[2] - Print Queue Status\n");
	xil_printf("[3] - Print all Observed Statistics\n");
	xil_printf("[4] - Scan channel traffic\n");
	xil_printf("[5] - Channel utilization menu\n");
	xil_printf("\n");
	xil_printf("[c/C] - change channel (note: changing channel will\n");
	xil_printf("        purge any associations, forcing stations to\n");
	xil_printf("        join the network again)\n");
	xil_printf("[r/R] - change default unicast rate\n");
	xil_printf("[s]   - change SSID (note: changing SSID will purge)\n");
	xil_printf("        any associations)\n");
	xil_printf("*****************************************************\n");
}





void print_station_status(){

//	xil_printf("print_station_status\n");

//#if 0
	station_info* curr_station_info;
	dl_entry*	  curr_entry;

	u64 timestamp;

	if(uart_mode == UART_MODE_INTERACTIVE){
		timestamp = get_usec_timestamp();
		xil_printf("\f");
		//xil_printf("next_free_assoc_index = %d\n", next_free_assoc_index);

		curr_entry = my_bss_info->associated_stations.first;

		while(curr_entry != NULL){
			curr_station_info = (station_info*)(curr_entry->data);
			xil_printf("---------------------------------------------------\n");
			if(curr_station_info->hostname[0] != 0){
				xil_printf(" Hostname: %s\n", curr_station_info->hostname);
			}
			xil_printf(" AID: %02x -- MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", curr_station_info->AID,
					curr_station_info->addr[0],curr_station_info->addr[1],curr_station_info->addr[2],curr_station_info->addr[3],curr_station_info->addr[4],curr_station_info->addr[5]);

			xil_printf("     - Last heard from         %d ms ago\n",((u32)(timestamp - (curr_station_info->latest_activity_timestamp)))/1000);
			xil_printf("     - Last Rx Power:          %d dBm\n",curr_station_info->rx.last_power);
			xil_printf("     - # of queued MPDUs:      %d\n", queue_num_queued(AID_TO_QID(curr_station_info->AID)));
			xil_printf("     - # Tx High Data MPDUs:   %d (%d successful)\n", curr_station_info->stats->data.tx_num_packets_total, curr_station_info->stats->data.tx_num_packets_success);
			xil_printf("     - # Tx High Data bytes:   %d (%d successful)\n", (u32)(curr_station_info->stats->data.tx_num_bytes_total), (u32)(curr_station_info->stats->data.tx_num_bytes_success));
			xil_printf("     - # Tx Low Data MPDUs:    %d\n", curr_station_info->stats->data.tx_num_packets_low);
			xil_printf("     - # Tx High Mgmt MPDUs:   %d (%d successful)\n", curr_station_info->stats->mgmt.tx_num_packets_total, curr_station_info->stats->mgmt.tx_num_packets_success);
			xil_printf("     - # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(curr_station_info->stats->mgmt.tx_num_bytes_total), (u32)(curr_station_info->stats->mgmt.tx_num_bytes_success));
			xil_printf("     - # Tx Low Mgmt MPDUs:    %d\n", curr_station_info->stats->mgmt.tx_num_packets_low);
			xil_printf("     - # Rx Data MPDUs:        %d\n", curr_station_info->stats->data.rx_num_packets);
			xil_printf("     - # Rx Data Bytes:        %d\n", curr_station_info->stats->data.rx_num_bytes);
			xil_printf("     - # Rx Mgmt MPDUs:        %d\n", curr_station_info->stats->mgmt.rx_num_packets);
			xil_printf("     - # Rx Mgmt Bytes:        %d\n", curr_station_info->stats->mgmt.rx_num_bytes);

			curr_entry = dl_entry_next(curr_entry);

		}

			xil_printf("---------------------------------------------------\n");
			xil_printf("\n");
			xil_printf("[r] - reset statistics\n");
			xil_printf("[d] - deauthenticate all stations\n\n");

	}

//
}

void print_all_observed_statistics(){
	statistics_txrx* curr_statistics;
	dl_entry* curr_statistics_entry;

	curr_statistics_entry = statistics_table.first;

	xil_printf("\nAll Statistics:\n");
	while(curr_statistics_entry != NULL){
		curr_statistics = (statistics_txrx*)(curr_statistics_entry->data);

		xil_printf("---------------------------------------------------\n");
		xil_printf("%02x:%02x:%02x:%02x:%02x:%02x\n", curr_statistics->addr[0],curr_statistics->addr[1],curr_statistics->addr[2],curr_statistics->addr[3],curr_statistics->addr[4],curr_statistics->addr[5]);
		xil_printf("     - Last timestamp:         %d usec\n", (u32)curr_statistics->latest_txrx_timestamp);
		xil_printf("     - Associated?             %d\n", curr_statistics->is_associated);
		xil_printf("     - # Tx High Data MPDUs:   %d (%d successful)\n", curr_statistics->data.tx_num_packets_total, curr_statistics->data.tx_num_packets_success);
		xil_printf("     - # Tx High Data bytes:   %d (%d successful)\n", (u32)(curr_statistics->data.tx_num_bytes_total), (u32)(curr_statistics->data.tx_num_bytes_success));
		xil_printf("     - # Tx Low Data MPDUs:    %d\n", curr_statistics->data.tx_num_packets_low);
		xil_printf("     - # Tx High Mgmt MPDUs:   %d (%d successful)\n", curr_statistics->mgmt.tx_num_packets_total, curr_statistics->mgmt.tx_num_packets_success);
		xil_printf("     - # Tx High Mgmt bytes:   %d (%d successful)\n", (u32)(curr_statistics->mgmt.tx_num_bytes_total), (u32)(curr_statistics->mgmt.tx_num_bytes_success));
		xil_printf("     - # Tx Low Mgmt MPDUs:    %d\n", curr_statistics->mgmt.tx_num_packets_low);
		xil_printf("     - # Rx Data MPDUs:        %d\n", curr_statistics->data.rx_num_packets);
		xil_printf("     - # Rx Data Bytes:        %d\n", curr_statistics->data.rx_num_bytes);
		xil_printf("     - # Rx Mgmt MPDUs:        %d\n", curr_statistics->mgmt.rx_num_packets);
		xil_printf("     - # Rx Mgmt Bytes:        %d\n", curr_statistics->mgmt.rx_num_bytes);

		curr_statistics_entry = dl_entry_next(curr_statistics_entry);
	}
}

void start_periodic_print(){
	stop_periodic_print();
	print_station_status();
	print_scheduled = 1;
	schedule_ID = wlan_mac_schedule_event_repeated(SCHEDULE_COARSE, 1000000, SCHEDULE_REPEAT_FOREVER, (void*)print_station_status);
}

void stop_periodic_print(){
	if(print_scheduled){
		print_scheduled = 0;
		wlan_mac_remove_schedule(SCHEDULE_COARSE, schedule_ID);
	}
}

void scan_traffic(){
	scan_traffic_sweep(&mpdu_data, &chan_data, mac_chan_sel);
}

void start_periodic_channel_sweep(){
	stop_periodic_channel_sweep();
	sweep_scheduled = 1;
	ap_mode = SCAN_ENABLED;
	schedule_ID = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, 1000000, SCHEDULE_REPEAT_FOREVER, (void*)scan_traffic);
}

void stop_periodic_channel_sweep(){
	if(sweep_scheduled){
		sweep_scheduled = 0;
		wlan_mac_remove_schedule(SCHEDULE_FINE, schedule_ID);

		if(uart_mode != UART_MODE_INTERACTIVE_SORT){
			mac_param_chan = mac_chan_prev;
			wlan_mac_high_set_channel( mac_param_chan );
		}
	}
	// consider to add ALL_DISABLED ap mode here
}

void chan_util(){
	get_mac_chan_util(&mpdu_data,&chan_info,mac_param_chan);
	u64 time_end = get_usec_timestamp();
	time_elapsed = time_end - time_start;
	time_start = time_end;
}

void start_periodic_util(){
	stop_periodic_util();
	chan_util_scheduled = 1;
	//chan_util();
	time_start = get_usec_timestamp();
	schedule_ID = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, 1000000, SCHEDULE_REPEAT_FOREVER, (void*)chan_util);
}

void stop_periodic_util(){
	if(chan_util_scheduled){
		chan_util_scheduled = 0;
		wlan_mac_remove_schedule(SCHEDULE_FINE, schedule_ID);
	}
}

int is_qwerty_row(u8 rxByte){
	int return_value = 0;

	switch(rxByte){
		case ASCII_Q:
		case ASCII_W:
		case ASCII_E:
		case ASCII_R:
		case ASCII_T:
		case ASCII_Y:
		case ASCII_U:
		case ASCII_I:
		case ASCII_O:
		case ASCII_P:
			return_value = 1;
		break;
	}

	return return_value;
}

int qwerty_row_to_number(u8 rxByte){
	int return_value = -1;

	switch(rxByte){
		case ASCII_Q:
			return_value = 1;
		break;
		case ASCII_W:
			return_value = 2;
		break;
		case ASCII_E:
			return_value = 3;
		break;
		case ASCII_R:
			return_value = 4;
		break;
		case ASCII_T:
			return_value = 5;
		break;
		case ASCII_Y:
			return_value = 6;
		break;
		case ASCII_U:
			return_value = 7;
		break;
		case ASCII_I:
			return_value = 8;
		break;
		case ASCII_O:
			return_value = 9;
		break;
		case ASCII_P:
			return_value = 0;
		break;
	}

	return return_value;
}

#endif


