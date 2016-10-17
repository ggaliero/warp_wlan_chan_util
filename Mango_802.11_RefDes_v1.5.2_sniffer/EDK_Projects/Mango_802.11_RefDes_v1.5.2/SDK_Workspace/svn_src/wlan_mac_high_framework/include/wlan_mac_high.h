/** @file wlan_mac_high.h
 *  @brief Top-level WLAN MAC High Framework
 *
 *  This contains the top-level code for accessing the WLAN MAC High Framework.
 *
 *  @copyright Copyright 2014-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *                See LICENSE.txt included in the design archive or
 *                at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */


/*************************** Constant Definitions ****************************/
#ifndef WLAN_MAC_HIGH_H_
#define WLAN_MAC_HIGH_H_

#include "wlan_mac_high_sw_config.h"

#include "xil_types.h"
#include "wlan_mac_common.h"

//-----------------------------------------------
// Boot memory defines
//
#define INIT_DATA_BASEADDR                  XPAR_MB_HIGH_INIT_BRAM_CTRL_S_AXI_BASEADDR   ///< Base address of memory used for storing boot data
#define INIT_DATA_DOTDATA_IDENTIFIER        0x1234ABCD                                   ///< "Magic number" used as an identifier in boot data memory
#define INIT_DATA_DOTDATA_START            (INIT_DATA_BASEADDR+0x200)                    ///< Offset into memory for boot data
#define INIT_DATA_DOTDATA_SIZE             (4*(XPAR_MB_HIGH_INIT_BRAM_CTRL_S_AXI_HIGHADDR - INIT_DATA_DOTDATA_START))  ///< Amount of space available in boot data memory



//-----------------------------------------------
// Auxiliary (AUX) BRAM and DRAM (DDR) Memory Maps
//
#define AUX_BRAM_BASE                      (XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR)
#define AUX_BRAM_SIZE                      (XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_HIGHADDR - XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_BASEADDR + 1)
#define AUX_BRAM_HIGH                      (XPAR_MB_HIGH_AUX_BRAM_CTRL_S_AXI_HIGHADDR)

#define DRAM_BASE                          (XPAR_DDR3_SODIMM_S_AXI_BASEADDR)
#define DRAM_SIZE                          (XPAR_DDR3_SODIMM_S_AXI_HIGHADDR - XPAR_DDR3_SODIMM_S_AXI_BASEADDR + 1)
#define DRAM_HIGH                          (XPAR_DDR3_SODIMM_S_AXI_HIGHADDR)


/********************************************************************
 *
 * --------------------------------------------------------
 *      Aux. BRAM (64 KB)    |      DRAM (1048576 KB)
 * --------------------------------------------------------
 *                           |
 *                           |  CPU High Data Linker Space
 *                           |
 *   Tx Queue DL_ENTRY       |-----------------------------
 *                           |
 *                          -->      Tx Queue Buffer
 *                           |
 * --------------------------|-----------------------------
 *                           |
 *   BSS Info DL_ENTRY      -->      BSS Info Buffer
 *                           |
 * --------------------------|-----------------------------
 *                           |
 *  Station Info DL_ENTRY   -->    Station Info Buffer
 *                           |
 * --------------------------|-----------------------------
 *                           |
 *       Eth Tx BD           |      User Scratch Space
 *                           |
 * --------------------------|-----------------------------
 *                           |
 *       Eth Rx BD           |          Event Log
 *                           |
 * --------------------------|-----------------------------
 *
 ********************************************************************/

#define high_addr_calc(base, size)         ((base)+((size)-1))



/********************************************************************
 * CPU High Linker Data Space
 *
 * In order for the linker to store data sections in the DDR, we must reserve the
 * beginning of DDR.  You can see how this is used by looking for the .cpu_high_data
 * section in the linker command file (lscript.ld).  By default, 1024 KB (ie 1 MB)
 * of space is reserved for this section.
 *
 * In the default reference design, only the WARP IP/UDP library is loaded in this
 * section since it requires memory space to store buffers / information for sending /
 * receiving packets.
 *
 * NOTE:  Please be aware that while the linker can link into this section, it cannot
 *     be loaded by the SDK.  Therefore, the CPU must perform any necessary
 *     initialization.
 *
 ********************************************************************/
