/** @file wlan_mac_dl_list.h
 *  @brief Doubly-linked List Framework
 *
 *  This contains the code for managing doubly-linked lists.
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


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_DL_LIST_H_
#define WLAN_MAC_DL_LIST_H_


//-----------------------------------------------
// Macros
//
#define dl_entry_next(x)                                (((dl_entry*)(x))->next)
#define dl_entry_prev(x)                                (((dl_entry*)(x))->prev)



/*********************** Global Structure Definitions ************************/

typedef struct dl_entry dl_entry;

struct dl_entry{
    dl_entry* next;
    dl_entry* prev;
    void*     data;
};

typedef struct {
    dl_entry* first;
    dl_entry* last;
    u32       length;
} dl_list;



/*************************** Function Prototypes *****************************/

void dl_list_init(dl_list* list);
void dl_entry_insertAfter(dl_list* list, dl_entry* entry, dl_entry* entry_new);
void dl_entry_insertBefore(dl_list* list, dl_entry* entry, dl_entry* entry_new);
void dl_entry_insertBeginning(dl_list* list, dl_entry* entry_new);
void dl_entry_insertEnd(dl_list* list, dl_entry* entry_new);
int  dl_entry_move(dl_list * src_list, dl_list * dest_list, u16 num_entries);
void dl_entry_remove(dl_list* list, dl_entry* entry);


#endif /* WLAN_MAC_DL_LIST_H_ */
