/** @file wlan_mac_entries.c
 *  @brief Event log
 *
 *  This contains the code for accessing event log.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  @note  This is the only code that the user should modify in order to add entries
 *  to the event log.  To add a new entry, please follow the template provided
 *  and create:
 *    1) A new entry type in wlan_mac_entries.h
 *    2) Wrapper function:  get_next_empty_*_entry()
 *    3) Update the print function so that it is easy to print the log to the
 *    terminal
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/***************************** Include Files *********************************/

#include "wlan_mac_high_sw_config.h"

// SDK includes
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xil_types.h"

// WLAN includes
#include "wlan_mac_common.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_sysmon_util.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_entries.h"

// WLAN Exp includes
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"

// Includes for wlan_exp_log_get_txrx_entry_sizes()
#include "xintc.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_high.h"
#include "wlan_mac_ltg.h"

#if WLAN_SW_CONFIG_ENABLE_LOGGING


/*************************** Constant Definitions ****************************/



/*********************** Global Variable Definitions *************************/

static u8    log_entry_en_mask;
static u32   system_time_id;


//-----------------------------------------------
// mac_payload_log_len
//
// Global variable that defines the number of payload bytes that are recorded
// for each transmission / reception.  This value must be between:
//     MIN_MAC_PAYLOAD_LOG_LEN
//     MAX_MAC_PAYLOAD_LOG_LEN
// and be 4-byte aligned.  Use the wlan_exp_log_set_mac_payload_len() method
// to change the value of this variable.  By default, this is set to the minimum
// payload length to save space in the log and can be altered by C code or
// through WLAN Exp.
//

u32 mac_payload_log_len = MIN_MAC_PAYLOAD_LOG_LEN;
// u32 mac_payload_log_len = MAX_MAC_PAYLOAD_LOG_LEN;


/*************************** Variable Definitions ****************************/



/*************************** Functions Prototypes ****************************/

void wlan_exp_log_get_txrx_entry_sizes( u32 type, u16 packet_payload_size, u32 * min_log_len, u32 * entry_size, u32 * payload_size );



/******************************** Functions **********************************/

/*****************************************************************************/
/**
 * Get / Set entry enable mask.
 *
 * @param   mask             - Enable Mask.  Bitwise OR of:
 *                             - ENTRY_EN_MASK_TXRX_CTRL
 *                             - ENTRY_EN_MASK_TXRX_MPDU
 *
 * @return  None
 *
 *****************************************************************************/
u8 wlan_exp_log_get_entry_en_mask(){
    return log_entry_en_mask;
}

void wlan_exp_log_set_entry_en_mask(u8 mask){
    log_entry_en_mask = mask;
}



/*****************************************************************************/
/**
 * Reset system time ID
 *
 * Resets the system time ID to the TIME_INFO_ENTRY_BASE_SYSTEM_TIME_ID constant
 *
 * @param   None
 * @return  None
 *
 *****************************************************************************/
void wlan_exp_log_reset_system_time_id(){
    system_time_id = TIME_INFO_ENTRY_BASE_SYSTEM_TIME_ID;
}



/*****************************************************************************/
/**
 * Get / Set max_mac_payload_log_len
 *
 * @param   payload_len      - Number of bytes to set aside for payload.
 *                               NOTE:  This needs to be 4-byte aligned.
 *
 * @return  None
 *
 *****************************************************************************/
u32  wlan_exp_log_get_mac_payload_len() {
    return mac_payload_log_len;
}

void wlan_exp_log_set_mac_payload_len(u32 payload_len){
    u32 value;
    u32 offset;

    // Make sure that value is 4-byte aligned.
    offset = payload_len % 4;
    if (offset != 0) {
        value = payload_len;
    } else {
        value = payload_len + (4 - offset);
    }

    // If the value is less than the minimum, then set it to the minimum
    if (value < MIN_MAC_PAYLOAD_LOG_LEN) {
        value = MIN_MAC_PAYLOAD_LOG_LEN;
    }

    // If the value is greater than the maximum, then set it to the maximum
    if (value > MAX_MAC_PAYLOAD_LOG_LEN) {
        value = MAX_MAC_PAYLOAD_LOG_LEN;
    }

    // Set the global variable
    mac_payload_log_len = value;
}



/*****************************************************************************/
/**
 * Get the next empty log entry
 *
 * @param   entry_type_id    - ID of the entry being requested
 * @param   entry_size       - Number of total bytes in the entry.
 *                               NOTE: This needs to be 4-byte aligned.
 *
 * @return  void *           - Pointer to memory that was allocated for the entry in the log
 *                               NOTE: This can be NULL if an entry was not allocated
 *
 *****************************************************************************/
void * wlan_exp_log_create_entry(u16 entry_type_id, u16 entry_size){

    void *    ret_val   = NULL;

    //
    // NOTE:  This is where filtering on entry_type_id would be in future implementations
    //

    ret_val = event_log_get_next_empty_entry( entry_type_id, entry_size );

    return ret_val;
}



/*****************************************************************************/
/**
 * Create a TX Low Log entry
 *
 * @param   tx_frame_info    - Pointer to frame info of the associated TX low entry
 * @param   tx_low_details   - Pointer to specific information of the TX low transmission
 * @param   tx_low_count     - Indicates which TX low this is for the TX MPDU (starts at zero)
 *
 * @return  tx_low_entry *   - Pointer to tx_low_entry log entry
 *                               NOTE: This can be NULL if an entry was not allocated
 *
 *****************************************************************************/