#define CPU_HIGH_DDR_LINKER_DATA_BASE      (DRAM_BASE)
#define CPU_HIGH_DDR_LINKER_DATA_SIZE      (1024 * 1024)
#define CPU_HIGH_DDR_LINKER_DATA_HIGH       high_addr_calc(CPU_HIGH_DDR_LINKER_DATA_BASE, CPU_HIGH_DDR_LINKER_DATA_HIGH)



/********************************************************************
 * TX Queue
 *
 * The Tx Queue consists of two pieces:
 *     (1) dl_entry structs that live in the AUX BRAM
 *     (2) Data buffers for the packets themselves than live in DRAM
 *
 * The below definitions carve out the sizes of memory for these two pieces.  The default
 * value of 40 kB do the dl_entry memory space was chosen. Because each dl_entry has a
 * size of 12 bytes, this space allows for a potential of 3413 dl_entry structs describing
 * Tx queue elements.
 *
 * As far as the actual payload space in DRAM, 14000 kB was chosen because this is enough
 * to store each of the 3413 Tx queue elements. Each queue element points to a unique 4KB-sized
 * buffer.
 *
 ********************************************************************/
#define TX_QUEUE_DL_ENTRY_MEM_BASE         (AUX_BRAM_BASE)
#define TX_QUEUE_DL_ENTRY_MEM_SIZE         (40 * 1024)
#define TX_QUEUE_DL_ENTRY_MEM_HIGH          high_addr_calc(TX_QUEUE_DL_ENTRY_MEM_BASE, TX_QUEUE_DL_ENTRY_MEM_SIZE)

#define TX_QUEUE_BUFFER_BASE               (CPU_HIGH_DDR_LINKER_DATA_BASE + CPU_HIGH_DDR_LINKER_DATA_SIZE)
#define TX_QUEUE_BUFFER_SIZE               (14000 * 1024)
#define TX_QUEUE_BUFFER_HIGH                high_addr_calc(TX_QUEUE_BUFFER_BASE, TX_QUEUE_BUFFER_SIZE)



/********************************************************************
 * BSS Info
 *
 * The BSS Info storage consists of two pieces:
 *     (1) dl_entry structs that live in the aux. BRAM and
 *     (2) bss_info_t buffers with the actual content that live in DRAM
 *
 ********************************************************************/
#define BSS_INFO_DL_ENTRY_MEM_BASE         (TX_QUEUE_DL_ENTRY_MEM_BASE + TX_QUEUE_DL_ENTRY_MEM_SIZE)
#define BSS_INFO_DL_ENTRY_MEM_SIZE         (WLAN_OPTIONS_AUX_SIZE_KB_BSS_INFO)
#define BSS_INFO_DL_ENTRY_MEM_HIGH          high_addr_calc(BSS_INFO_DL_ENTRY_MEM_BASE, BSS_INFO_DL_ENTRY_MEM_SIZE)

#define BSS_INFO_BUFFER_BASE               (TX_QUEUE_BUFFER_BASE + TX_QUEUE_BUFFER_SIZE)
#define BSS_INFO_BUFFER_SIZE			   ((BSS_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))*sizeof(bss_info_t))
#define BSS_INFO_BUFFER_HIGH                high_addr_calc(BSS_INFO_BUFFER_BASE, BSS_INFO_BUFFER_SIZE)


/********************************************************************
 * Station Info
 *
 * The Station Info storage consists of two pieces:
 *     (1) dl_entry structs that live in the aux. BRAM and
 *     (2) station_info_t buffers with the actual content that live in DRAM
 *
 ********************************************************************/
#define STATION_INFO_DL_ENTRY_MEM_BASE     (BSS_INFO_DL_ENTRY_MEM_BASE + BSS_INFO_DL_ENTRY_MEM_SIZE)
#define STATION_INFO_DL_ENTRY_MEM_SIZE     (WLAN_OPTIONS_AUX_SIZE_KB_STATION_INFO)
#define STATION_INFO_DL_ENTRY_MEM_NUM      (STATION_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))
#define STATION_INFO_DL_ENTRY_MEM_HIGH      high_addr_calc(STATION_INFO_DL_ENTRY_MEM_BASE, STATION_INFO_DL_ENTRY_MEM_SIZE)

