/** @file wlan_mac_high.c
 *  @brief Top-level WLAN MAC High Framework
 *  
 *  This contains the top-level code for accessing the WLAN MAC High Framework.
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

// Xilinx Includes
#include "stdlib.h"
#include "malloc.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xuartlite.h"
#include "xaxicdma.h"

// WLAN Includes
#include "wlan_mac_common.h"
#include "wlan_mac_dl_list.h"
#include "wlan_mac_mailbox_util.h"
#include "wlan_mac_pkt_buf_util.h"
#include "wlan_mac_time_util.h"
#include "wlan_mac_userio_util.h"
#include "wlan_mac_802_11_defs.h"
#include "wlan_mac_high.h"
#include "wlan_mac_packet_types.h"
#include "wlan_mac_queue.h"
#include "wlan_mac_eth_util.h"
#include "wlan_mac_ltg.h"
#include "wlan_mac_entries.h"
#include "wlan_mac_event_log.h"
#include "wlan_mac_schedule.h"
#include "wlan_mac_addr_filter.h"
#include "wlan_mac_bss_info.h"
#include "wlan_mac_station_info.h"
#include "wlan_exp_common.h"
#include "wlan_exp_node.h"
#include "wlan_mac_scan.h"
#include "w3_iic_eeprom.h"

/*********************** Global Variable Definitions *************************/

//These variables are defined by the linker at compile time
extern int                   __data_start;                 ///< Start address of the data section
extern int                   __data_end;                   ///< End address of the data section

extern int                   __cpu_high_ddr_linker_data_start;  ///< Start address of the CPU High linker data section
extern int                   __cpu_high_ddr_linker_data_end;    ///< End address of the CPU High linker data section

extern int                   __bss_start;                  ///< Start address of the bss section
extern int                   __bss_end;                    ///< End address of the bss section

extern int                   _heap_start;                  ///< Start address of the heap
extern int                   _HEAP_SIZE;                   ///< Size of the heap

extern int                   _stack_end;                   ///< Start of the stack (stack counts backwards)
extern int                   __stack;                      ///< End of the stack



/*************************** Variable Definitions ****************************/

// Constants
const  u8                    bcast_addr[MAC_ADDR_LEN]    = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const  u8                    zero_addr[MAC_ADDR_LEN]     = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// HW structures
static XGpio                 Gpio_userio;                  ///< GPIO driver instnace for user IO inputs

XIntc                        InterruptController;          ///< Interrupt Controller instance
XUartLite                    UartLite;                     ///< UART Device instance
XAxiCdma                     cdma_inst;                    ///< Central DMA instance

// UART interface
u8                           uart_rx_buffer[UART_BUFFER_SIZE];       ///< Buffer for received byte from UART

// Callback function pointers
volatile function_ptr_t      		pb_u_callback;                ///< User callback for "up" pushbutton
volatile function_ptr_t      		pb_m_callback;                ///< User callback for "middle" pushbutton
volatile function_ptr_t      		pb_d_callback;                ///< User callback for "down" pushbutton
volatile function_ptr_t      		uart_callback;                ///< User callback for UART reception
volatile function_ptr_t      		mpdu_tx_high_done_callback;   ///< User callback for lower-level message that MPDU transmission is complete
volatile function_ptr_t      		mpdu_tx_low_done_callback;    ///< User callback for lower-level message that MPDU transmission is complete
volatile function_ptr_t      		mpdu_rx_callback;             ///< User callback for lower-level message that MPDU reception is ready for processing
volatile function_ptr_t      		tx_poll_callback;             ///< User callback when higher-level framework is ready to send a packet to low
volatile function_ptr_t		 		beacon_tx_done_callback;	   ///< User callback for low-level message that a Beacon transmission is complete
volatile function_ptr_t      		mpdu_tx_dequeue_callback;     ///< User callback for higher-level framework dequeuing a packet
volatile function_ptr_t      		cpu_low_reboot_callback;     ///< User callback for CPU_LOW boot

// CPU_LOW parameters that MAC High Framework tracks and is responsible for re-applying
//  in the event of a CPU_LOW reboot.
volatile	u32			 low_param_channel;
volatile	u32			 low_param_dsss_en;
volatile	u8			 low_param_rx_ant_mode;
volatile 	s8			 low_param_tx_ctrl_pow;
volatile	u32			 low_param_rx_filter;
volatile	u32			 low_param_random_seed;

// Node information
volatile u8                  dram_present;                 ///< Indication variable for whether DRAM SODIMM is present on this hardware

// Status information
wlan_mac_hw_info_t         * hw_info;
volatile static u32          cpu_low_status;               ///< Tracking variable for lower-level CPU status

// CPU Low Register Read Buffer
volatile static u32        * cpu_low_reg_read_buffer;
volatile static u8           cpu_low_reg_read_buffer_status;

#define EEPROM_BASEADDR                                    XPAR_W3_IIC_EEPROM_ONBOARD_BASEADDR

#define CPU_LOW_REG_READ_BUFFER_STATUS_READY               1
#define CPU_LOW_REG_READ_BUFFER_STATUS_NOT_READY           0

// Interrupt State
volatile static interrupt_state_t interrupt_state;

// Debug GPIO State
volatile static u8           debug_gpio_state;             ///< Current state of debug GPIO pins

// IPC variables
wlan_ipc_msg_t               ipc_msg_from_low;                                           ///< IPC message from lower-level
u32                          ipc_msg_from_low_payload[MAILBOX_BUFFER_MAX_NUM_WORDS];     ///< Buffer space for IPC message from lower-level

// Memory Allocation Debugging
volatile static u32          num_malloc;                   ///< Tracking variable for number of times malloc has been called
volatile static u32          num_free;                     ///< Tracking variable for number of times free has been called
volatile static u32          num_realloc;                  ///< Tracking variable for number of times realloc has been called


/*************************** Functions Prototypes ****************************/

#ifdef _DEBUG_
void wlan_mac_high_copy_comparison();
#endif


/******************************** Functions **********************************/

/**
 * @brief Initialize Heap and Data Sections
 *
 * Dynamic memory allocation through malloc uses metadata in the data section
 * of the elf binary. This metadata is not reset upon software reset (i.e., when a
 * user presses the reset button on the hardware). This will cause failures on
 * subsequent boots because this metadata has not be reset back to its original
 * state at the first boot.
 *
 * This function backs up the original data section to a dedicated BRAM, so it can
 * be copied back on subsequent soft resets.
 *
 * @param None
 * @return None
 *
 * @note This function should be the first thing called after boot. If it is
 * called after other parts have the code have started dynamic memory access,
 * there will be unpredictable results on software reset.
 */
void wlan_mac_high_heap_init(){
	u32 data_size;
	volatile u32* identifier;

	data_size = 4*(&__data_end - &__data_start);
	identifier = (u32*)INIT_DATA_BASEADDR;

	//Zero out the heap
	bzero((void*)&_heap_start, (int)&_HEAP_SIZE);

	//Zero out the bss
	bzero((void*)&__bss_start, 4*(&__bss_end - &__bss_start));

#ifdef INIT_DATA_BASEADDR
	if(*identifier == INIT_DATA_DOTDATA_IDENTIFIER){
		//This program has run before. We should copy the .data out of the INIT_DATA memory.
		if(data_size <= INIT_DATA_DOTDATA_SIZE){
			memcpy((void*)&__data_start, (void*)INIT_DATA_DOTDATA_START, data_size);
		}

	} else {
		//This is the first time this program has been run.
		if(data_size <= INIT_DATA_DOTDATA_SIZE){
			*identifier = INIT_DATA_DOTDATA_IDENTIFIER;
			memcpy((void*)INIT_DATA_DOTDATA_START, (void*)&__data_start, data_size);
		}

	}
#endif
}



/**
 * @brief Initialize CPU High DDR Linker Data Sections
 *
 * The linker command file defines the cpu_high_linker_data section that is used
 * to link sections that need memory outside of what is provided by BRAM.  However,
 * the SDK cannot load this section.  Therefore, any initialization of that section
 * must be done in this function.
 *
 * @param   None
 *
 * @return  None
 *
 * @note    This function should be the first thing called after the DDR has been
 *     detected. If it is called after other parts have the code have started
 *     accessing this memory, there will be unpredictable results.
 *
 */
void wlan_mac_high_cpu_ddr_linker_data_init(u8 dram_present) {
    u32            data_size;
    // volatile u32 * identifier;                             // Uncomment if needed

    data_size  = 4 * (&__cpu_high_ddr_linker_data_end - &__cpu_high_ddr_linker_data_start);
    // identifier = (u32 *)CPU_HIGH_DDR_LINKER_DATA_BASE;     // Uncomment if needed

    // Check that the section has not exceeded the allocated size
    if (data_size > CPU_HIGH_DDR_LINKER_DATA_SIZE) {
        xil_printf("!!! ERROR: CPU High DDR linker data exceeds allocated size. !!!\n");
        xil_printf("    Please allocate more space in wlan_mac_high.h by \n");
        xil_printf("    updating CPU_HIGH_LINKER_DATA_SIZE.\n");
    }


#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
    // Check if DRAM is present
    if (!dram_present) {
        xil_printf("!!! ERROR: WLAN Exp requires DDR for Ethernet communication. !!!\n");
    }

    // Perform any WLAN Exp initialization of the memory
    if (WLAN_EXP_CLEAR_DDR_ON_BOOT) {
        clear_ddr(WLAN_EXP_VERBOSE);
    }
#endif

    //-----------------------------------------------------
    // Perform any User initialization of the memory
    //-----------------------------------------------------



    //-----------------------------------------------------
    // End Perform any User initialization of the memory
    //-----------------------------------------------------
}



/**
 * @brief Initialize MAC High Framework
 *
 * This function initializes the MAC High Framework by setting
 * up the hardware and other subsystems in the framework.
 *
 * @param None
 * @return None
 */