tx_low_entry * wlan_exp_log_create_tx_low_entry(tx_frame_info_t* tx_frame_info, wlan_mac_low_tx_details_t* tx_low_details, u32 tx_low_count){

    tx_low_entry*     tx_low_event_log_entry  = NULL;
    void*             mac_payload;
    u8*               mac_payload_ptr_u8;
    mac_header_80211* tx_80211_header;
    u32               packet_payload_size;
    u16               entry_type;
    u32               entry_size;
    u32               entry_payload_size;
    u32               min_entry_payload_size;
    u8				  is_ltg;

    mac_payload             = (u8*)tx_frame_info + PHY_TX_PKT_BUF_MPDU_OFFSET;
    mac_payload_ptr_u8      = (u8*)mac_payload;
    tx_80211_header         = (mac_header_80211*)((void *)mac_payload_ptr_u8);
    is_ltg					= 0;

    // ----------------------------------------------------
    // Create RTS log entry
    //
    if (((tx_low_details->tx_details_type == TX_DETAILS_RTS_ONLY) || (tx_low_details->tx_details_type == TX_DETAILS_RTS_MPDU)) &&
        (log_entry_en_mask & ENTRY_EN_MASK_TXRX_CTRL)) {

        entry_type          = ENTRY_TYPE_TX_LOW;
        packet_payload_size = sizeof(mac_header_80211_RTS) + WLAN_PHY_FCS_NBYTES;

        // Get all the necessary sizes to log the packet
        wlan_exp_log_get_txrx_entry_sizes(entry_type, packet_payload_size, &entry_size, &entry_payload_size, &min_entry_payload_size);

        // Request space for a TX_LOW log entry
        tx_low_event_log_entry = (tx_low_entry *) wlan_exp_log_create_entry(entry_type, entry_size);

        if (tx_low_event_log_entry != NULL) {
            // Store the payload size in the log entry
            tx_low_event_log_entry->mac_payload_log_len = entry_payload_size;

            // Create the payload for the log entry
            //     The actual RTS bytes that were transmitted aren't visible to this function. We only
            //     have the underlying MPDU that the RTS was trying to reserve the medium for. Instead,
            //     we can reconstruct what the RTS payload actually was in this log entry.
            //
            wlan_create_rts_frame((void*)(&((tx_low_entry*)tx_low_event_log_entry)->mac_payload),
                                  tx_80211_header->address_1,
                                  tx_80211_header->address_2,
                                  tx_low_details->duration);

            // Zero pad out the rest of the payload
            //     A RTS is smaller than a typical 24-byte 802.11 header.
            //
            // TODO: There is no good way to get a valid FCS for CTRL transmissions since the packet
            //     buffer is long gone. Instead, we'll explicitly zero out those bytes as well.
            //
            if ((packet_payload_size - WLAN_PHY_FCS_NBYTES) < entry_payload_size) {
                bzero((u8*)(((u32)((tx_low_entry*)tx_low_event_log_entry)->mac_payload) + (packet_payload_size - WLAN_PHY_FCS_NBYTES)),
                      (entry_payload_size - (packet_payload_size - WLAN_PHY_FCS_NBYTES)));
            }

            // Update the log entry fields
            tx_low_event_log_entry->timestamp_send         = tx_low_details->tx_start_timestamp_ctrl;
            tx_low_event_log_entry->unique_seq             = tx_frame_info->unique_seq; // NOTE: RTS frames don't have sequence numbers. However, for easier processing
                                                                                        //     we'll include the MPDU's unique sequence number in this RTS TX LOW entry
            // Copy:  MCS, PHY mode, Antenna mode, and Power
            memcpy((&((tx_low_entry*)tx_low_event_log_entry)->phy_params), &(tx_low_details->phy_params_ctrl), sizeof(phy_tx_params_t));

            tx_low_event_log_entry->transmission_count     = tx_low_count + 1;
            tx_low_event_log_entry->chan_num               = tx_low_details->chan_num;
            tx_low_event_log_entry->length                 = packet_payload_size;
            tx_low_event_log_entry->num_slots              = tx_low_details->num_slots;
            tx_low_event_log_entry->cw                     = tx_low_details->cw;
            tx_low_event_log_entry->flags                  = 0;
            tx_low_event_log_entry->pkt_type                = MAC_FRAME_CTRL1_SUBTYPE_RTS;
            tx_low_event_log_entry->flags                   = 0;

            if(tx_low_details->flags & TX_DETAILS_FLAGS_RECEIVED_RESPONSE){
            	tx_low_event_log_entry->flags					|= TX_LOW_FLAGS_RECEIVED_RESPONSE;
            } else {
            	tx_low_event_log_entry->flags					&= ~TX_LOW_FLAGS_RECEIVED_RESPONSE;
            }

            tx_low_event_log_entry->timestamp_send_frac    = tx_low_details->tx_start_timestamp_frac_ctrl;
            tx_low_event_log_entry->phy_samp_rate          = tx_frame_info->phy_samp_rate;
        }
    }

    // ----------------------------------------------------
    // Create MPDU log entry
    //
    if (((tx_low_details->tx_details_type == TX_DETAILS_MPDU) || (tx_low_details->tx_details_type == TX_DETAILS_RTS_MPDU)) &&
        (log_entry_en_mask & ENTRY_EN_MASK_TXRX_MPDU)) {

        packet_payload_size = tx_frame_info->length;

        // Determine the type of the packet
        is_ltg = wlan_mac_high_is_pkt_ltg(mac_payload, packet_payload_size);

        // Determine the entry type
        if (is_ltg) {
            entry_type = ENTRY_TYPE_TX_LOW_LTG;
        } else {
            entry_type = ENTRY_TYPE_TX_LOW;
        }

        // Get all the necessary sizes to log the packet
        wlan_exp_log_get_txrx_entry_sizes(entry_type, packet_payload_size, &entry_size, &entry_payload_size, &min_entry_payload_size);

        // Request space for a TX_LOW log entry
        tx_low_event_log_entry = (tx_low_entry *) wlan_exp_log_create_entry(entry_type, entry_size);

        if(tx_low_event_log_entry != NULL){
            // Store the payload size in the log entry
            tx_low_event_log_entry->mac_payload_log_len = entry_payload_size;

            // Transfer the payload to the log entry
            wlan_mac_high_cdma_start_transfer((&((tx_low_entry*)tx_low_event_log_entry)->mac_payload), tx_80211_header, entry_payload_size);

            // Zero pad log entry if payload_size was less than the allocated space in the log (ie min_log_len)
            if(entry_payload_size < min_entry_payload_size){
                bzero((u8*)(((u32)((tx_low_entry*)tx_low_event_log_entry)->mac_payload) + entry_payload_size), (min_entry_payload_size - entry_payload_size));
            }

            // Update the log entry fields
            tx_low_event_log_entry->timestamp_send         = tx_low_details->tx_start_timestamp_mpdu;
            tx_low_event_log_entry->unique_seq             = tx_frame_info->unique_seq;

            // Copy:  MCS, PHY mode, Antenna mode, and Power
            memcpy((&((tx_low_entry*)tx_low_event_log_entry)->phy_params), &(tx_low_details->phy_params_mpdu), sizeof(phy_tx_params_t));

            tx_low_event_log_entry->transmission_count     = tx_low_count + 1;
            tx_low_event_log_entry->chan_num               = tx_low_details->chan_num;
            tx_low_event_log_entry->length                 = tx_frame_info->length;
            tx_low_event_log_entry->num_slots              = tx_low_details->num_slots;
            tx_low_event_log_entry->cw                     = tx_low_details->cw;
            tx_low_event_log_entry->pkt_type               = tx_80211_header->frame_control_1;

            if(is_ltg){
            	tx_low_event_log_entry->flags |= TX_LOW_FLAGS_LTG_PYLD;
            	tx_low_event_log_entry->flags |= TX_LOW_FLAGS_LTG;
            } else {
            	tx_low_event_log_entry->flags &= ~TX_LOW_FLAGS_LTG_PYLD;
            	tx_low_event_log_entry->flags &= ~TX_LOW_FLAGS_LTG;
            }

            if(tx_low_details->flags & TX_DETAILS_FLAGS_RECEIVED_RESPONSE){
            	tx_low_event_log_entry->flags					|= TX_LOW_FLAGS_RECEIVED_RESPONSE;
            } else {
            	tx_low_event_log_entry->flags					&= ~TX_LOW_FLAGS_RECEIVED_RESPONSE;
            }

            tx_low_event_log_entry->timestamp_send_frac    = tx_low_details->tx_start_timestamp_frac_mpdu;
            tx_low_event_log_entry->phy_samp_rate          = tx_frame_info->phy_samp_rate;

            // Finish CDMA transfer of the payload
            wlan_mac_high_cdma_finish_transfer();

            // CPU Low updates the retry flag in the header for any re-transmissions
            //   Re-create the original header for the first TX_LOW by de-asserting the flag
            if(tx_low_count == 0) {
                // This is the first transmission
                ((mac_header_80211*)(tx_low_event_log_entry->mac_payload))->frame_control_2 &= ~MAC_FRAME_CTRL2_FLAG_RETRY;
            } else {
                // This is all subsequent transmissions
                ((mac_header_80211*)(tx_low_event_log_entry->mac_payload))->frame_control_2 |= MAC_FRAME_CTRL2_FLAG_RETRY;
            }

    #ifdef _DEBUG_
            xil_printf("TX LOW  : %8d    %8d    \n", transfer_len, MIN_MAC_PAYLOAD_LOG_LEN);
            print_buf((u8 *)((u32)tx_low_event_log_entry - 8), sizeof(tx_low_entry) + 12);
    #endif
        }
    }

    return tx_low_event_log_entry;
}