#define STATION_INFO_BUFFER_BASE          (BSS_INFO_BUFFER_BASE + BSS_INFO_BUFFER_SIZE)
#define STATION_INFO_BUFFER_SIZE		  ((STATION_INFO_DL_ENTRY_MEM_SIZE/sizeof(dl_entry))*sizeof(station_info_t))
#define STATION_INFO_BUFFER_HIGH           high_addr_calc(STATION_INFO_BUFFER_BASE, STATION_INFO_BUFFER_SIZE)



/********************************************************************
 * Ethernet TX Buffer Descriptors
 *
 * The current architecture blocks on Ethernet transmissions. As such, only a single
 * Eth DMA buffer descriptor (BD) is needed. Each BD is 64 bytes in size (see
 * XAXIDMA_BD_MINIMUM_ALIGNMENT), so we set ETH_TX_BD_SIZE to 64.
 *
 ********************************************************************/
#define ETH_TX_BD_BASE                     (STATION_INFO_DL_ENTRY_MEM_BASE + STATION_INFO_DL_ENTRY_MEM_SIZE)
#define ETH_TX_BD_SIZE                     (64)
#define ETH_TX_BD_HIGH                      high_addr_calc(ETH_TX_BD_BASE, ETH_TX_BD_SIZE)



/********************************************************************
 * Ethernet RX Buffer Descriptors
 *
 * The last section we are defining in the aux. BRAM is for ETH_RX.
 * Like TX, each BD is 64 bytes. Unlike TX, we need more than a single
 * BD to be able to handle bursty Ethernet receptions.
 *
 ********************************************************************/
#define ETH_RX_BD_BASE                     (ETH_TX_BD_BASE + ETH_TX_BD_SIZE)
#define ETH_RX_BD_SIZE                     (WLAN_OPTIONS_AUX_SIZE_KB_RX_ETH_BD)
#define ETH_RX_BD_HIGH                      high_addr_calc(ETH_RX_BD_BASE, ETH_RX_BD_SIZE)



/********************************************************************
 * User Scratch Space
 *
 * We have set aside 10MB of space for users to use the DRAM in their applications.
 * We do not use the below definitions in any part of the reference design.
 *
 ********************************************************************/
#define USER_SCRATCH_BASE                  (STATION_INFO_BUFFER_BASE + STATION_INFO_BUFFER_SIZE)
#define USER_SCRATCH_SIZE                  (10000 * 1024)
#define USER_SCRATCH_HIGH                   high_addr_calc(USER_SCRATCH_BASE, USER_SCRATCH_SIZE)


/********************************************************************
 * Event Log
 *
 * The remaining space in DRAM is used for the WLAN Experiment Framwork event log. The above
 * sections in DRAM are much smaller than the space set aside for the event log. In the current
 * implementation, the event log is ~995 MB.
 *
 ********************************************************************/
#define EVENT_LOG_BASE                     (USER_SCRATCH_BASE + USER_SCRATCH_SIZE)
#define EVENT_LOG_SIZE                     (DRAM_SIZE - (CPU_HIGH_DDR_LINKER_DATA_SIZE + TX_QUEUE_BUFFER_SIZE + BSS_INFO_BUFFER_SIZE + STATION_INFO_BUFFER_SIZE + USER_SCRATCH_SIZE))
#define EVENT_LOG_HIGH                      high_addr_calc(EVENT_LOG_BASE, EVENT_LOG_SIZE)



//-----------------------------------------------
// Device IDs
//
// NOTE:  These are re-definitions of xparameters.h #defines so that the name of the
//     underlying hardware component can change and it will only require modifying
//     one line of code in the SDK project.
//
#define INTC_DEVICE_ID                                     XPAR_INTC_0_DEVICE_ID              ///< XParameters rename of interrupt controller device ID
#define UARTLITE_DEVICE_ID                                 XPAR_UARTLITE_0_DEVICE_ID          ///< XParameters rename for UART
#define GPIO_USERIO_DEVICE_ID                              XPAR_MB_HIGH_SW_GPIO_DEVICE_ID     ///< XParameters rename of device ID of GPIO