void wlan_mac_high_init(){
	int              Status;
    u32              i;
	tx_frame_info_t* tx_frame_info;
	rx_frame_info_t* rx_frame_info;
	u64              timestamp;
#if WLAN_SW_CONFIG_ENABLE_LOGGING
	u32              log_size;
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
	XAxiCdma_Config* cdma_cfg_ptr;

	// ***************************************************
	// Initialize XIntc
	// ***************************************************
	/* XIntc_Initialize() doesn't deal well with a reboot
	 * of CPU_HIGH where interrupt peripherals are still
	 * running. We'll explicitly do the following before
	 * re-initializing the interrupt controller:
	 *
	 * - Disable IRQ output signal
	 * - Disable all interrupt sources
	 * - Acknowledge all sources
	 */
	XIntc_Config *CfgPtr = XIntc_LookupConfig(INTC_DEVICE_ID);

	XIntc_Out32(CfgPtr->BaseAddress + XIN_MER_OFFSET, 0);
	XIntc_Out32(CfgPtr->BaseAddress + XIN_IER_OFFSET, 0);
	XIntc_Out32(CfgPtr->BaseAddress + XIN_IAR_OFFSET, 0xFFFFFFFF);

	bzero(&InterruptController, sizeof(XIntc));

	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if ((Status != XST_SUCCESS)) {
		xil_printf("Error in initializing Interrupt Controller\n");
	}

	// Check that right shift works correctly
	//   Issue with -Os in Xilinx SDK 14.7
	if (wlan_mac_high_right_shift_test() != 0) {
		cpu_error_halt(WLAN_ERROR_CODE_RIGHT_SHIFT);
	}

	// Sanity check memory map of aux. BRAM and DRAM
	//Aux. BRAM Check
	Status = (AUX_BRAM_BASE <= TX_QUEUE_DL_ENTRY_MEM_BASE) && (TX_QUEUE_DL_ENTRY_MEM_HIGH < BSS_INFO_DL_ENTRY_MEM_BASE) && (BSS_INFO_DL_ENTRY_MEM_HIGH < STATION_INFO_DL_ENTRY_MEM_BASE ) && (STATION_INFO_DL_ENTRY_MEM_HIGH < ETH_TX_BD_BASE) && (ETH_TX_BD_HIGH < ETH_RX_BD_BASE) && (ETH_RX_BD_HIGH <= AUX_BRAM_HIGH);
	if(Status != 1){
		xil_printf("Error: Overlap detected in Aux. BRAM. Check address assignments\n");
	}

	//DRAM Check
	Status = (DRAM_BASE <= TX_QUEUE_BUFFER_BASE) && (TX_QUEUE_BUFFER_HIGH < BSS_INFO_BUFFER_BASE) && (BSS_INFO_BUFFER_HIGH < STATION_INFO_BUFFER_BASE) && (STATION_INFO_BUFFER_HIGH < USER_SCRATCH_BASE) && (USER_SCRATCH_HIGH < EVENT_LOG_BASE) && (EVENT_LOG_HIGH <= DRAM_HIGH);
	if(Status != 1){
		xil_printf("Error: Overlap detected in DRAM. Check address assignments\n");
	}

	// ***************************************************
	// Initialize libraries
	// ***************************************************

	// Initialize mailbox
	init_mailbox();

    // Initialize packet buffers
	init_pkt_buf();

	// Set stack protection addresses
	mtshr(&__stack);
	mtslr(&_stack_end);

    // Initialize the EEPROM read/write core
    iic_eeprom_init(EEPROM_BASEADDR, 0x64, XPAR_CPU_ID);

	// Initialize the HW info structure
	init_mac_hw_info();

	// ***************************************************
    // Initialize callbacks and global state variables
	// ***************************************************
	pb_u_callback            		= (function_ptr_t)wlan_null_callback;
	pb_m_callback            		= (function_ptr_t)wlan_null_callback;
	pb_d_callback            		= (function_ptr_t)wlan_null_callback;
	uart_callback            		= (function_ptr_t)wlan_null_callback;
	mpdu_rx_callback         		= (function_ptr_t)wlan_null_callback;
	mpdu_tx_high_done_callback    	= (function_ptr_t)wlan_null_callback;
	mpdu_tx_low_done_callback    	= (function_ptr_t)wlan_null_callback;
	beacon_tx_done_callback	 		= (function_ptr_t)wlan_null_callback;
	tx_poll_callback         		= (function_ptr_t)wlan_null_callback;
	mpdu_tx_dequeue_callback 		= (function_ptr_t)wlan_null_callback;
	cpu_low_reboot_callback  		= (function_ptr_t)wlan_null_callback;

	set_mailbox_rx_callback((function_ptr_t)wlan_mac_high_ipc_rx);

	interrupt_state = INTERRUPTS_DISABLED;

	num_malloc  = 0;
	num_realloc = 0;
	num_free    = 0;

	low_param_channel 		= 0xFFFFFFFF;
	low_param_dsss_en 		= 0xFFFFFFFF;
	low_param_rx_ant_mode 	= 0xFF;
	low_param_tx_ctrl_pow 	= -1;
	low_param_rx_filter 	= 0xFFFFFFFF;
	low_param_random_seed	= 0xFFFFFFFF;

	cpu_low_reg_read_buffer        = NULL;

	// ***************************************************
	// Initialize Transmit Packet Buffers
	// ***************************************************
	for(i = 0; i < NUM_TX_PKT_BUFS; i++){
		tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(i);
		switch(i){
			case TX_PKT_BUF_MPDU_1:
			case TX_PKT_BUF_MPDU_2:
			case TX_PKT_BUF_MPDU_3:
				switch(tx_frame_info->tx_pkt_buf_state){
					case TX_PKT_BUF_UNINITIALIZED:
					case TX_PKT_BUF_HIGH_CTRL:
						// Buffer already clean on boot or reboot
					case TX_PKT_BUF_DONE:
						// CPU High rebooted while CPU Low finished old Tx
						//  Ignore the packet buffer contents and clean up
					default:
						// Something went wrong if tx_pkt_buf_state is something
						// other than one of the tx_pkt_buf_state_t enums. We'll
						// attempt to resolve the problem by explicitly setting
						// the state.
						force_lock_tx_pkt_buf(i);
						tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
					break;
					case TX_PKT_BUF_READY:
					case TX_PKT_BUF_LOW_CTRL:
						// CPU High rebooted after submitting packet for transmission
						//  Will be handled by CPU Low, either because CPU Low is about
						//  to transmit p or just rebooted and will clean up
					break;
				}
			break;
			case TX_PKT_BUF_BEACON:
				tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
			case TX_PKT_BUF_RTS:
			case TX_PKT_BUF_ACK_CTS:
				unlock_tx_pkt_buf(i);
			default:
			break;
		}
	}

	// ***************************************************
	// Initialize Receive Packet Buffers
	// ***************************************************
	for(i = 0; i < NUM_RX_PKT_BUFS; i++){
		rx_frame_info = (rx_frame_info_t*)RX_PKT_BUF_TO_ADDR(i);
		switch(rx_frame_info->rx_pkt_buf_state){
			case RX_PKT_BUF_UNINITIALIZED:
			case RX_PKT_BUF_LOW_CTRL:
				//CPU_LOW will initialize
			break;
			case RX_PKT_BUF_HIGH_CTRL:
			case RX_PKT_BUF_READY:
	            // CPU HIGH rebooted after CPU Low submitted packet for de-encap/logging
	            //  Release lock and reset state
	            //  Note: this will not cause CPU_LOW to re-lock this packet buffer.
	            //  The effects of this are subtle. CPU_LOW will see that the buffer
	            //  Is under LOW_CTRL and will assume it has a mutex lock. It will
	            //  fill the packet buffer all while the mutex is unlocked. Once the
	            //  state transitions to READY and is passed to CPU_HIGH, this ambiguous
	            //  state will be resolved.
			default:
				// Something went wrong if rx_pkt_buf_state is something
				// other than one of the rx_pkt_buf_state_t enums. We'll
				// attempt to resolve the problem by explicitly setting
				// the state.
				rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_LOW_CTRL;
				unlock_rx_pkt_buf(i);
			break;
		}
	}

	// ***************************************************
	// Initialize CDMA, GPIO, and UART drivers
	// ***************************************************

	// Initialize the central DMA (CDMA) driver
	cdma_cfg_ptr = XAxiCdma_LookupConfig(XPAR_AXI_CDMA_0_DEVICE_ID);
	Status = XAxiCdma_CfgInitialize(&cdma_inst, cdma_cfg_ptr, cdma_cfg_ptr->BaseAddress);
	if (Status != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "ERROR: Could not initialize CDMA: %d\n", Status);
	}
	XAxiCdma_IntrDisable(&cdma_inst, XAXICDMA_XR_IRQ_ALL_MASK);

	// Initialize the GPIO driver instances
	Status = XGpio_Initialize(&Gpio_userio, GPIO_USERIO_DEVICE_ID);

	if (Status != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "ERROR: Could not initialize GPIO\n");
		return;
	}
	// Set direction of GPIO channels
	//  User IO GPIO instance has 1 channel, all inputs
	XGpio_SetDataDirection(&Gpio_userio, GPIO_USERIO_INPUT_CHANNEL, 0xFFFFFFFF);

	// Initialize the UART driver
	Status = XUartLite_Initialize(&UartLite, UARTLITE_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "ERROR: Could not initialize UART\n");
		return;
	}

	// Test to see if DRAM SODIMM is connected to board
	dram_present = 0;
	timestamp = get_system_time_usec();

	while((get_system_time_usec() - timestamp) < 100000){
		if((XGpio_DiscreteRead(&Gpio_userio, GPIO_USERIO_INPUT_CHANNEL) & GPIO_MASK_DRAM_INIT_DONE)) {
			xil_printf("------------------------\nDRAM SODIMM Detected\n");
			if(wlan_mac_high_memory_test()==0){
				dram_present = 1;
			} else {
				dram_present = 0;
			}
			break;
		}
	}

    // Initialize the CPU High DDR linker data
	wlan_mac_high_cpu_ddr_linker_data_init(dram_present);


	// ***************************************************
	// Initialize various subsystems in the MAC High Framework
	// ***************************************************
	queue_init(dram_present);

#if WLAN_SW_CONFIG_ENABLE_LOGGING
	if( dram_present ) {
		// The event_list lives in DRAM immediately following the queue payloads.
		if(MAX_EVENT_LOG == -1){
			log_size = EVENT_LOG_SIZE;
		} else {
			log_size = min(EVENT_LOG_SIZE, MAX_EVENT_LOG );
		}

		event_log_init( (void*)EVENT_LOG_BASE, log_size );

	} else {
		// No DRAM, so the log has nowhere to be stored.
		log_size = 0;
	}
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING

	bss_info_init(dram_present);
	station_info_init(dram_present);
	wlan_eth_init();
	wlan_mac_schedule_init();
#if WLAN_SW_CONFIG_ENABLE_LTG
	wlan_mac_ltg_sched_init();
#endif //WLAN_SW_CONFIG_ENABLE_LTG
	wlan_mac_addr_filter_init();
	wlan_mac_scan_init();

	//Non-blocking request for CPU_LOW to send its state. This handles the case that
	//CPU_HIGH reboots some point after CPU_LOW had already booted.
	wlan_mac_high_request_low_state();

	wlan_mac_high_set_radio_channel(1); // Set a sane default channel. The top-level project (AP/STA/IBSS/etc) is free to change this

	//Create IPC message to receive into
	ipc_msg_from_low.payload_ptr = &(ipc_msg_from_low_payload[0]);
}



/**
 * @brief Initialize MAC High Framework's Interrupts
 *
 * This function initializes sets up the interrupt subsystem
 * of the MAC High Framework.
 *
 * @param None
 * @return int
 *      - nonzero if error
 */
