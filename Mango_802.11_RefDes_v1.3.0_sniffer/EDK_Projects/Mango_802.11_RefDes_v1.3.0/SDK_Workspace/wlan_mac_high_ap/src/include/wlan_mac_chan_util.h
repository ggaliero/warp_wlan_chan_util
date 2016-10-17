/*
 * wlan_mac_chan_util.h
 *
 *  Created on: 12 jul 2016
 *      Author: egagali
 */

#ifndef WLAN_MAC_CHAN_UTIL_H_
#define WLAN_MAC_CHAN_UTIL_H_

#define PHY_RX_MODE_DSSS      				0
#define PHY_RX_MODE_11AG					1

#define WLAN_CHANNELS					    15  	/// Number of WLAN channels
#define WLAN_24G_BAND						0		/// 2.4GHz Band
#define WLAN_5G_BAND						1		/// 5GHz Band
#define WLAN_INVALID_BAND					2

#define MAX_NUM_MPDU						200		/// Maximum number of MPDU entries
#define MAX_NUM_USER						20		/// Maximum number of user entries
#define MAX_NUM_SCAN						2

#define WLAN_MAC_NUM_MCS					8		/// Number of modulation and coding schemes (MCS)

// Traffic Channel Status
#define WLAN_CHAN_UTIL_NUM_STATES			3
#define WLAN_CHAN_UTIL_IDLE					0
#define WLAN_CHAN_UTIL_LIGHT				1
#define WLAN_CHAN_UTIL_HEAVY				2

#define WLAN_TRAFFIC_THRS_IDLE				10
#define WLAN_TRAFFIC_THRS_LIGHT				50

// AP Sniff Modes
#define ALL_DISABLED			       		0		/// Monitor modes disabled, does not capture MPDUs
#define SNIFF_ENABLED				   		1		/// Sniff mode enabled, channel utilization
#define SCAN_ENABLED						2		/// Scan mode enabled, only captured MPDUs

// Sort Users Modes
#define SORT_POWER_MODE						0		/// User list sorted by power level
#define SORT_MPDUS_MODE						1		/// User list sorted by traffic

// Physical Layer Convergence Protocol Parameters (in us)
#define TXTIME_T_PREAMBLE					16		/// Transmission time for the physical layer preamble
#define TXTIME_T_SIGNAL						4		/// Transmission time for
#define TXTIME_T_SYM						4		/// Transmission time for an OFDM symbol
#define T_SIG_EXT							6

// Physical Layer Convergence Protocol DSSS Parameters (in us)
#define TXTIME_DSSS_PLCP_PREAMBLE			144
#define TXTIME_DSSS_PLCP_HEADER				48

// MAC parameters
#define T_ACK								38		/// Transmission time for ACK MPDUs
#define T_SLOT_PHY_OFDM						9		/// Time slot
#define T_SLOT_PHY_DSSS						20
#define T_SIFS_PHY_DSSS						10
#define T_SIFS_PHY_OFDM_5GHZ				16		/// Short Inter-frame Space for amendment 802.11a
#define T_SIFS_PHY_OFDM_24GHZ				10		///
#define T_DIFS_PHY_DSSS						(T_SIFS_PHY_DSSS + 2*T_SLOT_PHY_DSSS)
#define T_DIFS_PHY_OFDM_5GHZ				(T_SIFS_PHY_OFDM_5GHZ  + 2*T_SLOT_PHY_OFDM)
#define T_DIFS_PHY_OFDM_24GHZ				(T_SIFS_PHY_OFDM_24GHZ + 2*T_SLOT_PHY_OFDM)

/* Include other framework headers */
#include "wlan_mac_misc_util.h"
#include "wlan_mac_802_11_defs.h"

typedef struct{
	u8		addr[6];								/// MAC address
}mac_addr;

