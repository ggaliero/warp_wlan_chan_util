/** @file wlan_mac_sta_join.c
 *  @brief Join FSM
 *
 *  This contains code for the STA join process.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 *
 *
 *   The join process allows the STA to join a given BSS.  There are two major
 * components to the join process:  SEARCHING and ATTEMPTING.  If all the necessary
 * information is known about the BSS, then the join process only has to perform
 * the "ATTEMPTING" process.  However, if only a SSID is known, then the join
 * process must use the scan infrastructure in wlan_mac_scan.c/h to perform
 * the "SEARCHING" process to get more information about the BSS so that it can
 * proceed to the "ATTEMPTING" process.
 *
 *   In order to make the join process robust to any wireless environment (ie
 * robust to packet loss), the scheduler is used to "poll" functions as part
 * of the "SEARCHING" and "ATTEMPTING" process.  This will ensure that even
 * with packet loss, the join process will "re-attempt" its current step.
 *
 */

/***************************** Include Files *********************************/
#include "wlan_mac_high_sw_config.h"

// Xilinx SDK includes
#include "xparameters.h"
#include "xil_types.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// WLAN includes
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_scan.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_sta_join.h"
#include "wlan_mac_sta.h"


/*************************** Constant Definitions ****************************/


/*********************** Global Variable Definitions *************************/

extern mac_header_80211_common    tx_header_common;
extern u8                         pause_data_queue;
extern tx_params_t                default_unicast_mgmt_tx_params;
extern u8                         my_aid;
extern bss_info_t* 				  active_bss_info;

/*************************** Variable Definitions ****************************/

// Join FSM states
typedef enum {
    IDLE,
    SEARCHING,
    ATTEMPTING
} join_state_t;

typedef enum {
	UNAUTHENTICATED,
	AUTHENTICATED,
	ASSOCIATED
} authentication_state_t;

// Global join parameters
//     This variable needs to be treated as volatile since it is expected to be
//     modified by other contexts after a call to wlan_mac_join_get_parameters
volatile join_parameters_t        gl_join_parameters;


// Scan state variables
static join_state_t               join_state;
static authentication_state_t	  authentication_state;
static bss_info_t*                attempt_bss_info;
char*                             scan_ssid_save;

static u32                        search_sched_id;
static u32                        attempt_sched_id;

// Callback Function
//     Used to perform any tasks after a successful join
static function_ptr_t             join_success_callback;


/*************************** Functions Prototypes ****************************/

void start_join_attempt();

void wlan_mac_sta_join_bss_search_poll(u32 schedule_id);

void transmit_join_auth_req();
void transmit_join_assoc_req();

/******************************** Functions **********************************/


/*****************************************************************************/
/**
 * Initialize the join state
 *
 * This function will initialize the join state machine an set the join
 * parameters to the default values.
 *
 * @return  int              - Status: XST_SUCCESS or XST_FAILURE
 *
 *****************************************************************************/
int wlan_mac_sta_join_init(){

    // Initialize join success callback
	join_success_callback = (function_ptr_t)wlan_null_callback;

    // Set default join parameters
    bzero((u8*)gl_join_parameters.bssid, MAC_ADDR_LEN);
    gl_join_parameters.ssid    = NULL;
    gl_join_parameters.channel = 0;

    // Set global join state variables
    join_state       	 = IDLE;
    authentication_state = UNAUTHENTICATED;

    attempt_bss_info = NULL;
    scan_ssid_save   = NULL;

    search_sched_id  = SCHEDULE_ID_RESERVED_MAX;
    attempt_sched_id = SCHEDULE_ID_RESERVED_MAX;

    return XST_SUCCESS;
}



/*****************************************************************************/
/**
 * Set callbacks
 *
 *****************************************************************************/
void wlan_mac_sta_set_join_success_callback(function_ptr_t callback){
	join_success_callback = callback;
}



/*****************************************************************************/
/**
 * Get global join parameters structure
 *
 * This is in lieu of getter / setter methods for all of the join parameters.
 *
 * @return  volatile join_parameters_t*     - Pointer to join parameters
 *
 *****************************************************************************/
volatile join_parameters_t* wlan_mac_sta_get_join_parameters(){
	return &gl_join_parameters;
}

/*****************************************************************************/
/**
 * Get global bss_info pointer that STA is attempting to join
 *
 * @return  volatile bss_info*     - Pointer to bss_info
 *
 *****************************************************************************/
