/** @file wlan_mac_misc_util.c
 *  @brief Miscellaneous Utilities
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to interact with the MAC Time and User IO cores.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "w3_userio.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_time_util.h"


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * @brief Mapping of hexadecimal values to the 7-segment display
 *
 * @param   hex_value        - Hexadecimal value to be converted (between 0 and 15)
 * @return  u8               - LED map value of the 7-segment display
 */
u8 hex_to_seven_segment(u8 hex_value) {
    switch(hex_value) {
        case(0x0) : return 0x3F;
        case(0x1) : return 0x06;
        case(0x2) : return 0x5B;
        case(0x3) : return 0x4F;
        case(0x4) : return 0x66;
        case(0x5) : return 0x6D;
        case(0x6) : return 0x7D;
        case(0x7) : return 0x07;
        case(0x8) : return 0x7F;
        case(0x9) : return 0x6F;

        case(0xA) : return 0x77;
        case(0xB) : return 0x7C;
        case(0xC) : return 0x39;
        case(0xD) : return 0x5E;
        case(0xE) : return 0x79;
        case(0xF) : return 0x71;
        default   : return 0x00;
    }
}



/*****************************************************************************/
/**
 * @brief Enable the PWM functionality of the hex display
 *
 * This function will tell the User I/O to enable the PWM to pulse the hex display.
 *
 * @param   None
 * @return  None
 */
void enable_hex_pwm() {
    userio_set_pwm_ramp_en(USERIO_BASEADDR, 1);
}



/*****************************************************************************/
/**
 * @brief Disable the PWM functionality of the hex display
 *
 * This function will tell the User I/O to disable the PWM to pulse the hex display.
 *
 * @param   None
 * @return  None
 */
void disable_hex_pwm() {
    userio_set_pwm_ramp_en(USERIO_BASEADDR, 0);
}



/*****************************************************************************/
/**
 * @brief Set the PWM period for the Hex display
 *
 * This function will set the period used to pulse the hex display
 *
 * @param   period           - Period of the PWM ramp (u16)
 * @return  None
 */
void set_hex_pwm_period(u16 period) {
    userio_set_pwm_period(USERIO_BASEADDR, period);
}



/*****************************************************************************/
/**
 * @brief Set the Min / Max timing for the PWM ramp
 *
 * This function will set the Min / Max timing parameters for the PWM pulse of
 * the hex display.  The values should be less that the period set by
 * set_hex_pwm_period(), but that condition is not enforced.
 *
 * @param   min              - Timing parameter for when the PWM reaches minimum value (u16)
 * @param   max              - Timing parameter for when the PWM reaches maximum value (u16)
 * @return  None
 *
 * @note   This function will disable the PWM.  Therefore, the PWM must be enabled
 *     after calling this function.
 */
void set_hex_pwm_min_max(u16 min, u16 max) {
    // Ramp must be disabled when changing ramp params
    userio_set_pwm_ramp_en(USERIO_BASEADDR, 0);
    userio_set_pwm_ramp_min(USERIO_BASEADDR, min);
    userio_set_pwm_ramp_max(USERIO_BASEADDR, max);
}



/*****************************************************************************/
/**
 * @brief Write a Decimal Value to the Hex Display
 *
 * This function will write a decimal value to the board's two-digit hex displays.
 * The display is right justified; WLAN Exp will indicate its connection state
 * using the right decimal point.
 *
 * @param   val              - Value to be displayed
 *                                 0 -  99: Displayed normally
 *                                99 - 254: "00" displayed
 *                                     255: "--" displayed
 * @return  None
 */
void write_hex_display(u8 val) {
    u32 right_dp;
    u8  left_val;
    u8  right_val;

    // Need to retain the value of the right decimal point
    right_dp = userio_read_hexdisp_right(USERIO_BASEADDR) & W3_USERIO_HEXDISP_DP;

    userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE))));

    if (val < 10) {
        left_val  = 0;
        right_val = hex_to_seven_segment(val);
    } else {
        if (val < 100) {
            left_val  = hex_to_seven_segment(((val / 10) % 10));
            right_val = hex_to_seven_segment((val % 10));
        } else {
            if (val != 255) {
                left_val  = hex_to_seven_segment(0x00);
                right_val = hex_to_seven_segment(0x00);
            } else {
                left_val  = 0x40;
                right_val = 0x40;
            }
        }
    }

    userio_write_hexdisp_left(USERIO_BASEADDR, left_val);
    userio_write_hexdisp_right(USERIO_BASEADDR, (right_val | right_dp));
}



/*****************************************************************************/
/**
 * @brief Write a Decimal Value to the Hex Display with PWM pulsing
 *
 * This function will write a decimal value to the board's two-digit hex displays.
 * The display is right justified and will pulse using the pwms; WLAN Exp will
 * indicate its connection state using the right decimal point.
 *
 * @param   val              - Value to be displayed
 *                                 0 -  99: Displayed normally
 *                                99 - 254: "00" displayed
 *                                     255: "--" displayed
 * @return  None
 */