//-----------------------------------------------
// Interrupt IDs
//
//  These macros define the index of each interrupt signal in the axi_intc input
//  The defines below rename macros from xparameters.h to remove instance-name-specific
//    strings from the application code
#define INTC_GPIO_USERIO_INTERRUPT_ID                      XPAR_MB_HIGH_INTC_MB_HIGH_SW_GPIO_IP2INTC_IRPT_INTR               ///< XParameters rename of GPIO interrupt ID
#define UARTLITE_INT_IRQ_ID                                XPAR_INTC_0_UARTLITE_0_VEC_ID      ///< XParameters rename of UART interrupt ID
#define TMRCTR_INTERRUPT_ID                                XPAR_INTC_0_TMRCTR_0_VEC_ID        ///< XParameters rename of timer interrupt ID



//-----------------------------------------------
// WLAN Constants
//
#define ENCAP_MODE_AP                                      0                                  ///< Used as a flag for AP encapsulation and de-encapsulation
#define ENCAP_MODE_STA                                     1                                  ///< Used as a flag for STA encapsulation and de-encapsulation
#define ENCAP_MODE_IBSS                                    2                                  ///< Used as a flag for IBSS encapsulation and de-encapsulation

#define TX_BUFFER_NUM                                      2                                  ///< Number of PHY transmit buffers to use. This should remain 2 (ping/pong buffering).

#define GPIO_USERIO_INPUT_CHANNEL                          1                                  ///< Channel used as input user IO inputs (buttons, DIP switch)
#define GPIO_USERIO_INPUT_IR_CH_MASK                       XGPIO_IR_CH1_MASK                  ///< Mask for enabling interrupts on GPIO input

#define GPIO_MASK_DRAM_INIT_DONE                           0x00000100                         ///< Mask for GPIO -- DRAM initialization bit
#define GPIO_MASK_PB_U                                     0x00000040                         ///< Mask for GPIO -- "Up" Pushbutton
#define GPIO_MASK_PB_M                                     0x00000020                         ///< Mask for GPIO -- "Middle" Pushbutton
#define GPIO_MASK_PB_D                                     0x00000010                         ///< Mask for GPIO -- "Down" Pushbutton
#define GPIO_MASK_DS_3                                     0x00000008                         ///< Mask for GPIO -- MSB of Dip Switch

#define UART_BUFFER_SIZE                                   1                                  ///< UART is configured to read 1 byte at a time

#define NUM_VALID_RATES                                    12                                 ///< Number of supported rates


//-----------------------------------------------
// Callback Return Flags
//
#define MAC_RX_CALLBACK_RETURN_FLAG_DUP							0x00000001
#define MAC_RX_CALLBACK_RETURN_FLAG_NO_COUNTS					0x00000002
#define MAC_RX_CALLBACK_RETURN_FLAG_NO_LOG_ENTRY				0x00000004


/***************************** Include Files *********************************/

/********************************************************************
 * Include other framework headers
 *
 * NOTE:  Includes have to be after any #define that are needed in the typedefs within the includes.
 *
 ********************************************************************/
#include "wlan_mac_queue.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_mac_dl_list.h"



/************************** Global Type Definitions **************************/

typedef enum {INTERRUPTS_DISABLED, INTERRUPTS_ENABLED} interrupt_state_t;



/******************** Global Structure/Enum Definitions **********************/

/***************************** Global Constants ******************************/

extern const  u8 bcast_addr[MAC_ADDR_LEN];
extern const  u8 zero_addr[MAC_ADDR_LEN];


/*************************** Function Prototypes *****************************/
void               wlan_mac_high_init();
void               wlan_mac_high_heap_init();

int                          wlan_mac_high_interrupt_init();
inline int                   wlan_mac_high_interrupt_restore_state(interrupt_state_t new_interrupt_state);
inline interrupt_state_t     wlan_mac_high_interrupt_stop();

void               wlan_mac_high_uart_rx_handler(void *CallBackRef, unsigned int EventData);
void               wlan_mac_high_userio_gpio_handler(void *InstancePtr);

