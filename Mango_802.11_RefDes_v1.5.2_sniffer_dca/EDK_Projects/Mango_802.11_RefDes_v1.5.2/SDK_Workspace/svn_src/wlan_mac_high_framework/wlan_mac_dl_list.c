/** @file wlan_mac_dl_list.c
 *  @brief Doubly-linked List Framework
 *
 *  This contains the code for managing doubly-linked (DL) lists.
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

// Xilinx / Standard library includes
#include "stdio.h"

// WLAN includes
#include "wlan_mac_high.h"
#include "wlan_mac_dl_list.h"


/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * NOTE:  Given that list operations can occur outside of an interrupt context,
 * specifically wlan_exp can add and remove entries from lists in code that is
 * interruptible, all list operations should be atomic (ie interrupts should be
 * disabled before manipulating the list and re-enabled after the list has been
 * modified.
 *
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   Initialize a dl_list
 *
 * @param   dl_list * list        - DL list to initialize
 *
 * @return  None
 *
 *****************************************************************************/
void dl_list_init(dl_list* list) {
    if (list != NULL) {
        list->first  = NULL;
        list->last   = NULL;
        list->length = 0;
    } else {
        xil_printf("ERROR: Attempted to initialize a NULL list\n");
    }
    return;
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry after provided dl_entry in provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * entry      - DL entry in DL list to insert entry after
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertAfter(dl_list* list, dl_entry* entry, dl_entry* new_entry){
    interrupt_state_t   curr_interrupt_state;

    if (new_entry != NULL) {
        curr_interrupt_state = wlan_mac_high_interrupt_stop();

        dl_entry_prev(new_entry) = entry;
        dl_entry_next(new_entry) = dl_entry_next(entry);

        if(dl_entry_next(entry) == NULL){
            list->last = new_entry;
        } else {
            dl_entry_prev(dl_entry_next(entry)) = new_entry;
        }
        dl_entry_next(entry) = new_entry;

        (list->length)++;

        wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry before provided dl_entry in provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * entry      - DL entry in DL list to insert entry before
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertBefore(dl_list* list, dl_entry* entry, dl_entry* new_entry) {
    interrupt_state_t   curr_interrupt_state;

    if (new_entry != NULL) {
        curr_interrupt_state = wlan_mac_high_interrupt_stop();

        dl_entry_prev(new_entry) = dl_entry_prev(entry);
        dl_entry_next(new_entry) = entry;

        if(dl_entry_prev(entry) == NULL){
            list->first = new_entry;
        } else {
            dl_entry_next(dl_entry_prev(entry)) = new_entry;
        }
        dl_entry_prev(entry) = new_entry;

        (list->length)++;

        wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry at the beginning of the provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertBeginning(dl_list* list, dl_entry* new_entry) {
    interrupt_state_t   curr_interrupt_state;

    if (new_entry != NULL) {
        if(list->first == NULL){
            curr_interrupt_state = wlan_mac_high_interrupt_stop();

            list->first              = new_entry;
            list->last               = new_entry;

            dl_entry_prev(new_entry) = NULL;
            dl_entry_next(new_entry) = NULL;

            (list->length)++;

            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
        } else {
            dl_entry_insertBefore(list, list->first, new_entry);
        }
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Insert dl_entry at the end of the provided dl_list
 *
 * @param   dl_list  * list       - DL list to insert new entry
 * @param   dl_entry * new_entry  - DL entry to insert into list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_insertEnd(dl_list* list, dl_entry* new_entry) {
    if (new_entry != NULL) {
        if (list->last == NULL) {
            dl_entry_insertBeginning(list, new_entry);
        } else {
            dl_entry_insertAfter(list, list->last, new_entry);
        }
    } else {
        xil_printf("ERROR: Attempted to insert a NULL dl_entry\n");
    }
}



/*****************************************************************************/
/**
 * @brief   Move multiple dl_entrys from the beginning of the src_list to the end
 *     of the dest_list
 *
 * @param   dl_list  * src_list   - DL list to remove entries (from beginning)
 * @param   dl_list  * dest_list  - DL list to add entries (to end)
 * @param   u16 num_entries       - Number of entries to move
 *
 * @return  int                   - Number of entries moved
 *
 *****************************************************************************/
int dl_entry_move(dl_list * src_list, dl_list * dest_list, u16 num_entries){
    u32                 i;
    u32                 num_moved;
    dl_entry          * curr_dl_entry;
    dl_entry          * next_dl_entry;
    interrupt_state_t   curr_interrupt_state;

    // If the caller is not moving any entries or if there are no entries
    // in the source list, then just return
    if ((num_entries == 0) || (src_list->length == 0)) { return 0; }

    //
    // NOTE:  At this point, the caller is trying to move at least 1
    //     entry and the source list has at least one entry to move.
    //

    // Stop the interrupts since we are modifying list state
    curr_interrupt_state = wlan_mac_high_interrupt_stop();

    if (num_entries < src_list->length) {
        // There will be entries left in the source list after moving
        //
        // Process:
        //   - Get the head of the src_list
        //   - Move thru the src_list for num_entries
        //   - Update the dest_list to add the moved entries
        //   - Update the src_list to remove the moved entries
        //   - Separate the lists by modifying the last entry of the
        //     dest_list and the new first entry of the src_list
        //
        num_moved     = num_entries;
        curr_dl_entry = src_list->first;

        // Move thru the src_list to find the last item to be moved
        //     NOTE:  This iteration starts at 1 since we have already "moved"
        //         one item to the dest_list.
        for (i = 1; i < num_moved; i++) {
            next_dl_entry = dl_entry_next(curr_dl_entry);

            if (next_dl_entry == NULL) {
                xil_printf("WARNING:  List not in sync:  Length = %d but only had %d entries.\n", src_list->length, i);
                num_moved = i;
                break;
            }

            curr_dl_entry = next_dl_entry;
        }

        //
        // NOTE: At this point, curr_dl_entry should be pointing to the last
        //     entry to be moved and num_moved should have the number of
        //     entries being moved.
        //

        // Update the dest_list to add the moved entries
        if (dest_list->length == 0) {
            // Transfer entries to beginning of the list
            dest_list->first                = src_list->first;
        } else {
            // Transfer entries to end of the list
            dl_entry_next(dest_list->last)  = src_list->first;
            dl_entry_prev(src_list->first)  = dest_list->last;
        }

        dest_list->last      = curr_dl_entry;
        dest_list->length   += num_moved;

        // Update the src_list to remove the moved entries
        src_list->first      = dl_entry_next(curr_dl_entry);
        src_list->length    -= num_moved;

        // Modify the entries to break the two lists
        dl_entry_next(dest_list->last)      = NULL;
        dl_entry_prev(src_list->first)      = NULL;

    } else {
        // The src_list will be empty after move
        num_moved     = src_list->length;
        curr_dl_entry = src_list->last;

        // Update the dest_list to add the moved entries
        if (dest_list->length == 0) {
            // Transfer entries to beginning of the list
            dest_list->first                = src_list->first;
        } else {
            // Transfer entries to end of the list
            dl_entry_next(dest_list->last)  = src_list->first;
            dl_entry_prev(src_list->first)  = dest_list->last;
        }

        dest_list->last      = curr_dl_entry;
        dest_list->length   += num_moved;

        // Update the src_list to be empty
        src_list->first      = NULL;
        src_list->last       = NULL;
        src_list->length     = 0;
    }

    // Restore interrupts
    wlan_mac_high_interrupt_restore_state(curr_interrupt_state);

    return num_moved;
}



/*****************************************************************************/
/**
 * @brief   Remove dl_entry from dl_list
 *
 * @param   dl_list  * list       - DL list to remove entry
 * @param   dl_entry * entry      - DL entry to remove from list
 *
 * @return  None
 *
 *****************************************************************************/
void dl_entry_remove(dl_list* list, dl_entry* entry) {
    interrupt_state_t   curr_interrupt_state;

    if (list->length > 0) {
        if (entry != NULL) {
            curr_interrupt_state = wlan_mac_high_interrupt_stop();

            if(dl_entry_prev(entry) == NULL){
                list->first = dl_entry_next(entry);
            } else {
                dl_entry_next(dl_entry_prev(entry)) = dl_entry_next(entry);
            }

            if(dl_entry_next(entry) == NULL){
                list->last = dl_entry_prev(entry);
            } else {
                dl_entry_prev(dl_entry_next(entry)) = dl_entry_prev(entry);
            }

            (list->length)--;

            // NULL fields in removed entry
            //     NOTE:  This will help in the case of pointers to "stale" entries.
            //
            //     NOTE:  There was discussion about whether to set the entry "data" pointer
            //         to NULL.  Currently, the code does not do this because the first
            //         priority of the reference design is to not crash in hard to debug
            //         ways.  Trying to access or fill in a NULL data pointer would cause
            //         the node to crash in non-obvious ways.  This decision will be revisited
            //         in future revisions of the reference design.
            //
            entry->next = NULL;
            entry->prev = NULL;

            wlan_mac_high_interrupt_restore_state(curr_interrupt_state);
        } else {
            xil_printf("ERROR: Attempted to remove a NULL dl_entry\n");
        }
    } else {
        xil_printf("ERROR: Attempted to remove dl_entry from dl_list that has nothing in it\n");
    }
}


