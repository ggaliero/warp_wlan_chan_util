/** @file wlan_mac_event_log.c
 *  @brief MAC Event Log Framework
 *
 *  Contains code for logging MAC events in DRAM.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @note
 *      The event log implements a circular buffer that will record various
 * event entries that occur within a WLAN node.  If the buffer is full, then
 * entries will be dropped with only a single warning printed to the screen.
 *
 *    There are configuration options to enable / disable wrapping (ie if
 * wrapping is enabled, then the buffer is never "full" and the oldest
 * events will be overwritten when there is no more free space).  Wrapping
 * is disabled by default.
 *
 *    Internally, the event log is just an array of bytes which can be externally
 * viewed as indexed from 0 to log_size (address translation is done internally).
 * When a new entry is requested, the size of the entry is allocated from the
 * buffer and a pointer to the allocated entry is provided so that the caller
 * can fill in the event information.  By default, the event log will set up all
 * header information (defined in wlan_mac_event_log.h) and that information
 * will not be exposed to user code.
 *
 *    The event log will always provide a contiguous piece of memory for events.
 * Therefore, some space could be wasted at the wrap boundary since a single event
 * will never wrap.
 *
 *    Also, if an entry cannot be allocated due to it overflowing the array, then
 * the event log will check to see if wrapping is enabled.  If wrapping is disabled,
 * the event log will set the full flag and not allow any more events to be
 * allocated.  Otherwise, the event log will wrap and begin to overwrite the
 * oldest entries.
 *
 *   Finally, the log does not keep track of event entries and it is up to
 * calling functions to interpret the bytes within the log correctly.
 *
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */

/***************************** Include Files *********************************/
#include "wlan_mac_high_sw_config.h"

// SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xstatus.h"
#include "xil_types.h"

// WLAN includes
#include "wlan_mac_time_util.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_high.h"

// WLAN Exp includes
#include "wlan_exp_node.h"


#if WLAN_SW_CONFIG_ENABLE_LOGGING

/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/



/*************************** Variable Definitions ****************************/

// Log definition variables
volatile static u32          log_start_address;        // Absolute start address of the log
volatile static u32          log_soft_end_address;     // Soft end address of the log
volatile static u32          log_max_address;          // Absolute end address of the log

volatile static u32          log_size;                 // Size of the log in bytes

// Log index variables
volatile static u32          log_oldest_address;       // Pointer to the oldest entry
volatile static u32          log_next_address;         // Pointer to the next entry
volatile static u32          log_num_wraps;            // Number of times the log has wrapped

// Log config variables
volatile static u8           log_wrap_enabled;         // Will the log wrap or stop; By default wrapping is DISABLED
volatile static u8           event_logging_enabled;    // Will events be logged or not; By default logging is ENABLED
volatile static u16          wrap_buffer;              // Number of additional bytes that will be "erased" when
                                                       //   the log increments the oldest address (default = EVENT_LOG_WRAP_BUFFER)

// Log status variables
volatile static u8           log_empty;                // log_empty = (log_oldest_address == log_next_address);
volatile static u8           log_full;                 // log_full  = (log_tail_address == log_next_address);
volatile static u16          log_count;                // Monotonic counter for log entry sequence number
                                                       //   (wraps every (2^16 - 1) entries)

// Mutex for critical allocation loop
volatile static u8           allocation_mutex;


/*************************** Functions Prototypes ****************************/

// Internal functions;  Should not be called externally
//
void event_log_move_oldest_address(u32 end_address);
void event_log_increment_oldest_address(u64 end_address, u32 size);
int  event_log_get_next_empty_address(u32 size, u32 * address);


/******************************** Functions **********************************/




/*****************************************************************************/
/**
 * Initialize the event log
 *
 * @param   start_address    - Starting address of the event log
 * @param   size             - Size in bytes of the event log
 *
 * @return  None
 *
 * @note    The event log will only use a integer number of event entries so
 *          any bytes over an integer number will be unused.
 *
 *****************************************************************************/
