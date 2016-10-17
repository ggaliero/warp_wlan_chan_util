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

#include "wlan_mac_time_hw_axiw.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xcope.h"

inline xc_status_t xc_wlan_mac_time_hw_axiw_create(xc_iface_t **iface, void *config_table)
{
    // set up iface
    *iface = (xc_iface_t *) config_table;

#ifdef XC_DEBUG
    WLAN_MAC_TIME_HW_AXIW_Config *_config_table = config_table;

    if (_config_table->xc_create == NULL) {
        print("config_table.xc_create == NULL\r\n");
        exit(1);
    }
#endif

    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_time_hw_axiw_release(xc_iface_t **iface) 
{
    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_time_hw_axiw_open(xc_iface_t *iface)
{
    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_time_hw_axiw_close(xc_iface_t *iface)
{
    // does nothing
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_time_hw_axiw_read(xc_iface_t *iface, xc_r_addr_t addr, uint32_t *value)
{
    *value = Xil_In32((uint32_t *) addr);
    return XC_SUCCESS;
}

inline xc_status_t xc_wlan_mac_time_hw_axiw_write(xc_iface_t *iface, xc_w_addr_t addr, const uint32_t value)
{
    Xil_Out32((uint32_t *) addr, value);
    return XC_SUCCESS;
}

xc_status_t xc_wlan_mac_time_hw_axiw_getshmem(xc_iface_t *iface, const char *name, void **shmem)
{
    WLAN_MAC_TIME_HW_AXIW_Config *_config_table = (WLAN_MAC_TIME_HW_AXIW_Config *) iface;

    if (strcmp("SYSTEM_TIME_USEC_MSB", name) == 0) {
        *shmem = (void *) & _config_table->SYSTEM_TIME_USEC_MSB;
    } else if (strcmp("SYSTEM_TIME_USEC_LSB", name) == 0) {
        *shmem = (void *) & _config_table->SYSTEM_TIME_USEC_LSB;
    } else if (strcmp("MAC_TIME_USEC_MSB", name) == 0) {
        *shmem = (void *) & _config_table->MAC_TIME_USEC_MSB;
    } else if (strcmp("MAC_TIME_USEC_LSB", name) == 0) {
        *shmem = (void *) & _config_table->MAC_TIME_USEC_LSB;
    } else if (strcmp("NEW_MAC_TIME_MSB", name) == 0) {
        *shmem = (void *) & _config_table->NEW_MAC_TIME_MSB;
    } else if (strcmp("NEW_MAC_TIME_LSB", name) == 0) {
        *shmem = (void *) & _config_table->NEW_MAC_TIME_LSB;
    } else if (strcmp("Control", name) == 0) {
        *shmem = (void *) & _config_table->Control;
    }
    else { *shmem = NULL; return XC_FAILURE; }

    return XC_SUCCESS;
}