int wlan_mac_high_interrupt_init(){
	int Result;

	// ***************************************************
	// Connect interrupt devices "owned" by wlan_mac_high
	// ***************************************************

	// GPIO for User IO inputs
	Result = XIntc_Connect(&InterruptController, INTC_GPIO_USERIO_INTERRUPT_ID, (XInterruptHandler)wlan_mac_high_userio_gpio_handler, &Gpio_userio);
	if (Result != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "Failed to set up GPIO interrupt\n");
		return Result;
	}
	XIntc_Enable(&InterruptController, INTC_GPIO_USERIO_INTERRUPT_ID);
	XGpio_InterruptEnable(&Gpio_userio, GPIO_USERIO_INPUT_IR_CH_MASK);
	XGpio_InterruptGlobalEnable(&Gpio_userio);

	// UART
	Result = XIntc_Connect(&InterruptController, UARTLITE_INT_IRQ_ID, (XInterruptHandler)XUartLite_InterruptHandler, &UartLite);
	if (Result != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "Failed to set up UART interrupt\n");
		return Result;
	}
	XIntc_Enable(&InterruptController, UARTLITE_INT_IRQ_ID);
	XUartLite_SetRecvHandler(&UartLite, wlan_mac_high_uart_rx_handler, &UartLite);
	XUartLite_EnableInterrupt(&UartLite);

	// ***************************************************
	// Connect interrupt devices in other subsystems
	// ***************************************************
	Result = wlan_mac_schedule_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "Failed to set up scheduler interrupt\n");
		return -1;
	}

	Result = setup_mailbox_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		wlan_printf(PL_ERROR, "Failed to set up wlan_lib mailbox interrupt\n");
		return -1;
	}

	Result = wlan_eth_setup_interrupt(&InterruptController);
	if (Result != XST_SUCCESS) {
		wlan_printf(PL_ERROR,"Failed to set up Ethernet interrupt\n");
		return Result;
	}

	// ***************************************************
	// Enable MicroBlaze exceptions
	// ***************************************************
	Xil_ExceptionInit();

	// NOTE:  Replaces void XIntc_InterruptHandler(XIntc * InstancePtr) to improve execution time
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	                             (Xil_ExceptionHandler) XIntc_DeviceInterruptHandler,
	                             (void *)((u32)(InterruptController.CfgPtr->DeviceId)));

	Xil_ExceptionEnable();


	// Finish setting up any subsystems that were waiting on interrupts to be configured
	bss_info_init_finish();


	return 0;
}



/**
 * @brief Restore the state of the interrupt controller
 *
 * This function starts the interrupt controller, allowing the executing code
 * to be interrupted.
 *
 * @param interrupt_state_t new_interrupt_state
 * 		- State to return interrupts. Typically, this argument is the output of a previous
 * 		call to wlan_mac_high_interrupt_stop()
 * @return int
 *      - nonzero if error
 */
inline int wlan_mac_high_interrupt_restore_state(interrupt_state_t new_interrupt_state){
	interrupt_state = new_interrupt_state;
	if(interrupt_state == INTERRUPTS_ENABLED){
		if(InterruptController.IsReady && InterruptController.IsStarted == 0){
			return XIntc_Start(&InterruptController, XIN_REAL_MODE);
		} else {
			return -1;
		}
	} else {
		return 0;
	}
}



/**
 * @brief Stop the interrupt controller
 *
 * This function stops the interrupt controller, effectively pausing interrupts. This can
 * be used alongside wlan_mac_high_interrupt_start() to wrap code that is not interrupt-safe.
 *
 * @param None
 * @return interrupt_state_t
 * 		- INTERRUPTS_ENABLED if interrupts were enabled at the time this function was called
 * 		- INTERRUPTS_DISABLED if interrupts were disabled at the time this function was called
 *
 * @note Interrupts that occur while the interrupt controller is off will be executed once it is
 * turned back on. They will not be "lost" as the interrupt inputs to the controller will remain
 * high.
 */
inline interrupt_state_t wlan_mac_high_interrupt_stop(){
	interrupt_state_t curr_state = interrupt_state;
	if(InterruptController.IsReady && InterruptController.IsStarted) XIntc_Stop(&InterruptController);
	interrupt_state = INTERRUPTS_DISABLED;
	return curr_state;
}



/**
 * @brief UART Receive Interrupt Handler
 *
 * This function is the interrupt handler for UART receptions. It, in turn,
 * will execute a callback that the user has previously registered.
 *
 * @param void* CallBackRef
 *  - Argument supplied by the XUartLite driver. Unused in this application.
 * @param unsigned int EventData
 *  - Argument supplied by the XUartLite driver. Unused in this application.
 * @return None
 *
 * @see wlan_mac_high_set_uart_rx_callback()
 */
void wlan_mac_high_uart_rx_handler(void* CallBackRef, unsigned int EventData){
#ifdef _ISR_PERF_MON_EN_
	wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
	XUartLite_Recv(&UartLite, uart_rx_buffer, UART_BUFFER_SIZE);
	uart_callback(uart_rx_buffer[0]);
#ifdef _ISR_PERF_MON_EN_
	wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
}

/**
 * @brief User IO GPIO Interrupt Handler
 *
 * Handles GPIO interrupts that occur from the user IO GPIO core's input
 * channel. Depending on the signal, this function will execute
 * one of several different user-provided callbacks.
 *
 * @param void* InstancePtr
 *  - Pointer to the GPIO instance
 * @return None
 *
 * @see wlan_mac_high_set_pb_u_callback()
 * @see wlan_mac_high_set_pb_m_callback()
 * @see wlan_mac_high_set_pb_d_callback()
 *
 */
void wlan_mac_high_userio_gpio_handler(void *InstancePtr){
	XGpio *GpioPtr;
	u32 gpio_read;

#ifdef _ISR_PERF_MON_EN_
	wlan_mac_set_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif

	GpioPtr = (XGpio *)InstancePtr;

	XGpio_InterruptDisable(GpioPtr, GPIO_USERIO_INPUT_IR_CH_MASK);
	gpio_read = XGpio_DiscreteRead(GpioPtr, GPIO_USERIO_INPUT_CHANNEL);

	if(gpio_read & GPIO_MASK_PB_U) pb_u_callback();
	if(gpio_read & GPIO_MASK_PB_M) pb_m_callback();
	if(gpio_read & GPIO_MASK_PB_D) pb_d_callback();

	(void)XGpio_InterruptClear(GpioPtr, GPIO_USERIO_INPUT_IR_CH_MASK);
	XGpio_InterruptEnable(GpioPtr, GPIO_USERIO_INPUT_IR_CH_MASK);

#ifdef _ISR_PERF_MON_EN_
	wlan_mac_clear_dbg_hdr_out(ISR_PERF_MON_GPIO_MASK);
#endif
	return;
}


u32 wlan_mac_high_get_user_io_state(){
	return XGpio_DiscreteRead(&Gpio_userio, GPIO_USERIO_INPUT_CHANNEL);
}


/**
 * @brief Set "Up" Pushbutton Callback
 *
 * Tells the framework which function should be called when
 * the "up" button in the User I/O section of the hardware
 * is pressed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_pb_u_callback(function_ptr_t callback){
	pb_u_callback = callback;
}



/**
 * @brief Set "Middle" Pushbutton Callback
 *
 * Tells the framework which function should be called when
 * the "middle" button in the User I/O section of the hardware
 * is pressed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_pb_m_callback(function_ptr_t callback){
	pb_m_callback = callback;
}



/**
 * @brief Set "Down" Pushbutton Callback
 *
 * Tells the framework which function should be called when
 * the "down" button in the User I/O section of the hardware
 * is pressed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_pb_d_callback(function_ptr_t callback){
	pb_d_callback = callback;
}



/**
 * @brief Set UART Reception Callback
 *
 * Tells the framework which function should be called when
 * a byte is received from UART.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_uart_rx_callback(function_ptr_t callback){
	// xil_printf("assigning uart_callback = 0x%08x\n", (u32)uart_callback);
	uart_callback = callback;
}



/**
 * @brief Set MPDU Transmission Complete Callback (High-level)
 *
 * Tells the framework which function should be called when
 * the lower-level CPU confirms that a high-level MPDU transmission has
 * completed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 * @note This callback is not executed for individual retransmissions.
 * It is instead only executed after a chain of retransmissions is complete
 * either through the reception of an ACK or the number of retransmissions
 * reaching the maximum number of retries specified by the MPDU's
 * tx_frame_info metadata.
 *
 */
void wlan_mac_high_set_mpdu_tx_high_done_callback(function_ptr_t callback){
	mpdu_tx_high_done_callback = callback;
}

/**
 * @brief Set MPDU Transmission Complete Callback (Low-level)
 *
 * Tells the framework which function should be called when
 * the lower-level CPU confirms that a low-level MPDU transmission has
 * completed.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_mpdu_tx_low_done_callback(function_ptr_t callback){
	mpdu_tx_low_done_callback = callback;
}


void wlan_mac_high_set_beacon_tx_done_callback(function_ptr_t callback){
	beacon_tx_done_callback = callback;
}


/**
 * @brief Set MPDU Reception Callback
 *
 * Tells the framework which function should be called when
 * the lower-level CPU receives a valid MPDU frame.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_mpdu_rx_callback(function_ptr_t callback){
	mpdu_rx_callback = callback;
}



/**
 * @brief Set Poll Tx Queue Callback
 *
 * Tells the framework which function should be called whenever
 * the framework knows that CPU_LOW is ready to send a new packet.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_poll_tx_queues_callback(function_ptr_t callback){
	tx_poll_callback = callback;
}


/**
 * @brief Set MPDU Dequeue Callback
 *
 * Tells the framework which function should be called when
 * a packet is dequeued and about to be passed to the
 * lower-level CPU.
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_mpdu_dequeue_callback(function_ptr_t callback){
	mpdu_tx_dequeue_callback = callback;
}


/**
 * @brief Set CPU_LOW Reboot Callback
 *
 * Tells the framework which function should be called when
 * CPU_LOW boots or reboots. This allows a high-level application
 * to restore any parameters it previously set in CPU_LOW
 *
 * @param function_ptr_t callback
 *  - Pointer to callback function
 * @return None
 *
 */
void wlan_mac_high_set_cpu_low_reboot_callback(function_ptr_t callback){
	cpu_low_reboot_callback = callback;
}


/**
 * @brief Display Memory Allocation Information
 *
 * This function is a wrapper around a call to mallinfo(). It prints
 * the information returned by mallinfo() to aid in the debugging of
 * memory leaks and other dynamic memory allocation issues.
 *
 * @param None
 * @return None
 *
 */