void event_log_init( char * start_address, u32 size ) {

    u32 alignment;
    u32 disable_log = 0;

    // Make sure that the start_address is 64-bit aligned
    alignment = ((u32)start_address) % 8;

    if (alignment != 0) {
        start_address = (char *)(((u32)start_address) + (8 - alignment));
    }

    // The log needs to be at least 4kB long otherwise there is not enough space
    // to put entries (~2x the largest entry)
    if (size < 4096) {
        xil_printf("WARNING: Event log (%d bytes) at 0x%x too small!!!\n", size, start_address);
        xil_printf("         Disabled event log.\n");
        disable_log = 1;
    } else {
        xil_printf("Initializing Event log (%d bytes) at 0x%x \n", size, start_address);
    }

    // Set default state of the logging
    if (EVENT_LOG_DEFAULT_LOGGING) {
        event_log_config_logging(EVENT_LOG_LOGGING_ENABLE);
    } else {
        event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
    }

    // Set the global variables that describe the log
    log_size          = size;
    log_start_address = (u32) start_address;
    log_max_address   = log_start_address + log_size - 1;

    // Set wrapping to be disabled
    log_wrap_enabled  = 0;

    // Set the wrap buffer to EVENT_LOG_WRAP_BUFFER
    wrap_buffer       = EVENT_LOG_WRAPPING_BUFFER;

    // Reset all the event log variables
    event_log_reset();

    // Disable the log if needed
    if (disable_log == 1) {
        event_log_config_logging(EVENT_LOG_LOGGING_DISABLE);
    }

#ifdef _DEBUG_
    xil_printf("    log_size             = 0x%x;\n", log_size );
    xil_printf("    log_start_address    = 0x%x;\n", log_start_address );
    xil_printf("    log_max_address      = 0x%x;\n", log_max_address );
    xil_printf("    log_soft_end_address = 0x%x;\n", log_soft_end_address );
    xil_printf("    log_oldest_address   = 0x%x;\n", log_oldest_address );
    xil_printf("    log_next_address     = 0x%x;\n", log_next_address );
    xil_printf("    log_empty            = 0x%x;\n", log_empty );
    xil_printf("    log_full             = 0x%x;\n", log_full );
    xil_printf("    allocation_mutex     = 0x%x;\n", allocation_mutex );
#endif
}



/*****************************************************************************/
/**
 * Reset the event log
 *
 * @param   None
 *
 * @return  None
 *
 * @note    This will not change the state of the wrapping configuration
 *
 *****************************************************************************/
void event_log_reset(){
    log_soft_end_address = log_max_address;

    log_oldest_address   = log_start_address;
    log_next_address     = log_start_address;
    log_num_wraps        = 0;

    log_empty            = 1;
    log_full             = 0;
    log_count            = 0;

    allocation_mutex     = 0;

    // Add a node info entry to the log
    //
    // NOTE:  A node_info_entry is guaranteed to be present as the first entry in the log
    //
    add_node_info_entry();
}



/*****************************************************************************/
/**
 * Set the wrap configuration parameter
 *
 * @param   enable           - Is wrapping enabled?
 *                                 EVENT_LOG_WRAP_ENABLE
 *                                 EVENT_LOG_WRAP_DISABLE
 *
 * @return  int              - Status of the command
 *                               - SUCCESS = 0
 *                               - FAILURE = -1
 *
 *****************************************************************************/
int event_log_config_wrap(u32 enable) {
    int ret_val = 0;

    switch ( enable ) {
        case EVENT_LOG_WRAP_ENABLE:
            log_wrap_enabled  = 1;
            break;

        case EVENT_LOG_WRAP_DISABLE:
            log_wrap_enabled  = 0;
            break;

        default:
            ret_val = -1;
            break;
    }

    return ret_val;
}



/*****************************************************************************/
/**
 * Set the event logging enable variable
 *
 * @param   enable           - Is logging enabled?
 *                                 EVENT_LOG_LOGGING_ENABLE
 *                                 EVENT_LOG_LOGGING_DISABLE
 *
 * @return  int              - Status of the command
 *                               - SUCCESS = 0
 *                               - FAILURE = -1
 *
 *****************************************************************************/
int event_log_config_logging( u32 enable ) {
    int ret_val = 0;

    switch ( enable ) {
        case EVENT_LOG_LOGGING_ENABLE:
            event_logging_enabled  = 1;
            break;

        case EVENT_LOG_LOGGING_DISABLE:
            event_logging_enabled  = 0;
            break;

        default:
            ret_val = -1;
            break;
    }

    return ret_val;
}