void write_hex_display_with_pwm(u8 val) {
    u32 hw_control;
    u32 temp_control;
    u32 right_dp;
    u8  left_val;
    u8  right_val;
    u32 pwm_val;

    // Need to retain the value of the right decimal point
    right_dp = userio_read_hexdisp_right(USERIO_BASEADDR) & W3_USERIO_HEXDISP_DP;

    if (val < 10) {
        left_val  = 0;
        right_val = hex_to_seven_segment(val);
    } else {
        if (val < 100) {
            left_val  = hex_to_seven_segment(((val / 10) % 10));
            right_val = hex_to_seven_segment((val % 10));
        } else {
            if (val != 255) {
                left_val  = hex_to_seven_segment(0x00);
                right_val = hex_to_seven_segment(0x00);
            } else {
                left_val  = 0x40;
                right_val = 0x40;
            }
        }
    }

    // Store the original value of what is under HW control
    hw_control   = userio_read_control(USERIO_BASEADDR);

    // Need to zero out all of the HW control of the hex displays; Change to raw hex mode
    temp_control = (hw_control & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE | W3_USERIO_CTRLSRC_HEXDISP_R | W3_USERIO_CTRLSRC_HEXDISP_L)));

    // Set the hex display mode to raw bits
    userio_write_control(USERIO_BASEADDR, temp_control);

    // Write the display
    userio_write_hexdisp_left(USERIO_BASEADDR, left_val);
    userio_write_hexdisp_right(USERIO_BASEADDR, (right_val | right_dp));

    pwm_val   = (right_val << 8) + left_val;

    // Set the HW / SW control of the user io (raw mode w/ the new display value)
    userio_write_control(USERIO_BASEADDR, (temp_control | pwm_val));

    // Set the pins that are using PWM mode
    userio_set_hw_ctrl_mode_pwm(USERIO_BASEADDR, pwm_val);
}



/*****************************************************************************/
/**
 * @brief Set Error Status for Node
 *
 * Function will set the hex display to be "Ex", where x is the value of the
 * status error
 *
 * @param   status           - Number from 0 - 0xF to indicate status error
 * @return  None
 */
void set_hex_display_error_status(u8 status) {
    u32 right_dp;

    // Need to retain the value of the right decimal point
    right_dp = userio_read_hexdisp_right(USERIO_BASEADDR) & W3_USERIO_HEXDISP_DP;

    userio_write_control(USERIO_BASEADDR, (userio_read_control(USERIO_BASEADDR) & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE))));

    userio_write_hexdisp_left(USERIO_BASEADDR,  hex_to_seven_segment(0xE));
    userio_write_hexdisp_right(USERIO_BASEADDR, (hex_to_seven_segment(status % 16) | right_dp));
}



/*****************************************************************************/
/**
 * @brief Blink LEDs
 *
 * For WARP v3 Hardware, this function will blink the hex display.
 *
 * @param   num_blinks       - Number of blinks (0 means blink forever)
 * @param   blink_time       - Time in microseconds between blinks
 *
 * @return  None
 */
void blink_hex_display(u32 num_blinks, u32 blink_time) {
    u32          i;
    u32          hw_control;
    u32          temp_control;
    u8           right_val;
    u8           left_val;

    // Get left / right values
    left_val  = userio_read_hexdisp_left(USERIO_BASEADDR);
    right_val = userio_read_hexdisp_right(USERIO_BASEADDR);

    // Store the original value of what is under HW control
    hw_control   = userio_read_control(USERIO_BASEADDR);

    // Need to zero out all of the HW control of the hex displays; Change to raw hex mode
    temp_control = (hw_control & (~(W3_USERIO_HEXDISP_L_MAPMODE | W3_USERIO_HEXDISP_R_MAPMODE | W3_USERIO_CTRLSRC_HEXDISP_R | W3_USERIO_CTRLSRC_HEXDISP_L)));

    // Set the hex display mode to raw bits
    userio_write_control(USERIO_BASEADDR, temp_control);

    // Use usleep to blink the display
    if (num_blinks > 0) {
        // Perform standard blink
        for (i = 0; i < num_blinks; i++) {
            userio_write_hexdisp_left(USERIO_BASEADDR,  (((i % 2) == 0) ? left_val  : 0x00));
            userio_write_hexdisp_right(USERIO_BASEADDR, (((i % 2) == 0) ? right_val : 0x00));
            usleep(blink_time);
        }
    } else {
        // Perform an infinite blink
        i = 0;
        while (1) {
            userio_write_hexdisp_left(USERIO_BASEADDR,  (((i % 2) == 0) ? left_val  : 0x00));
            userio_write_hexdisp_right(USERIO_BASEADDR, (((i % 2) == 0) ? right_val : 0x00));
            usleep(blink_time);
            i++;
        }
    }

    // Set control back to original value
    userio_write_control( USERIO_BASEADDR, hw_control );
}



/*****************************************************************************/
/**
 * @brief Set Right Decimal Point on the Hex Display
 *
 * This function will set the right decimal point on the hex display
 *
 * @param   val              - 0 - Clear decimal point; 1 - Set decimal point
 * @return  None
 */
void set_hex_display_right_dp(u8 val) {
    if (val) {
        userio_write_hexdisp_right(USERIO_BASEADDR, (userio_read_hexdisp_right(USERIO_BASEADDR) | W3_USERIO_HEXDISP_DP));
    } else {
        userio_write_hexdisp_right(USERIO_BASEADDR, (userio_read_hexdisp_right(USERIO_BASEADDR) & ~W3_USERIO_HEXDISP_DP));
    }
}



/*****************************************************************************/
/**
 * @brief Set Left Decimal Point on the Hex Display
 *
 * This function will set the left decimal point on the hex display
 *
 * @param   val              - 0 - Clear decimal point; 1 - Set decimal point
 * @return  None
 */
void set_hex_display_left_dp(u8 val) {
    if (val) {
        userio_write_hexdisp_left(USERIO_BASEADDR, (userio_read_hexdisp_left(USERIO_BASEADDR) | W3_USERIO_HEXDISP_DP));
    } else {
        userio_write_hexdisp_left(USERIO_BASEADDR, (userio_read_hexdisp_left(USERIO_BASEADDR) & ~W3_USERIO_HEXDISP_DP));
    }
}





