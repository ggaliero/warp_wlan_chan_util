# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework 
    - Transport Default Constants
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides Transport default constants.

"""

# Default Variables
PACKAGE_NAME                      = 'warp_transport'


# Nodes Configuration INI Files
NODES_CONFIG_INI_FILE             = 'nodes_config.ini'


# Node Types
DEFAULT_NODE_TYPE                 = 0
DEFAULT_NODE_CLASS                = 'Node(network_config)'
DEFAULT_NODE_DESCRIPTION          = 'Default Node'


# Transport Default values
#     - All defaults are strings; Numerical values will be evaluated and
#       converted to integers before being used
NETWORK                           = '10.0.0.0'
HOST_ID                           = 250
UNICAST_PORT                      = 9500
BROADCAST_PORT                    = 9750
TRANSPORT_TYPE                    = 'python'
JUMBO_FRAME_SUPPORT               = False
TX_BUFFER_SIZE                    = 2**23        # 8 MB buffer
RX_BUFFER_SIZE                    = 2**23        # 8 MB buffer