/*****************************************************************************/
/**
 * Get event log data
 *
 *   Based on the start address and the size, the function will fill in the
 * appropriate number of bytes in to the buffer.  It is up to the caller
 * to determine if the bytes are "valid".
 *
 * @param   start_address    - Address in the event log to start the transfer
 *                               (ie byte index from 0 to log_size)
 * @param   size             - Size in bytes of the buffer
 * @param   buffer           - Pointer to the buffer to be filled in with event data
 *                               (buffer must be pre-allocated and be at least size bytes)
 * @param   copy_data        - Flag to copy the data:
 *                                1 = Copy data to buffer
 *                                0 = Do not copy data to buffer
 *
 * @return  u32              - The number of bytes filled in to the buffer
 *
 * @note    Any requests for data that is out of bounds will print a warning and
 *          return 0 bytes.  If a request exceeds the size of the array, then
 *          the request will be truncated.
 *
 *****************************************************************************/
u32  event_log_get_data(u32 start_index, u32 size, void * buffer, u8 copy_data) {
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
	//TODO: Using the log without wlan_exp requires a different retrieval method

    u32 start_address;
    u64 end_address;
    u32 num_bytes     = 0;

    // If the log is empty, then return 0
    if (log_empty == 1) { return num_bytes; }

    // Check that the start_address is less than the log_size
    if (start_index > log_size) {
        xil_printf("EVENT LOG: WARNING: Index out of bounds: Request starts at 0x%08x; Log only has 0x%08x bytes\n", start_index, log_size);
        return num_bytes;
    }

    // Translate the start address from an index to the actual memory location
    start_address = log_start_address + start_index;

    // Compute the end address for validity checks
    end_address = start_address + size;

    // Check that the end address is less than the end of the buffer
    if ( end_address > log_soft_end_address ) {
        num_bytes = log_soft_end_address - start_address;
    } else {
        num_bytes = size;
    }

    if (copy_data) {
        // Copy the data in to the buffer
        memcpy((void *) buffer, (void *) start_address, num_bytes);
    } else {
        // Assume that the buffer is a WARP IP/UDP buffer and populate the fields accordingly
        ((warp_ip_udp_buffer *)buffer)->data   = (u8 *)start_address;
        ((warp_ip_udp_buffer *)buffer)->offset = (u8 *)start_address;
        ((warp_ip_udp_buffer *)buffer)->length = num_bytes;
        ((warp_ip_udp_buffer *)buffer)->size   = num_bytes;
    }

    return num_bytes;
#else
    return 0;
#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP
}



/*****************************************************************************/
/**
 * Get the size of the log in bytes from the start_index to the "end" of the log
 *
 * The "end" of the log is:
 *   - log_next_address if start_address < log_next_address
 *   - log_soft_end_address if start_address >= log_next_address
 *
 * This function will no longer return the total number of bytes in the log.
 * To get that, you would need to call:  event_log_get_total_size()
 *
 * @param   start_index      - Starting byte of the log to use for size calculation
 *
 * @return  u32              - Size of the log in bytes
 *
 *****************************************************************************/
u32  event_log_get_size(u32 start_index) {
    u32 size;
    u32 start_address = log_start_address + start_index;

    // Implemented this way b/c we are using unsigned integers, so we always need
    //   to have positive integers at each point in the calculation
    if (start_address < log_next_address) {
        size = log_next_address - start_address;
    } else {
        size = log_soft_end_address - start_address;
    }

#ifdef _DEBUG_
    xil_printf("EVENT LOG:  size                 = 0x%x\n", size );
    xil_printf("EVENT LOG:  log_next_address     = 0x%x\n", log_next_address );
    xil_printf("EVENT LOG:  log_oldest_address   = 0x%x\n", log_oldest_address );
    xil_printf("EVENT LOG:  log_soft_end_address = 0x%x\n", log_soft_end_address );
    xil_printf("EVENT LOG:  log_max_address      = 0x%x\n", log_max_address );
#endif

    return size;
}



/*****************************************************************************/
/**
 * Get the total size of the valid log entries in bytes
 *
 * @param   None
 *
 * @return  u32              - Total size of the valid log entries in bytes
 *
 *****************************************************************************/
u32  event_log_get_total_size(void) {
    u32 size;
    u32 oldest_index;
    u32 next_index;

    // Implemented this way b/c we are using unsigned integers, so we always need
    //   to have positive integers at each point in the calculation
    oldest_index = event_log_get_oldest_entry_index();
    next_index   = event_log_get_next_entry_index();

    if (oldest_index < next_index) {    // Log has not wrapped
        size = event_log_get_size(oldest_index);
    } else {                              // Log has wrapped
        size = event_log_get_size(oldest_index) + next_index;
    }

    return size;
}



