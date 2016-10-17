# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Default Constants
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a ejw  1/23/14  Initial release

------------------------------------------------------------------------------

This module provides WLAN Exp default constants.

"""

# Default Variables
PACKAGE_NAME                      = 'wlan_exp'


# WARPNet Node Types
#   NOTE:  The C counterparts are found in wlan_exp_common.h
WLAN_EXP_BASE                     = 0x00010000

WLAN_EXP_HIGH_AP                  = 0x00000100
WLAN_EXP_HIGH_STA                 = 0x00000200
WLAN_EXP_HIGH_IBSS                = 0x00000300

WLAN_EXP_LOW_DCF                  = 0x00000001
WLAN_EXP_LOW_NOMAC                = 0x00000002

WLAN_EXP_AP_DCF_TYPE              = WLAN_EXP_BASE + WLAN_EXP_HIGH_AP + WLAN_EXP_LOW_DCF
WLAN_EXP_AP_DCF_CLASS_INST        = 'node_ap.WlanExpNodeAp(network_config, defaults.WLAN_EXP_LOW_DCF)'
WLAN_EXP_AP_DCF_DESCRIPTION       = 'WLAN Exp (AP/DCF) '

WLAN_EXP_STA_DCF_TYPE             = WLAN_EXP_BASE + WLAN_EXP_HIGH_STA + WLAN_EXP_LOW_DCF
WLAN_EXP_STA_DCF_CLASS_INST       = 'node_sta.WlanExpNodeSta(network_config, defaults.WLAN_EXP_LOW_DCF)'
WLAN_EXP_STA_DCF_DESCRIPTION      = 'WLAN Exp (STA/DCF)'

WLAN_EXP_IBSS_DCF_TYPE            = WLAN_EXP_BASE + WLAN_EXP_HIGH_IBSS + WLAN_EXP_LOW_DCF
WLAN_EXP_IBSS_DCF_CLASS_INST      = 'node_ibss.WlanExpNodeIBSS(network_config, defaults.WLAN_EXP_LOW_DCF)'
WLAN_EXP_IBSS_DCF_DESCRIPTION     = 'WLAN Exp (IBSS/DCF) '

WLAN_EXP_AP_NOMAC_TYPE            = WLAN_EXP_BASE + WLAN_EXP_HIGH_AP + WLAN_EXP_LOW_NOMAC
WLAN_EXP_AP_NOMAC_CLASS_INST      = 'node_ap.WlanExpNodeAp(network_config, defaults.WLAN_EXP_LOW_NOMAC)'
WLAN_EXP_AP_NOMAC_DESCRIPTION     = 'WLAN Exp (AP/NOMAC) '

WLAN_EXP_STA_NOMAC_TYPE           = WLAN_EXP_BASE + WLAN_EXP_HIGH_STA + WLAN_EXP_LOW_NOMAC
WLAN_EXP_STA_NOMAC_CLASS_INST     = 'node_sta.WlanExpNodeSta(network_config, defaults.WLAN_EXP_LOW_NOMAC)'
WLAN_EXP_STA_NOMAC_DESCRIPTION    = 'WLAN Exp (STA/NOMAC)'

WLAN_EXP_IBSS_NOMAC_TYPE          = WLAN_EXP_BASE + WLAN_EXP_HIGH_IBSS + WLAN_EXP_LOW_NOMAC
WLAN_EXP_IBSS_NOMAC_CLASS_INST    = 'node_ibss.WlanExpNodeIBSS(network_config, defaults.WLAN_EXP_LOW_NOMAC)'
WLAN_EXP_IBSS_NOMAC_DESCRIPTION   = 'WLAN Exp (IBSS/NOMAC) '