void wlan_mac_high_display_mallinfo(){
	struct mallinfo mi;
	mi = mallinfo();

	xil_printf("\n");
	xil_printf("--- Malloc Info ---\n");
	xil_printf("Summary:\n");
	xil_printf("   num_malloc:              %d\n", num_malloc);
	xil_printf("   num_realloc:             %d\n", num_realloc);
	xil_printf("   num_free:                %d\n", num_free);
	xil_printf("   num_malloc-num_free:     %d\n", (int)num_malloc - (int)num_free);
	xil_printf("   System:                  %d bytes\n", mi.arena);
	xil_printf("   Total Allocated Space:   %d bytes\n", mi.uordblks);
	xil_printf("   Total Free Space:        %d bytes\n", mi.fordblks);
#ifdef _DEBUG_
	xil_printf("Details:\n");
	xil_printf("   arena:                   %d\n", mi.arena);
	xil_printf("   ordblks:                 %d\n", mi.ordblks);
	xil_printf("   smblks:                  %d\n", mi.smblks);
	xil_printf("   hblks:                   %d\n", mi.hblks);
	xil_printf("   hblkhd:                  %d\n", mi.hblkhd);
	xil_printf("   usmblks:                 %d\n", mi.usmblks);
	xil_printf("   fsmblks:                 %d\n", mi.fsmblks);
	xil_printf("   uordblks:                %d\n", mi.uordblks);
	xil_printf("   fordblks:                %d\n", mi.fordblks);
	xil_printf("   keepcost:                %d\n", mi.keepcost);
#endif
}



/**
 * @brief Dynamically Allocate Memory
 *
 * This function wraps malloc() and uses its same API.
 *
 * @param u32 size
 *  - Number of bytes that should be allocated
 * @return void*
 *  - Memory address of allocation if the allocation was successful
 *  - NULL if the allocation was unsuccessful
 *
 * @note The purpose of this function is to funnel all memory allocations through one place in
 * code to enable easier debugging of memory leaks when they occur. This function also updates
 * a variable maintained by the framework to track the number of memory allocations and prints
 * this value, along with the other data from wlan_mac_high_display_mallinfo() in the event that
 * malloc() fails to allocate the requested size.
 *
 */
void* wlan_mac_high_malloc(u32 size){
	void* return_value;
	return_value = malloc(size);

	if(return_value == NULL){
		xil_printf("malloc error. Try increasing heap size in linker script.\n");
		wlan_mac_high_display_mallinfo();
	} else {
#ifdef _DEBUG_
		xil_printf("MALLOC - 0x%08x    %d\n", return_value, size);
#endif
		num_malloc++;
	}
	return return_value;
}



/**
 * @brief Dynamically Allocate and Initialize Memory
 *
 * This function wraps wlan_mac_high_malloc() and uses its same API. If successfully allocated,
 * this function will explicitly zero-initialize the allocated memory.
 *
 * @param u32 size
 *  - Number of bytes that should be allocated
 * @return void*
 *  - Memory address of allocation if the allocation was successful
 *  - NULL if the allocation was unsuccessful
 *
 * @see wlan_mac_high_malloc()
 *
 */
void* wlan_mac_high_calloc(u32 size){
	//This is just a simple wrapper around calloc to aid in debugging memory leak issues
	void* return_value;
	return_value = wlan_mac_high_malloc(size);

	if(return_value == NULL){
	} else {
		memset(return_value, 0 , size);
	}
	return return_value;
}



/**
 * @brief Dynamically Reallocate Memory
 *
 * This function wraps realloc() and uses its same API.
 *
 * @param void* addr
 *  - Address of dynamically allocated array that should be reallocated
 * @param u32 size
 *  - Number of bytes that should be allocated
 * @return void*
 *  - Memory address of allocation if the allocation was successful
 *  - NULL if the allocation was unsuccessful
 *
 * @note The purpose of this function is to funnel all memory allocations through one place in
 * code to enable easier debugging of memory leaks when they occur. This function also updates
 * a variable maintained by the framework to track the number of memory allocations and prints
 * this value, along with the other data from wlan_mac_high_display_mallinfo() in the event that
 * realloc() fails to allocate the requested size.
 *
 */
void* wlan_mac_high_realloc(void* addr, u32 size){
	void* return_value;
	return_value = realloc(addr, size);

	if(return_value == NULL){
		xil_printf("realloc error. Try increasing heap size in linker script.\n");
		wlan_mac_high_display_mallinfo();
	} else {
#ifdef _DEBUG_
		xil_printf("REALLOC - 0x%08x    %d\n", return_value, size);
#endif
		num_realloc++;
	}

	return return_value;
}



/**
 * @brief Free Dynamically Allocated Memory
 *
 * This function wraps free() and uses its same API.
 *
 * @param void* addr
 *  - Address of dynamically allocated array that should be freed
 * @return None
 *
 * @note The purpose of this function is to funnel all memory freeing through one place in
 * code to enable easier debugging of memory leaks when they occur. This function also updates
 * a variable maintained by the framework to track the number of memory frees.
 *
 */
void wlan_mac_high_free(void* addr){
#ifdef _DEBUG_
	xil_printf("FREE - 0x%08x\n", addr);
#endif
	free(addr);
	num_free++;
}



/**
 * @brief Test DDR3 SODIMM Memory Module
 *
 * This function tests the integrity of the DDR3 SODIMM module attached to the hardware
 * by performing various write and read tests. Note, this function will destory contents
 * in DRAM, so it should only be called immediately after booting.
 *
 * @param None
 * @return int
 * 	- 0 for memory test pass
 *	- -1 for memory test fail
 */
int wlan_mac_high_memory_test(){

#define READBACK_DELAY_USEC	10000

	volatile u8 i,j;

	volatile u8  test_u8;
	volatile u16 test_u16;
	volatile u32 test_u32;
	volatile u64 test_u64;

	volatile u8  readback_u8;
	volatile u16 readback_u16;
	volatile u32 readback_u32;
	volatile u64 readback_u64;

	volatile void* memory_ptr;

	for(i=0;i<6;i++){
		memory_ptr = (void*)((u8*)DRAM_BASE + (i*100000*1024));
		for(j=0;j<3;j++){
			//Test 1 byte offsets to make sure byte enables are all working
			test_u8 = rand()&0xFF;
			test_u16 = rand()&0xFFFF;
			test_u32 = rand()&0xFFFFFFFF;
			test_u64 = (((u64)rand()&0xFFFFFFFF)<<32) + ((u64)rand()&0xFFFFFFFF);

			*((u8*)memory_ptr) = test_u8;
			usleep(READBACK_DELAY_USEC);
			readback_u8 = *((u8*)memory_ptr);

			if(readback_u8!= test_u8){
				xil_printf("0x%08x: %2x = %2x\n", memory_ptr, readback_u8, test_u8);
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u8\n",memory_ptr);
				return -1;
			}
			*((u16*)memory_ptr) = test_u16;
			usleep(READBACK_DELAY_USEC);
			readback_u16 = *((u16*)memory_ptr);

			if(readback_u16 != test_u16){
				xil_printf("0x%08x: %4x = %4x\n", memory_ptr, readback_u16, test_u16);
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u16\n",memory_ptr);
				return -1;
			}
			*((u32*)memory_ptr) = test_u32;
			usleep(READBACK_DELAY_USEC);
			readback_u32 = *((u32*)memory_ptr);

			if(readback_u32 != test_u32){
				xil_printf("0x%08x: %8x = %8x\n", memory_ptr, readback_u32, test_u32);
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u32\n",memory_ptr);
				return -1;
			}
			*((u64*)memory_ptr) = test_u64;
			usleep(READBACK_DELAY_USEC);
			readback_u64 = *((u64*)memory_ptr);

			if(readback_u64!= test_u64){
				xil_printf("DRAM Failure: Addr: 0x%08x -- Unable to verify write of u64\n",memory_ptr);
				return -1;
			}
			memory_ptr++;
		}
	}
	return 0;
}



/**
 * @brief Test Right Shift Operator
 *
 * This function tests the compiler right shift operator.  This is due to a bug in
 * the Xilinx 14.7 toolchain when the '-Os' flag is used during compilation.  Please
 * see:  http://warpproject.org/forums/viewtopic.php?id=2472 for more information.
 *
 * @param None
 * @return int
 * 	-  0 for right shift test pass
 *	- -1 for right shift test fail
 */
u32 right_shift_test = 0xFEDCBA98;

int wlan_mac_high_right_shift_test(){
    u8 val_3, val_2, val_1, val_0;

    u32 test_val   = right_shift_test;
    u8 *test_array = (u8 *)&right_shift_test;

    val_3 = (u8)((test_val & 0xFF000000) >> 24);
    val_2 = (u8)((test_val & 0x00FF0000) >> 16);
    val_1 = (u8)((test_val & 0x0000FF00) >>  8);
    val_0 = (u8)((test_val & 0x000000FF) >>  0);

    if ((val_3 != test_array[3]) || (val_2 != test_array[2]) || (val_1 != test_array[1]) || (val_0 != test_array[0])) {
	    xil_printf("Right shift operator is not operating correctly in this toolchain.\n");
	    xil_printf("Please use Xilinx 14.4 or an optimization level other than '-Os'\n");
        xil_printf("See http://warpproject.org/forums/viewtopic.php?id=2472 for more info.\n");
        return -1;
    }

	return 0;
}



/**
 * @brief Start Central DMA Transfer
 *
 * This function wraps the XAxiCdma call for a CDMA memory transfer and mimics the well-known
 * API of memcpy(). This function does not block once the transfer is started.
 *
 * @param void* dest
 *  - Pointer to destination address where bytes should be copied
 * @param void* src
 *  - Pointer to source address from where bytes should be copied
 * @param u32 size
 *  - Number of bytes that should be copied
 * @return int
 *	- XST_SUCCESS for success of submission
 *	- XST_FAILURE for submission failure
 *	- XST_INVALID_PARAM if:
 *	 Length out of valid range [1:8M]
 *	 Or, address not aligned when DRE is not built in
 *
 *	 @note This function will block until any existing CDMA transfer is complete. It is therefore
 *	 safe to call this function successively as each call will wait on the preceeding call.
 *
 */
int wlan_mac_high_cdma_start_transfer(void* dest, void* src, u32 size){
	//This is a wrapper function around the central DMA simple transfer call. It's arguments
	//are intended to be similar to memcpy. Note: This function does not block on the transfer.
	int return_value = XST_SUCCESS;
	u8 out_of_range  = 0;


	if((u32)src > XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_BASEADDR && (u32)src < XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_HIGHADDR){
		out_of_range = 1;
	} else if((u32)src > XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_BASEADDR && (u32)src < XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_HIGHADDR){
		out_of_range = 1;
	} else if((u32)dest > XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_BASEADDR && (u32)dest < XPAR_MB_HIGH_DLMB_BRAM_CNTLR_0_HIGHADDR){
		out_of_range = 1;
	} else if((u32)dest > XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_BASEADDR && (u32)dest < XPAR_MB_HIGH_DLMB_BRAM_CNTLR_1_HIGHADDR){
		out_of_range = 1;
	}

	if( size == 0 ){
		xil_printf("CDMA Error: size argument must be > 0\n");
		return XST_FAILURE;
	}

	if(out_of_range == 0){
		wlan_mac_high_cdma_finish_transfer();
		return_value = XAxiCdma_SimpleTransfer(&cdma_inst, (u32)src, (u32)dest, size, NULL, NULL);

		if(return_value != 0){
			xil_printf("CDMA Error: code %d, (0x%08x,0x%08x,%d)\n", return_value, dest,src,size);
		}
	} else {
		xil_printf("CDMA Error: source and destination addresses must not located in the DLMB. Using memcpy instead. memcpy(0x%08x,0x%08x,%d)\n",dest,src,size);
		memcpy(dest,src,size);
	}

	return return_value;
}



