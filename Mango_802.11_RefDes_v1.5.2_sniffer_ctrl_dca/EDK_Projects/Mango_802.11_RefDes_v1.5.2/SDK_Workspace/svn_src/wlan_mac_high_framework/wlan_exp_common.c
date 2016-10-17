/** @file wlan_exp_common.c
 *  @brief Experiment Framework (Common)
 *
 *  This contains the code for WLAN Experimental Framework.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */



/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

// Xilinx / Standard library includes

#include <xparameters.h>
#include <xil_io.h>
#include <xio.h>

#include "ctype.h"
#include "string.h"
#include "stdarg.h"

// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_high.h"

// WLAN Exp includes
#include "wlan_exp_common.h"

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP

/*************************** Constant Definitions ****************************/

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

/*************************** Functions Prototypes ****************************/

/**************************** Common Functions *******************************/


/*****************************************************************************/
/**
* Debug Printing Functions
*
******************************************************************************/
u8       wlan_exp_print_level     = WLAN_EXP_DEFAULT_DEBUG_PRINT_LEVEL;
char   * print_type_node          = "NODE";
char   * print_type_transport     = "TRANSPORT";
char   * print_type_event_log     = "EVENT LOG";
char   * print_type_counts        = "COUNTS";
char   * print_type_ltg           = "LTG";
char   * print_type_queue         = "QUEUE";


void wlan_exp_print_header(u8 level, char * type, char* filename, u32 line) {
    char * basename = NULL;

    if (type != NULL) {
        xil_printf("%s", type);

        if ((level <= WLAN_EXP_PRINT_WARNING) || (wlan_exp_print_level == WLAN_EXP_PRINT_DEBUG)) {
            basename = strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename;
        }

        if (wlan_exp_print_level == WLAN_EXP_PRINT_DEBUG) {
            xil_printf(" (%s:%d): ", basename, line);
        } else {
            xil_printf(": ");
        }

        switch (level) {
            case WLAN_EXP_PRINT_ERROR:
                xil_printf("ERROR (%s:%d): ", basename, line);
            break;

            case WLAN_EXP_PRINT_WARNING:
                xil_printf("WARNING (%s:%d): ", basename, line);
            break;
        }
    }
}


void print_mac_address(u8 * mac_address) {
    u32 i;

    xil_printf("%02x", mac_address[0]);

    for ( i = 1; i < MAC_ADDR_LEN; i++ ) {
        xil_printf(":%02x", mac_address[i]);
    }
}


void print_mac_timestamp() {
    u64            timestamp;

    timestamp = get_mac_time_usec();

    xil_printf("0x%08x 0x%08x\n", (u32)(timestamp >> 32), (u32)(timestamp));
}


void wlan_exp_print_mac_address(u8 level, u8 * mac_address) {    
    if (level <= wlan_exp_print_level) {
        print_mac_address(mac_address);
    }
}


void wlan_exp_set_print_level(u8 level) {

    switch (level) {
        case WLAN_EXP_PRINT_NONE:
        case WLAN_EXP_PRINT_ERROR:
        case WLAN_EXP_PRINT_WARNING:
        case WLAN_EXP_PRINT_INFO:
        case WLAN_EXP_PRINT_DEBUG:
            wlan_exp_print_level = level;
        break;

        default:
            xil_printf("Unsupported print level.  Setting to WLAN_EXP_PRINT_ERROR.\n");
            wlan_exp_print_level = WLAN_EXP_PRINT_ERROR;
        break;
    }
}


/********************************************************************
 * WLAN Exp Null Callback
 *
 * This function is part of the callback system for processing commands.
 *
 * @param   void * param     - Parameters for the callback
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS  - Command successful
 *
 ********************************************************************/
int wlan_exp_null_callback(void* param){
    wlan_exp_printf(WLAN_EXP_PRINT_INFO, print_type_node, "WLAN Exp NULL callback\n");
    return XST_SUCCESS;
}

/****************************************************************************/
/**
 * Get MAC Address
 *
 * This function will populate the MAC address buffer, dest, with the MAC
 * address coming over the network (big endian).  This uses the same formating
 * as the HW address parameter from transport.c
 *
 * @param    src  - Source buffer of MAC address (u32, byte swapped)
 *           dest - Destination buffer of MAC address
 *
 * @return    None.
 *
 * @note     None.
 *
 ****************************************************************************/
