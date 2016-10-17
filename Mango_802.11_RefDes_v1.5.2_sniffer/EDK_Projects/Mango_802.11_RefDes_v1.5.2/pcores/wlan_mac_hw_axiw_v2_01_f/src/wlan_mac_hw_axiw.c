///////////////////////////////////////////////////////////////-*-C-*-
//
// Copyright (c) 2011 Xilinx, Inc.  All rights reserved.
//
// Xilinx, Inc.  XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION
// "AS IS" AS  A COURTESY TO YOU.  BY PROVIDING  THIS DESIGN, CODE, OR
// INFORMATION  AS  ONE   POSSIBLE  IMPLEMENTATION  OF  THIS  FEATURE,
// APPLICATION OR  STANDARD, XILINX  IS MAKING NO  REPRESENTATION THAT
// THIS IMPLEMENTATION  IS FREE FROM  ANY CLAIMS OF  INFRINGEMENT, AND
// YOU ARE  RESPONSIBLE FOR OBTAINING  ANY RIGHTS YOU MAY  REQUIRE FOR
// YOUR  IMPLEMENTATION.   XILINX  EXPRESSLY  DISCLAIMS  ANY  WARRANTY
// WHATSOEVER  WITH RESPECT  TO  THE ADEQUACY  OF THE  IMPLEMENTATION,
// INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR REPRESENTATIONS THAT
// THIS IMPLEMENTATION  IS FREE  FROM CLAIMS OF  INFRINGEMENT, IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// 
//////////////////////////////////////////////////////////////////////

#include "wlan_mac_hw_axiw.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xcope.h"

inline xc_status_t xc_wlan_mac_hw_axiw_create(xc_iface_t **iface, void *config_table)
{
    // set up iface
    *iface = (xc_iface_t *) config_table;

#ifdef XC_DEBUG
    WLAN_MAC_HW_AXIW_Config *_config_table = config_table;

    if (_config_table->xc_create == NULL) {
        print("config_table.xc_create == NULL\r\n");
        exit(1);
    }
#endif

    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_hw_axiw_release(xc_iface_t **iface) 
{
    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_hw_axiw_open(xc_iface_t *iface)
{
    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_hw_axiw_close(xc_iface_t *iface)
{
    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_hw_axiw_read(xc_iface_t *iface, xc_r_addr_t addr, uint32_t *value)
{
    *value = Xil_In32((uint32_t *) addr);
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_hw_axiw_write(xc_iface_t *iface, xc_w_addr_t addr, const uint32_t value)
{
    Xil_Out32((uint32_t *) addr, value);
    return XC_SUCCESS;
}

xc_status_t xc_wlan_mac_hw_axiw_getshmem(xc_iface_t *iface, const char *name, void **shmem)
{
    WLAN_MAC_HW_AXIW_Config *_config_table = (WLAN_MAC_HW_AXIW_Config *) iface;

    if (strcmp("NAV_VALUE", name) == 0) {
        *shmem = (void *) & _config_table->NAV_VALUE;
    } else if (strcmp("BACKOFF_COUNTERS", name) == 0) {
        *shmem = (void *) & _config_table->BACKOFF_COUNTERS;
    } else if (strcmp("PHY_RX_PARAMS", name) == 0) {
        *shmem = (void *) & _config_table->PHY_RX_PARAMS;
    } else if (strcmp("LATEST_RX_BYTE", name) == 0) {
        *shmem = (void *) & _config_table->LATEST_RX_BYTE;
    } else if (strcmp("TX_START_TIMESTAMP_MSB", name) == 0) {
        *shmem = (void *) & _config_table->TX_START_TIMESTAMP_MSB;
    } else if (strcmp("TX_START_TIMESTAMP_LSB", name) == 0) {
        *shmem = (void *) & _config_table->TX_START_TIMESTAMP_LSB;
    } else if (strcmp("RX_START_TIMESTAMP_MSB", name) == 0) {
        *shmem = (void *) & _config_table->RX_START_TIMESTAMP_MSB;
    } else if (strcmp("RX_START_TIMESTAMP_LSB", name) == 0) {
        *shmem = (void *) & _config_table->RX_START_TIMESTAMP_LSB;
    } else if (strcmp("TXRX_START_TIMESTAMPS_FRAC", name) == 0) {
        *shmem = (void *) & _config_table->TXRX_START_TIMESTAMPS_FRAC;
    } else if (strcmp("Tx_Ctrl_Status", name) == 0) {
        *shmem = (void *) & _config_table->Tx_Ctrl_Status;
    } else if (strcmp("Status", name) == 0) {
        *shmem = (void *) & _config_table->Status;
    } else if (strcmp("TX_CTRL_B_PARAMS", name) == 0) {
        *shmem = (void *) & _config_table->TX_CTRL_B_PARAMS;
    } else if (strcmp("TX_CTRL_A_GAINS", name) == 0) {
        *shmem = (void *) & _config_table->TX_CTRL_A_GAINS;
    } else if (strcmp("TX_CTRL_C_PARAMS", name) == 0) {
        *shmem = (void *) & _config_table->TX_CTRL_C_PARAMS;
    } else if (strcmp("TX_CTRL_A_PARAMS", name) == 0) {
        *shmem = (void *) & _config_table->TX_CTRL_A_PARAMS;
    } else if (strcmp("TX_CTRL_C_GAINS", name) == 0) {
        *shmem = (void *) & _config_table->TX_CTRL_C_GAINS;
    } else if (strcmp("POST_RX_TIMERS", name) == 0) {
        *shmem = (void *) & _config_table->POST_RX_TIMERS;
    } else if (strcmp("TX_CTRL_B_GAINS", name) == 0) {
        *shmem = (void *) & _config_table->TX_CTRL_B_GAINS;
    } else if (strcmp("POST_TX_TIMERS", name) == 0) {
        *shmem = (void *) & _config_table->POST_TX_TIMERS;
    } else if (strcmp("CALIB_TIMES", name) == 0) {
        *shmem = (void *) & _config_table->CALIB_TIMES;
    } else if (strcmp("IFS_INTERVALS2", name) == 0) {
        *shmem = (void *) & _config_table->IFS_INTERVALS2;
    } else if (strcmp("IFS_INTERVALS1", name) == 0) {
        *shmem = (void *) & _config_table->IFS_INTERVALS1;
    } else if (strcmp("TX_START", name) == 0) {
        *shmem = (void *) & _config_table->TX_START;
    } else if (strcmp("TU_TARGET", name) == 0) {
        *shmem = (void *) & _config_table->TU_TARGET;
    } else if (strcmp("Control", name) == 0) {
        *shmem = (void *) & _config_table->Control;
    } else if (strcmp("NAV_MATCH_ADDR_1", name) == 0) {
        *shmem = (void *) & _config_table->NAV_MATCH_ADDR_1;
    } else if (strcmp("NAV_MATCH_ADDR_2", name) == 0) {
        *shmem = (void *) & _config_table->NAV_MATCH_ADDR_2;
    } else if (strcmp("BACKOFF_CTRL", name) == 0) {
        *shmem = (void *) & _config_table->BACKOFF_CTRL;
    }
    else { *shmem = NULL; return XC_FAILURE; }

    return XC_SUCCESS;
}