dl_entry*          wlan_mac_high_find_counts_ADDR(dl_list* list, u8* addr);

u32                wlan_mac_high_get_user_io_state();

void               wlan_mac_high_set_pb_u_callback(function_ptr_t callback);
void               wlan_mac_high_set_pb_m_callback(function_ptr_t callback);
void               wlan_mac_high_set_pb_d_callback(function_ptr_t callback);
void               wlan_mac_high_set_uart_rx_callback(function_ptr_t callback);
void               wlan_mac_high_set_mpdu_tx_high_done_callback(function_ptr_t callback);
void               wlan_mac_high_set_mpdu_tx_low_done_callback(function_ptr_t callback);
void               wlan_mac_high_set_beacon_tx_done_callback(function_ptr_t callback);
void 			   wlan_mac_high_set_mpdu_rx_callback(function_ptr_t callback);
void               wlan_mac_high_set_poll_tx_queues_callback(function_ptr_t callback);
void               wlan_mac_high_set_mpdu_dequeue_callback(function_ptr_t callback);
void 			   wlan_mac_high_set_cpu_low_reboot_callback(function_ptr_t callback);

void*              wlan_mac_high_malloc(u32 size);
void*              wlan_mac_high_calloc(u32 size);
void*              wlan_mac_high_realloc(void* addr, u32 size);
void               wlan_mac_high_free(void* addr);
void               wlan_mac_high_display_mallinfo();

int                wlan_mac_high_memory_test();
int                wlan_mac_high_right_shift_test();

int                wlan_mac_high_cdma_start_transfer(void* dest, void* src, u32 size);
void               wlan_mac_high_cdma_finish_transfer();

void               wlan_mac_high_mpdu_transmit(tx_queue_element_t* packet, int tx_pkt_buf);

u8                 wlan_mac_high_valid_tagged_rate(u8 rate);
void               wlan_mac_high_tagged_rate_to_readable_rate(u8 rate, char* str);

void               wlan_mac_high_setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 );
void               wlan_mac_high_setup_tx_frame_info( mac_header_80211_common * header, tx_queue_element_t * curr_tx_queue_element, u32 tx_length, u8 flags, u8 QID );

void               wlan_mac_high_ipc_rx();
void               wlan_mac_high_process_ipc_msg(wlan_ipc_msg_t * msg);

void               wlan_mac_high_set_srand(u32 seed);
u8                 wlan_mac_high_bss_channel_spec_to_radio_chan(chan_spec_t chan_spec);
void               wlan_mac_high_set_radio_channel(u32 mac_channel);
void               wlan_mac_high_config_txrx_beacon(beacon_txrx_configure_t* beacon_txrx_configure);
void               wlan_mac_high_set_rx_ant_mode(u8 ant_mode);
void               wlan_mac_high_set_tx_ctrl_pow(s8 pow);
void               wlan_mac_high_set_rx_filter_mode(u32 filter_mode);
void               wlan_mac_high_set_dsss(u32 dsss_value);

int                wlan_mac_high_write_low_mem(u32 num_words, u32* payload);
int                wlan_mac_high_read_low_mem(u32 num_words, u32 baseaddr, u32* payload);
int                wlan_mac_high_write_low_param(u32 num_words, u32* payload);

void               wlan_mac_high_request_low_state();
int 			   wlan_mac_high_is_cpu_low_initialized();
inline int         wlan_mac_high_is_dequeue_allowed();
int                wlan_mac_high_get_empty_tx_packet_buffer();
u8                 wlan_mac_high_is_pkt_ltg(void* mac_payload, u16 length);


int                wlan_mac_high_configure_beacon_tx_template(mac_header_80211_common* tx_header_common_ptr, bss_info_t* bss_info, tx_params_t* tx_params_ptr, u8 flags);
int                wlan_mac_high_update_beacon_tx_params(tx_params_t* tx_params_ptr);

void               wlan_mac_high_print_station_infos(dl_list* assoc_tbl);


// Common functions that must be implemented by users of the framework
// TODO: Make these into callback. We should not require these implementations
dl_list*          get_bss_member_list();
u8*          	  get_wlan_mac_addr();

#endif /* WLAN_MAC_HIGH_H_ */