volatile bss_info_t* wlan_mac_sta_get_attempt_bss_info(){
	if(wlan_mac_sta_is_joining()){
		return attempt_bss_info;
	} else {
		return NULL;
	}
}


/*****************************************************************************/
/**
 * Is the node joining?
 *
 * @return  u32              - Is Joining?
 *                                 1 - Currently joining
 *                                 0 - Not joining
 *
 *****************************************************************************/
u32 wlan_mac_sta_is_joining(){
    if (join_state == IDLE) {
        return 0;
    } else {
        return 1;
    }
}

void wlan_mac_sta_successfully_authenticated(u8* bssid){
	if(attempt_bss_info && wlan_addr_eq(bssid, attempt_bss_info->bssid)){
		if(authentication_state == UNAUTHENTICATED){
			authentication_state = AUTHENTICATED;
			wlan_mac_sta_join_bss_attempt_poll(0);
		}
	}
}

void wlan_mac_sta_successfully_associated(u8* bssid, u16 AID){
	if(attempt_bss_info && wlan_addr_eq(bssid, attempt_bss_info->bssid)){
		if(authentication_state == AUTHENTICATED){
			authentication_state = ASSOCIATED;
			wlan_mac_sta_join_bss_attempt_poll(AID);
		}
	}
}

/*****************************************************************************/
/**
 * @brief Join an AP
 *
 * This function begins the process to join an AP with the parameters
 * present in the global gl_join_parameters.  Depending on the information
 * present in the gl_join_parameters, the STA may perform both the
 * "SEARCHING" and "ATTEMPTING" process or just the "ATTEMPTING" process.
 *
 * A value of NULL for the SSID element in join_parameters_t indicates
 * that the function should halt any ongoing attempts to join an AP.
 *
 * A value of {0,0,0,0,0,0} for the BSSID or a value of 0 for the channel,
 * will result in the join process first "SEARCHING" for the BSS.
 *
 *****************************************************************************/
void wlan_mac_sta_join(){
    volatile scan_parameters_t*     scan_parameters;

    // If the SSID is NULL, then we need to halt any joins
    if (gl_join_parameters.ssid == NULL) {
        wlan_mac_sta_join_return_to_idle();
    } else {

        // Check if the join state machine is already running:
        //     - Return the join state machine to IDLE
        //     - Re-enter function
        //
        // Currently, the join state machine is out of sync with the gl_join_parameters
        // since they have already been changed by a user.
        //
        if (wlan_mac_sta_is_joining()) {
            wlan_mac_sta_join_return_to_idle();
            wlan_mac_sta_join();
            return;
        }

        // Check if the BSSID or Channel are reserved values
        if (wlan_addr_eq(gl_join_parameters.bssid, zero_addr) || (gl_join_parameters.channel == 0)) {
            // Start an active scan and find the AP
            join_state = SEARCHING;

            // Get the current scan parameters
            scan_parameters = wlan_mac_scan_get_parameters();

            // Set the scan SSID
            //     - Save the current scan parameters SSID so to revert after the join has finished
            //     - strndup automatically mallocs memory for strings, but previous strings need to
            //       be freed manually
            //
            scan_ssid_save = strndup(scan_parameters->ssid, SSID_LEN_MAX);
            wlan_mac_high_free(scan_parameters->ssid);
            scan_parameters->ssid = strndup(gl_join_parameters.ssid, SSID_LEN_MAX);

            // Start the scan
            wlan_mac_scan_start();

            // Schedule wlan_mac_sta_bss_search_poll() to determine when the BSS is found
            //     - Timing for this poll can be adjusted using defines in wlan_mac_sta_join.h
            //
            search_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, BSS_SEARCH_POLL_INTERVAL_USEC, SCHEDULE_REPEAT_FOREVER, (void*)wlan_mac_sta_join_bss_search_poll);

        } else {
            // Create a BSS info from the current join parameters
            attempt_bss_info = wlan_mac_high_create_bss_info((u8*)gl_join_parameters.bssid,
                                                             (char*)gl_join_parameters.ssid,
                                                             (u8)gl_join_parameters.channel);

            // Stop any on-going scans
            wlan_mac_scan_stop();

            // Start the "ATTEMPTING" process
            start_join_attempt();
        }
    }
}



/*****************************************************************************/
/**
 * @brief Start the "ATTEMPTING" part of the join process
 *
 * This function begins the "ATTEMPTING" part of the join process.
 *
 *****************************************************************************/