/*****************************************************************************/
/**
 * Get the maximum size of the log (capacity)
 *
 * @param   None
 *
 * @return  u32              - Maximum size of the log (capacity)
 *
 *****************************************************************************/
u32  event_log_get_capacity(void) {
    return log_size;
}



/*****************************************************************************/
/**
 * Get the address of the write pointer to the next entry index
 *
 * @param   None
 *
 * @return  u32              - Index of the event log of the write pointer
 *
 *****************************************************************************/
u32  event_log_get_next_entry_index(void) {
    return (log_next_address - log_start_address);
}



/*****************************************************************************/
/**
 * Get the index of the oldest entry
 *
 * @param   None
 *
 * @return  u32              - Index of the event log of the oldest entry
 *
 *****************************************************************************/
u32  event_log_get_oldest_entry_index(void) {
    return (log_oldest_address - log_start_address);
}



/*****************************************************************************/
/**
 * Get the number of times the log has wrapped
 *
 * @param   None
 *
 * @return  u32              - Number of times the log has wrapped
 *
 *****************************************************************************/
u32  event_log_get_num_wraps(void) {
    return log_num_wraps;
}



/*****************************************************************************/
/**
 * Get the flags associated with the log
 *
 * @param   None
 *
 * @return  u32              - Flags:
 *                             [ 0] - event_logging_enabled;
 *                             [ 1] - log_wrap_enabled;
 *
 * @note    Should match CMD_PARAM_LOG_CONFIG_FLAG_ in wlan_exp_node.h
 *
 *****************************************************************************/
u32  event_log_get_flags(void) {
    return ((log_wrap_enabled & 0x1) << 1) + (event_logging_enabled & 0x1);
}



/*****************************************************************************/
/**
 * Update the entry type
 *
 * @param   entry_ptr        - Pointer to entry contents
 *          entry_type       - Value to update entry_type field
 *
 * @return  int              - Status of the command
 *                               - SUCCESS = 0
 *                               - FAILURE = -1
 *
 *****************************************************************************/
int  event_log_update_type(void * entry_ptr, u16 entry_type) {
    int            return_value = -1;
    entry_header * entry_hdr;

    // If the entry_ptr is within the event log, then update the type field of the entry
    if ((((u32) entry_ptr) > log_start_address) && (((u32) entry_ptr) < log_max_address)) {

        entry_hdr = (entry_header *) (((u32) entry_ptr) - sizeof(entry_header));

        // Check to see if the entry has a valid magic number
        if ((entry_hdr->entry_id & 0xFFFF0000) == EVENT_LOG_MAGIC_NUMBER) {

            entry_hdr->entry_type = entry_type;

            return_value = 0;
        } else {
            xil_printf("EVENT LOG: ERROR: event_log_update_type() - entry_ptr (0x%8x) is not valid \n", entry_ptr );
        }
    } else {
        xil_printf("EVENT LOG: ERROR: event_log_update_type() - entry_ptr (0x%8x) is not in event log \n", entry_ptr );
    }

    return return_value;
}



/*****************************************************************************/
/**
 * Move the oldest address past the end_address while still being aligned
 * to an entry boundary
 *
 * @param   end_address      - Address that the oldest address must move past
 *                             by an integer number of entries.
 *
 * @return  None
 *
 * @note    This function will blindly increment the oldest address past the
 *          end_address.  It does not check for the end of the log and that
 *          is the responsibility of the calling function.
 *
 *****************************************************************************/
void event_log_move_oldest_address(u32 end_address) {

    entry_header * entry;

    // Move the oldest address an integer number of entries until it points to the
    //   first entry after the allocation
    entry = (entry_header *) log_oldest_address;

    // Move the address in increments of an entry
    while (log_oldest_address <= end_address) {

        // Check that the entry is still valid.  Otherwise, print a warning and
        //   issue a log reset.
        if ((entry->entry_id & 0xFFFF0000) != EVENT_LOG_MAGIC_NUMBER) {
            xil_printf("EVENT LOG: ERROR: Oldest entry corrupted. Resetting event log\n");
            xil_printf("    Please verify that no other code / data is using the event log memory space\n");

            event_log_reset();
            return;
        }

        // Increment the address and get the next entry
        log_oldest_address += (entry->entry_length + sizeof(entry_header));
        entry               = (entry_header *) log_oldest_address;
    }
}



