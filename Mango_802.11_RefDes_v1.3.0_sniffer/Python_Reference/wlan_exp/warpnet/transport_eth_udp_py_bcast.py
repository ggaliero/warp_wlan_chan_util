# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport - Broadcast Ethernet UDP Python Socket Implementation
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

This module provides the WARPNet broadcast Ethernet UDP transport based on 
the python socket class.

Functions:
    TransportEthUdpPyBcast() -- Broadcast Ethernet UDP transport based on 
        python sockets

Integer constants:
    REQUESTED_BUF_SIZE -- Size of TX/RX buffer that will requested from the 
        operating system.

"""

import re

from . import transport_eth_udp as tp


__all__ = ['TransportEthUdpPyBcast']


class TransportEthUdpPyBcast(tp.TransportEthUdp):
    """Class for WARPNet Ethernet UDP Broadcast Transport class using Python libraries.
       
    Attributes:
        See TransportEthUdp for attributes
        network_config -- A NetworkConfiguration that describes the transport configuration.
    
    The transport will send packets to 
    of the first host interface as the subnet for the broadcast address.
    """
    network_config = None
    
    def __init__(self, network_config=None):
        super(TransportEthUdpPyBcast, self).__init__()

        if network_config is not None:
            self.network_config = network_config
        else:
            from . import config as wn_config

            self.network_config = wn_config.NetworkConfiguration()
        
        self.set_default_config()

        
    def set_default_config(self):
        """Set the default configuration of a Broadcast transport."""
        unicast_port = self.network_config.get_param('unicast_port')
        bcast_port   = self.network_config.get_param('bcast_port')
        host_id      = self.network_config.get_param('host_id')
        bcast_addr   = self.network_config.get_param('bcast_address')
        
        # Set default values of the Transport
        self.set_ip_address(bcast_addr)
        self.set_unicast_port(unicast_port)
        self.set_bcast_port(bcast_port)
        self.set_src_id(host_id)
        self.set_dest_id(0xFFFF)
        self.timeout = 1


    def set_ip_address(self, ip_addr):
        """Sets the IP address of the Broadcast transport.
        
        This method will take an IP address of the for W.X.Y.Z and set the
        transport IP address to W.X.Y.255 so that it is a proper broadcast
        address.
        """
        expr = re.compile('\.')
        tmp = [int(n) for n in expr.split(ip_addr)]        
        self.ip_address = "{0:d}.{1:d}.{2:d}.255".format(tmp[0], tmp[1], tmp[2])

    
    def send(self, payload, robust=False, pkt_type=None):
        """Send a broadcast packet over the transport.
        
        Attributes:
            data -- Data to be sent over the socket
        """
        if robust:
            print("WARNING: Not able to send broadcast robust packets.")        
        
        if pkt_type is not None:
            self.hdr.set_type(pkt_type)

        self.hdr.response_not_required()
        self.hdr.set_length(len(payload))
        self.hdr.increment()

        data = bytes(b'\x00\x00' + self.hdr.serialize() + payload)
        
        size = self.sock.sendto(data, (self.ip_address, self.bcast_port))
        
        if size != len(data):
            print("Only {} of {} bytes of data sent".format(size, len(data)))


    def receive(self, timeout=None):
        """Not used on a broadcast transport"""
        super(TransportEthUdpPyBcast, self).receive()


# End Class
