void start_join_attempt() {

    // Check the BSS info that STA is attempting to join
    if (attempt_bss_info == NULL) {
        xil_printf("Join FSM Error: attempting to join BSS without setting attempt_bss_info\n");
        return;
    }

    // Set the join state
    join_state = ATTEMPTING;

    // Check the current association
    if (active_bss_info != NULL) {
        if (wlan_addr_eq(gl_join_parameters.bssid, active_bss_info->bssid)) {
            // Already associated with this BSS
            wlan_mac_sta_join_return_to_idle();
            return;
        } else {
            // Currently associated with a different BSS.  Disassociate
            // and continue to the join the new AP.
            sta_disassociate();
        }
    }

    // Pause the STA data queue since this will change association state
    //     - The STA data queue will be un-paused during the call to
    //       sta_set_association_state() if the join was successful
    //
    pause_data_queue = 1;

    // Move the AP's channel
    wlan_mac_high_set_radio_channel(wlan_mac_high_bss_channel_spec_to_radio_chan(attempt_bss_info->chan_spec));

    // Attempt to join the BSS
    wlan_mac_sta_join_bss_attempt_poll(0);

    // Schedule the "Attempt to join the BSS" in case a packet is lost
    //     - Timing for this poll can be adjusted using defines in wlan_mac_sta_join.h
    //
    attempt_sched_id = wlan_mac_schedule_event_repeated(SCHEDULE_FINE, BSS_ATTEMPT_POLL_INTERVAL_USEC, SCHEDULE_REPEAT_FOREVER, (void*)wlan_mac_sta_join_bss_attempt_poll);
}



/*****************************************************************************/
/**
 * @brief Stop the join process
 *
 * This function stops the join process and returns the FSM to "IDLE"
 *
 *****************************************************************************/
void wlan_mac_sta_join_return_to_idle(){

    volatile scan_parameters_t*     scan_parameters;
    interrupt_state_t               prev_interrupt_state;

    // Stop any on-going scans
    wlan_mac_scan_stop();

    // Stop interrupts while removing scheduled events
    prev_interrupt_state = wlan_mac_high_interrupt_stop();

    // Set the join state
    join_state = IDLE;
    authentication_state = UNAUTHENTICATED;

    // Remove any scheduled search polls
    if(search_sched_id != SCHEDULE_ID_RESERVED_MAX){
        wlan_mac_remove_schedule(SCHEDULE_FINE, search_sched_id);
        search_sched_id = SCHEDULE_ID_RESERVED_MAX;
    }

    // Remove any scheduled attempt polls
    if(attempt_sched_id != SCHEDULE_ID_RESERVED_MAX){
        wlan_mac_remove_schedule(SCHEDULE_FINE, attempt_sched_id);
        attempt_sched_id = SCHEDULE_ID_RESERVED_MAX;
    }

    // Restore interrupt state
    wlan_mac_high_interrupt_restore_state(prev_interrupt_state);

    // Remove "attempt_bss_info"
    attempt_bss_info = NULL;

    // Return the scan SSID parameter back to its previous state
    //     - Since wlan_mac_sta_return_to_idle() can be called at any time, the
    //       SSID should only be returned once lest the saved SSID in join get
    //       out of sync with the current scan SSID since the SSID is only saved
    //       when wlan_mac_sta_join() is called.
    //
    if (scan_ssid_save != NULL) {
        // Get the scan parameters
        scan_parameters = wlan_mac_scan_get_parameters();

        // Restore the SSID in the scan parameters with the value stored when
        // wlan_mac_sta_join() was called
        wlan_mac_high_free(scan_parameters->ssid);
        scan_parameters->ssid = strndup(scan_ssid_save, SSID_LEN_MAX);
        wlan_mac_high_free(scan_ssid_save);

        // Set the saved SSID to NULL
        scan_ssid_save = NULL;
     }
}



/*****************************************************************************/
/**
 * @brief Polling function during "SEARCHING" process
 *
 * This function is used during the "SEARCHING" process to check the state of
 * node to determine if it is able to move on to the "ATTEMPTING" part of the
 * join process.
 *
 * Once started, the "SEARCHING" process will continue forever until explicitly
 * stopped by calling wlan_mac_sta_join_return_to_idle().
 *
 *****************************************************************************/