void wlan_exp_get_mac_addr(u32 * src, u8 * dest) {

    dest[0] = (src[0] >> 16) & 0xFF;
    dest[1] = (src[0] >> 24) & 0xFF;
    dest[2] = (src[1] >>  0) & 0xFF;
    dest[3] = (src[1] >>  8) & 0xFF;
    dest[4] = (src[1] >> 16) & 0xFF;
    dest[5] = (src[1] >> 24) & 0xFF;

}


void wlan_exp_put_mac_addr(u8 * src, u32 * dest) {

    dest[0] = (src[1] << 24) + (src[0] << 16);
    dest[1] = (src[5] << 24) + (src[4] << 16) + (src[3] <<  8) + (src[2] <<  0);

}



/********************************************************************
 * @brief Clear DDR3 SODIMM Memory Module
 *
 * This function will clear the contents of the DDR
 *
 * @param  verbose           - Print information on time to clear the DDR
 *
 * @return None
 *
 ********************************************************************/
void clear_ddr(u32 verbose) {
    u32 i;
    u32 num_step;
    u32 step_size;

    u64 start_timestamp;
    u64 end_timestamp;
    u32 processing_time;

    u32 start_address = DRAM_BASE;
    u32 size          = DRAM_SIZE;

    start_timestamp = get_system_time_usec();

#if 0
    // Implementation 1:
    //     Use CPU to bzero the entire DDR  (approx 84769092 usec)
    bzero((void *)start_address, size);
#endif

#if 1
    // Implementation 2:
    //     Use CPU to bzero the first block of DDR
    //     Use the DMA to zero out the rest of the DDR
    //
    // For num_step (all times approx):
    //     2^10  --> 1149215 usec
    //     2^11  --> 1107146 usec
    //     2^12  --> 1089062 usec
    //     2^13  --> 1082326 usec
    //     2^14  --> 1080768 usec  <-- Minimum
    //     2^15  --> 1093902 usec
    //     2^16  --> 1150738 usec
    //     2^17  --> 1265897 usec
    //
    num_step  = 1 << 14;
    step_size = size / num_step;

    bzero((void *)start_address, step_size);

    for (i = 1; i < num_step; i++) {
        wlan_mac_high_cdma_start_transfer((void *)start_address, (void *)(start_address + (step_size * i)), step_size);
    }
#endif

    end_timestamp   = get_system_time_usec();
    processing_time = (end_timestamp - start_timestamp) & 0xFFFFFFFF;

    if (verbose == WLAN_EXP_VERBOSE) {
        wlan_exp_printf(WLAN_EXP_PRINT_NONE, NULL, "  Contents cleared in %d (usec)\n", processing_time);
    }
}



/*****************************************************************************/
/**
 * @ brief Initialize a TAG parameter structure
 *
 * This function will initialize a given tag parameter structure based on the input arguments
 *
 * @param   parameters       - Pointer to structure from which to put all the tag parameter values
 * @param   group            - Group tag parameters belong to
 * @param   num_parameters   - Number of tag parameters to initialize
 * @param   values           - u32 array of values for the tag parameters
 * @param   lengths          - u16 array of lengths for each tag parameter value
 *     * @return  int                   Total number of bytes of the TAG     arameter structure
 *
 * @note       Please make sure that the parameters     tructure and the paramete     values
 *             maintain the same order    
 *
 ***********************    **********************    ***********    ******************/
int  wlan_exp_init_parameters(wlan_exp_tag_parameter * parameters, u8 group, u32 num_parameters, u32 * values, u16 * lengths) {

    u32                      i;
    u32                      offset;
    u32                      size;
    wlan_exp_tag_parameter * temp_param;

    // Initialize variables
    offset = 0;
    size   = sizeof(wlan_exp_tag_parameter);

    for(i = 0; i < num_parameters; i++) {

        // Fill in the necessary tag parameter
        temp_param = &parameters[i];

        // Set parameter values
        temp_param->reserved = 0xFF;
        temp_param->group    = group;
        temp_param->length   = lengths[i];
        temp_param->command  = i;
        temp_param->value    = &values[offset];

        // Increment offset so that we get the correct index in to info structure
        offset    += temp_param->length;
    }

    return ((size * i) + (offset * 4)) ;
}