typedef struct{
	mac_addr	src_addr;							/// Source MAC address
	u32			duration;							/// Total time occupancy of the MPDU
	u32			tx_time;							/// Time required to transmit the data frame
	char		power;								/// Received power
	u8			fcs;								/// Frame checksum sequence
	u8			is_beacon;							/// Flag for beacon type of frame
}mpdu_entry;

typedef struct{
	u16			num_elem;							/// Number of MPDU entries
	mpdu_entry	mpdu_entries[MAX_NUM_MPDU];			/// MPDU entries
}rx_mpdu_info;

typedef struct{
    mac_addr    user_mac;                           /// User MAC address
    u32         num_mpdu[3];                        /// Number of MPDUs transmitted
    u32			duration[3];						/// Transmission time
    char		last_power;							/// Last power level received
    u32			tot_mpdus;							/// Cumulative number of MPDUs
    u32         avg_mpdus;							/// Average number of MPDUs
    u32			avg_duration;						/// Average duration in us
    u8			is_ap;								/// Flag identification for AP
}user_entry;

typedef struct{
	u16			num_elem;							/// For average mpdu
	u16			num_users;							/// Number of user entries
	user_entry	user_entries[MAX_NUM_USER];			/// User entries
}mac_chan_info;

typedef struct{
	u16			num_elem;
	u16			num_mpdu[WLAN_CHANNELS][MAX_NUM_SCAN];
}mac_chan_data;

typedef struct{
	u8          elem;
	u32			chan_util[WLAN_CHAN_UTIL_NUM_STATES];
	u8			curr_state;
}mac_chan_state;

//////////// Initialization Functions ////////////
void	wlan_mac_high_init_mpdu_info(rx_mpdu_info* mpdu_info);
void 	wlan_mac_high_init_chan_info(mac_chan_info* chan_info);
void	wlan_mac_high_init_chan_data(mac_chan_data* chan_data);
void    wlan_mac_high_init_chan_state(mac_chan_state* chan_state);
void    wlan_mac_high_reset_chan_info(mac_chan_info* chan_info);
void    wlan_mac_high_refresh_chan_info(mac_chan_info* chan_info);
void    wlan_mac_high_refresh_chan_data(mac_chan_data* chan_data, u32 mac_param_chan);

//////////// Processing Functions ////////////////
void capture_mpdu(void* pkt_buf_addr, rx_mpdu_info* rx_info, mac_chan_info* chan_info, mac_chan_data* chan_data, u8 ap_mode);
void extract_mpdu_info(rx_mpdu_info* rx_info, rx_frame_info* mpdu_info, mac_header_80211* rx_header);
void set_mac_chan_info(rx_mpdu_info* mpdu_info, mac_chan_info* chan_info);
void set_mac_chan_data(rx_mpdu_info* rx_info, mac_chan_data* chan_data, u32 mac_channel);
void set_mac_chan_state(mac_chan_state* chan_state);

u32  get_mpdu_duration(u16 length, u8 mcs, u8 phy_mode);
void get_mac_chan_util(rx_mpdu_info* mpdu_info, mac_chan_info* chan_info, u32 mac_chan);

u32  get_tot_mpdu(user_entry* user);
void get_avg_mpdu(user_entry* user);
void get_avg_duration(user_entry* user);

int  is_known_mac(u8* addr, user_entry* users, u16 num_users);
void sort_users(mac_chan_info* chan_info, u8 sort_mode);
void remove_inactive_users(mac_chan_info* chan_info);
void remove_user_at_index(mac_chan_info* chan_info, u16 index);
void copy_user_entry(const user_entry in, user_entry* out);

void scan_traffic_sweep(rx_mpdu_info* rx_info, mac_chan_data* chan_data, u32 chan_sel);

u8   get_mac_channel_index(u32 mac_channel);
u8   get_channel_band(u32 mac_param_chan);

void print_chan_data(mac_chan_data* chan_data, u32 chan_sel);
void print_chan_util(u32 mac_channel, mac_chan_info* chan_info, u64 time_elapsed);

#endif /* WLAN_MAC_CHAN_UTIL_H_ */