void wlan_mac_sta_join_bss_search_poll(u32 schedule_id){
	dl_list*  ssid_match_list = NULL;
    dl_entry* curr_dl_entry   = NULL;

    switch(join_state){
        case IDLE:
            xil_printf("JOIN FSM Error: Searching/Idle mismatch\n");
        break;

        case SEARCHING:
            if (wlan_mac_scan_get_num_scans() >= 0) {
            	// In a future release, we may modify the join process to prioritize among
            	// networks with matching SSIDs. In this scenario, the condition on
            	// wlan_mac_scan_get_num_scans() may be made more strict to require at least
            	// one full "loop" through all channels.
                ssid_match_list = wlan_mac_high_find_bss_info_SSID(gl_join_parameters.ssid);

                if (ssid_match_list->length > 0) {
                    // Join the first entry in the list
                    //     - This could be modified in the future to use some other selection,
                    //       for example RX power.
                    //
                    curr_dl_entry = ssid_match_list->first;

                    // Return to the IDLE state
                    //     - Will stop the current scan
                    //     - This must be done before setting "attempt_bss_info"
                    wlan_mac_sta_join_return_to_idle();

                    // Set bss info to attempt to join
                    attempt_bss_info = (bss_info_t*)(curr_dl_entry->data);

	               // Start the "ATTEMPTING" process
	               start_join_attempt();
                }
            }
        break;

        case ATTEMPTING:
            xil_printf("Join FSM Error: Searching/Attempting mismatch\n");
        break;
    }
}



/*****************************************************************************/
/**
 * Attempt to join the BSS
 *
 * This function is used during the "ATTEMPTING" process to check the state of
 * node to determine if it is able to finish the join process.  This function
 * is designed to be called multiple times during each join attempt and will
 * perform a different action based on the state of the BSS.  This function will
 * attempt to move the BSS from:
 *       UNAUTHENTICATED --> AUTHENTICATED --> ASSOCIATED
 *
 * This function can be called by the STA directly as part of the mpdu_rx_process()
 * function when receiving responses from the AP or it will be called periodically
 * by the scheduler.  This will ensure that the join process is robust even if a
 * packet is lost.
 *
 * Once started, the "ATTEMPTING" process will continue forever until explicitly
 * stopped by calling wlan_mac_sta_join_return_to_idle().
 *
 * @param  aid               - Association ID of the join
 *
 * Given that this function can be called while the BSS is is different states, the
 * association ID can be ignored (and will have a bogus value) in certain contexts.
 * Depending on who is calling this function, the argument means different things:
 *   (1) When called by the scheduler, the argument is automatically filled in with the schedule ID
 *       but the state will always be BSS_STATE_UNAUTHENTICATED or BSS_STATE_AUTHENTICATED, so the
 *       aid will be ignored.
 *   (2) STA will call this when it receives an authentication packet that elevates the state
 *       to BSS_AUTHENTICATED. In this context, the argument is meaningless (explicitly 0 valued)
 *   (3) STA will call this when it receives an association response that elevates the state to
 *       BSS_ASSOCIATED. In this context, the argument will be the AID provided by the AP sending
 *       the association response.
 *
 *****************************************************************************/
void wlan_mac_sta_join_bss_attempt_poll(u32 aid){
    bss_config_t   bss_config;

    if (attempt_bss_info == NULL) {
        wlan_mac_sta_join_return_to_idle();
        return;
    }

    switch(join_state){
        case IDLE:
             xil_printf("JOIN FSM Error: Attempting/Idle mismatch\n");
        break;

        case SEARCHING:
             xil_printf("JOIN FSM Error: Attempting/Searching mismatch\n");
        break;

        case ATTEMPTING:
            switch(authentication_state){
                case UNAUTHENTICATED:
                    transmit_join_auth_req();
                break;

                case AUTHENTICATED:
                    transmit_join_assoc_req();
                break;

                case ASSOCIATED:
                    // Important: wlan_mac_sta_join_return_to_idle() will NULL out attempt_bss_info,
                    //     so it should not be called before actually setting the association state

                    // Set AID
                    my_aid = aid;

                    // Create BSS config from attempt_bss_info
                    memcpy(bss_config.bssid, attempt_bss_info->bssid, MAC_ADDR_LEN);
                    strncpy(bss_config.ssid, attempt_bss_info->ssid, SSID_LEN_MAX);

                    bss_config.chan_spec       = attempt_bss_info->chan_spec;
                    bss_config.ht_capable      = 1;
                    bss_config.beacon_interval = attempt_bss_info->beacon_interval;
                    bss_config.update_mask     = (BSS_FIELD_MASK_BSSID           |
                                                  BSS_FIELD_MASK_CHAN            |
                                                  BSS_FIELD_MASK_SSID            |
                                                  BSS_FIELD_MASK_BEACON_INTERVAL |
                                                  BSS_FIELD_MASK_HT_CAPABLE);

                    if (configure_bss(&bss_config) == 0) {
                        // Join was successful, so execute callback
                        join_success_callback(attempt_bss_info);
                    } else {
                        // Join was not successful
                        xil_printf("Unable to associate with BSS %s\n", attempt_bss_info->ssid);
                    }

                    // Stop the join process
                    wlan_mac_sta_join_return_to_idle();
                break;

                default:
                    xil_printf("Error: STA attempt poll: Unknown state %d for BSS info %s\n", authentication_state, attempt_bss_info->ssid);
                break;
            }
        break;
    }
}



