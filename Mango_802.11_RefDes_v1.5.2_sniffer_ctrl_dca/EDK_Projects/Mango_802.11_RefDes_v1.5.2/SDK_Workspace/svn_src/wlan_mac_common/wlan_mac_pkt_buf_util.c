/** @file wlan_mac_pkt_buf_util.c
 *  @brief Packet Buffer Communication Framework
 *
 *  This contains code common to both CPU_LOW and CPU_HIGH that allows them
 *  to pass messages to one another.
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
#include "stdio.h"

#include "xstatus.h"
#include "xmutex.h"
#include "xparameters.h"

#include "wlan_mac_common.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_802_11_defs.h"

/*********************** Global Variable Definitions *************************/

/*************************** Variable Definitions ****************************/

static XMutex                pkt_buf_mutex;


/*************************** Functions Prototypes ****************************/


/******************************** Functions **********************************/

int init_pkt_buf() {
    u32            i;
    XMutex_Config *mutex_config_ptr;

    //Initialize the pkt buffer mutex core
    mutex_config_ptr = XMutex_LookupConfig(PKT_BUF_MUTEX_DEVICE_ID);
    XMutex_CfgInitialize(&pkt_buf_mutex, mutex_config_ptr, mutex_config_ptr->BaseAddress);

    // Unlock all mutexes this CPU might own at boot
    //     Most unlocks will fail harmlessly, but this helps cleanup state on soft reset
    for (i = 0; i < NUM_TX_PKT_BUFS; i++) {
        unlock_tx_pkt_buf(i);
    }
    for (i = 0; i < NUM_RX_PKT_BUFS; i++) {
        unlock_rx_pkt_buf(i);
    }

    return 0;
}



/************** Pkt Buffer Mutex Management ************/
int lock_tx_pkt_buf(u8 pkt_buf_ind) {
    int status;

    //Check inputs
    if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
        return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    status = XMutex_Trylock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE));

    if(status == XST_SUCCESS)
        return PKT_BUF_MUTEX_SUCCESS;
    else
        return PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED;
}

int lock_rx_pkt_buf(u8 pkt_buf_ind) {
    int status;

    //Check inputs
    if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
        return PKT_BUF_MUTEX_FAIL_INVALID_BUF;


    status = XMutex_Trylock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE));

    if(status == XST_SUCCESS)
        return PKT_BUF_MUTEX_SUCCESS;
    else
        return PKT_BUF_MUTEX_FAIL_ALREADY_LOCKED;
}

int force_lock_tx_pkt_buf(u8 pkt_buf_ind) {
    u32 UnLockPattern;

    //Check inputs
	if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    //Unlock the packet buffer
    if(unlock_tx_pkt_buf(pkt_buf_ind) == PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER ){
    	//Force an unlock as the other CPU_ID
		UnLockPattern = (((XPAR_CPU_ID+1)%2 << OWNER_SHIFT) | 0x0);

		XMutex_ReadReg(pkt_buf_mutex.Config.BaseAddress, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE),
					XMU_MUTEX_REG_OFFSET);

		XMutex_WriteReg(pkt_buf_mutex.Config.BaseAddress, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE),
				XMU_MUTEX_REG_OFFSET, UnLockPattern);

    }

    //At this point in the function, the packet buffer will be unlocked regardless of who originally
    //owned an existing lock.

    return lock_tx_pkt_buf(pkt_buf_ind);
}

int force_lock_rx_pkt_buf(u8 pkt_buf_ind) {
    u32 UnLockPattern;

    //Check inputs
	if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
		return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    //Unlock the packet buffer
    if(unlock_rx_pkt_buf(pkt_buf_ind) == PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER ){
    	//Force an unlock as the other CPU_ID
		UnLockPattern = (((XPAR_CPU_ID+1)%2 << OWNER_SHIFT) | 0x0);

		XMutex_ReadReg(pkt_buf_mutex.Config.BaseAddress, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE),
					XMU_MUTEX_REG_OFFSET);

		XMutex_WriteReg(pkt_buf_mutex.Config.BaseAddress, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE),
				XMU_MUTEX_REG_OFFSET, UnLockPattern);

    }

    //At this point in the function, the packet buffer will be unlocked regardless of who originally
    //owned an existing lock.

    return lock_rx_pkt_buf(pkt_buf_ind);
}

int unlock_tx_pkt_buf(u8 pkt_buf_ind) {
    int status;

    //Check inputs
    if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
        return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    if(XMutex_IsLocked(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE))){
        status = XMutex_Unlock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE));

        if(status == XST_SUCCESS)
            return PKT_BUF_MUTEX_SUCCESS;
        else
            return PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER;
    } else {
    	return PKT_BUF_MUTEX_ALREADY_UNLOCKED;
    }
}

int unlock_rx_pkt_buf(u8 pkt_buf_ind) {
    int status;

    //Check inputs
    if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
        return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    status = XMutex_Unlock(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE));

    if(status == XST_SUCCESS)
        return PKT_BUF_MUTEX_SUCCESS;
    else
        return PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER;
}

int get_tx_pkt_buf_status(u8 pkt_buf_ind, u32 * locked, u32 * owner){

    //Check inputs
    if(pkt_buf_ind >= NUM_TX_PKT_BUFS)
        return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    XMutex_GetStatus(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_TX_BASE), locked, owner);

    return PKT_BUF_MUTEX_SUCCESS;
}

int get_rx_pkt_buf_status(u8 pkt_buf_ind, u32 * locked, u32 * owner){

    //Check inputs
    if(pkt_buf_ind >= NUM_RX_PKT_BUFS)
        return PKT_BUF_MUTEX_FAIL_INVALID_BUF;

    XMutex_GetStatus(&pkt_buf_mutex, (pkt_buf_ind + PKT_BUF_MUTEX_RX_BASE), locked, owner);

    return PKT_BUF_MUTEX_SUCCESS;
}

