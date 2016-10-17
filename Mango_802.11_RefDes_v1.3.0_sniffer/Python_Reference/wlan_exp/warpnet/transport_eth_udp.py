# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport - Ethernet UDP base class
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

This module provides the base class for WARPNet Ethernet UDP transports.

Functions:
    TransportEthUdp() -- Base class for Ethernet UDP transports
    int_to_ip()       -- Convert 32 bit integer to 'w.x.y.z' IP address string
    ip_to_int()       -- Convert 'w.x.y.z' IP address string to 32 bit integer
    mac_addr_to_str()      -- Convert 6 byte MAC address to 'uu:vv:ww:xx:yy:zz' string

"""

import re
import time
import errno
import socket
from socket import error as socket_error

from . import message as wn_message
from . import exception as wn_ex
from . import transport as tp


__all__ = ['TransportEthUdp']


class TransportEthUdp(tp.Transport):
    """Base Class for WARPNet Ethernet UDP Transport class.
       
    Attributes:
        timeout-- Maximum time spent waiting before retransmission
        transport_type -- Unique type of the WARPNet transport
        sock -- UDP socket
        status -- Status of the UDP socket
        max_payload -- Maximum payload size (eg. MTU - ETH/IP/UDP headers)
        
        mac_address -- Node's MAC address
        ip_address -- IP address of destination
        unicast_port -- Unicast port of destination
        bcast_port -- Broadcast port of destination
        group_id -- Group ID of the node attached to the transport
        rx_buffer_size -- OS's receive buffer size (in bytes)        
        tx_buffer_size -- OS's transmit buffer size (in bytes)        
    """
    timeout         = None
    transport_type  = None
    node_eth_device = None
    sock            = None
    status          = None
    max_payload     = None
    mac_address     = None
    ip_address      = None
    unicast_port    = None
    bcast_port      = None
    group_id        = None
    rx_buffer_size  = None
    tx_buffer_size  = None
    
    def __init__(self):
        self.hdr = wn_message.TransportHeader()
        self.status = 0
        self.timeout = 1
        self.max_payload = 1000   # Sane default.  Overwritten by payload test.
        
        self.check_setup()

    def __del__(self):
        """Closes any open sockets"""
        self.wn_close()

    def __repr__(self):
        """Return transport IP address and Ethernet MAC address"""
        msg  = "Eth UDP Transport: {0} ".format(self.ip_address)
        msg += "({0})".format(self.mac_address)
        return msg

    def check_setup(self):
        """Check the setup of the transport."""
        pass

    def set_max_payload(self, value):  self.max_payload = value
    def set_ip_address(self, value):   self.ip_address = value
    def set_mac_address(self, value):  self.mac_address = value
    def set_unicast_port(self, value): self.unicast_port = value
    def set_bcast_port(self, value):   self.bcast_port = value
    def set_src_id(self, value):       self.hdr.set_src_id(value)
    def set_dest_id(self, value):      self.hdr.set_dest_id(value)

    def get_max_payload(self):         return self.max_payload
    def get_ip_address(self):          return self.ip_address
    def get_mac_address(self):         return self.mac_address
    def get_unicast_port(self):        return self.unicast_port
    def get_bcast_port(self):          return self.bcast_port
    def get_src_id(self):              return self.hdr.get_src_id()
    def get_dest_id(self):             return self.hdr.get_dest_id()


    def wn_open(self, tx_buf_size=None, rx_buf_size=None):
        """Opens an Ethernet UDP socket.""" 
        self.sock = socket.socket(socket.AF_INET,       # Internet
                                  socket.SOCK_DGRAM);   # UDP
        
        self.sock.settimeout(self.timeout)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        if tx_buf_size is not None:
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, tx_buf_size)
            except socket_error as serr:
                # On some HW we cannot set the buffer size
                if serr.errno != errno.ENOBUFS:
                    raise serr

        if rx_buf_size is not None:
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, rx_buf_size)
            except socket_error as serr:
                # On some HW we cannot set the buffer size
                if serr.errno != errno.ENOBUFS:
                    raise serr

        self.tx_buffer_size = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
        self.rx_buffer_size = self.sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
        self.status = 1
        
        if (self.tx_buffer_size != tx_buf_size):
            msg  = "OS reduced send buffer size from "
            msg += "{0} to {1}".format(self.tx_buffer_size, tx_buf_size) 
            print(msg)
            
        if (self.rx_buffer_size != rx_buf_size):
            msg  = "OS reduced receive buffer size from "
            msg += "{0} to {1}".format(self.rx_buffer_size, rx_buf_size) 
            print(msg)


    def wn_close(self):
        """Closes an Ethernet UDP socket."""
        if self.sock:
            try:
                self.sock.close()
            except socket.error as err:
                print("Error closing socket:  {0}".format(err))

        self.status = 0


    # -------------------------------------------------------------------------
    # Commands that must be implemented by child classes
    # -------------------------------------------------------------------------
    def send(self, payload, robust=None, pkt_type=None):
        """Send a message over the transport."""
        raise NotImplementedError

    def receive(self, timeout=None):
        """Return a response from the transport."""
        raise NotImplementedError


    # -------------------------------------------------------------------------
    # WARPNet Commands for the Transport
    # -------------------------------------------------------------------------
    def ping(self, node, output=False):
        """Issues a ping command to the given node.
        
        Will optionally print the result of the ping.
        """
        from . import cmds as wn_cmds

        start_time = time.clock()
        node.send_cmd(wn_cmds.Ping())
        end_time = time.clock()
        
        if output:
            ms_time = (end_time - start_time) * 1000
            print("Reply from {0}:  time = {1:.3f} ms".format(self.ip_address, 
                                                              ms_time))

    
    def test_payload_size(self, node, jumbo_frame_support=False):
        """Determines the object's max_payload parameter."""
        from . import cmds as wn_cmds

        if jumbo_frame_support:
            payload_test_sizes = [1000, 1470, 5000, 8966]
        else:
            payload_test_sizes = [1000, 1470]
        
        for size in payload_test_sizes:
            cmd_size    = wn_message.Cmd().sizeof()
            my_arg_size = (size - (self.hdr.sizeof() + cmd_size + 4)) // 4
 
            try:
                resp = node.send_cmd(wn_cmds.TestPayloadSize(my_arg_size))
                self.set_max_payload(resp)
                
                if (self.get_max_payload() < (4*my_arg_size)):
                    # If the node received a smaller payload, then break
                    break
            except wn_ex.TransportError:
                # If there was a transmission error, then break
                break

    
    def add_node_group_id(self, node, group):
        from . import cmds as wn_cmds

        node.send_cmd(wn_cmds.AddNodeGrpId(group))

    
    def clear_node_group_id(self, node, group):
        from . import cmds as wn_cmds

        node.send_cmd(wn_cmds.ClearNodeGrpId(group))


    # -------------------------------------------------------------------------
    # WARPNet Parameter Framework
    #   Allows for processing of hardware parameters
    # -------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if   (identifier == tp.TRANSPORT_TYPE):
            if (length == 1):
                self.transport_type  = (values[0] & 0xFFFF)
                self.node_eth_device = node_eth_device_to_str((values[0] >> 16) & 0xFFFF)
            else:
                raise wn_ex.ParameterError("TRANSPORT_TYPE", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_HW_ADDR):
            if (length == 2):
                self.mac_address = ((2**32) * (values[0] & 0xFFFF) + values[1])
            else:
                raise wn_ex.ParameterError("TRANSPORT_HW_ADDR", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_IP_ADDR):
            if (length == 1):
                self.ip_address = self.int_to_ip(values[0])
            else:
                raise wn_ex.ParameterError("TRANSPORT_IP_ADDR", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_UNICAST_PORT):
            if (length == 1):
                self.unicast_port = values[0]
            else:
                raise wn_ex.ParameterError("TRANSPORT_UNICAST_PORT", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_BCAST_PORT):
            if (length == 1):
                self.bcast_port = values[0]
            else:
                raise wn_ex.ParameterError("TRANSPORT_BCAST_PORT", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_GRP_ID):
            if (length == 1):
                self.group_id = values[0]
            else:
                raise wn_ex.ParameterError("TRANSPORT_GRP_ID", "Incorrect length")
                
        else:
            raise wn_ex.ParameterError(identifier, "Unknown transport parameter")


    # -------------------------------------------------------------------------
    # Misc methods for the Transport
    # -------------------------------------------------------------------------
    def int_to_ip(self, ip_address):    return int_to_ip(ip_address)

    def ip_to_int(self, ip_address):    return ip_to_int(ip_address)

    def mac_addr_to_str(self, mac_address):  return mac_addr_to_str(mac_address)

    def __str__(self):
        """Pretty print the Transport parameters"""
        msg = ""
        if self.mac_address is not None:
            msg += "{0} Transport ({1}):\n".format(self.node_eth_device, transport_type_to_str(self.transport_type))
            msg += "    IP address    :  {0}\n".format(self.ip_address)
            msg += "    MAC address   :  {0}\n".format(self.mac_addr_to_str(self.mac_address)) 
            msg += "    Max payload   :  {0}\n".format(self.max_payload)
            msg += "    Unicast port  :  {0}\n".format(self.unicast_port)
            msg += "    Broadcast port:  {0}\n".format(self.bcast_port)
            msg += "    Timeout       :  {0}\n".format(self.timeout)
            msg += "    Rx Buffer Size:  {0}\n".format(self.rx_buffer_size)
            msg += "    Tx Buffer Size:  {0}\n".format(self.tx_buffer_size)
            msg += "    Group ID      :  {0}\n".format(self.group_id)
        return msg
        