/*****************************************************************************/
/**
 * Increment the oldest address
 *
 * @param   end_address      - Current ending address
 * @param   size             - Number of bytes to increment the oldest address
 *
 * @return  None
 *
 * @note    This function will blindly increment the oldest address by 'size'
 *          bytes (ie it does not check the log_oldest_address relative to the
 *          log_next_address).  It is the responsibility of the calling
 *          function to make sure this is only called when appropriate.
 *
 *****************************************************************************/
void event_log_increment_oldest_address(u64 end_address, u32 size) {

    u64            final_end_address;

    // Calculate end address (need to make sure we don't overflow u32)
    final_end_address = end_address + wrap_buffer;

    // Check to see if we will wrap with the current increment
    if (final_end_address >= log_soft_end_address) {
        // We will wrap the log

        // Reset the log_soft_end_address to the end of the array
        log_soft_end_address = log_max_address;

        // Move the log_oldest_address to the beginning of the array and move it
        //   at least 'size' bytes from the front of the array.  This is done to mirror
        //   how allocation of the log_next_address works.  Also, b/c of this allocation
        //   scheme, we are guaranteed that log_start_address is the beginning of an entry.
        //
        //   NOTE:  We need to skip the node_info at the beginning of the buffer.
        //
        log_oldest_address = log_start_address + sizeof(node_info_entry) + sizeof(entry_header);
        final_end_address  = log_oldest_address + size + wrap_buffer;
    }

    // Move the oldest address
    event_log_move_oldest_address(final_end_address);
}



/*****************************************************************************/
/**
 * Get the address of the next empty entry in the log and allocate size bytes
 *   for that entry
 *
 * @param   size             - Size (in bytes) of entry to allocate
 * @param   address *        - Pointer to address of empty entry of length 'size'
 *
 * @return  int              - Status of command
 *                               - 0 = Success
 *                               - 1 = Failure
 *
 * @note    This will handle the circular nature of the buffer.  It will also
 *          set the log_full flag if there is no additional space and print
 *          a warning message.  If this function is called while the event log
 *          is full, then it will always return max_entry_index
 *
 *****************************************************************************/