/**
 * @brief Finish Central DMA Transfer
 *
 * This function will block until an ongoing CDMA transfer is complete.
 * If there is no CDMA transfer underway when this function is called, it
 * returns immediately.
 *
 * @param None
 * @return None
 *
 */
void wlan_mac_high_cdma_finish_transfer(){
	while(XAxiCdma_IsBusy(&cdma_inst)) {}
	return;
}



/**
 * @brief Transmit MPDU
 *
 * This function passes off an MPDU to the lower-level processor for transmission.
 *
 * @param tx_queue_entry_t* packet
 *  - Pointer to the packet that should be transmitted
 * @return None
 *
 */
void wlan_mac_high_mpdu_transmit(tx_queue_element_t* packet, int tx_pkt_buf) {
	wlan_ipc_msg_t 		ipc_msg_to_low;
	tx_frame_info_t* 	tx_frame_info;
	station_info_t* 	station_info;
	void* 				dest_addr;
	void* 				src_addr;
	u32 				xfer_len;


	tx_frame_info   = (tx_frame_info_t*) TX_PKT_BUF_TO_ADDR(tx_pkt_buf);

    // Call user code to notify it of dequeue
	mpdu_tx_dequeue_callback(packet);

    // Set local variables
    //     NOTE:  This must be done after the mpdu_tx_dequeue_callback since that call can
    //         modify the packet contents.
	dest_addr = (void*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
	src_addr  = (void*) (&(((tx_queue_buffer_t*)(packet->data))->tx_frame_info));
	xfer_len  = ((tx_queue_buffer_t*)(packet->data))->tx_frame_info.length + sizeof(tx_frame_info_t) + PHY_TX_PKT_BUF_PHY_HDR_SIZE - WLAN_PHY_FCS_NBYTES;

	// Transfer the frame info
	wlan_mac_high_cdma_start_transfer( dest_addr, src_addr, xfer_len);

	// Wait for transfer to finish
	wlan_mac_high_cdma_finish_transfer();

	// Unique_seq will be filled in by CPU_LOW
	tx_frame_info->unique_seq = 0;

	switch(((tx_queue_buffer_t*)(packet->data))->metadata.metadata_type){
	    case QUEUE_METADATA_TYPE_IGNORE:
		break;

		case QUEUE_METADATA_TYPE_STATION_INFO:
			station_info = (station_info_t*)(((tx_queue_buffer_t*)(packet->data))->metadata.metadata_ptr);

			//
			// NOTE: this would be a good place to add code to handle the automatic adjustment of transmission properties like rate
			//

			memcpy(&(tx_frame_info->params), &(station_info->tx), sizeof(tx_params_t));
		break;

		case QUEUE_METADATA_TYPE_TX_PARAMS:
			memcpy(&(tx_frame_info->params), (void*)(((tx_queue_buffer_t*)(packet->data))->metadata.metadata_ptr), sizeof(tx_params_t));
		break;
	}

	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_TX_MPDU_READY);
	ipc_msg_to_low.arg0              = tx_pkt_buf;
	ipc_msg_to_low.num_payload_words = 0;

	// Set the packet buffer state to READY
	tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_READY;

	if(unlock_tx_pkt_buf(tx_pkt_buf) == PKT_BUF_MUTEX_FAIL_NOT_LOCK_OWNER){
		// The unlock failed because CPU_LOW currently has the mutex lock. We will not
		// submit a READY message for this packet. Instead, we'll drop it and revert
		// the state of the packet buffer to TX_PKT_BUF_HIGH_CTRL. CPU_LOW will unlock the
		// packet buffer when it is done transmitting from it or if it reboots. At that
		// point, we will clean up and be able to use this packet buffer again for future
		// transmissions.
		wlan_printf(PL_ERROR, "Error: unable to unlock tx pkt_buf %d\n",tx_pkt_buf);
		tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
	} else {
		// We successfully unlocked the packet buffer or we failed to unlock it because
		// it was already unlocked. In either case, we can submit this READY message.
		write_mailbox_msg(&ipc_msg_to_low);
	}
}

/**
 * @brief Check Validity of Tagged Rate
 *
 * This function checks the validity of a given rate from a tagged field in a management frame.
 *
 * @param u8 rate
 *     - Tagged rate
 * @return u8
 *     - 1 if valid
 *     - 0 if invalid
 *
 *  @note This function checks against the 12 possible valid rates sent in 802.11b/a/g.
 *  The faster 802.11n rates will return as invalid when this function is used.
 *
 */
u8 wlan_mac_high_valid_tagged_rate(u8 rate){
	u32 i;
	u8 valid_rates[NUM_VALID_RATES] = {0x02, 0x04, 0x0b, 0x16, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c};

	for(i = 0; i < NUM_VALID_RATES; i++ ){
		if((rate & ~RATE_BASIC) == valid_rates[i]) return 1;
	}

	return 0;
}



/**
 * @brief Convert Tagged Rate to Human-Readable String (in Mbps)
 *
 * This function takes a tagged rate as an input and fills in a provided
 * string with the rate in Mbps.
 *
 * @param u8 rate
 *  - Tagged rate
 * @param char* str
 *  - Empty string that will be filled in by this function
 * @return u8
 *  - 1 if valid
 *  - 0 if invalid
 *
 *  @note The str argument must have room for 4 bytes at most ("5.5" followed by NULL)
 *
 */
void wlan_mac_high_tagged_rate_to_readable_rate(u8 rate, char* str){

	switch(rate & ~RATE_BASIC){
		case 0x02:  strcpy(str,"1");    break;
		case 0x04:  strcpy(str,"2");    break;
		case 0x0b:  strcpy(str,"5.5");  break;
		case 0x16:  strcpy(str,"11");   break;
		case 0x0c:  strcpy(str,"6");    break;
		case 0x12:  strcpy(str,"9");    break;
		case 0x18:  strcpy(str,"12");   break;
		case 0x24:  strcpy(str,"18");   break;
		case 0x30:  strcpy(str,"24");   break;
		case 0x48:  strcpy(str,"36");   break;
		case 0x60:  strcpy(str,"48");   break;
		case 0x6c:  strcpy(str,"54");   break;

		default:    // Unknown rate
			*str = NULL;
		break;
	}
}



/**
 * @brief Set up the 802.11 Header
 *
 * @param  mac_header_80211_common * header
 *     - Pointer to the 802.11 header
 * @param  u8 * addr_1
 *     - Address 1 of the packet header
 * @param  u8 * addr_3
 *     - Address 3 of the packet header
 * @return None
 */
void wlan_mac_high_setup_tx_header( mac_header_80211_common * header, u8 * addr_1, u8 * addr_3 ) {
	// Set up Addresses in common header
	header->address_1 = addr_1;
    header->address_3 = addr_3;
}



/**
 * @brief Set up the 802.11 Header
 *
 * @param  mac_header_80211_common * header
 *     - Pointer to the 802.11 header
 * @param  tx_queue_element_t * curr_tx_queue_element
 *     - Pointer to the TX queue element
 * @param  u32 tx_length
 *     - Length of the frame info
 * @param  tx_frame_info_flags_bf flags
 *     - Flags for the frame info
 * @param  u8 QID
 *     - Queue ID
 * @return None
 */
void wlan_mac_high_setup_tx_frame_info(mac_header_80211_common * header, tx_queue_element_t * curr_tx_queue_element, u32 tx_length, u8 flags, u8 QID) {

	u16 occupancy;

	tx_queue_buffer_t* curr_tx_queue_buffer = ((tx_queue_buffer_t*)(curr_tx_queue_element->data));

    // Get occupancy that was filled in during the enqueue_after_tail() call
	occupancy = curr_tx_queue_buffer->tx_frame_info.queue_info.occupancy;

	bzero(&(curr_tx_queue_buffer->tx_frame_info), sizeof(tx_frame_info_t));

	// Set up frame info data
	curr_tx_queue_buffer->tx_frame_info.timestamp_create            = get_mac_time_usec();
	curr_tx_queue_buffer->tx_frame_info.length                      = tx_length;
	curr_tx_queue_buffer->tx_frame_info.flags                       = flags;
	curr_tx_queue_buffer->tx_frame_info.queue_info.QID              = QID;
	curr_tx_queue_buffer->tx_frame_info.queue_info.occupancy        = occupancy;

}



/**
 * @brief WLAN MAC IPC receive
 *
 * IPC receive function that will poll the mailbox for as many messages as are
 * available and then call the CPU high IPC processing function on each message
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_ipc_rx(){

#ifdef _DEBUG_
	u32 numMsg = 0;
	xil_printf("Mailbox Rx:  ");
#endif

	while (read_mailbox_msg(&ipc_msg_from_low) == IPC_MBOX_SUCCESS) {
		wlan_mac_high_process_ipc_msg(&ipc_msg_from_low);

#ifdef _DEBUG_
		numMsg++;
#endif
	}

#ifdef _DEBUG_
	xil_printf("Processed %d msg in one ISR\n", numMsg);
#endif
}



/**
 * @brief WLAN MAC IPC processing function for CPU High
 *
 * Process IPC message from CPU low
 *
 * @param  wlan_ipc_msg* msg
 *     - Pointer to the IPC message
 * @return None
 */
