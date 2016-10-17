/** @file wlan_mac_scan.c
 *  @brief Scan FSM
 *
 *  This contains code for the active scan finite state machine.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 *
 *
 *   The scan process allows a node to search for a given BSS given an SSID.  The
 * behavior of the scan is controlled with the global scan parameters
 * (gl_scan_parameters).  The scan process follows a simple state machine:
 *    - After initialization, the scan state is IDLE
 *    - By calling wlan_mac_scan_start(), the scan transitions to RUNNING
 *    - Once the scan is RUNNING, it can either be stopped (ie back to IDLE)
 *      or PAUSED
 *    - If a scan is PAUSED, it can either be stopped (ie back to IDLE) or
 *      resumed (ie back to RUNNING)
 *
 * When a scan is paused, the node will stay on the current scan channel and the
 * global scan variables will not be updated.
 *
 * When the scan is RUNNING, it will transition through each channel in the
 * channel_vec array of the scan parameters.  Once it reaches the end of the
 * channel_vec array, it will start over at the beginning and continue doing this
 * until stopped or paused.  On a given channel, the scan will call the
 * tx_probe_request_callback every probe_tx_interval_usec.  If the interval is
 * zero, then the callback will never be called resulting in a passive scan.
 *
 * The fine scheduler is used to schedule both the probe request transmissions
 * and the interval to change channels.  There is no error checking on the scan
 * timing parameters.
 *
 */

/***************************** Include Files *********************************/
#include "wlan_mac_high_sw_config.h"

// Xilinx SDK includes
#include "xparameters.h"
#include "xio.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// WLAN includes
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_scan.h"

/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/


/*************************** Variable Definitions ****************************/

// Default Scan Channels
//     These channels will be scanned by default at boot.  This uses the standard
//     channel numbering scheme.
//
// u8 default_channel_selections[11] = {1,2,3,4,5,6,7,8,9,10,11};             // Scan only 2.4GHz channels
u8 default_channel_selections[15] = {1,2,3,4,5,6,7,8,9,10,11,36,40,44,48}; // Scan 2.4GHz and 5GHz channels


// Global scan parameters
//     This variable needs to be treated as volatile since it is expected to be
//     modified by other contexts after a call to wlan_mac_scan_get_parameters
volatile scan_parameters_t   gl_scan_parameters;


// Scan state variables
static   s8                  curr_scan_chan_idx;

static   u32                 scan_sched_id;
static   u32                 probe_sched_id;
static   scan_state_t        scan_state;
static   int                 num_full_scans;


// Callback Function
//     Used to transmit probe requests during the scan process
volatile function_ptr_t      tx_probe_request_callback;
volatile function_ptr_t      scan_state_change_callback;



/*************************** Functions Prototypes ****************************/

void wlan_mac_scan_state_transition();


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Initialize the scan state
 *
 * This function will initialize the scan state machine an set the scan
 * parameters to the default values.
 *
 * @return  int              - Status: XST_SUCCESS or XST_FAILURE
 *
 *****************************************************************************/