/*****************************************************************************/
/**
 * @ brief Get values from a TAG parameter structure
 *
 * This function will populate a buffer with the information from the tag parameter structure based on
 * the input arguments
 *
 * @param   parameters       - Pointer to structure from which to get all the tag parameter information
 * @param   num_parameters   - Number of tag parameters to initialize
 * @param   buffer           - u32 array to place tag parameter information
 * @param   max_resp_len     - Maximum number of u32 words allowed in response
 * @param   values_only      - Flag to populate all tag parameter information (WLAN_EXP_FALSE) or
 *                                 just the tag parameter values (WLAN_EXP_TRUE)
 * @param   transmit         - Flag to adjust the values for network transmission (WLAN_EXP_TRANSMIT) or
 *                                 leave them alone (WLAN_EXP_NO_TRANSMIT)
 *
 * @return  int              - Total number of words placed in the buffer
 *
 * @note    The tag parameters must be initialize     before this function is called.
 *
 **************************************************************************    **/
int  wlan_exp_get_parameters(wlan_exp_tag_parameter * parameters, u32 num_parameters, u32 * buffer, u32 max_resp_len, u8 values_only, u8 transmit) {

    u32 i, j;
    u32 num_total_words;
    u32 num_param_header_words;
    u32 num_param_words;

    u32 length;
    u32 temp_word;

    // Set the number of header words to process
    if (values_only == WLAN_EXP_FALSE) {
        num_param_header_words = 2;              // There are two header words for each tag parameter
    } else {
        num_param_header_words = 0;              // Do not get any header words since only values were requested
    }

    // Initialize the total number of words used
    num_total_words = 0;

    // Iterate through all tag parameters
    for( i = 0; i < num_parameters; i++ ) {

        // Get the length of the parameter
        length = parameters[i].length;

        // The number of words in a tag parameter is the number of value words + number of header words
        num_param_words = length + num_param_header_words;

        // Make sure we have space in the buffer to put the parameter
        if ((num_total_words + num_param_words) < max_resp_len) {

            // If we need to transmit these values, then we must convert for network transmission
            if (transmit == WLAN_EXP_TRANSMIT) {

                if (values_only == WLAN_EXP_FALSE) {
                    temp_word = ((parameters[i].reserved  < 24) | (parameters[i].group << 16) | (length));
                    buffer[num_total_words]     = Xil_Htonl(temp_word);
                    buffer[num_total_words + 1] = Xil_Htonl(parameters[i].command);
                }

                for( j = 0; j < length; j++ ) {
                    buffer[num_total_words + num_param_header_words + j] = Xil_Htonl(parameters[i].value[j]);
                }

            } else {
                if (values_only == WLAN_EXP_FALSE) {
                    temp_word = ((parameters[i].reserved << 24) | (parameters[i].group << 16) | (length));

                    buffer[num_total_words]     = temp_word;
                    buffer[num_total_words + 1] = parameters[i].command;
                }

                for( j = 0; j < length; j++ ) {
                    buffer[num_total_words + num_param_header_words + j] = parameters[i].value[j];
                }
            }

            // Update the total number of words processed
            num_total_words += num_param_words;

        } else {
            // Exit the loop because there is no more space
            wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_node, "Buffer too small for all tag parameters.\n");
            break;
        }
    }

    return num_total_words;
}



#ifdef _DEBUG_

/*****************************************************************************/
/**
* Print Tag Parameters
*
* This function will print a list of wn_tag_parameter structures
*
* @param    param      - pointer to the wn_tag_parameter list
*           num_params - number of wn_tag_parameter structures in the list
*
* @return    None.
*
* @note        None.
*
******************************************************************************/
void print_wlan_exp_parameters(wlan_exp_tag_parameter *param, int num_params) {

    int i, j;

    xil_printf("Parameters: \n");

    for( i = 0; i < num_params; i++ ){
        xil_printf("  Parameter %d:\n", i);
        xil_printf("    Group:            %d \n",   param[i].group);
        xil_printf("    Length:           %d \n",   param[i].length);
        xil_printf("    Command:          %d \n",   param[i].command);

        for( j = 0; j < param[i].length; j++ ) {
            xil_printf("    Value[%2d]:        0x%8x \n",   j, param[i].value[j]);
        }
    }

    xil_printf("\n");
}

#endif
#endif    // End WLAN_SW_CONFIG_ENABLE_WLAN_EXP