void wlan_mac_high_process_ipc_msg(wlan_ipc_msg_t * msg) {

	u8                  		rx_pkt_buf;
	u8							tx_pkt_buf;
	rx_frame_info_t*    		rx_frame_info;
	tx_frame_info_t*    		tx_frame_info;

    // Determine what type of message this is
	switch(IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id)) {

		//---------------------------------------------------------------------
		case IPC_MBOX_TX_BEACON_DONE:{
			wlan_mac_low_tx_details_t* 	tx_low_details;
			tx_low_entry*				tx_low_event_log_entry = NULL;

			tx_pkt_buf = msg->arg0;
			if(tx_pkt_buf == TX_PKT_BUF_BEACON){
				if(lock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
					xil_printf("Error: CPU_LOW had lock on Beacon packet buffer during IPC_MBOX_TX_BEACON_DONE\n");
				} else {
					tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
					tx_low_details = (wlan_mac_low_tx_details_t*)(msg->payload_ptr);

#if WLAN_SW_CONFIG_ENABLE_LOGGING
					// Log the TX low
					tx_low_event_log_entry = wlan_exp_log_create_tx_low_entry(tx_frame_info, tx_low_details, 0);
#endif

					beacon_tx_done_callback( tx_frame_info, tx_low_details, tx_low_event_log_entry );




					tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_READY;
					if(unlock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
						xil_printf("Error: Unable to unlock Beacon packet buffer during IPC_MBOX_TX_BEACON_DONE\n");
						return;
					}
				}
			} else {
				xil_printf("Error: IPC_MBOX_TX_BEACON_DONE with invalid pkt buf index %d\n ", tx_pkt_buf);
			}
		}
		break;

		//---------------------------------------------------------------------
		case IPC_MBOX_RX_MPDU_READY: {
			// CPU Low has received an MPDU addressed to this node or to the broadcast address
			//

			station_info_t* 		station_info;
			u32						mpdu_rx_process_flags;
			rx_common_entry* 		rx_event_log_entry = NULL;

			rx_pkt_buf = msg->arg0;
			if(rx_pkt_buf < NUM_RX_PKT_BUFS){
				rx_frame_info = (rx_frame_info_t*)RX_PKT_BUF_TO_ADDR(rx_pkt_buf);

				switch(rx_frame_info->rx_pkt_buf_state){
				   case RX_PKT_BUF_READY:
					   // Normal Rx process - buffer contains packet ready for de-encap and logging
					   // Attempt to lock the indicated Rx pkt buf (CPU Low must unlock it before sending this msg)
						if(lock_rx_pkt_buf(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
							wlan_printf(PL_ERROR, "Error: unable to lock pkt_buf %d\n", rx_pkt_buf);
						} else {

							rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_HIGH_CTRL;

							//Before calling the user's callback, we'll pass this reception off to the BSS info subsystem so it can scrape for BSS metadata
							bss_info_rx_process((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)));

							//We will also pass this reception off to the Station Info subsystem
							station_info = station_info_rx_process((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)));

#if WLAN_SW_CONFIG_ENABLE_LOGGING
							//Log this RX event
							rx_event_log_entry = wlan_exp_log_create_rx_entry(rx_frame_info);
#endif

							// Call the RX callback function to process the received packet
							mpdu_rx_process_flags = mpdu_rx_callback((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), station_info, rx_event_log_entry);

#if	WLAN_SW_CONFIG_ENABLE_TXRX_COUNTS
							if( (mpdu_rx_process_flags & MAC_RX_CALLBACK_RETURN_FLAG_NO_COUNTS) == 0 ){
								if(mpdu_rx_process_flags & MAC_RX_CALLBACK_RETURN_FLAG_DUP){
									station_info_rx_process_counts((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), station_info, RX_PROCESS_COUNTS_OPTION_FLAG_IS_DUPLICATE);
								} else {
									station_info_rx_process_counts((void*)(RX_PKT_BUF_TO_ADDR(rx_pkt_buf)), station_info, 0);
								}
							}
#endif
							// Free up the rx_pkt_buf
							rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_LOW_CTRL;

							if(unlock_rx_pkt_buf(rx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
								wlan_printf(PL_ERROR, "Error: unable to unlock rx pkt_buf %d\n", rx_pkt_buf);
							}
						}
				   break;
				   default:
				   case RX_PKT_BUF_HIGH_CTRL:
					   //  Don't de-encap - just clean up and return
					   rx_frame_info->rx_pkt_buf_state = RX_PKT_BUF_LOW_CTRL;
				   case RX_PKT_BUF_UNINITIALIZED:
				   case RX_PKT_BUF_LOW_CTRL:
					   if(unlock_rx_pkt_buf(rx_pkt_buf) == PKT_BUF_MUTEX_SUCCESS){
							wlan_printf(PL_ERROR, "Error: state mismatch; CPU_HIGH owned the lock on rx pkt_buf %d\n", rx_pkt_buf);
					   }
				   break;
				}
			} else {
				xil_printf("Error: IPC_MBOX_RX_MPDU_READY with invalid pkt buf index %d\n ", rx_pkt_buf);
			}
		} break;

		//---------------------------------------------------------------------
		case IPC_MBOX_TX_MPDU_DONE: {
			// CPU Low has finished the Tx process for the previously submitted-accepted frame
			//     CPU High should do any necessary post-processing, then recycle the packet buffer
            //

		    u32                 		num_tx_low_details, i;
		    wlan_mac_low_tx_details_t* 	tx_low_details;
		    tx_high_entry*				tx_high_event_log_entry = NULL;
		    tx_low_entry*				tx_low_event_log_entry = NULL;

			tx_pkt_buf = msg->arg0;
			if(tx_pkt_buf < NUM_TX_PKT_BUFS){
				tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(tx_pkt_buf);
				switch(tx_frame_info->tx_pkt_buf_state){
					case TX_PKT_BUF_DONE:
						// Expected state after CPU Low finishes Tx
						//  Run normal post-tx processing
						// Lock this packet buffer
						if(lock_tx_pkt_buf(tx_pkt_buf) != PKT_BUF_MUTEX_SUCCESS){
							xil_printf("Error: DONE Lock Tx Pkt Buf State Mismatch\n");
							tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
							return;
						}

						//We can now attempt to dequeue any pending transmissions before we fully process
						//this done message.
						tx_poll_callback();

						//We will pass this completed transmission off to the Station Info subsystem
						station_info_tx_process((void*)(TX_PKT_BUF_TO_ADDR(tx_pkt_buf)));

						num_tx_low_details  = (4*(msg->num_payload_words)) / sizeof(wlan_mac_low_tx_details_t);
						tx_low_details = (wlan_mac_low_tx_details_t*)(msg->payload_ptr);

						// Log the low-level transmissions and call the application callback for each one
						//  Note: eventually, this step will be separated from IPC_MBOX_TX_MPDU_DONE
						for(i = 0; i < num_tx_low_details; i++) {
							// Log the TX low
#if WLAN_SW_CONFIG_ENABLE_LOGGING
							tx_low_event_log_entry = wlan_exp_log_create_tx_low_entry(tx_frame_info, &tx_low_details[i], i);
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
							mpdu_tx_low_done_callback(tx_frame_info, &(tx_low_details[i]), tx_low_event_log_entry);
						}

#if WLAN_SW_CONFIG_ENABLE_LOGGING
						// Log the high-level transmission and call the application callback
						tx_high_event_log_entry = wlan_exp_log_create_tx_high_entry(tx_frame_info);
#endif //WLAN_SW_CONFIG_ENABLE_LOGGING
						mpdu_tx_high_done_callback(tx_frame_info, tx_high_event_log_entry);

						tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
					break;
					// Something has gone wrong - TX_DONE message disagrees
					//  with state of Tx pkt buf
					case TX_PKT_BUF_UNINITIALIZED:
					case TX_PKT_BUF_HIGH_CTRL:
						// CPU High probably rebooted, initialized Tx pkt buffers
						//  then got TX_DONE message from pre-reboot
						// Ignore the contents, force-lock the buffer, and
						//  leave it TX_PKT_BUF_HIGH_CTRL, will be used by future ping-pong rotation
						force_lock_tx_pkt_buf(tx_pkt_buf);
						tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_HIGH_CTRL;
					case TX_PKT_BUF_READY:
					case TX_PKT_BUF_LOW_CTRL:
						//CPU Low will clean up
						// Unlikely CPU High holds lock, but unlock just in case
						unlock_tx_pkt_buf(tx_pkt_buf);
					break;
				}
			} else {
				xil_printf("Error: IPC_MBOX_TX_MPDU_DONE with invalid pkt buf index %d\n ", tx_pkt_buf);
			}


		} break;


		//---------------------------------------------------------------------
		case IPC_MBOX_CPU_STATUS:
			// CPU low's status
			//

			// cpu_low_status isn't needed to process this IPC message since there is an explicit
			// reason provided in the arg0 field of the message. However, we'll copy the information
			// into the global in case any future process wants record of it.
			cpu_low_status = ipc_msg_from_low_payload[0];

			switch( msg->arg0 ){
				case CPU_STATUS_REASON_EXCEPTION:
					wlan_printf(PL_ERROR, "ERROR:  An unrecoverable exception has occurred in CPU_LOW, halting...\n");
					wlan_printf(PL_ERROR, "    Reason code: %d\n", ipc_msg_from_low_payload[1]);
					cpu_error_halt(WLAN_ERROR_CPU_STOP);
				break;

				case CPU_STATUS_REASON_BOOTED:
					// Set any of low parameters that have been modified and
					//  the MAC High Framework is responsible for tracking
					if(low_param_channel != 0xFFFFFFFF)		wlan_mac_high_set_radio_channel(low_param_channel);
					if(low_param_dsss_en != 0xFFFFFFFF)		wlan_mac_high_set_dsss(low_param_dsss_en);
					if(low_param_rx_ant_mode != 0xFF) 		wlan_mac_high_set_rx_ant_mode(low_param_rx_ant_mode);
					if(low_param_tx_ctrl_pow != -1) 		wlan_mac_high_set_tx_ctrl_pow(low_param_tx_ctrl_pow);
					if(low_param_rx_filter != 0xFFFFFFFF) 	wlan_mac_high_set_rx_filter_mode(low_param_rx_filter);
					if(low_param_random_seed != 0xFFFFFFFF) wlan_mac_high_set_srand(low_param_random_seed);

					// Notify the high-level project that CPU_LOW has rebooted
					cpu_low_reboot_callback();
				case CPU_STATUS_REASON_RESPONSE:
					// Set the CPU_LOW wlan_exp type
#if WLAN_SW_CONFIG_ENABLE_WLAN_EXP
					wlan_exp_node_set_type_low(ipc_msg_from_low_payload[1]);
#endif
				default:

				break;
			}
		break;


		//---------------------------------------------------------------------
		case IPC_MBOX_MEM_READ_WRITE:
			// Memory Read / Write message
			//   - Allows CPU High to read / write arbitrary memory locations in CPU low
			//
			if(cpu_low_reg_read_buffer != NULL){
				memcpy( (u8*)cpu_low_reg_read_buffer, (u8*)ipc_msg_from_low_payload, (msg->num_payload_words) * sizeof(u32));
				cpu_low_reg_read_buffer_status = CPU_LOW_REG_READ_BUFFER_STATUS_READY;

			} else {
				wlan_printf(PL_ERROR, "ERROR: Received low-level register buffer from CPU_LOW and was not expecting it.\n");
			}
		break;


        //---------------------------------------------------------------------
        case IPC_MBOX_LOW_PARAM:
            // CPU Low Parameter IPC message
            //   - Processes any CPU Low Parameter IPC messages sent to CPU High
            //   - This is an error condition.  CPU High should never expect this IPC message
            //
            // NOTE:  This is due to the fact that IPC messages in CPU low can take an infinitely long amount of
            //     to return given that the sending and receiving of wireless data takes precedent.  Therefore,
            //     it is not good to try to return values from CPU low since there is no guarantee when the values
            //     will be available.
            //
            wlan_printf(PL_ERROR, "ERROR: Received low-level parameter buffer from CPU_LOW and was not expecting it.\n");
        break;


		//---------------------------------------------------------------------
		default:
			wlan_printf(PL_ERROR, "ERROR: Unknown IPC message type %d\n", IPC_MBOX_MSG_ID_TO_MSG(msg->msg_id));
		break;
	}
}



