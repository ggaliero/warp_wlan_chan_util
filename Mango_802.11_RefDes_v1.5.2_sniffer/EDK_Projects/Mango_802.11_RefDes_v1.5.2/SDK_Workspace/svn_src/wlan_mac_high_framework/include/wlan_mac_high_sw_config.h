/** @file wlan_mac_high_sw_config.h
 *  @brief Software Configuration Options
 *
 *  This contains definitions that affect the size of the CPU_HIGH projects.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

#ifndef WLAN_MAC_HIGH_SW_CONFIG_H_
#define WLAN_MAC_HIGH_SW_CONFIG_H_


//---------- COMPILATION TOGGLES ----------
//  The following toggles directly affect the size of the .text section after compilation.
//  They also implicitly affect DRAM usage since DRAM is used for the storage of
//  station_info_t structs as well as Tx/Rx logs.


#define WLAN_SW_CONFIG_ENABLE_WLAN_EXP      1       //Top-level switch for compiling wlan_exp. Setting to 0 implicitly removes
                                                    // logging code if set to 0 since there would be no way to retrieve the log.

#define WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS   1       //Top-level switch for compiling counts_txrx.  Setting to 0 removes counts
                                                    // from station_info_t struct definition and disables counts retrieval via
                                                    // wlan_exp.

#define WLAN_SW_CONFIG_ENABLE_LOGGING       1       //Top-level switch for compiling Tx/Rx logging. Setting to 0 will not cause
                                                    // the design to not log any entries to DRAM. It will also disable any log
                                                    // retrieval capabilities in wlan_exp. Note: this is logically distinct from
                                                    // WLAN_SW_CONFIG_ENABLE_WLAN_EXP. (WLAN_SW_CONFIG_ENABLE_WLAN_EXP 1, COMPILE_LOGGING 0)
													// still allows wlan_exp control over a node but no logging capabilities.

#define WLAN_SW_CONFIG_ENABLE_LTG           1       //Top-level switch for compiling LTG functionality. Setting to 0 will remove
                                                    // all LTG-related code from the design as well we disable any wlan_exp
                                                    // commands that control LTGs.


//---------- AUX BRAM SIZE PARAMETERS ----------
// These options affect the usage of the AUX. BRAM memory. By disabling USAGE TOGGLES options
// above, these definitions can be reduced and still guarantee safe performance of the node.

#define WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO   4608   	 			// dl_entry structs will fill WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO
                                                                    // kilobytes of memory. This parameter directly controls the number
                                                                    // of station_info_t structs that can be created. Note:
                                                                    // WLAN_OPTIONS_COMPILE_COUNTS_TXRX will affect the size of the
                                                                    // station_info_t structs in DRAM, but will not change the number
                                                                    // of station_info_t structs that can exists because that number
                                                                    // is constrained by the size of dl_entry and
                                                                    // WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO

#define WLAN_OPTIONS_AUX_SIZE_KB_BSS_INFO       4608    			// dl_entry structs will fill WLAN_OPTIONS_AUX_SIZE_KB_BSS_INFO
                                                                    // kilobytes of memory. This parameter directly controls the number
                                                                    // of bss_info_t structs that can be created.

#define WLAN_OPTIONS_AUX_SIZE_KB_RX_ETH_BD      15296  				// The XAxiDma_BdRing for Ethernet receptions will fill
													   	   	   	   	// WLAN_OPTIONS_AUX_SIZE_KB_RX_ETH_BD kilobytes of memory. This
													   	   	   	   	// parameter has a soft performance implication on the number of
												 	   	   	   	    // bursty Ethernet receptions the design can handle.



#endif /* WLAN_MAC_HIGH_SW_CONFIG_H_ */