int  event_log_get_next_empty_address( u32 size, u32 * address ) {

    int               status         = 1;            // Initialize status to FAILED
    u32               return_address = 0;

    u64               end_address;
    node_info_entry * entry;


    // If the log is empty, then set the flag to zero to indicate the log is not empty
    if (log_empty) { log_empty = 0; }

    // If the log is not full, then find the next address
    if (!log_full && !allocation_mutex) {

        // Set the mutex to '1' so that if an interrupt occurs, the event log allocation
        //   will not be ruined
        allocation_mutex = 1;

        // Compute the end address of the newly allocated entry
        end_address = (u64)(log_next_address) + (u64)(size);

        // Check if the log has wrapped
        if ((log_next_address > log_oldest_address) ||
            ((log_next_address == log_start_address) && (log_oldest_address == log_start_address))) {
            // The log has not wrapped

            // Check to see if we will wrap with the current allocation
            if ( end_address > log_soft_end_address ) {
                // Current allocation will wrap the log

                // Check to see if wrapping is enabled
                if ( log_wrap_enabled ) {

                    xil_printf("EVENT LOG: LOG WRAP: Has wrapped %d times\n", log_num_wraps);

                    // Compute new end address
                    end_address = log_start_address + sizeof(node_info_entry) + sizeof(entry_header) + size;

                    // Check that we are not going to pass the oldest address
                    if (end_address > log_oldest_address) {
                        event_log_increment_oldest_address( end_address, size );
                    }

                    // Increment the number of wraps
                    log_num_wraps += 1;

                    // Update the node_info_entry timestamp
                    entry = (node_info_entry *)(log_start_address + sizeof(entry_header));
                    entry->timestamp = get_mac_time_usec();

                    // Set the log_soft_end_address and allocate the new entry from the beginning of the buffer
                    log_soft_end_address = log_next_address;
                    log_next_address     = end_address;

                    // Return address is the beginning of the buffer
                    //  (skipping the node_info at the beginning of the buffer)
                    return_address = log_start_address + sizeof(node_info_entry) + sizeof(entry_header);
                    status         = 0;

                } else {
                    // Set the full flag and the soft end; then return
                    log_full             = 1;
                    log_soft_end_address = log_next_address;

                    // Set the value to the appropriate "full" state
                    log_oldest_address = log_start_address;
                    log_next_address   = log_start_address;
                    log_num_wraps     += 1;

                    // Log is now full, print warning
                    xil_printf("---------------------------------------- \n");
                    xil_printf("EVENT LOG:  WARNING - Event Log FULL !!! \n");
                    xil_printf("---------------------------------------- \n");
                }
            } else {
                // Current allocation does not wrap

                // NOTE: This should be the most common case; since we know the log has not wrapped
                //   we do not need to increment the log_oldest_address.

                // Set the return address and then move the log_next_address
                return_address   = log_next_address;
                log_next_address = end_address;
                status           = 0;
            }
        } else {
            // The log has wrapped
            //   NOTE:  Even though the log has wrapped, we cannot assume that the wrap flag
            //     continues to allow the log to wrap.

            // Check that we are not going to pass the oldest address
            //   NOTE:  This will set the log_soft_end_address if the oldest_address passes the end of the array
            if (end_address > log_oldest_address) {
                event_log_increment_oldest_address( end_address, size );
            }

            // Check to see if we will wrap with the current allocation
            if (end_address > log_soft_end_address) {
                // Current allocation will wrap the log

                // Check to see if wrapping is enabled
                if ( log_wrap_enabled ) {

                    xil_printf("EVENT LOG: LOG WRAP: Has wrapped %d times\n", log_num_wraps);

                    // Compute new end address
                    end_address = log_start_address + sizeof(node_info_entry) + sizeof(entry_header) + size;

                    // NOTE:  We have already incremented the log_oldest_address by size.  Since the
                    //   event_log_increment_oldest_address() function follows the same allocation scheme
                    //   we are guaranteed that at least 'size' bytes are available at the beginning of the
                    //   array if we wrapped.  Therefore, we do not need to check the log_oldest_address again.

                    // Increment the number of wraps
                    log_num_wraps += 1;

                    // Update the node_info_entry timestamp
                    entry = (node_info_entry *)(log_start_address + sizeof(entry_header));
                    entry->timestamp = get_mac_time_usec();

                    // Set the log_soft_end_address and allocate the new entry from the beginning of the buffer
                    log_soft_end_address = log_next_address;
                    log_next_address     = end_address;

                    // Return address is the beginning of the buffer
                    //  (skipping the node_info at the beginning of the buffer)
                    return_address = log_start_address + sizeof(node_info_entry) + sizeof(entry_header);
                    status         = 0;

                } else {
                    // Set the full flag and the soft end; then return
                    log_full             = 1;
                    log_soft_end_address = log_next_address;

                    // Set the value to the appropriate "full" state
                    log_oldest_address = log_start_address;
                    log_next_address   = log_start_address;
                    log_num_wraps     += 1;

                    // Log is now full, print warning
                    xil_printf("---------------------------------------- \n");
                    xil_printf("EVENT LOG:  WARNING - Event Log FULL !!! \n");
                    xil_printf("---------------------------------------- \n");
                }
            } else {
                // Current allocation does not wrap

                // Set the return address and then move the log_next_address
                return_address   = log_next_address;
                log_next_address = end_address;
                status           = 0;
            }
        }

        // Set the mutex to '0' to allow for future allocations
        allocation_mutex = 0;
    }

    // Set return parameter
    *address = return_address;

    return status;
}



/*****************************************************************************/
/**
 * Get the next empty entry
 *
 * @param   entry_type       - Type of entry
 * @param   entry_size       - Size of the entry payload
 *
 * @return  void *           - Pointer to the next entry payload
 *
 *****************************************************************************/