/**
 * @brief Set Random Seed
 *
 * Send an IPC message to CPU Low to set the Random Seed
 *
 * @param  u32 seed
 *     - Random number generator seed
 * @return None
 */
void wlan_mac_high_set_srand(u32 seed) {

	wlan_ipc_msg_t     ipc_msg_to_low;
	u32                ipc_msg_to_low_payload = seed;

	//Set tracking global
	low_param_random_seed = seed;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_LOW_RANDOM_SEED);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload);

	write_mailbox_msg(&ipc_msg_to_low);
}


/**
 * @brief Convert BSS Channel Specification to Radio Channel
 *
 * Converts a BSS channel specification to a radio channel
 * for use in wlan_mac_high_set_radio_channel. When extended
 * to support HT40, this function will grow more complex.
 *
 * @param  chan_spec_t* chan_spec
 *     - Pointer to BSS Channel Specification
 * @return None
 */
u8 wlan_mac_high_bss_channel_spec_to_radio_chan(chan_spec_t chan_spec) {
	return chan_spec.chan_pri;
}

/**
 * @brief Set Radio Channel
 *
 * Send an IPC message to CPU Low to set the MAC Channel
 *
 * @param  u32 mac_channel
 *     - 802.11 Channel to set
 * @return None
 */
void wlan_mac_high_set_radio_channel(u32 mac_channel) {

	wlan_ipc_msg_t     ipc_msg_to_low;
	u32                ipc_msg_to_low_payload = mac_channel;

	if(wlan_verify_channel(mac_channel) == XST_SUCCESS){
		//Set tracking global
		low_param_channel = mac_channel;

		// Send message to CPU Low
		ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_CHANNEL);
		ipc_msg_to_low.num_payload_words = 1;
		ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload);

		write_mailbox_msg(&ipc_msg_to_low);
	} else {
		xil_printf("Channel %d not allowed\n", mac_channel);
	}
}

void wlan_mac_high_config_txrx_beacon(beacon_txrx_configure_t* beacon_txrx_configure){
	wlan_ipc_msg_t ipc_msg_to_low;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_TXRX_BEACON_CONFIGURE);
	ipc_msg_to_low.num_payload_words = sizeof(beacon_txrx_configure_t)/sizeof(u32);
	ipc_msg_to_low.payload_ptr       = (u32*)beacon_txrx_configure;

	write_mailbox_msg(&ipc_msg_to_low);

}

/**
 * @brief Set Rx Antenna Mode
 *
 * Send an IPC message to CPU Low to set the Rx antenna mode.
 *
 * @param  u8 ant_mode
 *     - Antenna mode selection
 * @return None
 */
void wlan_mac_high_set_rx_ant_mode(u8 ant_mode) {
	wlan_ipc_msg_t     ipc_msg_to_low;
	u32                ipc_msg_to_low_payload = (u32)ant_mode;

	//Sanity check input
	switch(ant_mode){
		case RX_ANTMODE_SISO_ANTA:
		case RX_ANTMODE_SISO_ANTB:
		case RX_ANTMODE_SISO_ANTC:
		case RX_ANTMODE_SISO_ANTD:
		case RX_ANTMODE_SISO_SELDIV_2ANT:
			//Set tracking global
			low_param_rx_ant_mode = ant_mode;
		break;
		default:
			xil_printf("Error: unsupported antenna mode %x\n", ant_mode);
			return;
		break;
	}

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RX_ANT_MODE);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload);

	write_mailbox_msg(&ipc_msg_to_low);
}



/**
 * @brief Set Tx Control Packet Power
 *
 * Send an IPC message to CPU Low to set the Tx control packet power
 *
 * @param  s8 pow
 *     - Tx control packet power
 * @return None
 */
void wlan_mac_high_set_tx_ctrl_pow(s8 pow) {

	wlan_ipc_msg_t     ipc_msg_to_low;
	u32                ipc_msg_to_low_payload = (u32)pow;

	//Set tracking global
	low_param_tx_ctrl_pow = pow;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_TX_CTRL_POW);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload);

	write_mailbox_msg(&ipc_msg_to_low);
}



/**
 * @brief Set Rx Filter
 *
 * Send an IPC message to CPU Low to set the filter for receptions. This will
 * allow or disallow different packets from being passed up to CPU_High
 *
 * @param    filter_mode
 *              - RX_FILTER_FCS_GOOD
 *              - RX_FILTER_FCS_ALL
 *              - RX_FILTER_ADDR_STANDARD   (unicast to me or multicast)
 *              - RX_FILTER_ADDR_ALL_MPDU   (all MPDU frames to any address)
 *              - RX_FILTER_ADDR_ALL        (all observed frames, including control)
 *
 * @note    FCS and ADDR filter selections must be bit-wise ORed together. For example,
 *     wlan_mac_high_set_rx_filter_mode(RX_FILTER_FCS_ALL | RX_FILTER_ADDR_ALL)
 *
 * @return  None
 */
void wlan_mac_high_set_rx_filter_mode(u32 filter_mode) {

	wlan_ipc_msg_t     ipc_msg_to_low;
	u32                ipc_msg_to_low_payload = (u32)filter_mode;

	//Set tracking global
	low_param_rx_filter = filter_mode;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_RX_FILTER);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload);

	write_mailbox_msg(&ipc_msg_to_low);
}



/**
 * @brief Write a memory location in CPU low
 *
 * Send an IPC message to CPU Low to write the given data
 *
 * @param   u32 num_words    - Number of words in the message payload
 * @param   u32 * payload    - Pointer to the message payload
 *
 * @return  int              - Status of command:  0 = Success; -1 = Failure
 */
int wlan_mac_high_write_low_mem(u32 num_words, u32* payload) {
	wlan_ipc_msg_t ipc_msg_to_low;

	if (num_words > MAILBOX_BUFFER_MAX_NUM_WORDS) {
	    return -1;
	}

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_MEM_READ_WRITE);
	ipc_msg_to_low.num_payload_words = num_words;
	ipc_msg_to_low.arg0              = IPC_REG_WRITE_MODE;
	ipc_msg_to_low.payload_ptr       = payload;

	write_mailbox_msg(&ipc_msg_to_low);

	return 0;
}



/**
 * @brief Read a memory location in CPU low
 *
 * Send an IPC message to CPU Low to read the given data
 *
 * @param   u32 num_words    - Number of words to read from CPU low
 * @param   u32 baseaddr     - Base address of the data to read from CPU low
 * @param   u32 * payload    - Pointer to the buffer to be populated with data
 *
 * @return  int              - Status of command:  0 = Success; -1 = Failure
 */
int wlan_mac_high_read_low_mem(u32 num_words, u32 baseaddr, u32* payload) {

    u64                  start_timestamp;
    wlan_ipc_msg_t       ipc_msg_to_low;
    ipc_reg_read_write_t ipc_msg_to_low_payload;

    if(InterruptController.IsStarted == XIL_COMPONENT_IS_STARTED){
        // Send message to CPU Low
        ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_MEM_READ_WRITE);
        ipc_msg_to_low.num_payload_words = sizeof(ipc_reg_read_write_t) / sizeof(u32);
        ipc_msg_to_low.arg0              = IPC_REG_READ_MODE;
        ipc_msg_to_low.payload_ptr       = (u32*)(&(ipc_msg_to_low_payload));

        ipc_msg_to_low_payload.baseaddr  = baseaddr;
        ipc_msg_to_low_payload.num_words = num_words;

        // Set the read buffer to the payload pointer
        cpu_low_reg_read_buffer          = payload;
        cpu_low_reg_read_buffer_status   = CPU_LOW_REG_READ_BUFFER_STATUS_NOT_READY;

        write_mailbox_msg(&ipc_msg_to_low);

        // Get start time
        start_timestamp = get_system_time_usec();

        // Wait for CPU low to finish the read or timeout to occur
        while(cpu_low_reg_read_buffer_status != CPU_LOW_REG_READ_BUFFER_STATUS_READY){
            if ((get_system_time_usec() - start_timestamp) > WLAN_EXP_CPU_LOW_DATA_REQ_TIMEOUT) {
                xil_printf("Error: Reading CPU_LOW memory timed out\n");

                // Reset the read buffer
                cpu_low_reg_read_buffer  = NULL;

                return -1;
            }
        }

        // Reset the read buffer
        cpu_low_reg_read_buffer          = NULL;

    } else {
        xil_printf("Error: Reading CPU_LOW memory requires interrupts being enabled\n");
        return -1;
    }

    return 0;
}



/**
 * @brief Write a parameter in CPU low
 *
 * Send an IPC message to CPU Low to write the given parameter
 *
 * @param   u32 num_words    - Number of words in the message payload
 * @param   u32 * payload    - Pointer to the message payload (includes parameter ID)
 *
 * @return  int              - Status of command:  0 = Success; -1 = Failure
 */
int wlan_mac_high_write_low_param(u32 num_words, u32* payload) {
    wlan_ipc_msg_t ipc_msg_to_low;

    if (num_words > MAILBOX_BUFFER_MAX_NUM_WORDS) {
        return -1;
    }

    // Send message to CPU Low
    ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_LOW_PARAM);
    ipc_msg_to_low.num_payload_words = num_words;
    ipc_msg_to_low.arg0              = IPC_REG_WRITE_MODE;
    ipc_msg_to_low.payload_ptr       = payload;

    write_mailbox_msg(&ipc_msg_to_low);

    return 0;
}



/**
 * @brief Enable/Disable DSSS
 *
 * Send an IPC message to CPU Low to set the DSSS value
 *
 * @param  u32 dsss_value
 *     - DSSS Enable/Disable value
 * @return None
 */
void wlan_mac_high_set_dsss(u32 dsss_value) {

	wlan_ipc_msg_t       ipc_msg_to_low;
	u32                  ipc_msg_to_low_payload = dsss_value;

	//Set tracking global
	low_param_dsss_en = dsss_value;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CONFIG_DSSS_EN);
	ipc_msg_to_low.num_payload_words = 1;
	ipc_msg_to_low.payload_ptr       = &(ipc_msg_to_low_payload);

	write_mailbox_msg(&ipc_msg_to_low);
}



