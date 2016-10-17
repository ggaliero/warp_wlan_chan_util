# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport - Unicast Ethernet UDP Python Socket Implementation
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

This module provides the WARPNet unicast Ethernet UDP transport based on 
the python socket class.

Functions:
    TransportEthUdpPy() -- Unicast Ethernet UDP transport based on python
        sockets

Integer constants:
    REQUESTED_BUF_SIZE -- Size of TX/RX buffer that will requested from the 
        operating system.

"""

import re
import time
import socket

from . import transport_eth_udp as tp
from . import exception as wn_ex


__all__ = ['TransportEthUdpPy']


class TransportEthUdpPy(tp.TransportEthUdp):
    """Class for WARPNet Ethernet UDP Transport class using Python libraries.
       
    Attributes:
        See WnTransportEthUdp for all attributes
    """
    def __init__(self):
        super(TransportEthUdpPy, self).__init__()
    
    
    def send(self, payload, robust=True, pkt_type=None):
        """Send a message over the transport.
        
        Attributes:
            data -- Data to be sent over the socket
            robust -- Do we want a response to the sent data
            max_attempts -- Maximum attempts to transmit the data
        """
        
        if robust:
            self.hdr.response_required()
        else:
            self.hdr.response_not_required()
        
        if pkt_type is not None:
            self.hdr.set_type(pkt_type)        

        self.hdr.set_length(len(payload))
        self.hdr.increment()

        # Pad the data with two extra bytes for 32 bit alignment after the 
        #   ethernet header
        data = bytes(b'\x00\x00' + self.hdr.serialize() + payload)        
 
        size = self.sock.sendto(data, (self.ip_address, self.unicast_port))
        
        if size != len(data):
            print("Only {} of {} bytes of data sent".format(size, len(data)))


    def receive(self, timeout=None):
        """Return a response from the transport.

        Attributes:
            timeout  -- Time (in float seconds) to wait before raising an execption
                        If no value is specified, then it will use the default 
                        transport timeout (self.timeout)
        
        NOTE:  This function will block until a response is received or a
        timeout occurs.  If a timeout occurs, it will raise a WnTransportError
        exception.
        """
        reply = b''

        if timeout is None:
            timeout  = self.timeout
        else:
            timeout += self.timeout         # Extend timeout by a bit so we don't run in to race conditions

        max_pkt_len = self.get_max_payload() + 100;
        received_resp = 0
        start_time = time.time()
        
        while received_resp == 0:
            recv_data = []
            
            try:
                (recv_data, _) = self.sock.recvfrom(max_pkt_len)  # recv_addr is not used
            except socket.error as err:
                expr = re.compile("timed out")
                if not expr.match(str(err)):
                    print("Failed to receive UDP packet.\nError message:\n{}".format(err))
            
            if len(recv_data) > 0:
                hdr_len = 2 + self.hdr.sizeof()
                # print("Received pkt from", recv_addr)
                if (self.hdr.is_reply(recv_data[2:hdr_len])):
                    reply = recv_data[hdr_len:]
                    received_resp = 1
            
            if ((time.time() - start_time) > timeout ) and (received_resp == 0):
                raise wn_ex.TransportError(self, "Transport receive timed out.")

        return reply


    def receive_nb(self):
        """Return a response from the transport.
        
        NOTE:  This function will not block and should be called in a polling
        loop until a response is received.
        """
        reply = b''
        recv_data = []

        max_pkt_len = self.get_max_payload() + 100;
        
        try:
            (recv_data, _) = self.sock.recvfrom(max_pkt_len)  # recv_addr is not used
        except socket.error as err:
            expr = re.compile("timed out")
            if not expr.match(str(err)):
                print("Failed to receive UDP packet.\nError message:\n{}".format(err))
        
        if len(recv_data) > 0:
            hdr_len = 2 + self.hdr.sizeof()
            if (self.hdr.is_reply(recv_data[2:hdr_len])):
                reply = recv_data[hdr_len:]
            
        return reply


# End Class
