int wlan_mac_scan_init(){

    // Initialize probe request callback
    tx_probe_request_callback = (function_ptr_t)wlan_null_callback;
    scan_state_change_callback = (function_ptr_t)wlan_null_callback;

    // Set default scan parameters
    gl_scan_parameters.channel_vec = wlan_mac_high_malloc(sizeof(default_channel_selections));

    if (gl_scan_parameters.channel_vec != NULL) {
        memcpy(gl_scan_parameters.channel_vec, default_channel_selections, sizeof(default_channel_selections));
        gl_scan_parameters.channel_vec_len = sizeof(default_channel_selections);
    }

    gl_scan_parameters.probe_tx_interval_usec   = DEFAULT_SCAN_PROBE_TX_INTERVAL_USEC;
    gl_scan_parameters.time_per_channel_usec    = DEFAULT_SCAN_TIME_PER_CHANNEL_USEC;
    gl_scan_parameters.ssid                     = strndup("", SSID_LEN_MAX);

    // Set global scan parameters
    //     - Other global variables will be initialized when wlan_mac_scan_start() is called
    scan_sched_id  = SCHEDULE_ID_RESERVED_MAX;
    probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
    scan_state     = SCAN_IDLE;

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Set callbacks
 *
 *****************************************************************************/
void wlan_mac_scan_set_tx_probe_request_callback(function_ptr_t callback){
	tx_probe_request_callback = callback;
}
void wlan_mac_scan_set_state_change_callback(function_ptr_t callback){
	scan_state_change_callback = callback;
}



/*****************************************************************************/
/**
 * Get global scan parameters structure
 *
 * This is in lieu of getter / setter methods for all of the scan parameters.
 *
 * @return  volatile scan_parameters_t*     - Pointer to scan parameters
 *
 *****************************************************************************/
volatile scan_parameters_t* wlan_mac_scan_get_parameters(){
	return &gl_scan_parameters;
}



/*****************************************************************************/
/**
 * Start scan
 *
 * This function will start a scan using the current scan parameters.
 *
 *****************************************************************************/
void wlan_mac_scan_start(){

    // Only start a scan if state machine is IDLE
    if (scan_state == SCAN_IDLE) {

        // Initialize a variable that tracks how many times the scanner
        // as looped around the channel list.
        num_full_scans = -1;

        // Initialize scan variables
        curr_scan_chan_idx = -1;
        scan_state = SCAN_RUNNING;
        scan_state_change_callback(scan_state);

        // Start the scan
        wlan_mac_scan_state_transition();
    }
}



/*****************************************************************************/
/**
 * Stop scan
 *
 * This function will stop the current scan and return to the original channel
 * that the node was on when wlan_mac_scan_start() was called.
 *
 *****************************************************************************/
void wlan_mac_scan_stop() {
    interrupt_state_t   prev_interrupt_state;

    // Stop scan if it is running or paused
    if ((scan_state == SCAN_RUNNING) || (scan_state == SCAN_PAUSED)) {

        // Stop interrupts while removing scheduled events
        prev_interrupt_state = wlan_mac_high_interrupt_stop();

        // Remove any scheduled scan state transitions
        if (scan_sched_id != SCHEDULE_ID_RESERVED_MAX) {
            wlan_mac_remove_schedule(SCHEDULE_FINE, scan_sched_id);
            scan_sched_id  = SCHEDULE_ID_RESERVED_MAX;
        }

        // Remove any scheduled probe requests
        if (probe_sched_id != SCHEDULE_ID_RESERVED_MAX) {
            wlan_mac_remove_schedule(SCHEDULE_FINE, probe_sched_id);
            probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
        }

        // Reset the number of full scans to an invalid number
        num_full_scans = -1;

        // Restore interrupt state
        wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

        // Update scan state variables
        curr_scan_chan_idx = -1;
        scan_state = SCAN_IDLE;
        scan_state_change_callback(scan_state);
    }
}



/*****************************************************************************/
/**
 * Pause scan
 *
 * This function will pause the current scan (ie it stops all scheduled events
 * but does not revert the channel or update the scan state variables).
 *
 *****************************************************************************/
void wlan_mac_scan_pause(){
    interrupt_state_t   prev_interrupt_state;

    // Can only pause when running
    if (scan_state == SCAN_RUNNING) {

        // Stop interrupts while removing scheduled events
        prev_interrupt_state = wlan_mac_high_interrupt_stop();

        // Remove any scheduled scan state transitions
        if (scan_sched_id != SCHEDULE_ID_RESERVED_MAX) {
            wlan_mac_remove_schedule(SCHEDULE_FINE, scan_sched_id);
            scan_sched_id  = SCHEDULE_ID_RESERVED_MAX;
        }

        // Remove any scheduled probe requests
        if (probe_sched_id != SCHEDULE_ID_RESERVED_MAX) {
            wlan_mac_remove_schedule(SCHEDULE_FINE, probe_sched_id);
            probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
        }

        // Restore interrupt state
        wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

        // Update scan state variables
        scan_state = SCAN_PAUSED;
        scan_state_change_callback(scan_state);
    }
}



/*****************************************************************************/
/**
 * Resume scan
 *
 * This function will resume a paused scan.  When a scan is resumed, it will
 * immediately proceed to the next channel in the scan parameters.
 *
 *****************************************************************************/
void wlan_mac_scan_resume(){

    // Can only pause when running
    if (scan_state == SCAN_PAUSED) {

        // Update scan state variables
        scan_state = SCAN_RUNNING;
        scan_state_change_callback(scan_state);

        // Resume scan
        wlan_mac_scan_state_transition();
    }
}



/*****************************************************************************/
/**
 * Is the node scanning?
 *
 * @return  u32              - Is Scanning?
 *                                 1 - Currently scanning
 *                                 0 - Not scanning
 *
 *****************************************************************************/
u32 wlan_mac_scan_is_scanning(){
    if ((scan_state == SCAN_RUNNING) || (scan_state == SCAN_PAUSED)) {
        return 1;
    } else {
        return 0;
    }
}



/*****************************************************************************/
/**
 * Scan state transition
 *
 * This internal function will stop any existing scheduled probe requests, change
 * the channel and start sending probe requests on the new channel.  It will also
 * schedule an event to execute the function again until scan is paused or
 * stopped.
 *
 *****************************************************************************/
void wlan_mac_scan_state_transition(){

    // Remove existing scheduled probe requests
    if (probe_sched_id != SCHEDULE_ID_RESERVED_MAX) {
        wlan_mac_remove_schedule(SCHEDULE_FINE, probe_sched_id);
        probe_sched_id = SCHEDULE_ID_RESERVED_MAX;
    }

    // Update the channel
    curr_scan_chan_idx = (curr_scan_chan_idx + 1) % (gl_scan_parameters.channel_vec_len);
    wlan_mac_high_set_radio_channel(gl_scan_parameters.channel_vec[(u8)curr_scan_chan_idx]);

    // Update the number of full scan loops variable
    if (curr_scan_chan_idx == 0) {
        num_full_scans++;
    }

    // Send a probe request
    //     - A probe interval of 0 results in a passive scan
    if (gl_scan_parameters.probe_tx_interval_usec > 0) {
        tx_probe_request_callback();

        // Schedule future probe requests on this channel
        //     - This will be stopped on the next call to wlan_mac_scan_state_transition()
        //       when the channel is changed.
        //
        probe_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, gl_scan_parameters.probe_tx_interval_usec, SCHEDULE_REPEAT_FOREVER, (void*)tx_probe_request_callback);
    }

    // Schedule the scan state transition
    //     - This will only be executed when moving from IDLE to RUNNING
    //     - The scheduled event will only be stopped when scan is paused or stopped
    //
    if (scan_sched_id == SCHEDULE_ID_RESERVED_MAX) {
        scan_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, gl_scan_parameters.time_per_channel_usec, SCHEDULE_REPEAT_FOREVER, (void*)wlan_mac_scan_state_transition);
    }
}

int wlan_mac_scan_get_num_scans(){
    return num_full_scans;
}