/*****************************************************************************/
/**
 * Create a TX High Log entry
 *
 * @param   tx_frame_info    - Pointer to frame info of the associated TX entry
 * @param   channel_number   - Indicates the channel on which the transmission occurred
 *
 * @return  tx_high_entry *  - Pointer to the tx_high_entry log entry
 *                               NOTE: This can be NULL if an entry was not allocated
 *
 *****************************************************************************/
tx_high_entry * wlan_exp_log_create_tx_high_entry(tx_frame_info_t* tx_frame_info){

    tx_high_entry*    tx_high_event_log_entry = NULL;
    void*             mac_payload             = (u8*)tx_frame_info + PHY_TX_PKT_BUF_MPDU_OFFSET;
    u8*               mac_payload_ptr_u8      = (u8*)mac_payload;
    mac_header_80211* tx_80211_header         = (mac_header_80211*)((void *)mac_payload_ptr_u8);
    u32               packet_payload_size     = tx_frame_info->length;
    u16               entry_type;
    u32               entry_size;
    u32               entry_payload_size;
    u32               min_entry_payload_size;
    u32               transfer_len;
    u8				  is_ltg;

    // MPDU logging is disabled
    if((log_entry_en_mask & ENTRY_EN_MASK_TXRX_MPDU) == 0){
        return NULL;
    }

    is_ltg = wlan_mac_high_is_pkt_ltg(mac_payload, packet_payload_size);

    // Determine the entry type
    if (is_ltg) {
        entry_type = ENTRY_TYPE_TX_HIGH_LTG;
    } else {
        entry_type = ENTRY_TYPE_TX_HIGH;
    }

    // Get all the necessary sizes to log the packet
    wlan_exp_log_get_txrx_entry_sizes( entry_type, packet_payload_size, &entry_size, &entry_payload_size, &min_entry_payload_size );

    // Request space for a TX entry
    tx_high_event_log_entry = (tx_high_entry *)wlan_exp_log_create_entry( entry_type, entry_size );

    if(tx_high_event_log_entry != NULL){

        // Fill in the TX log entry
        //   This is done one field at a time, as the TX log entry format is not a byte-for-byte copy of the tx_frame_info
        tx_high_event_log_entry->mac_payload_log_len = entry_payload_size;

        // Compute the length of the DMA transfer to log the packet:
        //
        // We have two arrays in memory that we have to be aware of:
        //     1) The packet payload that has "packet_payload_size" bytes;
        //     2) The TX log entry that has "entry_payload_size" bytes;
        //
        // Because the packet payload does not have to be 32-bit aligned and we could be logging an arbitrary number of
        // bytes of the packet, we have to be careful about not walking off the end of either array. Therefore, we need
        // to transfer the shorter of the two arrays and then zero pad the log entry if we transfered less than
        // "entry_payload_size" bytes.
        //
        transfer_len = min(entry_payload_size, packet_payload_size);

        wlan_mac_high_cdma_start_transfer((&((tx_high_entry*)tx_high_event_log_entry)->mac_payload), tx_80211_header, transfer_len);

        // Zero pad log entry if transfer_len was less than the allocated space in the log (ie entry_payload_size)
        if(transfer_len < entry_payload_size){
            bzero((u8*)(((u32)((tx_high_entry*)tx_high_event_log_entry)->mac_payload) + transfer_len), (entry_payload_size - transfer_len) );
        }

        // Populate the log entry
        tx_high_event_log_entry->timestamp_create         = tx_frame_info->timestamp_create;
        tx_high_event_log_entry->delay_accept             = tx_frame_info->delay_accept;
        tx_high_event_log_entry->delay_done               = tx_frame_info->delay_done;
        tx_high_event_log_entry->unique_seq               = tx_frame_info->unique_seq;
        tx_high_event_log_entry->phy_params.antenna_mode  = tx_frame_info->params.phy.antenna_mode;
        tx_high_event_log_entry->phy_params.mcs           = tx_frame_info->params.phy.mcs;
        tx_high_event_log_entry->phy_params.phy_mode      = tx_frame_info->params.phy.phy_mode;
        tx_high_event_log_entry->phy_params.power         = tx_frame_info->params.phy.power;
        tx_high_event_log_entry->num_tx                   = tx_frame_info->num_tx_attempts;              // TODO: Add long/short distinction to event log
        tx_high_event_log_entry->length                   = tx_frame_info->length;
        tx_high_event_log_entry->flags					  = 0;

        if(is_ltg){
        	tx_high_event_log_entry->flags				  |= TX_LOW_FLAGS_LTG_PYLD;
        	tx_high_event_log_entry->flags				  |= TX_LOW_FLAGS_LTG;
        } else {
        	tx_high_event_log_entry->flags				  &= ~TX_LOW_FLAGS_LTG_PYLD;
        	tx_high_event_log_entry->flags				  &= ~TX_LOW_FLAGS_LTG;
        }
        if(tx_frame_info->tx_result == TX_FRAME_INFO_RESULT_SUCCESS){
        	tx_high_event_log_entry->flags				  |= TX_HIGH_FLAGS_SUCCESSFUL;
        } else {
        	tx_high_event_log_entry->flags				  &= ~TX_HIGH_FLAGS_SUCCESSFUL;
        }
        tx_high_event_log_entry->pkt_type                 = tx_80211_header->frame_control_1;
        tx_high_event_log_entry->queue_id                 = tx_frame_info->queue_info.QID;
        tx_high_event_log_entry->queue_occupancy          = tx_frame_info->queue_info.occupancy;

#ifdef _DEBUG_
        xil_printf("TX HIGH : %8d    %8d    %8d    %8d    %8d\n", transfer_len, MIN_MAC_PAYLOAD_LOG_LEN, total_payload_len, extra_payload, payload_log_len);
        print_buf((u8 *)((u32)tx_high_event_log_entry - 8), sizeof(tx_high_entry) + extra_payload + 12);
#endif
    }

    return tx_high_event_log_entry;
}



/*****************************************************************************/
/**
 * Create a RX Log entry
 *
 * @param   rx_mpdu          - Pointer to RX MPDU of the associated RX entry
 * @param   rate             - Indicates the rate at which the reception occurred
 *
 * @return  rx_common_entry *     - Pointer to the rx_common_entry log entry
 *                                    NOTE: This can be NULL if an entry was not allocated
 *
 *****************************************************************************/
