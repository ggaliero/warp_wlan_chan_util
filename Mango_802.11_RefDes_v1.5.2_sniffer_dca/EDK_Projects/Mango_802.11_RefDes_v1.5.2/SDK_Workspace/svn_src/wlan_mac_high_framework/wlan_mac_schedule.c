/** @file wlan_mac_schedule.c
 *  @brief Scheduler
 *
 *  This set of functions allows upper-level MAC implementations
 *	to schedule the execution of a provided callback for some point
 *	in the future.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/
#include "wlan_mac_high_sw_config.h"

#include "xil_types.h"
#include "xil_exception.h"

#include "xparameters.h"
#include "xtmrctr.h"
#include "xintc.h"

#include "wlan_mac_time_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_schedule.h"


/*************************** Constant Definitions *****************************/

#define WLAN_SCHED_EXEC_MONITOR                            0
#define WLAN_SCHED_EXEC_MONITOR_DEFAULT_THRESHOLD          100000         // 100 ms


/*************************** Variable Definitions ****************************/
static XTmrCtr               timer_instance;

volatile static u32          schedule_count;


static wlan_sched_state_t    wlan_sched_coarse;
static wlan_sched_state_t    wlan_sched_fine;
static dl_list				 disabled_list;

#if WLAN_SCHED_EXEC_MONITOR
static u64                   last_exec_timestamp;
static u64                   schedule_exec_monitor;
static u32                   monitor_threshold;
#endif


/*************************** Function Prototypes *****************************/

void          timer_interrupt_handler(void * instancePtr);
void          schedule_handler(void * callback_ref, u8 timer_number);


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Initializes the schedulers.
 *
 * @param    None
 *
 * @return   None
 *
 *****************************************************************************/
