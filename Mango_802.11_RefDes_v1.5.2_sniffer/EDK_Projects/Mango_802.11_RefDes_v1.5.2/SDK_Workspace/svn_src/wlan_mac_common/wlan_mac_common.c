/** @file wlan_mac_common.c
 *  @brief Common Code
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/

#include "stdlib.h"
#include "string.h"

#include "xil_io.h"
#include "xstatus.h"
#include "xparameters.h"

#include "w3_iic_eeprom.h"

#include "wlan_mac_common.h"
#include "wlan_mac_userio_util.h"


/*********************** Global Variable Definitions *************************/

#define EEPROM_BASEADDR                                    XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR


/*************************** Variable Definitions ****************************/

static wlan_mac_hw_info_t         mac_hw_info;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Null Callback
 *
 * This function will always return XST_SUCCESS and should be used to initialize
 * callbacks.  All input parameters will be ignored.
 *
 * @param   param            - Void pointer for parameters
 *
 * @return  int              - Status:
 *                                 XST_SUCCESS - Command completed successfully
 *****************************************************************************/
int wlan_null_callback(void* param) {
    return XST_SUCCESS;
};



/*****************************************************************************/
/**
 * Verify channel is supported
 *
 * @param   channel          - Channel to verify
 *
 * @return  int              - Channel supported?
 *                                 XST_SUCCESS - Channel supported
 *                                 XST_FAILURE - Channel not supported
 *****************************************************************************/
int wlan_verify_channel(u32 channel) {
    int return_value;

    // The 802.11 reference design allows a subset of 2.4 and 5 GHz channels
    //     Channel number follows 802.11 conventions:
    //         https://en.wikipedia.org/wiki/List_of_WLAN_channels
    //
#if 1
    switch (channel) {
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
        // 5GHz channels
        case 36:
        case 40:
        case 44:
        case 48:
            return_value = XST_SUCCESS;
        break;
        default:
            return_value = XST_FAILURE;
        break;
    }
#else
    switch (channel) {
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
            // 5GHz channels
            case 36: // 5180 MHz
            case 38: // 5190 MHz
            case 40: // 5200 MHz
            case 44: // 5220 MHz
            case 46: // 5230 MHz
            case 48: // 5240 MHz
            case 52: // 5260 MHz
            case 54: // 5270 MHz
            case 56: // 5280 MHz
            case 60: // 5300 MHz
            case 62: // 5310 MHz
            case 64: // 5320 MHz
            case 100: // 5500 MHz
            case 102: // 5510 MHz
            case 104: // 5520 MHz
            case 108: // 5540 MHz
            case 110: // 5550 MHz
            case 112: // 5560 MHz
            case 116: // 5580 MHz
            case 118: // 5590 MHz
            case 120: // 5600 MHz
            case 124: // 5620 MHz
            case 126: // 5630 MHz
            case 128: // 5640 MHz
            case 132: // 5660 MHz
            case 134: // 5670 MHz
            case 136: // 5680 MHz
            case 140: // 5700 MHz
            case 142: // 5710 MHz
            case 144: // 5720 MHz
            case 149: // 5745 MHz
            case 151: // 5755 MHz
            case 153: // 5765 MHz
            case 157: // 5785 MHz
            case 159: // 5795 MHz
            case 161: // 5805 MHz
            case 165: // 5825 MHz
            case 172: // 5860 MHz
            case 174: // 5870 MHz
            case 175: // 5875 MHz
            case 176: // 5880 MHz
            case 177: // 5885 MHz
            case 178: // 5890 MHz
                return_value = XST_SUCCESS;
            break;
            default:
            	xil_printf("ERROR (wlan_verify_channel): Channel %d invalid\n", channel);
                return_value = XST_FAILURE;
            break;
        }
#endif

    return return_value;
}



/*****************************************************************************/
/**
 * Halt CPU on Error
 *
 * @param   error_code       - Error code to display on WARP hex display
 *****************************************************************************/