rx_common_entry * wlan_exp_log_create_rx_entry(rx_frame_info_t* rx_frame_info){

    rx_common_entry*  rx_event_log_entry      = NULL;
    tx_low_entry*     tx_low_event_log_entry  = NULL; //This is for any inferred CTRL transmissions
    void*             mac_payload             = (u8*)rx_frame_info + PHY_RX_PKT_BUF_MPDU_OFFSET;
    u8*               mac_payload_ptr_u8      = (u8*)mac_payload;
    ltg_packet_id_t*  pkt_id;
    mac_header_80211* rx_80211_header         = (mac_header_80211*)((void *)mac_payload_ptr_u8);
    u32               packet_payload_size     = rx_frame_info->phy_details.length;
    u8                phy_mode                = rx_frame_info->phy_details.phy_mode;
    u32               entry_type;
    u8                rx_is_ltg               = 0;
    u32               entry_size;
    u32               entry_payload_size;
    u32               min_entry_payload_size;
    u32               transfer_len;

    pkt_id = (ltg_packet_id_t*)(mac_payload_ptr_u8 + sizeof(mac_header_80211));

    typedef enum {PAYLOAD_FIRST, CHAN_EST_FIRST} copy_order_t;
    copy_order_t      copy_order;

    if ((((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA) && (log_entry_en_mask & ENTRY_EN_MASK_TXRX_MPDU)) ||
        (((rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_CTRL) && (log_entry_en_mask & ENTRY_EN_MASK_TXRX_CTRL)) ||
        ( (rx_80211_header->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_MGMT)) {

        // Determine the type of the packet
    	rx_is_ltg = wlan_mac_high_is_pkt_ltg(mac_payload, packet_payload_size);

        // Determine the entry type based on the received waveform format
        if((phy_mode & PHY_MODE_HTMF) || (phy_mode & PHY_MODE_NONHT)) {
        	if(rx_is_ltg){
        		entry_type = ENTRY_TYPE_RX_OFDM_LTG;
        	} else {
        		entry_type = ENTRY_TYPE_RX_OFDM;
        	}
        } else {
        	// DSSS reception
            entry_type = ENTRY_TYPE_RX_DSSS;
        }

        // Get all the necessary sizes to log the packet
        wlan_exp_log_get_txrx_entry_sizes(entry_type, packet_payload_size, &entry_size, &entry_payload_size, &min_entry_payload_size);

        // Create the log entry
        rx_event_log_entry = (rx_common_entry*) wlan_exp_log_create_entry(entry_type, entry_size);

        // Populate the log entry
        if (rx_event_log_entry != NULL) {

            // For maximum pipelining, we'll break up the two major log copy operations (packet payload + [optional] channel estimates)
            // We will start the CDMA operation for whichever of those copies is shorter, then fill in the rest of the log entry
            // while that copy is under way, and then start the CDMA operation for the larger (which will first block on the shorter if
            // it is still going).

            if (phy_mode == PHY_MODE_DSSS) {
                // This is a DSSS packet that has no channel estimates
                copy_order = PAYLOAD_FIRST;
            } else {
                // This is an OFDM packet that contains channel estimates
    #ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
                if (sizeof(rx_frame_info->channel_est) < packet_payload_size) {
                    copy_order = CHAN_EST_FIRST;
                } else {
                    copy_order = PAYLOAD_FIRST;
                }
    #else
                copy_order = PAYLOAD_FIRST;
    #endif
            }

            // Compute the length of the DMA transfer to log the packet:
            //
            // We have two arrays in memory that we have to be aware of:
            //     1) The packet payload that has "packet_payload_size" bytes;
            //     2) The RX log entry that has "entry_payload_size" bytes;
            //
            // Because the packet payload does not have to be 32-bit aligned and we could be logging an arbitrary number of
            // bytes of the packet, we have to be careful about not walking off the end of either array. Therefore, we need
            // to transfer the shorter of the two arrays and then zero pad the log entry if we transfered less than
            // "entry_payload_size" bytes.
            //
            transfer_len = min(entry_payload_size, packet_payload_size);

            // Start copy based on the copy order
            switch (copy_order) {
                case PAYLOAD_FIRST:
                    if (phy_mode != PHY_MODE_DSSS) {
                        ((rx_ofdm_entry*)rx_event_log_entry)->mac_payload_log_len = entry_payload_size;
                        wlan_mac_high_cdma_start_transfer((((rx_ofdm_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, transfer_len);

                        // Zero pad log entry if transfer_len was less than the allocated space in the log (ie entry_payload_size)
                        if (transfer_len < entry_payload_size) {
                            bzero((u8*)(((u32)((rx_ofdm_entry*)rx_event_log_entry)->mac_payload) + transfer_len), (entry_payload_size - transfer_len));
                        }
                    } else {
                        ((rx_dsss_entry*)rx_event_log_entry)->mac_payload_log_len = entry_payload_size;
                        wlan_mac_high_cdma_start_transfer((((rx_dsss_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, transfer_len);

                        // Zero pad log entry if transfer_len was less than the allocated space in the log (ie entry_payload_size)
                        if (transfer_len < entry_payload_size) {
                            bzero((u8*)(((u32)((rx_dsss_entry*)rx_event_log_entry)->mac_payload) + transfer_len), (entry_payload_size - transfer_len));
                        }
                    }
                break;

                case CHAN_EST_FIRST:
    #ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
                    if (phy_mode != PHY_MODE_DSSS) wlan_mac_high_cdma_start_transfer(((rx_ofdm_entry*)rx_event_log_entry)->channel_est, rx_frame_info->channel_est, sizeof(rx_frame_info->channel_est));
    #endif
                break;
            }

            // Fill in Log Entry
            rx_event_log_entry->timestamp      = rx_frame_info->timestamp;
            rx_event_log_entry->timestamp_frac = rx_frame_info->timestamp_frac;
            rx_event_log_entry->phy_samp_rate  = rx_frame_info->phy_samp_rate;
            rx_event_log_entry->length         = rx_frame_info->phy_details.length;
            rx_event_log_entry->cfo_est        = rx_frame_info->cfo_est;
            rx_event_log_entry->mcs            = rx_frame_info->phy_details.mcs;
            rx_event_log_entry->phy_mode       = rx_frame_info->phy_details.phy_mode;
            rx_event_log_entry->ant_mode       = rx_frame_info->ant_mode;
            rx_event_log_entry->power          = rx_frame_info->rx_power;
            rx_event_log_entry->flags		   = 0;
            if(rx_is_ltg){
            	rx_event_log_entry->flags	   |= RX_FLAGS_LTG_PYLD;
            	rx_event_log_entry->flags	   |= RX_FLAGS_LTG;
            } else {
            	rx_event_log_entry->flags	   &= ~RX_FLAGS_LTG_PYLD;
            	rx_event_log_entry->flags	   &= ~RX_FLAGS_LTG;
            }


            if( (rx_frame_info->flags & RX_FRAME_INFO_FLAGS_FCS_GOOD) ){
            	rx_event_log_entry->flags	   |= RX_FLAGS_FCS_GOOD;
            } else {
            	rx_event_log_entry->flags	   &= ~RX_FLAGS_FCS_GOOD;
            }

            if( (rx_frame_info->flags & RX_FRAME_INFO_UNEXPECTED_RESPONSE) ){
            	rx_event_log_entry->flags	   |= RX_FLAGS_UNEXPECTED_RESPONSE;
            } else {
            	rx_event_log_entry->flags	   &= ~RX_FLAGS_UNEXPECTED_RESPONSE;
            }

            rx_event_log_entry->pkt_type       = rx_80211_header->frame_control_1;
            rx_event_log_entry->chan_num       = rx_frame_info->channel;
            rx_event_log_entry->rf_gain        = rx_frame_info->rf_gain;
            rx_event_log_entry->bb_gain        = rx_frame_info->bb_gain;

            if(rx_frame_info->channel > 11) {
            	rx_event_log_entry->seq_num	 = (((mac_payload_ptr_u8[62]<<8)&0xFF00)|(mac_payload_ptr_u8[63]&0x00FF));
            } else {
                rx_event_log_entry->seq_num	 = (((mac_payload_ptr_u8[64]<<8)&0xFF00)|(mac_payload_ptr_u8[65]&0x00FF));
            }

            // Start second copy based on the copy order
            switch(copy_order){
                case PAYLOAD_FIRST:
    #ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
                    if(phy_mode != PHY_MODE_DSSS) wlan_mac_high_cdma_start_transfer(((rx_ofdm_entry*)rx_event_log_entry)->channel_est, rx_frame_info->channel_est, sizeof(rx_frame_info->channel_est));
    #endif
                break;

                case CHAN_EST_FIRST:
                    if (phy_mode != PHY_MODE_DSSS) {
                        ((rx_ofdm_entry*)rx_event_log_entry)->mac_payload_log_len = entry_payload_size;
                        wlan_mac_high_cdma_start_transfer((((rx_ofdm_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, transfer_len);

                        // Zero pad log entry if transfer_len was less than the allocated space in the log (ie entry_payload_size)
                        if (transfer_len < entry_payload_size) {
                            bzero((u8*)(((u32)((rx_ofdm_entry*)rx_event_log_entry)->mac_payload) + transfer_len), (entry_payload_size - transfer_len));
                        }
                    } else {
                        ((rx_dsss_entry*)rx_event_log_entry)->mac_payload_log_len = entry_payload_size;
                        wlan_mac_high_cdma_start_transfer((((rx_dsss_entry*)rx_event_log_entry)->mac_payload), rx_80211_header, transfer_len);

                        // Zero pad log entry if transfer_len was less than the allocated space in the log (ie entry_payload_size)
                        if (transfer_len < entry_payload_size) {
                            bzero((u8*)(((u32)((rx_dsss_entry*)rx_event_log_entry)->mac_payload) + transfer_len), (entry_payload_size - transfer_len));
                        }
                    }
                break;
            }

    #ifdef _DEBUG_
            xil_printf("RX      : %8d    %8d    %8d    %8d    %8d\n", transfer_len, MIN_MAC_PAYLOAD_LOG_LEN, length, extra_payload, payload_log_len);
            print_buf((u8 *)((u32)rx_event_log_entry - 8), sizeof(rx_ofdm_entry) + extra_payload + 12);
    #endif
        }
    }

    if ((rx_frame_info->flags & RX_FRAME_INFO_FLAGS_CTRL_RESP_TX) && (log_entry_en_mask & ENTRY_EN_MASK_TXRX_CTRL)) {

        // ------------------------------------------------
        // Create CTS log entry
        //
        if (rx_80211_header->frame_control_1 == MAC_FRAME_CTRL1_SUBTYPE_RTS) {

            entry_type          = ENTRY_TYPE_TX_LOW;
            packet_payload_size = sizeof(mac_header_80211_CTS) + WLAN_PHY_FCS_NBYTES;

            // Get all the necessary sizes to log the packet
            wlan_exp_log_get_txrx_entry_sizes(entry_type, packet_payload_size, &entry_size, &entry_payload_size, &min_entry_payload_size);

            // Request space for a TX_LOW log entry
            tx_low_event_log_entry = (tx_low_entry *) wlan_exp_log_create_entry(entry_type, entry_size);

            if (tx_low_event_log_entry != NULL) {
                // Store the payload size in the log entry
                tx_low_event_log_entry->mac_payload_log_len = entry_payload_size;

                // Create the payload for the log entry
                //     The actual CTS bytes that were transmitted aren't visible to this function. We only
                //     have the underlying MPDU that the CTS was trying to reserve the medium for. Instead,
                //     we can reconstruct what the CTS payload actually was in this log entry.
                //
                wlan_create_cts_frame((void*)(&((tx_low_entry*)tx_low_event_log_entry)->mac_payload),
                                      rx_80211_header->address_2,
                                      rx_frame_info->resp_low_tx_details.duration);

                // Zero pad out the rest of the payload
                //     A CTS is smaller than a typical 24-byte 802.11 header.
                //
                // TODO: There is no good way to get a valid FCS for CTRL transmissions since the packet
                //     buffer is long gone. Instead, we'll explicitly zero out those bytes as well.
                //
                if ((packet_payload_size - WLAN_PHY_FCS_NBYTES) < entry_payload_size) {
                    bzero((u8*)(((u32)((tx_low_entry*)tx_low_event_log_entry)->mac_payload) + (packet_payload_size - WLAN_PHY_FCS_NBYTES)),
                          (entry_payload_size - (packet_payload_size - WLAN_PHY_FCS_NBYTES)));
                }

                // Update the log entry fields
                tx_low_event_log_entry->timestamp_send          = rx_frame_info->resp_low_tx_details.tx_start_timestamp_ctrl;
                tx_low_event_log_entry->unique_seq              = UNIQUE_SEQ_INVALID;

                // Copy:  MCS, PHY mode, Antenna mode, and Power
                memcpy((&((tx_low_entry*)tx_low_event_log_entry)->phy_params), &(rx_frame_info->resp_low_tx_details.phy_params_ctrl), sizeof(phy_tx_params_t));

                tx_low_event_log_entry->transmission_count      = 1;
                tx_low_event_log_entry->chan_num                = rx_frame_info->resp_low_tx_details.chan_num;
                tx_low_event_log_entry->length                  = packet_payload_size;
                tx_low_event_log_entry->num_slots               = rx_frame_info->resp_low_tx_details.num_slots;
                tx_low_event_log_entry->cw                      = rx_frame_info->resp_low_tx_details.cw;
                tx_low_event_log_entry->pkt_type                = MAC_FRAME_CTRL1_SUBTYPE_CTS;
                // Note: an Rx MPDU in response to this CTS will not be flagged as TX_LOW_FLAGS_RECEIVED_RESPONSE since there is no timeout after
                //  a CTS transmission.
                tx_low_event_log_entry->flags                   = 0;
                tx_low_event_log_entry->timestamp_send_frac     = rx_frame_info->resp_low_tx_details.tx_start_timestamp_frac_ctrl;
                tx_low_event_log_entry->phy_samp_rate           = rx_frame_info->phy_samp_rate; // TODO: Makes assumption that response uses same PHY BW as Rx
            }

        // ------------------------------------------------
        // Create ACK log entry
        //
        } else {
        	// Note: this clause will be called for any reception that led to a control frame response. Since we have dealt with the CTS case
        	// above, the only ever control response must be an ACK transmission.
            entry_type          = ENTRY_TYPE_TX_LOW;
            packet_payload_size = sizeof(mac_header_80211_ACK) + WLAN_PHY_FCS_NBYTES;

            // Get all the necessary sizes to log the packet
            wlan_exp_log_get_txrx_entry_sizes(entry_type, packet_payload_size, &entry_size, &entry_payload_size, &min_entry_payload_size);

            // Request space for a TX_LOW log entry
            tx_low_event_log_entry = (tx_low_entry *) wlan_exp_log_create_entry(entry_type, entry_size);

            if (tx_low_event_log_entry != NULL) {
                // Store the payload size in the log entry
                tx_low_event_log_entry->mac_payload_log_len = entry_payload_size;

                // Create the payload for the log entry
                //     The actual ACK bytes that were transmitted aren't visible to this function. We only
                //     have the underlying MPDU that the ACK was a response for. Instead, we can reconstruct
                //     what the ACK payload actually was in this log entry.
                //
                wlan_create_ack_frame((void*)(&((tx_low_entry*)tx_low_event_log_entry)->mac_payload),
                                      rx_80211_header->address_2);

                // Zero pad out the rest of the payload
                //     An ACK is smaller than a typical 24-byte 802.11 header.
                //
                // TODO: There is no good way to get a valid FCS for CTRL transmissions since the packet
                //     buffer is long gone. Instead, we'll explicitly zero out those bytes as well.
                //
                if ((packet_payload_size - WLAN_PHY_FCS_NBYTES) < entry_payload_size) {
                    bzero((u8*)(((u32)((tx_low_entry*)tx_low_event_log_entry)->mac_payload) + (packet_payload_size - WLAN_PHY_FCS_NBYTES)),
                          (entry_payload_size - (packet_payload_size - WLAN_PHY_FCS_NBYTES)));
                }

                // Update the log entry fields
                tx_low_event_log_entry->timestamp_send          = rx_frame_info->resp_low_tx_details.tx_start_timestamp_ctrl;

                // If the received MPDU is an LTG, we can mirror its unique seq into the Tx unique seq for the ACK
                if (rx_is_ltg) {
                    tx_low_event_log_entry->unique_seq          = pkt_id->unique_seq;
                } else {
                    tx_low_event_log_entry->unique_seq          = UNIQUE_SEQ_INVALID;
                }

                // Copy:  MCS, PHY mode, Antenna mode, and Power
                memcpy((&((tx_low_entry*)tx_low_event_log_entry)->phy_params), &(rx_frame_info->resp_low_tx_details.phy_params_ctrl), sizeof(phy_tx_params_t));

                tx_low_event_log_entry->transmission_count      = 1;
                tx_low_event_log_entry->chan_num                = rx_frame_info->resp_low_tx_details.chan_num;
                tx_low_event_log_entry->length                  = packet_payload_size;
                tx_low_event_log_entry->num_slots               = rx_frame_info->resp_low_tx_details.num_slots;
                tx_low_event_log_entry->cw                      = rx_frame_info->resp_low_tx_details.cw;
                tx_low_event_log_entry->pkt_type                = MAC_FRAME_CTRL1_SUBTYPE_ACK;
                tx_low_event_log_entry->flags                   = 0;
                tx_low_event_log_entry->timestamp_send_frac     = rx_frame_info->resp_low_tx_details.tx_start_timestamp_frac_ctrl;
                tx_low_event_log_entry->phy_samp_rate           = rx_frame_info->phy_samp_rate; // TODO: Makes assumption that response uses same PHY BW as Rx
                tx_low_event_log_entry->flags                   = 0;
            }
        }
    }

    return rx_event_log_entry;
}



/*****************************************************************************/
/**
 * Determine RX/TX entry size
 *
 * @param   rx_mpdu               - Pointer to RX MPDU of the associated RX entry
 * @param   channel_number        - Indicates the channel on which the reception occurred
 * @param   rate                  - Indicates the rate at which the reception occurred
 *
 * @return  rx_common_entry *     - Pointer to the rx_common_entry log entry
 *                                    NOTE: This can be NULL if an entry was not allocated
 *
 *****************************************************************************/
void wlan_exp_log_get_txrx_entry_sizes( u32 entry_type, u16 packet_payload_size,
                                        u32 * entry_size, u32 * entry_payload_size, u32 * min_entry_payload_size ) {
    u32  base_entry_size;
    u32  pkt_bytes_to_log;
    u32  log_bytes_to_log;
    u32  extra_entry_payload;

    u32  tmp_entry_size = 0;
    u32  tmp_entry_payload_size = 0;
    u32  tmp_min_entry_payload_size;

    // Set the base entry size based on the entry type
    switch (entry_type) {
        case ENTRY_TYPE_RX_OFDM:
        case ENTRY_TYPE_RX_OFDM_LTG:   base_entry_size = sizeof(rx_ofdm_entry);  break;

        case ENTRY_TYPE_RX_DSSS:       base_entry_size = sizeof(rx_dsss_entry);  break;

        case ENTRY_TYPE_TX_HIGH:
        case ENTRY_TYPE_TX_HIGH_LTG:   base_entry_size = sizeof(tx_high_entry);  break;

        case ENTRY_TYPE_TX_LOW:
        case ENTRY_TYPE_TX_LOW_LTG:    base_entry_size = sizeof(tx_low_entry);   break;

        default:                       base_entry_size = 0;                      break;
    }

    // Determine the minimum entry payload size based on entry type
    switch (entry_type) {
        // Non-LTG TX/RX Entry types
        case ENTRY_TYPE_RX_DSSS:
        case ENTRY_TYPE_RX_OFDM:
        case ENTRY_TYPE_TX_HIGH:
        case ENTRY_TYPE_TX_LOW:
            tmp_min_entry_payload_size = MIN_MAC_PAYLOAD_LOG_LEN;
        break;

        // LTG TX/RX Entry types
        case ENTRY_TYPE_RX_OFDM_LTG:
        case ENTRY_TYPE_TX_HIGH_LTG:
        case ENTRY_TYPE_TX_LOW_LTG:
            tmp_min_entry_payload_size = MIN_MAC_PAYLOAD_LTG_LOG_LEN;
        break;

        default:
            tmp_min_entry_payload_size = MIN_MAC_PAYLOAD_LOG_LEN;
        break;
    }

    // Determine the entry size and payload size based on the entry type
    switch (entry_type) {

        // Determine length required for RX / TX high log entry:
        //
        //   - mac_payload_log_len is a global variable that determines the maximum number of payload bytes to log.  This
        //         can be changed during runtime by WLAN Exp or other C code.
        //   - MIN_MAC_PAYLOAD_LOG_LEN and MIN_MAC_PAYLOAD_LTG_LOG_LEN define the minimum payload that should be logged
        //         for regular and LTG packets, respectively.  This value is guaranteed to be 32-bit aligned (ie a multiple
        //         of 4 bytes).
        //
        // Procedure:
        //   1) Determine the number of bytes we would log from the packet, enforcing the minimum number of bytes from 1).
        //   2) Determine the number of bytes the infrastructure is asking us to log, enforcing the minimum number of bytes from 1).
        //   3) Determine the number of bytes we will actually log by taking the minimum bytes from 2) and 3) so we don't
        //          add extra bytes to the log for no reason.
        //   4) Determine the number of bytes needed to be allocated for the log entry beyond the MIN_MAC_PAYLOAD_LOG_LEN which
        //          is part of the log entry definition.
        //
        case ENTRY_TYPE_RX_DSSS:
        case ENTRY_TYPE_RX_OFDM:
        case ENTRY_TYPE_RX_OFDM_LTG:
        case ENTRY_TYPE_TX_HIGH:
        case ENTRY_TYPE_TX_HIGH_LTG:
            // Determine if we need to log the minimum entry payload size or the 32-bit aligned packet payload, whichever is larger
            pkt_bytes_to_log       = max(tmp_min_entry_payload_size, ((1 + ((packet_payload_size - 1) / 4))*4));

            // Determine if we need to log the mimimum entry payload size or the mac_payload_log_len, whichever is larger
            log_bytes_to_log       = max(tmp_min_entry_payload_size, mac_payload_log_len);

            // Log the minimum of either the pkt_bytes_to_log or the log_bytes_to_log
            tmp_entry_payload_size = min( pkt_bytes_to_log, log_bytes_to_log  );

            // Determine the extra payload bytes needed for the entry beyond the MIN_MAC_PAYLOAD_LOG_LEN already allocated
            extra_entry_payload    = (tmp_entry_payload_size > MIN_MAC_PAYLOAD_LOG_LEN) ? (tmp_entry_payload_size - MIN_MAC_PAYLOAD_LOG_LEN) : 0;

            // Then entry size is the base_entry_size plus the extra payload
            tmp_entry_size         = base_entry_size + extra_entry_payload;
        break;


        // Determine length required for TX low log entry:
        //     - Log the MAC header.
        //
        case ENTRY_TYPE_TX_LOW:
            tmp_entry_size             = base_entry_size;
            tmp_entry_payload_size     = sizeof(mac_header_80211);
        break;


        // Determine length required for TX low LTG log entry:
        //     - Log the MAC header, LLC header, and LTG payload ID
        //
        case ENTRY_TYPE_TX_LOW_LTG:
            tmp_entry_size             = base_entry_size + sizeof(ltg_packet_id_t);
            tmp_entry_payload_size     = sizeof(mac_header_80211) + sizeof(ltg_packet_id_t);
        break;


        default:
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
            wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_event_log, "Unknown entry type:  %d", entry_type);
            tmp_entry_size             = 0;
            tmp_entry_payload_size     = 0;
#endif
        break;
    }

    // Assign output arguments
    *entry_size             = tmp_entry_size;
    *entry_payload_size     = tmp_entry_payload_size;
    *min_entry_payload_size = tmp_min_entry_payload_size;
}



#ifdef _DEBUG_


/*****************************************************************************/
/**
 * Prints an entry
 *
 * @param   entry_number     - Index of entry in the log
 * @param   entry_type       - Type of entry
 * @param   timestamp        - Lower 32 bits of the timestamp
 * @param   entry            - Pointer to the entry
 *
 * @return  None
 *
 *****************************************************************************/
void print_entry(u32 entry_number, u32 entry_type, void * entry){
    u32 i, j;

    node_info_entry    * node_info_entry_log_item;
    exp_info_entry     * exp_info_entry_log_item;
    wn_cmd_entry       * wn_cmd_entry_log_item;
    time_info_entry    * time_info_entry_log_item;
    txrx_counts_entry  * txrx_counts_entry_log_item;
    rx_common_entry    * rx_common_log_item;
    tx_high_entry      * tx_high_entry_log_item;
    tx_low_entry       * tx_low_entry_log_item;

    switch( entry_type ){
        case ENTRY_TYPE_NODE_INFO:
            node_info_entry_log_item = (node_info_entry*) entry;
            xil_printf("%d: - Log Info entry\n", entry_number );
            xil_printf("   Timestamp   :   0x%08x  0x%08x\n", (u32)(node_info_entry_log_item->timestamp >>32), (u32)(node_info_entry_log_item->timestamp) );
            xil_printf("   Type        :   %d\n",       node_info_entry_log_item->type);
            xil_printf("   ID          :   0x%4x\n",    node_info_entry_log_item->id);
            xil_printf("   HW Gen      :   %d\n",       node_info_entry_log_item->hw_gen);
            xil_printf("   WN Ver      :   0x%08x\n",   node_info_entry_log_item->wn_ver);
            xil_printf("   FPGA DNA    :   0x%08x  0x%08x\n", (u32)(node_info_entry_log_item->fpga_dna >>32), (u32)(node_info_entry_log_item->fpga_dna) );
            xil_printf("   Serial Num  :   %d\n",       node_info_entry_log_item->serial_number);
            xil_printf("   WLAN Exp Ver:   0x%08x\n",   node_info_entry_log_item->wlan_exp_ver);
            xil_printf("   MAC Addr    :   0x%08x  0x%08x\n", (u32)(node_info_entry_log_item->wlan_mac_addr[1]), (u32)(node_info_entry_log_item->wlan_mac_addr[0]) );
            xil_printf("   Sched Res   :   %d\n",       node_info_entry_log_item->wlan_scheduler_resolution);
        break;

        case ENTRY_TYPE_EXP_INFO:
            exp_info_entry_log_item = (exp_info_entry*) entry;
            xil_printf("%d: - Experiment Info entry\n", entry_number );
            xil_printf("   Timestamp:  %d\n", (u32)(exp_info_entry_log_item->timestamp));
            xil_printf("   Info Type:  %d\n",       exp_info_entry_log_item->info_type);
            xil_printf("   Message  :  \n");
            for( i = 0; i < exp_info_entry_log_item->info_length; i++) {
                xil_printf("        ");
                for( j = 0; j < 16; j++){
                    xil_printf("0x%02x ", (exp_info_entry_log_item->info_payload)[16*i + j]);
                }
                xil_printf("\n");
            }
        break;

        case ENTRY_TYPE_TIME_INFO:
            time_info_entry_log_item = (time_info_entry*) entry;
            xil_printf("%d: - Time Info entry\n", entry_number );
            xil_printf("   Timestamp:  %d\n", (u32)(time_info_entry_log_item->timestamp));
            xil_printf("   Time id  :  %d\n",       time_info_entry_log_item->time_id);
            xil_printf("   Reason   :  %d\n",       time_info_entry_log_item->reason);
            xil_printf("   Abs time :  $d\n", (u32)(time_info_entry_log_item->abs_time));
            xil_printf("   New time :  %d\n", (u32)(time_info_entry_log_item->new_time));
        break;

        case ENTRY_TYPE_RX_OFDM:
            rx_common_log_item = (rx_common_entry*) entry;
            xil_printf("%d: - Rx OFDM Event\n", entry_number );
#ifdef WLAN_MAC_ENTRIES_LOG_CHAN_EST
            xil_printf("   Channel Estimates:\n");

            for( i = 0; i < 16; i++) {
                xil_printf("        ");
                for( j = 0; j < 4; j++){
                    xil_printf("0x%8x ", (((rx_ofdm_entry*)rx_common_log_item)->channel_est)[4*i + j]);
                }
                xil_printf("\n");
            }
#endif
            xil_printf("   Time:     %d\n",     (u32)(rx_common_log_item->timestamp));
            xil_printf("   FCS:      %d\n",     rx_common_log_item->fcs_status);
            xil_printf("   Pow:      %d\n",     rx_common_log_item->power);
            xil_printf("   Rate:     %d\n",     rx_common_log_item->rate);
            xil_printf("   Length:   %d\n",     rx_common_log_item->length);
            xil_printf("   Pkt Type: 0x%x\n",   rx_common_log_item->pkt_type);
            xil_printf("   Channel:  %d\n",     rx_common_log_item->chan_num);
        break;

        case ENTRY_TYPE_RX_DSSS:
            rx_common_log_item = (rx_common_entry*) entry;
            xil_printf("%d: - Rx DSSS Event\n", entry_number );
            xil_printf("   Time:     %d\n",     (u32)(rx_common_log_item->timestamp));
            xil_printf("   FCS:      %d\n",     rx_common_log_item->fcs_status);
            xil_printf("   Pow:      %d\n",     rx_common_log_item->power);
            xil_printf("   Rate:     %d\n",     rx_common_log_item->rate);
            xil_printf("   Length:   %d\n",     rx_common_log_item->length);
            xil_printf("   Pkt Type: 0x%x\n",   rx_common_log_item->pkt_type);
            xil_printf("   Channel:  %d\n",     rx_common_log_item->chan_num);
        break;

        case ENTRY_TYPE_TX_HIGH:
            tx_high_entry_log_item = (tx_high_entry*) entry;
            xil_printf("%d: - Tx High Event\n", entry_number);
            xil_printf("   Creation Time:    %d\n",     (u32)(tx_high_entry_log_item->timestamp_create));
            xil_printf("   Accept Delay:     %d\n",     (u32)(tx_high_entry_log_item->delay_accept));
            xil_printf("   Done Delay:       %d\n",     (u32)(tx_high_entry_log_item->delay_done));
            xil_printf("   Tx Unique Seq:    %d\n",     (u32)(tx_high_entry_log_item->unique_seq));
            xil_printf("   Tx Power:         %d\n",     tx_high_entry_log_item->power);
            xil_printf("   Rate:             %d\n",     tx_high_entry_log_item->rate);
            xil_printf("   Length:           %d\n",     tx_high_entry_log_item->length);
            xil_printf("   Channel:          %d\n",     tx_high_entry_log_item->chan_num);
            xil_printf("   Result:           %d\n",     tx_high_entry_log_item->result);
            xil_printf("   Pkt Type:         0x%x\n",   tx_high_entry_log_item->pkt_type);
            xil_printf("   Num Tx:           %d\n",     tx_high_entry_log_item->num_tx);
        break;

        case ENTRY_TYPE_TX_LOW:
            tx_low_entry_log_item = (tx_low_entry*) entry;
            xil_printf("%d: - Tx Low Event\n", entry_number);
            xil_printf("   Tx Start Time:    %d\n",     (u32)(tx_low_entry_log_item->timestamp_send));
            xil_printf("   Tx Unique Seq:    %d\n",     (u32)(tx_low_entry_log_item->unique_seq));
            xil_printf("   Tx Count:         %d\n",     tx_low_entry_log_item->transmission_count);
            xil_printf("   Power:            %d\n",     tx_low_entry_log_item->phy_params.power);
            xil_printf("   Rate:             %d\n",     tx_low_entry_log_item->phy_params.rate);
            xil_printf("   Length:           %d\n",     tx_low_entry_log_item->length);
            xil_printf("   Channel:          %d\n",     tx_low_entry_log_item->chan_num);
            xil_printf("   Pkt Type:         0x%x\n",   tx_low_entry_log_item->pkt_type);
            xil_printf("   Antenna Mode:     %d\n",     tx_low_entry_log_item->phy_params.antenna_mode);
            xil_printf("   # of BO Slots     %d\n",     tx_low_entry_log_item->num_slots);
        break;

        default:
            xil_printf("%d: - Unknown Event\n", entry_number);
        break;
    }

}

#endif





/*****************************************************************************/
//
// Functions to add fields to the log
//
// NOTE:  These functions should not be modified unless necessary
//
/*****************************************************************************/


/*****************************************************************************/
/**
 * Add a node info entry
 *
 * @param   None
 *
 * @return  None
 *
 *
 *****************************************************************************/
void add_node_info_entry() {


    node_info_entry * entry;
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
    u32               temp0;
    u32               entry_words = sizeof(node_info_entry) >> 2;
#endif

    // For this entry, we do not use the wlan_exp_log_create_entry wrapper because this entry should never be
    // filtered due to assumptions on the log structure (ie the first entry in the log will always be a NODE_INFO
    // entry)
    //
    entry = (node_info_entry *)event_log_get_next_empty_entry(ENTRY_TYPE_NODE_INFO, sizeof(node_info_entry));

    if (entry != NULL) {
        entry->timestamp = get_mac_time_usec();

#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
        //TODO: The node info entry is tied to wlan_exp in a way that is unnecessary. The fields here
        // should not be encoded as wlan_exp_get_parameters()

        // Add the node parameters
        temp0 = node_get_parameter_values((u32 *)&(entry->node_type), entry_words);

        // Check to make sure that there was no mismatch in sizes
        //
        //   NOTE: During initialization of the log, the hardware parameters are not yet defined.
        //       Therefore, we need to ignore when we get zero and be sure to reset the log
        //       before normal operation.
        //
        if ((temp0 != (entry_words - 2)) && (temp0 != 0)) {
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
            wlan_exp_printf(WLAN_EXP_PRINT_WARNING, print_type_event_log, "Node info size mismatch: size = %d, param size = %d\n", entry_words, temp0);
#endif
        }

#endif //WLAN_SW_CONFIG_ENABLE_WLAN_EXP

#ifdef _DEBUG_
        print_entry(0, ENTRY_TYPE_NODE_INFO, entry);
#endif
    }
}



/*****************************************************************************/
/**
 * Add a Time Info log entry to the log
 *
 * @param   timestamp          - Timestamp to use for the log entry (MAC time old timebase)
 * @param   mac_time           - MAC time (new timebase)
 * @param   system_time        - System time
 * @param   host_time          - Host time
 * @param   reason             - Reason the time info entry was created
 * @param   time_id            - ID to use for the time info entry
 * @param   use_time_id        - Use the provided time_id or system_time_id
 *                                 0 = Use system_time_id
 *                                 1 = Use provided time_id
 *
 * @return  time_info_entry *  - Pointer to the time_info_entry log entry
 *                                 NOTE: This can be NULL if an entry was not allocated
 *
 *****************************************************************************/
void add_time_info_entry(u64 timestamp, u64 mac_time, u64 system_time, u64 host_time, u32 reason, u32 time_id, u8 use_time_id) {

	time_info_entry * time_entry;

    // Create a time info log entry
    time_entry = (time_info_entry *) wlan_exp_log_create_entry(ENTRY_TYPE_TIME_INFO, sizeof(time_info_entry));

    if (time_entry != NULL) {
        time_entry->timestamp     = timestamp;

        if (use_time_id) {
            time_entry->time_id   = time_id;
        } else {
            time_entry->time_id   = system_time_id;
            system_time_id++;
        }

        time_entry->reason             = reason;
        time_entry->mac_timestamp      = mac_time;
        time_entry->system_timestamp   = system_time;
        time_entry->host_timestamp     = host_time;
    }
}



/*****************************************************************************/
/**
 * Add the temperature to the log
 *
 * @param   None
 *
 * @return  u32              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *
 *****************************************************************************/
u32 add_temperature_to_log() {


    temperature_entry  * entry;
    u32                  entry_size        = sizeof(temperature_entry);
    wlan_mac_hw_info_t * hw_info;

    hw_info = get_mac_hw_info();

    entry = (temperature_entry *)wlan_exp_log_create_entry(ENTRY_TYPE_TEMPERATURE, entry_size);

    if (entry != NULL) {
        entry->timestamp     = get_mac_time_usec();
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
        entry->id            = node_get_node_id();
#else
        // TODO: The notion of a "node ID" is very wlan_exp-centric. The temperature log entry
        // should not have this field. I'd argue it doesn't need any identifying fields; the identity
        // of the the source of this temperature reading is unambiguous based upon which log file
        // it is in.
        entry->id            = 0;
#endif

        // TODO: serial number unnecessary for the same reason as above.
        entry->serial_number = hw_info->serial_number;
        entry->curr_temp     = get_current_temp();
        entry->min_temp      = get_min_temp();
        entry->max_temp      = get_max_temp();

#ifdef _DEBUG_
        xil_printf("[%d] Node %d (W3-a-%05d)= (%d %d %d)\n", (u32)entry->timestamp, entry->id, entry->serial_number,
                                                             entry->curr_temp, entry->min_temp, entry->max_temp);
#endif

        return XST_SUCCESS;
    }

    return XST_FAILURE;
}

#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