int wlan_mac_schedule_init(){
	int status;

	// Initialize internal variables
	schedule_count           = 1;

#if WLAN_SCHED_EXEC_MONITOR
	last_exec_timestamp      = 0;
	schedule_exec_monitor    = 0;
	monitor_threshold        = WLAN_SCHED_EXEC_MONITOR_DEFAULT_THRESHOLD;
#endif

	dl_list_init(&(wlan_sched_coarse.enabled_list));
	wlan_sched_coarse.next = NULL;
	dl_list_init(&(wlan_sched_fine.enabled_list));
	wlan_sched_fine.next = NULL;

	dl_list_init(&disabled_list);

	// Initialize the timer
	// The driver for the timer does not handle a reboot of CPU_HIGH
	// gracefully. Before initializing it, we should stop any running
	// timers. Note: we cannot use the official XTmrCtr_Stop() function
	// for this since it requires that the timer instance be initialized.
	u32 ControlStatusReg;
	u8	i;
	XTmrCtr_Config *TmrCtrConfigPtr = XTmrCtr_LookupConfig(TMRCTR_DEVICE_ID);

	if(TmrCtrConfigPtr != NULL){
		for(i=0; i<2; i++){
			//Loop over timer instances
			/*
			 * Read the current register contents
			 */
			ControlStatusReg = XTmrCtr_ReadReg(TmrCtrConfigPtr->BaseAddress,
										i, XTC_TCSR_OFFSET);

			/*
			 * Disable the timer counter such that it's not running
			 */
			ControlStatusReg &= ~(XTC_CSR_ENABLE_TMR_MASK);

			/*
			 * Write out the updated value to the actual register.
			 */
			XTmrCtr_WriteReg(TmrCtrConfigPtr->BaseAddress, i,
					  XTC_TCSR_OFFSET, ControlStatusReg);

		}



		status = XTmrCtr_Initialize(&timer_instance, TMRCTR_DEVICE_ID);
		if ( (status != XST_SUCCESS) ) {
			xil_printf("ERROR:  XTmrCtr failed to initialize\n");
			return -1;
		}

		// Set the handler for Timer
		XTmrCtr_SetHandler(&timer_instance, schedule_handler, &timer_instance);

		// Enable interrupt of timer and auto-reload so it continues repeatedly
		XTmrCtr_SetOptions(&timer_instance, TIMER_CNTR_FAST, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
		XTmrCtr_SetOptions(&timer_instance, TIMER_CNTR_SLOW, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

		return 0;
	} else {
		//Could not find XTmrCtr_Config for the timer
		return -1;
	}
}



/*****************************************************************************/
/**
 * Set up scheduler interrupt
 *
 * @param   intc             - pointer to instance of interrupt controller
 *
 * @return  status           - success or failure of attempt to connect
 *
 *****************************************************************************/
int wlan_mac_schedule_setup_interrupt(XIntc* intc){
	int status;

	status =  XIntc_Connect(intc, TMRCTR_INTERRUPT_ID, (XInterruptHandler)timer_interrupt_handler, &timer_instance);
	XIntc_Enable(intc, TMRCTR_INTERRUPT_ID);

	return status;
}



/*****************************************************************************/
/**
 * Schedules the execution of a callback for some time in the future
 *
 * @param   scheduler_sel    - SCHEDULE_COARSE or SCHEDULE_FINE
 * @param   delay            - Interval (in microseconds) until callback should be called
 * @param   num_calls        - Number of repetitions or SCHEDULE_REPEAT_FOREVER for permanent periodic
 * @param   callback         - Function pointer to callback
 *
 * @return  id               - ID of scheduled event or SCHEDULE_FAILURE if error
 *
 *****************************************************************************/
u32 wlan_mac_schedule_event_repeated(u8 scheduler_sel, u32 delay, u32 num_calls, void(*callback)()){
	u32            id;
	dl_entry     * entry_ptr;
	wlan_sched   * sched_ptr;
	u64            curr_system_time;

	// Allocate memory for data structures
	entry_ptr = wlan_mac_high_malloc(sizeof(dl_entry));
	if (entry_ptr == NULL) { return SCHEDULE_FAILURE; }

	sched_ptr = wlan_mac_high_malloc(sizeof(wlan_sched));
	if (sched_ptr == NULL) { wlan_mac_high_free(entry_ptr); return SCHEDULE_FAILURE; }

	// Attach the schedule struct to this dl_entry
	entry_ptr->data = sched_ptr;

	do{
		// Get Schedule ID from global counter
		id = (schedule_count++);

		// Check if we hit the section of reserved IDs; Wrap back to 0 and start again
		if ((id >= SCHEDULE_ID_RESERVED_MIN) && (id <= SCHEDULE_ID_RESERVED_MAX)) { id = 1; }

		// Check to make sure that this id has not been issued to any currently
		// scheduled events
	} while( (wlan_mac_schedule_find(&wlan_sched_coarse.enabled_list, id) != NULL) ||
			 (wlan_mac_schedule_find(&wlan_sched_fine.enabled_list, id) != NULL) ||
			 (wlan_mac_schedule_find(&disabled_list, id) != NULL));

	// Get the current system time
	curr_system_time     = get_system_time_usec();

	// Initialize the schedule struct
	sched_ptr->enabled	 = 1;
	sched_ptr->id        = id;
	sched_ptr->delay_us  = delay;
	sched_ptr->num_calls = num_calls;
	sched_ptr->target_us = curr_system_time + (u64)(delay);
	sched_ptr->callback  = (function_ptr_t)callback;

	// Add schedule struct to the appropriate schedule list
	switch(scheduler_sel){
		// ------------------------------------------------
		case SCHEDULE_COARSE:
			// Start timer if the list goes from 0 -> 1 event
			if (wlan_sched_coarse.enabled_list.length == 0) {
				XTmrCtr_SetResetValue(&timer_instance, TIMER_CNTR_SLOW, (SLOW_TIMER_DUR_US * TIMER_CLKS_PER_US));
				XTmrCtr_Start(&timer_instance, TIMER_CNTR_SLOW);
			}

			dl_entry_insertBeginning(&(wlan_sched_coarse.enabled_list), entry_ptr);
		break;

		// ------------------------------------------------
		case SCHEDULE_FINE:
			// Start timer if the list goes from 0 -> 1 event
			if(wlan_sched_fine.enabled_list.length == 0){
				XTmrCtr_SetResetValue(&timer_instance, TIMER_CNTR_FAST, (FAST_TIMER_DUR_US * TIMER_CLKS_PER_US));
				XTmrCtr_Start(&timer_instance, TIMER_CNTR_FAST);
			}

			dl_entry_insertBeginning(&(wlan_sched_fine.enabled_list), entry_ptr);
		break;

		// ------------------------------------------------
		default:
			xil_printf("Unknown scheduler selection.  No event scheduled.\n");
			return SCHEDULE_FAILURE;
		break;
	}

	return id;
}


dl_entry* wlan_mac_schedule_disable_id(u8 scheduler_sel, u32 sched_id){
	dl_entry* sched_entry;
	dl_list* enabled_list;
	u8 timer_sel;

	switch(scheduler_sel){
		case SCHEDULE_COARSE:
			enabled_list = &(wlan_sched_coarse.enabled_list);
			timer_sel = TIMER_CNTR_SLOW;
		break;

		case SCHEDULE_FINE:
			enabled_list = &(wlan_sched_fine.enabled_list);
			timer_sel = TIMER_CNTR_FAST;
		break;

		default:
			return NULL;
		break;
	}

	sched_entry = wlan_mac_schedule_find(enabled_list, sched_id);

	if(sched_entry != NULL){
		if( ((wlan_sched*)(sched_entry->data))->num_calls != 0 ){
			dl_entry_remove(enabled_list, sched_entry);
			dl_entry_insertEnd(&disabled_list, sched_entry);
			((wlan_sched*)(sched_entry->data))->enabled = 0;
		} else {
			// wlan_mac_schedule_disable_id() is not supported for a schedule whose num_calls
			// is 0. The most common occurence of this scenario is trying to disable a schedule
			// from the final execution of a callback of that schedule.
			return NULL;
		}
	}

	// Stop the timer if there are no more events
	//     NOTE:  Will be restarted when an event is added or re-enabled
	if (enabled_list->length == 0) {
		XTmrCtr_Stop(&timer_instance, timer_sel);
	}

	return sched_entry;
}

int wlan_mac_schedule_enable(u8 scheduler_sel, dl_entry* sched_entry){

	dl_list* enabled_list;
	u8 timer_sel;
	u32 reset_value;

	if(sched_entry == NULL){
		return -1;
	}
	switch(scheduler_sel){
		case SCHEDULE_COARSE:
			enabled_list = &(wlan_sched_coarse.enabled_list);
			timer_sel = TIMER_CNTR_SLOW;
			reset_value = (SLOW_TIMER_DUR_US * TIMER_CLKS_PER_US);
		break;

		case SCHEDULE_FINE:
			enabled_list = &(wlan_sched_fine.enabled_list);
			timer_sel = TIMER_CNTR_FAST;
			reset_value = (FAST_TIMER_DUR_US * TIMER_CLKS_PER_US);
		break;

		default:
			return -1;
		break;
	}

	dl_entry_remove(&disabled_list, sched_entry);
	dl_entry_insertEnd(enabled_list, sched_entry);

	((wlan_sched*)(sched_entry->data))->enabled = 1;
	((wlan_sched*)(sched_entry->data))->target_us = get_system_time_usec() + ((wlan_sched*)(sched_entry->data))->delay_us;

	// Start timer if the list goes from 0 -> 1 event
	if(wlan_sched_fine.enabled_list.length == 1){
		XTmrCtr_SetResetValue(&timer_instance, timer_sel, reset_value);
		XTmrCtr_Start(&timer_instance, timer_sel);
	}

	return 0;

}


/*****************************************************************************/
/**
 * @brief  Cancels the execution of a scheduled callback
 *
 * @param   scheduler_sel    - SCHEDULE_COARSE or SCHEDULE_FINE
 * @param   id               - ID of schedule that should be removed
 *
 * @return  None
 *
 * @note    This function will fail silently if the ID parameter does not match
 *          any currently running schedule event IDs.
 *
 *****************************************************************************/
void wlan_mac_remove_schedule(u8 scheduler_sel, u32 id){
	dl_entry     * curr_entry_ptr;
	wlan_sched   * curr_sched_ptr;

	dl_list* enabled_list;

	switch(scheduler_sel){
		case SCHEDULE_COARSE:
			enabled_list = &(wlan_sched_coarse.enabled_list);
		break;

		case SCHEDULE_FINE:
			enabled_list = &(wlan_sched_fine.enabled_list);
		break;

		default:
			return;
		break;
	}

	curr_entry_ptr = wlan_mac_schedule_find(enabled_list, id);

	if (curr_entry_ptr != NULL) {

		// Extract the schedule struct from dl_entry
		curr_sched_ptr = (wlan_sched*)(curr_entry_ptr->data);

		switch (scheduler_sel) {
			// ------------------------------------------------
			case SCHEDULE_COARSE:
				if(curr_sched_ptr != NULL){
					if(wlan_sched_coarse.next == curr_entry_ptr){
						// The next dl_entry in the schedule_handler iteration
						// is the schedule we are about to remove. We need to advance
						// this pointer prior to removing so that we do not break
						// the execution of schedule_handler.
						wlan_sched_coarse.next = dl_entry_next(curr_entry_ptr);
					}
					dl_entry_remove(&(wlan_sched_coarse.enabled_list), curr_entry_ptr);
					wlan_mac_high_free(curr_entry_ptr);
					wlan_mac_high_free(curr_sched_ptr);
				}

				// Stop the timer if there are no more events
				//     NOTE:  Will be restarted when new event is added
				if (wlan_sched_coarse.enabled_list.length == 0) {
					XTmrCtr_Stop(&timer_instance, TIMER_CNTR_SLOW);
				}
			break;

			// ------------------------------------------------
			case SCHEDULE_FINE:
				if(curr_sched_ptr != NULL){
					if(wlan_sched_fine.next == curr_entry_ptr){
						// The next dl_entry in the schedule_handler iteration
						// is the schedule we are about to remove. We need to advance
						// this pointer prior to removing so that we do not break
						// the execution of schedule_handler.
						wlan_sched_fine.next = dl_entry_next(curr_entry_ptr);
					}
					dl_entry_remove(&(wlan_sched_fine.enabled_list), curr_entry_ptr);
					wlan_mac_high_free(curr_entry_ptr);
					wlan_mac_high_free(curr_sched_ptr);
				}

				// Stop the timer if there are no more events
				//     NOTE:  Will be restarted when new event is added
				if (wlan_sched_fine.enabled_list.length == 0){
					XTmrCtr_Stop(&timer_instance, TIMER_CNTR_FAST);
				}
			break;

			// ------------------------------------------------
			default:
				xil_printf("Unknown scheduler selection.  No event removed.\n");
			break;
		}

#if WLAN_SCHED_EXEC_MONITOR
		// If both timers are stopped, then we need to reset the last_exec_timestamp
		//     NOTE:  This is so we do not get erroneous results when the timer is off
		//         for extended periods of time.
		if ((wlan_sched_coarse.length == 0) && (wlan_sched_fine.length == 0)) {
			last_exec_timestamp = 0;
		}
#endif
    }
}



/*****************************************************************************/
/**
 * Timer interrupt handler
 *
 * @param   instance_ptr     - Pointer to the timer instance
 *
 * @return  None
 *
 * @note    This function is a modified implementation of XTmrCtr_InterruptHandler
 *          in the xtmrctr_intr.c driver. It has been modified to remove an explicit
 *          reset of the timer.
 *
 *****************************************************************************/
void timer_interrupt_handler(void * instance_ptr){

	XTmrCtr      * timer_ptr;
	u8             timer_number;
	u32            base_addr;
	u32            csr_reg;

#ifdef _ISR_PERF_MON_EN_
	wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif

	// Verify inputs are valid
	if (instance_ptr == NULL) { return; }

	timer_ptr = (XTmrCtr *) instance_ptr;
	base_addr = timer_ptr->BaseAddress;

	// Loop thru each timer counter in the device and call the callback
	// function for each timer which has caused an interrupt
	for (timer_number = 0; timer_number < XTC_DEVICE_TIMER_COUNT; timer_number++) {

		// Read Control / Status register
		csr_reg = XTmrCtr_ReadReg(base_addr, timer_number, XTC_TCSR_OFFSET);

		// Check if interrupt is enabled
		if (csr_reg & XTC_CSR_ENABLE_INT_MASK) {

			// Check if timer expired and interrupt occured
			if (csr_reg & XTC_CSR_INT_OCCURED_MASK) {
				// Increment statistics for the number of interrupts
				timer_ptr->Stats.Interrupts++;

				// Call callback to handle timer processing
				timer_ptr->Handler(timer_ptr->CallBackRef, timer_number);

				// Read Control / Status register
				csr_reg = XTmrCtr_ReadReg(base_addr, timer_number, XTC_TCSR_OFFSET);

				// Acknowledge the interrupt
                //     NOTE: It's very important that this ack occurs after the handler call. This 
                //         will prevent a scenario where a fine schedule callback can "crash" a 
                //         node by taking longer than whatever the interval is of the fast timer.
				XTmrCtr_WriteReg(base_addr, timer_number, XTC_TCSR_OFFSET, (csr_reg | XTC_CSR_INT_OCCURED_MASK));
			}
		}
	}

#ifdef _ISR_PERF_MON_EN_
	wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
}



/*****************************************************************************/
/**
 * Internal callback used by the timer interrupt handler. This function should
 * not be called by the upper-level MAC.
 *
 * @param   callback_ref   - Reserved (Not currently used)
 * @param   timer_number   - ID of which timer has expired
 *
 * @return  None
 *
 *****************************************************************************/
void schedule_handler(void * callback_ref, u8 timer_number){
	dl_entry* 			curr_entry_ptr;
	wlan_sched*    		curr_sched_ptr;

	u8             		scheduler;
	volatile wlan_sched_state_t* wlan_sched_state;
	u32            		sched_id;
	function_ptr_t 		sched_callback;

	u64            curr_system_time;

	volatile static 		u8 debug_print = 0;

	// Get current system time
	curr_system_time = get_system_time_usec();

#if WLAN_SCHED_EXEC_MONITOR
	// ----------------------------------------------------
	// Scheduler Monitor Implementation
	//
	// Collect the "time between executions" of the schedule_handler function.
	//
	// The first time through the handler after the timer has been started,
	// the last_exec_timestamp will be zero.  Therefore, we want to wait until
	// subsequent loops to get the "time between executions" (ie the value
	// recorded by the schedule_exec_monitor variable).
	//
	if (last_exec_timestamp != 0) {
		schedule_exec_monitor = curr_system_time - last_exec_timestamp;
	}

    // If the monitor threshold is exceeded, print a warning.
	//     NOTE:  Since prints take a long time, update the system time so the
	//         print does not affect the collection of "time between executions"
	if (schedule_exec_monitor > monitor_threshold) {
		xil_printf("WARNING:  %d us between scheduler executions.\n", schedule_exec_monitor);
		curr_system_time = get_system_time_usec();
	}

	// Store the current time in the last_timer_timestamp
	last_exec_timestamp = curr_system_time;
#endif

	// Set any timer specific variables
	if (timer_number == TIMER_CNTR_FAST) {
		scheduler  = SCHEDULE_FINE;
		wlan_sched_state = &(wlan_sched_fine);
	} else {
		// All other timers default to coarse scheduler
		scheduler  = SCHEDULE_COARSE;
		wlan_sched_state = &(wlan_sched_coarse);
	}

	// Get the first entry of the schedule dl_list
	wlan_sched_state->next = wlan_sched_state->enabled_list.first;

	// Update the appropriate scheduler
	while (wlan_sched_state->next != NULL) {
		curr_entry_ptr = wlan_sched_state->next;
		wlan_sched_state->next = dl_entry_next(wlan_sched_state->next);
		curr_sched_ptr = (wlan_sched*)(curr_entry_ptr->data);

		if(debug_print){
			xil_printf("curr_sched_ptr = 0x%08x\n", curr_sched_ptr);
			xil_printf("curr_sched_ptr->callback = 0x%08x\n", curr_sched_ptr->callback);
			xil_printf("curr_sched_ptr->id = %d\n", curr_sched_ptr->id);
		}

		// If the target time has passed, then process the event
		if (curr_system_time >= (curr_sched_ptr->target_us)) {
			sched_id       = curr_sched_ptr->id;
			sched_callback = curr_sched_ptr->callback;

			// Update the number of scheduled event calls
			if ((curr_sched_ptr->num_calls != SCHEDULE_REPEAT_FOREVER) &&
				(curr_sched_ptr->num_calls != 0)) {
				(curr_sched_ptr->num_calls)--;
			}

			sched_callback(sched_id);

			// The update of the target or removal of the schedule occurs after the callback so that the callback
			// has the opportunity to "save" the schedule. It can do this by updating "num_calls" or by calling
			// wlan_mac_schedule_disable
			if(curr_sched_ptr->enabled == 1){
				if(curr_sched_ptr->num_calls == 0){
					// Remove event (stops timer if necessary)
					wlan_mac_remove_schedule(scheduler, sched_id);
				} else {
					// Update scheduled event for next execution
					curr_sched_ptr->target_us = curr_system_time + (u64)(curr_sched_ptr->delay_us);
				}
			}
		}
	}
}



/*****************************************************************************/
/**
 * Find schedule that corresponds to a given ID
 *
 * @param   dl_list*        - List to search through; typically the fine, coarse, and deactivated lists
 * @param   id              - ID of the scheduler that should be returned
 *
 * @return  dl_entry*       - Pointer to the list entry that contains the schedule
 *
 *****************************************************************************/
dl_entry* wlan_mac_schedule_find(dl_list* sched_list, u32 id){
	int            iter;

	dl_entry     * curr_dl_entry;
	dl_entry     * next_dl_entry;
	wlan_sched   * curr_wlan_sched;

	// Initialize the loop variables
	iter          = sched_list->length;
	next_dl_entry = sched_list->first;

	// Process the list
	while ((next_dl_entry != NULL) && (iter-- > 0)) {
		curr_dl_entry   = next_dl_entry;
		next_dl_entry   = dl_entry_next(next_dl_entry);
		curr_wlan_sched = (wlan_sched*)(curr_dl_entry->data);

		if(curr_wlan_sched->id == id){
			return curr_dl_entry;
		}
	}

	return NULL;
}

