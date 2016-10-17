# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Config
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

This module provides class definitions to manage the WARPNet configuration.

Functions (see below for more information):
    WlanExpNetworkConfiguration()  -- Specifies Network information for setup
    WlanExpNodesConfiguration() -- Specifies Node information for setup

"""

import wlan_exp.warpnet.config as wn_config


__all__ = ['WlanExpNetworkConfiguration', 'WlanExpNodesConfiguration']



class WlanExpNetworkConfiguration(wn_config.NetworkConfiguration):
    """Class for WLAN Exp Host configuration.

    This class is a child of the WARPNet host configuration.
    """
    def __init__(self, network=None, host_id=None, unicast_port=None,
                 bcast_port=None, tx_buffer_size=None, rx_buffer_size=None,
                 transport_type=None, jumbo_frame_support=None):
        """Initialize a WlanExpHostConfiguration
        
        Attributes:
            network             -- Network interface
            host_id             -- Host ID
            unicast_port        -- Host port for unicast traffic
            bcast_port          -- Host port for broadcast traffic
            tx_buf_size         -- Host TX buffer size
            rx_buf_size         -- Host RX buffer size
            transport_type      -- Host transport type
            jumbo_frame_support -- Host support for Jumbo Ethernet frames
        
        """
        super(WlanExpNetworkConfiguration, self).__init__(network=network, 
                                                          host_id=host_id, 
                                                          unicast_port=unicast_port,
                                                          bcast_port=bcast_port, 
                                                          tx_buffer_size=tx_buffer_size, 
                                                          rx_buffer_size=rx_buffer_size,
                                                          transport_type=transport_type, 
                                                          jumbo_frame_support=jumbo_frame_support)

# End Class



class WlanExpNodesConfiguration(wn_config.NodesConfiguration):
    """Class for WLAN Exp Node Configuration.
    
    This class is a child of the WARPNet Node configuration.
    """
    def __init__(self, ini_file=None, serial_numbers=None, network_config=None):
        """Initialize a WlanExpNodesConfiguration
        
        Attributes:
            ini_file       -- An INI file name that specified a nodes configuration
            serial_numbers -- A list of serial numbers of WARPv3 nodes
            network_config -- A NetworkConfiguration
        """
        super(WlanExpNodesConfiguration, self).__init__(ini_file=ini_file,
                                                        serial_numbers=serial_numbers,
                                                        network_config=network_config)


# End Class