/*****************************************************************************/
/**
 * Transmit authentication request
 *
 * This internal function will transmit an authentication request if the state
 * machine is in the "ATTEMPTING" to join state.
 *
 *****************************************************************************/
void transmit_join_auth_req(){
    u16                 tx_length;
    tx_queue_element_t*   curr_tx_queue_element;
    tx_queue_buffer_t*    curr_tx_queue_buffer;

    // Only transmit if FSM is "ATTEMPTING" to join
    if (join_state == ATTEMPTING) {

        curr_tx_queue_element = queue_checkout();

        if (curr_tx_queue_element != NULL) {
            curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

            // Setup the TX header
            wlan_mac_high_setup_tx_header(&tx_header_common, attempt_bss_info->bssid, attempt_bss_info->bssid);

            // Fill in the data
            tx_length = wlan_create_auth_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, AUTH_ALGO_OPEN_SYSTEM, AUTH_SEQ_REQ, STATUS_SUCCESS);

            // Setup the TX frame info
            wlan_mac_high_setup_tx_frame_info (&tx_header_common, curr_tx_queue_element, tx_length, (TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_REQ_TO), MANAGEMENT_QID );

            // Set the information in the TX queue buffer
            curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
            curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
            curr_tx_queue_buffer->tx_frame_info.ID         = 0;

            // Put the packet in the queue
            enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

            // Poll the TX queues to possibly send the packet
            poll_tx_queues();
        }
    }
}



/*****************************************************************************/
/**
 * Transmit association request
 *
 * This internal function will transmit an association request if the state
 * machine is in the "ATTEMPTING" to join state.
 *
 *****************************************************************************/
void transmit_join_assoc_req(){
    u16                 tx_length;
    tx_queue_element_t*   curr_tx_queue_element;
    tx_queue_buffer_t*    curr_tx_queue_buffer;

    // Only transmit if FSM is "ATTEMPTING" to join
    if (join_state == ATTEMPTING) {

        curr_tx_queue_element = queue_checkout();

        if (curr_tx_queue_element != NULL) {
            curr_tx_queue_buffer = (tx_queue_buffer_t*)(curr_tx_queue_element->data);

            // Setup the TX header
            wlan_mac_high_setup_tx_header(&tx_header_common, attempt_bss_info->bssid, attempt_bss_info->bssid);

            // Fill in the association request frame
            tx_length = wlan_create_association_req_frame((void*)(curr_tx_queue_buffer->frame), &tx_header_common, attempt_bss_info);

            // Setup the TX frame info
            wlan_mac_high_setup_tx_frame_info(&tx_header_common, curr_tx_queue_element, tx_length, (TX_FRAME_INFO_FLAGS_FILL_DURATION | TX_FRAME_INFO_FLAGS_REQ_TO), MANAGEMENT_QID);

            // Set the information in the TX queue buffer
            curr_tx_queue_buffer->metadata.metadata_type = QUEUE_METADATA_TYPE_TX_PARAMS;
            curr_tx_queue_buffer->metadata.metadata_ptr  = (u32)(&default_unicast_mgmt_tx_params);
            curr_tx_queue_buffer->tx_frame_info.ID          = 0;

            // Put the packet in the queue
            enqueue_after_tail(MANAGEMENT_QID, curr_tx_queue_element);

            // Poll the TX queues to possibly send the packet
            poll_tx_queues();
        }
    }
}


