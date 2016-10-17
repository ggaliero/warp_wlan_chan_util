# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Newtork / Node Configurations
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides class definitions to manage the configuration of 
 wlan_exp node objects and network interfaces.

Functions (see below for more information):
    WlanExpNetworkConfiguration() -- Specifies Network information for setup
    WlanExpNodesConfiguration()   -- Specifies Node information for setup
"""

import wlan_exp.transport.config as config

__all__ = ['WlanExpNetworkConfiguration', 'WlanExpNodesConfiguration']

class WlanExpNetworkConfiguration(config.NetworkConfiguration):
    """Class for wlan_exp network configuration.

    This class is a child of the transport network configuration.
    """
    def __init__(self, network=None, host_id=None, unicast_port=None,
                 broadcast_port=None, tx_buffer_size=None, rx_buffer_size=None,
                 transport_type=None, jumbo_frame_support=None):
        """Initialize a WlanExpHostConfiguration
        
        Attributes:
            network             -- Network interface
            host_id             -- Host ID
            unicast_port        -- Host port for unicast traffic
            broadcast_port      -- Host port for broadcast traffic
            tx_buf_size         -- Host TX buffer size
            rx_buf_size         -- Host RX buffer size
            transport_type      -- Host transport type
            jumbo_frame_support -- Host support for Jumbo Ethernet frames
        
        """
        super(WlanExpNetworkConfiguration, self).__init__(network=network, 
                                                          host_id=host_id, 
                                                          unicast_port=unicast_port,
                                                          broadcast_port=broadcast_port, 
                                                          tx_buffer_size=tx_buffer_size, 
                                                          rx_buffer_size=rx_buffer_size,
                                                          transport_type=transport_type, 
                                                          jumbo_frame_support=jumbo_frame_support)

# End Class



class WlanExpNodesConfiguration(config.NodesConfiguration):
    """Class for wlan_exp node Configuration.
    
    This class is a child of the transport node configuration.
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