void * event_log_get_next_empty_entry(u16 entry_type, u16 entry_size) {

    u32            log_address;
    u32            total_size;
    entry_header * header       = NULL;
    u32            header_size  = sizeof( entry_header );
    void *         return_entry = NULL;

    // If Event Logging is enabled, then allocate entry
    if (event_logging_enabled) {

        // Entries must be 4 byte aligned so that there are not a lot of mis-aligned
        // accesses that could lead to significant processing overhead.
        //
        if ((entry_size % 4) != 0) {
            entry_size = ((entry_size >> 2) + 1) << 2;
        }

        total_size = entry_size + header_size;

        // Try to allocate the next entry
        if (!event_log_get_next_empty_address(total_size, &log_address)) {

            // Use successfully allocated address for the entry
            header = (entry_header*) log_address;

            // Zero out entry
            // Note: this operation takes ~14usec for the 300 bytes Rx OFDM entry -- a substantial
            //   NOTE:  Based on characterization, this bzero operation was considerable
            //     overhead in relation to the tasks that are getting empty log entries.
            //     Therefore, we are removing it.  The one thing to note is that empty
            //     log entries will have random data and cannot be assumed to have a
            //     starting value.
            // bzero((void *) header, total_size);

            // Set header parameters
            //   - Use the upper 16 bits of the timestamp to place a magic number
            header->entry_id     = EVENT_LOG_MAGIC_NUMBER + (0x0000FFFF & log_count++);
            header->entry_type   = entry_type;
            header->entry_length = entry_size;

            // Get a pointer to the entry payload
            return_entry         = (void *)(log_address + header_size);

#ifdef _DEBUG_
            xil_printf("Entry (%6d bytes) = 0x%8x    0x%8x    0x%6x\n", entry_size, return_entry, header, total_size );
#endif
        }
    }

    return return_entry;
}


#ifdef _DEBUG_

/*****************************************************************************/
/**
 * Prints everything in the event log from log_start_index to log_curr_index
 *
 * @param   num_entrys       - Number of entries to print
 *
 * @return  None
 *
 *****************************************************************************/
void print_event_log(u32 num_entrys) {
    u32            i;
    u32            entry_address;
    u32            entry_count;
    entry_header * entry_hdr;
    u32            entry_hdr_size;
    void         * event;

    // Initialize count
    entry_hdr_size = sizeof(entry_header);
    entry_count    = 0;
    entry_address  = log_oldest_address;

    // Check if the log has wrapped
    if (log_next_address >= log_oldest_address) {
        // The log has not wrapped

        // Print the log from log_oldest_address to log_next_address
        for (i = entry_count; i < num_entrys; i++) {

            // Check that the current entry address is less than log_next_address
            if (entry_address < log_next_address) {

                entry_hdr = (entry_header*) entry_address;
                event     = (void *) (entry_address + entry_hdr_size);

                // Print entry
                print_entry((0x0000FFFF & entry_hdr->entry_id), entry_hdr->entry_type, event);

                // Get the next entry
                entry_address += (entry_hdr->entry_length + entry_hdr_size);

            } else {
                // Exit the loop
                break;
            }
        }

    } else {
        // The log has wrapped

        // Print the log from log_oldest_address to the end of the buffer
        for (i = entry_count; i < num_entrys; i++){

            // Check that the current entry address is less than the end of the log
            if (entry_address < log_soft_end_address) {

                entry_hdr = (entry_header*) entry_address;
                event     = (void * ) (entry_hdr + entry_hdr_size);

                // Print entry
                print_entry((0x0000FFFF & entry_hdr->entry_id), entry_hdr->entry_type, event);

                // Get the next entry
                entry_address += (entry_hdr->entry_length + entry_hdr_size);

            } else {
                // Exit the loop and set the entry to the beginning of the buffer
                entry_address  = log_start_address;
                entry_count    = i;
                break;
            }
        }

        // If we still have entries to print, then start at the beginning
        if (entry_count < num_entrys) {

            // Print the log from the beginning to log_next_address
            for (i = entry_count; i < num_entrys; i++){

                // Check that the current entry address is less than log_next_address
                if (entry_address < log_next_address) {

                    entry_hdr = (entry_header*) entry_address;
                    event     = (void * ) (entry_hdr + entry_hdr_size);

                    // Print entry
                    print_entry((0x0000FFFF & entry_hdr->entry_id), entry_hdr->entry_type, event);

                    // Get the next entry
                    entry_address += (entry_hdr->entry_length + entry_hdr_size);

                } else {
                    // Exit the loop
                    break;
                }
            }
        }
    }
}

#endif



/*****************************************************************************/
/**
 * Prints the size of the event log
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void print_event_log_size(){
    u32 size;
    u64 timestamp;

    size      = event_log_get_total_size();
    timestamp = get_mac_time_usec();

    xil_printf("EVENT LOG: (%10d us) %10d of %10d bytes used\n", (u32)timestamp, size, log_size);
}

#endif //WLAN_SW_CONFIG_ENABLE_LOGGING