void cpu_error_halt(u32 error_code) {

    if (error_code != WLAN_ERROR_CPU_STOP) {
        // Print error message
        xil_printf("\n\nERROR:  CPU is halting with error code: E%X\n\n", (error_code & 0xF));

        // Set the error code on the hex display
        set_hex_display_error_status(error_code & 0xF);

        // Enter infinite loop blinking the hex display
        blink_hex_display(0, 250000);
    } else {
        // Stop execution
        while (1) {};
    }
}




/*****************************************************************************/
/**
 * Initialize the MAC Hardware Info
 *
 * This function will initialize the MAC hardware information structure for
 * the CPU based on information contained in the EEPROM and the wlan_exp_type
 * provided.  This function should only be called after the EEPROM has been
 * initialized.
 *
 * @param   None
 *
 *****************************************************************************/
void init_mac_hw_info() {
    // Initialize the wlan_mac_hw_info_t structure to all zeros
    memset((void*)(&mac_hw_info), 0x0, sizeof(wlan_mac_hw_info_t));

    // Set General Node information
    mac_hw_info.serial_number = w3_eeprom_read_serial_num(EEPROM_BASEADDR);
    mac_hw_info.fpga_dna[1]   = w3_eeprom_read_fpga_dna(EEPROM_BASEADDR, 1);
    mac_hw_info.fpga_dna[0]   = w3_eeprom_read_fpga_dna(EEPROM_BASEADDR, 0);

    // Set HW Addresses
    //   - The w3_eeprom_read_eth_addr() function handles the case when the WARP v3
    //     hardware does not have a valid Ethernet address
    //
    // Use address 0 for the WLAN interface, address 1 for the WLAN Exp interface
    //
    w3_eeprom_read_eth_addr(EEPROM_BASEADDR, 0, mac_hw_info.hw_addr_wlan);
    w3_eeprom_read_eth_addr(EEPROM_BASEADDR, 1, mac_hw_info.hw_addr_wlan_exp);
}



/*****************************************************************************/
/**
 * Get the MAC Hardware Info
 *
 * Return the MAC hardware information structure.  This should only be used
 * after the structure is initialized.
 *
 * @return  wlan_mac_hw_info_t *  - Pointer to HW info structure
 *
 *****************************************************************************/
wlan_mac_hw_info_t * get_mac_hw_info()          { return &mac_hw_info; }
u8                 * get_mac_hw_addr_wlan()     { return mac_hw_info.hw_addr_wlan; }
u8                 * get_mac_hw_addr_wlan_exp() { return mac_hw_info.hw_addr_wlan_exp; }


#ifdef _DEBUG_

/*****************************************************************************/
/**
 * @brief Print Hardware Information
 *
 * @param wlan_mac_hw_info_t *    - Pointer to the hardware info struct
 *
 *****************************************************************************/
void wlan_print_hw_info(wlan_mac_hw_info_t * info) {
	int i;

	xil_printf("WLAN MAC HW INFO:  \n");
	xil_printf("  CPU Low Type     :  0x%8x\n", info->wlan_exp_type);
	xil_printf("  Serial Number    :  %d\n",    info->serial_number);
	xil_printf("  FPGA DNA         :  0x%8x  0x%8x\n", info->fpga_dna[1], info->fpga_dna[0]);
	xil_printf("  WLAN EXP HW Addr :  %02x",    info->hw_addr_wlan_exp[0]);

	for (i = 1; i < MAC_ADDR_LEN; i++) {
		xil_printf(":%02x", info->hw_addr_wlan_exp[i]);
	}
	xil_printf("\n");

	xil_printf("  WLAN HW Addr     :  %02x",    info->hw_addr_wlan[0]);
	for (i = 1; i < MAC_ADDR_LEN; i++) {
		xil_printf(":%02x", info->hw_addr_wlan[i]);
	}
	xil_printf("\n");
}



#endif
