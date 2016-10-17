/** @file wlan_mac_userio_util.h
 *  @brief User I/O Definitions
 *
 *  This contains WARP User I/O definitions for all CPUs
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_USERIO_UTIL_H_
#define WLAN_MAC_USERIO_UTIL_H_

// HW includes
#include "xparameters.h"
#include "w3_userio.h"


//-----------------------------------------------
// Base Addresses
//
#define USERIO_BASEADDR                                    XPAR_W3_USERIO_BASEADDR            ///< XParameters rename of base address of User I/O

// Wrapper macros for debug header IO
#define wlan_mac_set_dbg_hdr_dir(dir, pin_mask) userio_set_dbg_hdr_io_dir(USERIO_BASEADDR, (dir), (pin_mask))
#define wlan_mac_set_dbg_hdr_out(pin_mask) 	 	userio_set_dbg_hdr_out(USERIO_BASEADDR, (pin_mask))
#define wlan_mac_clear_dbg_hdr_out(pin_mask) 	userio_clear_dbg_hdr_out(USERIO_BASEADDR, (pin_mask))
#define wlan_mac_write_dbg_hdr_out(val) 	 	userio_write_dbg_hdr_out(USERIO_BASEADDR, (val))
#define wlan_mac_read_dbg_hdr_io(val) 		 	userio_read_dbg_hdr_io(USERIO_BASEADDR)

/*********************** Global Structure Definitions ************************/


/*************************** Function Prototypes *****************************/
u8                 hex_to_seven_segment(u8 hex_value);

void               enable_hex_pwm();
void               disable_hex_pwm();
void               set_hex_pwm_period(u16 period);
void               set_hex_pwm_min_max(u16 min, u16 max);

void               write_hex_display(u8 val);
void               write_hex_display_with_pwm(u8 val);

void               set_hex_display_error_status(u8 status);

void               blink_hex_display(u32 num_blinks, u32 blink_time);

void               set_hex_display_right_dp(u8 val);
void               set_hex_display_left_dp(u8 val);


#endif   // END WLAN_MAC_USERIO_UTIL_H_