# End Class



# -------------------------------------------------------------------------
# Global methods for the Transport
# -------------------------------------------------------------------------

def int_to_ip(ip_address):
    """Convert an integer to IP address string (dotted notation)."""
    msg = ""
    if ip_address is not None:
        msg += "{0:d}.".format((ip_address >> 24) & 0xFF)
        msg += "{0:d}.".format((ip_address >> 16) & 0xFF)
        msg += "{0:d}.".format((ip_address >>  8) & 0xFF)
        msg += "{0:d}".format(ip_address & 0xFF)
    return msg

# End def


def ip_to_int(ip_address):
    """Convert IP address string (dotted notation) to an integer."""
    ret_val = 0
    if ip_address is not None:
        expr = re.compile('\.')
        tmp = [int(n) for n in expr.split(ip_address)]        
        ret_val = (tmp[3]) + (tmp[2] * 2**8) + (tmp[1] * 2**16) + (tmp[0] * 2**24)
    return ret_val

# End def


def mac_addr_to_str(mac_address):
    """Convert an integer to a colon separated MAC address string."""
    msg = ""
    if mac_address is not None:

        # Force the input address to a Python int
        #   This handles the case of a numpy uint64 input, which kills the >> operator
        if(type(mac_address) is not int):
            mac_address = int(mac_address)

        msg += "{0:02x}:".format((mac_address >> 40) & 0xFF)
        msg += "{0:02x}:".format((mac_address >> 32) & 0xFF)
        msg += "{0:02x}:".format((mac_address >> 24) & 0xFF)
        msg += "{0:02x}:".format((mac_address >> 16) & 0xFF)
        msg += "{0:02x}:".format((mac_address >>  8) & 0xFF)
        msg += "{0:02x}".format(mac_address & 0xFF)

    return msg

# End def


def transport_type_to_str(transport_type):
    if (transport_type == tp.TRANSPORT_TYPE_UDP):
        return "UDP"
    else:
        return "Type Undefined"
    
# End def


def node_eth_device_to_str(eth_device):
    if (eth_device == 0):
        return 'ETH A'
    elif (eth_device == 1):
        return 'ETH B'
    else:
        return 'ETH Undefined'

# End def