/**
 * @brief Get CPU low's state
 *
 * Send an IPC message to CPU Low to get its state
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_request_low_state(){
	wlan_ipc_msg_t      ipc_msg_to_low;

	// Send message to CPU Low
	ipc_msg_to_low.msg_id            = IPC_MBOX_MSG_ID(IPC_MBOX_CPU_STATUS);
	ipc_msg_to_low.num_payload_words = 0;
	ipc_msg_to_low.arg0              = (u8)CPU_STATUS_REASON_BOOTED;

	write_mailbox_msg(&ipc_msg_to_low);
}


/**
* @brief Check that CPU low is initialized
*
* @param  None
* @return int
*     - 0 if CPU low is not initialized
*     - 1 if CPU low is initialized
*/
int wlan_mac_high_is_cpu_low_initialized(){
	wlan_mac_high_ipc_rx();
	return ( (cpu_low_status & CPU_STATUS_INITIALIZED) != 0 );
}

/**
 * @brief Check that CPU low is ready to transmit
 *
 * @param  None
 * @return int
 *     - 0 if CPU low is not ready to transmit
 *     - 1 if CPU low is ready to transmit
 */
inline int wlan_mac_high_is_dequeue_allowed(){
	u8 i, num_empty, num_done;

	num_empty = 0;
	num_done = 0;
	for( i = 0; i < 3; i++ ){
		if( ((tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(i))->tx_pkt_buf_state == TX_PKT_BUF_HIGH_CTRL ){
			num_empty++;
		} else if( ((tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(i))->tx_pkt_buf_state == TX_PKT_BUF_DONE ){
			num_done++;
		}
	}

	return (((num_empty + num_done) >= 2) && ((num_empty) > 0));
}


/**
 * @brief Return the index of the next free transmit packet buffer
 *
 * @param  None
 * @return int
 *     - packet buffer index of free, now-locked packet buffer
 *     - -1 if there are no free Tx packet buffers
 */
int wlan_mac_high_get_empty_tx_packet_buffer(){

	//TODO: Debate: This function assumes that it is currently safe to take control
	// of an empty Tx packet buffer. In other words, it is the responsibility of the
	// the calling function to ensure that wlan_mac_high_is_dequeue_allowed() == 1.
	// For extra safety, we could call wlan_mac_high_is_dequeue_allowed() here at the
	// expensive of another search through the three packet buffers.

	u8 i;
	int pkt_buf_sel = -1;

	for( i = 0; i < 3; i++ ){
		if( ((tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(i))->tx_pkt_buf_state == TX_PKT_BUF_HIGH_CTRL ){
			pkt_buf_sel = i;
			break;
		}
	}
	return pkt_buf_sel;
}

/**
 * @brief Determine if Packet is LTG
 * This function inspects the payload of the packet provided as an argument
 * and searches for the LTG-specific LLC header. If it finds such a header,
 * it returns a 1.
 *
 * @param  None
 * @return u8
 *     - 1 if LTG
 *     - 0 if not LTG
 */
u8 wlan_mac_high_is_pkt_ltg(void* mac_payload, u16 length){

	mac_header_80211* hdr_80211;
	llc_header_t* llc_hdr;

	hdr_80211 = (mac_header_80211*)((void*)mac_payload);

	if((hdr_80211->frame_control_1 & 0xF) == MAC_FRAME_CTRL1_TYPE_DATA) {

		//Check if this is an encrypted packet. If it is, we can't trust any of the MPDU
		//payload bytes for further classification
		if(hdr_80211->frame_control_2 & MAC_FRAME_CTRL2_FLAG_PROTECTED){
			return 0;
		}

		llc_hdr = (llc_header_t*)((u8*)mac_payload + sizeof(mac_header_80211));

		if(length < (sizeof(mac_header_80211) + sizeof(llc_header_t) + WLAN_PHY_FCS_NBYTES)){
			// This was a DATA packet, but it wasn't long enough to have an LLC header.
			return 0;

		} else {
			if(llc_hdr->type == LLC_TYPE_WLAN_LTG){
				return 1;
			}
		}
	}

	return 0;
}



/**
 * @brief Configure Beacon Transmissions
 *
 * This function will create a beacon and inform CPU_LOW to transmit it periodically.
 *
 * @param  None
 * @return None
 */
int wlan_mac_high_configure_beacon_tx_template(mac_header_80211_common* tx_header_common_ptr, bss_info_t* bss_info, tx_params_t* tx_params_ptr, u8 flags) {
	u16 tx_length;

	tx_frame_info_t*  tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_BEACON);
	if(lock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS){
		xil_printf("Error: CPU_LOW had lock on Beacon packet buffer during initial configuration\n");
		return -1;
	}

	// Fill in the data
	tx_length = wlan_create_beacon_frame(
		(void*)(tx_frame_info)+PHY_TX_PKT_BUF_MPDU_OFFSET,
		tx_header_common_ptr,
		bss_info);

	bzero(tx_frame_info, sizeof(tx_frame_info));

	// Set up frame info data
	tx_frame_info->timestamp_create            = get_mac_time_usec();
	tx_frame_info->length                      = tx_length;
	tx_frame_info->flags                       = flags;
	tx_frame_info->queue_info.QID			   = 0xFF;
	tx_frame_info->queue_info.occupancy 	   = 0;


	// Unique_seq will be filled in by CPU_LOW
	tx_frame_info->unique_seq = 0;

	memcpy(&(tx_frame_info->params), tx_params_ptr, sizeof(tx_params_t));

	tx_frame_info->tx_pkt_buf_state = TX_PKT_BUF_READY;
	if(unlock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS){
		xil_printf("Error: Unable to unlock Beacon packet buffer during initial configuration\n");
		return -1;
	}

	return 0;
}



/**
 * @brief Update Beacon TX parameters
 *
 * This function will update the beacon template in the packet buffer with the
 * new TX parameters.
 *
 * This function should be used inside a while loop in order to make sure that
 * the update is successful:
 *     while (wlan_mac_high_update_beacon_tx_params(&default_multicast_mgmt_tx_params) != 0) {}
 *
 * @param  tx_params_ptr     - Pointer to tx_params_t structure with new parameters
 * @return status            - 0 - Success;   -1 Failure
 */
int wlan_mac_high_update_beacon_tx_params(tx_params_t* tx_params_ptr) {
	tx_frame_info_t*  tx_frame_info = (tx_frame_info_t*)TX_PKT_BUF_TO_ADDR(TX_PKT_BUF_BEACON);

	if (lock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS) {
		xil_printf("Error: CPU_LOW had lock on Beacon packet buffer during initial configuration\n");
		return -1;
	}

	memcpy(&(tx_frame_info->params), tx_params_ptr, sizeof(tx_params_t));

	if (unlock_tx_pkt_buf(TX_PKT_BUF_BEACON) != PKT_BUF_MUTEX_SUCCESS) {
		xil_printf("Error: Unable to unlock Beacon packet buffer during initial configuration\n");
		return -1;
	}

	return 0;
}



#ifdef _DEBUG_

/**
 * @brief CDMA vs CPU copy performance comparison
 *
 * @param  None
 * @return None
 */
void wlan_mac_high_copy_comparison(){

	#define MAXLEN 10000

	u32 d_cdma;
	u32 d_memcpy;
	u8  isMatched_memcpy;
	u8  isMatched_cdma;

	u32 i;
	u32 j;
	u8* srcAddr = (u8*)RX_PKT_BUF_TO_ADDR(0);
	u8* destAddr = (u8*)DDR3_BASEADDR;
	//u8* destAddr = (u8*)TX_PKT_BUF_TO_ADDR(1);
	u64 t_start;
	u64 t_end;

	xil_printf("--- MEMCPY vs. CDMA Speed Comparison ---\n");
	xil_printf("LEN, T_MEMCPY, T_CDMA, MEMCPY Match?, CDMA Match?\n");
	for(i=0; i<MAXLEN; i++){
		memset(destAddr,0,MAXLEN);
		t_start = get_usec_timestamp();
		memcpy(destAddr,srcAddr,i+1);
		t_end = get_usec_timestamp();
		d_memcpy = (u32)(t_end - t_start);

		isMatched_memcpy = 1;
		for(j=0; j<i; j++){
			if(srcAddr[j] != destAddr[j]){
				isMatched_memcpy = 0;
			}
		}

		memset(destAddr,0,MAXLEN);

		t_start = get_usec_timestamp();
		wlan_mac_high_cdma_start_transfer((void*)destAddr,(void*)srcAddr,i+1);
		//wlan_mac_high_cdma_finish_transfer();
		t_end = get_usec_timestamp();
		wlan_mac_high_clear_debug_gpio(0x04);
		d_cdma = (u32)(t_end - t_start);

		isMatched_cdma = 1;
		for(j=0; j<i; j++){
			if(srcAddr[j] != destAddr[j]){
				isMatched_cdma = 0;
			}
		}
		xil_printf("%d, %d, %d, %d, %d\n", i+1, d_memcpy, d_cdma, isMatched_memcpy, isMatched_cdma);
	}
}



/**
 * @brief Print Hardware Information
 *
 * This function stops the interrupt controller, effectively pausing interrupts. This can
 * be used alongside wlan_mac_high_interrupt_start() to wrap code that is not interrupt-safe.
 *
 * @param wlan_mac_hw_info* info
 *  - pointer to the hardware info struct that should be printed
 * @return None
 *
 */
void wlan_mac_high_print_hw_info(wlan_mac_hw_info * info) {
	int i;

	xil_printf("WLAN MAC HW INFO:  \n");
	xil_printf("  CPU Low Type     :  0x%8x\n", info->cpu_low_type);
	xil_printf("  Serial Number    :  %d\n",    info->serial_number);
	xil_printf("  FPGA DNA         :  0x%8x  0x%8x\n", info->fpga_dna[1], info->fpga_dna[0]);
	xil_printf("  WLAN EXP HW Addr :  %02x",    info->hw_addr_wlan_exp[0]);

	for( i = 1; i < MAC_ADDR_LEN; i++ ) {
		xil_printf(":%02x", info->hw_addr_wlan_exp[i]);
	}
	xil_printf("\n");

	xil_printf("  WLAN HW Addr     :  %02x",    info->hw_addr_wlan[0]);
	for( i = 1; i < MAC_ADDR_LEN; i++ ) {
		xil_printf(":%02x", info->hw_addr_wlan[i]);
	}
	xil_printf("\n");

	xil_printf("END \n");
}



/**
 * @brief Pretty print a buffer of u8
 *
 * @param  u8 * buf
 *     - Buffer to be printed
 * @param  u32 size
 *     - Number of bytes to be printed
 * @return None
 */
void print_buf(u8 *buf, u32 size) {
	u32 i;
	for (i=0; i<size; i++) {
        xil_printf("%2x ", buf[i]);
        if ( (((i + 1) % 16) == 0) && ((i + 1) != size) ) {
            xil_printf("\n");
        }
	}
	xil_printf("\n\n");
}

#endif




