# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework 
    - Transport Unicast Ethernet IP/UDP Python Socket Implementation
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides the unicast Ethernet IP/UDP transport based on the python 
socket class.

Functions:
    TransportEthIpUdpPy() -- Unicast Ethernet UDP transport based on python
        sockets

"""

import re
import time
import socket

from . import transport_eth_ip_udp as tp
from . import exception as ex


__all__ = ['TransportEthIpUdpPy']


class TransportEthIpUdpPy(tp.TransportEthIpUdp):
    """Class for Ethernet IP/UDP Transport class using Python libraries.
       
    Attributes:
        See TransportEthIpUdp for all attributes
    """
    def __init__(self):
        super(TransportEthIpUdpPy, self).__init__()
    
    
    def send(self, payload, robust=True, pkt_type=None):
        """Send a message over the transport.
        
        Args:
            payload (bytes): Data to be sent over the socket
            robust (bool):   Is a response required
            pkt_type (int):  Type of packet to send
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

        Args:
            timeout (float):  Time (in float seconds) to wait before raising 
                an execption.  If no value is specified, then it will use the 
                default transport timeout (self.timeout)
        
        This function will block until a response is received or a timeout 
        occurs.  If a timeout occurs, it will raise a TransportError exception.
        """
        reply = b''

        if timeout is None:
            timeout  = self.timeout
        else:
            timeout += self.timeout         # Extend timeout to not run in to race conditions

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
                raise ex.TransportError(self, "Transport receive timed out.")

        return reply


    def receive_nb(self):
        """Return a response from the transport.
        
        This function will not block and should be called in a polling loop 
        until a response is received.
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
































