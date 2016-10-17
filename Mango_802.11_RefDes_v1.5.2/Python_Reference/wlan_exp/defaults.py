# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Default Constants
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides wlan_exp default constants.

"""

# Default Variables
PACKAGE_NAME                      = 'wlan_exp'


# Reference Design Node Types
#  Values here must match C counterparts in wlan_exp_common.h
WLAN_EXP_MASK                     = 0xFFFF0000
WLAN_EXP_80211_BASE               = 0x00010000

# CPU High Types
WLAN_EXP_HIGH_MASK                = 0x0000FF00
WLAN_EXP_HIGH_AP                  = 0x00000100
WLAN_EXP_HIGH_STA                 = 0x00000200
WLAN_EXP_HIGH_IBSS                = 0x00000300

WLAN_EXP_HIGH_TYPES               = {WLAN_EXP_HIGH_AP   : "AP",
                                     WLAN_EXP_HIGH_STA  : "STA", 
                                     WLAN_EXP_HIGH_IBSS : "IBSS"}
# CPU Low Types
WLAN_EXP_LOW_MASK                 = 0x000000FF
WLAN_EXP_LOW_DCF                  = 0x00000001
WLAN_EXP_LOW_NOMAC                = 0x00000002

WLAN_EXP_LOW_TYPES                = {WLAN_EXP_LOW_DCF   : "DCF",
                                     WLAN_EXP_LOW_NOMAC : "NOMAC"}

# Node Types (supported combinations of CPU High and CPU Low)
WLAN_EXP_AP_DCF_TYPE              = WLAN_EXP_80211_BASE + WLAN_EXP_HIGH_AP + WLAN_EXP_LOW_DCF
WLAN_EXP_AP_DCF_CLASS_INST        = 'node_ap.WlanExpNodeAp(network_config)'
WLAN_EXP_AP_DCF_DESCRIPTION       = '(AP/DCF) '

WLAN_EXP_STA_DCF_TYPE             = WLAN_EXP_80211_BASE + WLAN_EXP_HIGH_STA + WLAN_EXP_LOW_DCF
WLAN_EXP_STA_DCF_CLASS_INST       = 'node_sta.WlanExpNodeSta(network_config)'
WLAN_EXP_STA_DCF_DESCRIPTION      = '(STA/DCF)'

WLAN_EXP_IBSS_DCF_TYPE            = WLAN_EXP_80211_BASE + WLAN_EXP_HIGH_IBSS + WLAN_EXP_LOW_DCF
WLAN_EXP_IBSS_DCF_CLASS_INST      = 'node_ibss.WlanExpNodeIBSS(network_config)'
WLAN_EXP_IBSS_DCF_DESCRIPTION     = '(IBSS/DCF) '

WLAN_EXP_AP_NOMAC_TYPE            = WLAN_EXP_80211_BASE + WLAN_EXP_HIGH_AP + WLAN_EXP_LOW_NOMAC
WLAN_EXP_AP_NOMAC_CLASS_INST      = 'node_ap.WlanExpNodeAp(network_config)'
WLAN_EXP_AP_NOMAC_DESCRIPTION     = '(AP/NOMAC) '

WLAN_EXP_STA_NOMAC_TYPE           = WLAN_EXP_80211_BASE + WLAN_EXP_HIGH_STA + WLAN_EXP_LOW_NOMAC
WLAN_EXP_STA_NOMAC_CLASS_INST     = 'node_sta.WlanExpNodeSta(network_config)'
WLAN_EXP_STA_NOMAC_DESCRIPTION    = '(STA/NOMAC)'

WLAN_EXP_IBSS_NOMAC_TYPE          = WLAN_EXP_80211_BASE + WLAN_EXP_HIGH_IBSS + WLAN_EXP_LOW_NOMAC
WLAN_EXP_IBSS_NOMAC_CLASS_INST    = 'node_ibss.WlanExpNodeIBSS(network_config)'
WLAN_EXP_IBSS_NOMAC_DESCRIPTION   = '(IBSS/NOMAC) '
