# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Default Constants
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

This module provides WARPNet default constants.

"""

# Default Variables
PACKAGE_NAME                      = 'warpnet'


# WARPNet INI Files
NODES_CONFIG_INI_FILE             = 'nodes_config.ini'


# WARPNet Node Types
WN_NODE_TYPE                      = 0
WN_NODE_CLASS                     = 'WnNode(network_config)'
WN_NODE_DESCRIPTION               = 'WARPNet Node'


# WARPNet Default values
#   NOTE:  All defaults are strings; Numerical values will be evaluated and
#          converted to integers before being used
NETWORK                           = '10.0.0.0'
HOST_ID                           = 250
UNICAST_PORT                      = 9500
BCAST_PORT                        = 9750
TRANSPORT_TYPE                    = 'python'
JUMBO_FRAME_SUPPORT               = False
TX_BUFFER_SIZE                    = 2**22
RX_BUFFER_SIZE                    = 2**22



