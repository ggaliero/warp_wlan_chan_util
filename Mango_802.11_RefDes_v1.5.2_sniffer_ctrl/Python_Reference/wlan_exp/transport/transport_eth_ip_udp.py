# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework 
    - Transport Ethernet IP/UDP base class
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides the base class for Ethernet IP/UDP transports.

Functions:
    TransportEthUdp() -- Base class for Ethernet UDP transports
    int_to_ip()       -- Convert 32 bit integer to 'w.x.y.z' IP address string
    ip_to_int()       -- Convert 'w.x.y.z' IP address string to 32 bit integer
    mac_addr_to_str() -- Convert 6 byte MAC address to 'uu:vv:ww:xx:yy:zz' string

"""

import re
import time
import errno
import socket
from socket import error as socket_error

from . import message
from . import exception as ex
from . import transport as tp


__all__ = ['TransportEthIpUdp']


class TransportEthIpUdp(tp.Transport):
    """Base Class for Ethernet IP/UDP Transport class.
       
    Attributes:
        timeout        -- Maximum time spent waiting before retransmission
        transport_type -- Unique type of the transport
        sock           -- UDP socket
        status         -- Status of the UDP socket
        max_payload    -- Maximum payload size (eg. MTU - ETH/IP/UDP headers)
        
        mac_address    -- Node's MAC address
        ip_address     -- IP address of destination
        unicast_port   -- Unicast port of destination
        broadcast_port -- Broadcast port of destination
        group_id       -- Group ID of the node attached to the transport
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
    broadcast_port  = None
    group_id        = None
    rx_buffer_size  = None
    tx_buffer_size  = None
    
    def __init__(self):
        self.hdr = message.TransportHeader()
        self.status = 0
        self.timeout = 1
        self.max_payload = 1000   # Sane default.  Overwritten by payload test.
        
        self.check_setup()

    def __del__(self):
        """Closes any open sockets"""
        self.transport_close()

    def __repr__(self):
        """Return transport IP address and Ethernet MAC address"""
        msg  = "Eth UDP Transport: {0} ".format(self.ip_address)
        msg += "({0})".format(self.mac_address)
        return msg

    def check_setup(self):
        """Check the setup of the transport."""
        pass

    def set_max_payload(self, value):    self.max_payload = value
    def set_ip_address(self, value):     self.ip_address = value
    def set_mac_address(self, value):    self.mac_address = value
    def set_unicast_port(self, value):   self.unicast_port = value
    def set_broadcast_port(self, value): self.broadcast_port = value
    def set_src_id(self, value):         self.hdr.set_src_id(value)
    def set_dest_id(self, value):        self.hdr.set_dest_id(value)

    def get_max_payload(self):           return self.max_payload
    def get_ip_address(self):            return self.ip_address
    def get_mac_address(self):           return self.mac_address
    def get_unicast_port(self):          return self.unicast_port
    def get_broadcast_port(self):        return self.broadcast_port
    def get_src_id(self):                return self.hdr.get_src_id()
    def get_dest_id(self):               return self.hdr.get_dest_id()


    def transport_open(self, tx_buf_size=None, rx_buf_size=None):
        """Opens an Ethernet IP/UDP socket.""" 
        self.sock = socket.socket(socket.AF_INET,       # Internet
                                  socket.SOCK_DGRAM);   # UDP
        
        self.sock.settimeout(self.timeout)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        if tx_buf_size is not None:
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, tx_buf_size)
            except socket_error as serr:
                # Cannot set the buffer size on some hardware
                if serr.errno != errno.ENOBUFS:
                    raise serr

        if rx_buf_size is not None:
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, rx_buf_size)
            except socket_error as serr:
                # Cannot set the buffer size on some hardware
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


    def transport_close(self):
        """Closes an Ethernet IP/UDP socket."""
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
    # Commands for the Transport
    # -------------------------------------------------------------------------
    def ping(self, node, output=False):
        """Issues a ping command to the given node.
        
        Will optionally print the result of the ping.
        """
        from . import cmds

        start_time = time.clock()
        node.send_cmd(cmds.TransportPing())
        end_time = time.clock()
        
        if output:
            ms_time = (end_time - start_time) * 1000
            print("Reply from {0}:  time = {1:.3f} ms".format(self.ip_address, 
                                                              ms_time))

    
    def test_payload_size(self, node, jumbo_frame_support=False):
        """Determines the object's max_payload parameter."""
        from . import cmds

        if jumbo_frame_support:
            payload_test_sizes = [1000, 1470, 5000, 8966]
        else:
            payload_test_sizes = [1000, 1470]
        
        for size in payload_test_sizes:
            cmd_size    = message.Cmd().sizeof()
            my_arg_size = (size - (self.hdr.sizeof() + cmd_size + 4)) // 4
 
            try:
                resp = node.send_cmd(cmds.TransportTestPayloadSize(my_arg_size))
                self.set_max_payload(resp)
                
                if (self.get_max_payload() < (4*my_arg_size)):
                    # If the node received a smaller payload, then break
                    break
            except ex.TransportError:
                # If there was a transmission error, then break
                break

    
    def add_node_group_id(self, node, group):
        from . import cmds

        node.send_cmd(cmds.TransportAddNodeGroupId(group))

    
    def clear_node_group_id(self, node, group):
        from . import cmds

        node.send_cmd(cmds.TransportClearNodeGroupId(group))


    # -------------------------------------------------------------------------
    # Transport Parameter Framework
    #   Allows for processing of hardware parameters
    # -------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if   (identifier == tp.TRANSPORT_TYPE):
            if (length == 1):
                self.transport_type  = (values[0] & 0xFFFF)
                self.node_eth_device = node_eth_device_to_str((values[0] >> 16) & 0xFFFF)
            else:
                raise ex.ParameterError("TRANSPORT_TYPE", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_HW_ADDR):
            if (length == 2):
                self.mac_address = ((2**32) * (values[0] & 0xFFFF) + values[1])
            else:
                raise ex.ParameterError("TRANSPORT_HW_ADDR", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_IP_ADDR):
            if (length == 1):
                self.ip_address = self.int_to_ip(values[0])
            else:
                raise ex.ParameterError("TRANSPORT_IP_ADDR", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_GROUP_ID):
            if (length == 1):
                self.group_id = values[0]
            else:
                raise ex.ParameterError("TRANSPORT_GROUP_ID", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_UNICAST_PORT):
            if (length == 1):
                self.unicast_port = values[0]
            else:
                raise ex.ParameterError("TRANSPORT_UNICAST_PORT", "Incorrect length")
                
        elif (identifier == tp.TRANSPORT_BROADCAST_PORT):
            if (length == 1):
                self.broadcast_port = values[0]
            else:
                raise ex.ParameterError("TRANSPORT_BROADCAST_PORT", "Incorrect length")
                
        else:
            raise ex.ParameterError(identifier, "Unknown transport parameter")


    # -------------------------------------------------------------------------
    # Misc methods for the Transport
    # -------------------------------------------------------------------------
    def int_to_ip(self, ip_address):    return int_to_ip(ip_address)

    def ip_to_int(self, ip_address):    return ip_to_int(ip_address)

    def mac_addr_to_str(self, mac_address):  return mac_addr_to_str(mac_address)
        
    def str_to_mac_addr(self, mac_address):  return str_to_mac_addr(mac_address)

    def __str__(self):
        """Pretty print the Transport parameters"""
        msg = ""
        if self.mac_address is not None:
            msg += "{0} Transport ({1}):\n".format(self.node_eth_device, transport_type_to_str(self.transport_type))
            msg += "    IP address    :  {0}\n".format(self.ip_address)
            msg += "    MAC address   :  {0}\n".format(self.mac_addr_to_str(self.mac_address)) 
            msg += "    Max payload   :  {0}\n".format(self.max_payload)
            msg += "    Unicast port  :  {0}\n".format(self.unicast_port)
            msg += "    Broadcast port:  {0}\n".format(self.broadcast_port)
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
    ret_val = ""
    
    if ip_address is not None:
        ret_val += "{0:d}.".format((ip_address >> 24) & 0xFF)
        ret_val += "{0:d}.".format((ip_address >> 16) & 0xFF)
        ret_val += "{0:d}.".format((ip_address >>  8) & 0xFF)
        ret_val += "{0:d}".format(ip_address & 0xFF)
        
    return ret_val

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
    ret_val = ""
    
    if mac_address is not None:

        # Force the input address to a Python int
        #   This handles the case of a numpy uint64 input, which kills the >> operator
        if(type(mac_address) is not int):
            mac_address = int(mac_address)

        ret_val += "{0:02x}:".format((mac_address >> 40) & 0xFF)
        ret_val += "{0:02x}:".format((mac_address >> 32) & 0xFF)
        ret_val += "{0:02x}:".format((mac_address >> 24) & 0xFF)
        ret_val += "{0:02x}:".format((mac_address >> 16) & 0xFF)
        ret_val += "{0:02x}:".format((mac_address >>  8) & 0xFF)
        ret_val += "{0:02x}".format(mac_address & 0xFF)

    return ret_val

# End def


def str_to_mac_addr(mac_address):
    """Convert a MAC Address (colon separated) to an integer."""
    ret_val = 0
    
    if mac_address is not None:
        
        expr = re.compile(':')
        tmp = [int('0x{0}'.format(n), base=16) for n in expr.split(mac_address)]
        ret_val = (tmp[5]) + (tmp[4] * 2**8) + (tmp[3] * 2**16) + (tmp[2] * 2**24) + (tmp[1] * 2**32) + (tmp[0] * 2**40)

        # Alternate Method:
        #
        # ret_val = mac_address
        # ret_val = ''.join('{0:02X}:'.format(ord(x)) for x in ret_val)[:-1]
        # ret_val = '0x' + ret_val.replace(':', '')
        # mac_addr_int = int(ret_val, 0)
        
    return ret_val

# End def


def mac_addr_to_byte_str(mac_address):
    """Convert an integer to a MAC address byte string."""
    ret_val = b''
    
    if mac_address is not None:
        import sys
    
        # Fix to support Python 2.x and 3.x
        if sys.version[0]=="2":
            ret_val   = ""
            int_array = [((mac_address >> ((6 - i - 1) * 8)) % 256) for i in range(6)]
            for value in int_array:
                ret_val += "{0}".format(chr(value))
            ret_val = bytes(ret_val)
        elif sys.version[0]=="3":
            ret_val = bytes([((mac_address >> ((6 - i - 1) * 8)) % 256) for i in range(6)])
        else:
            msg  = "WARNING:  Cannot convert MAC Address to byte string:\n"
            msg += "    Unsupported Python version."
            print(msg)
        
    return ret_val

# End def


def byte_str_to_mac_addr(mac_address):
    """Convert a MAC Address (byte string) to an integer."""
    ret_val = 0
    
    if mac_address is not None:
        import sys
        
        # Fix to support Python 2.x and 3.x
        if sys.version[0]=="2":
            ret_val = sum([ord(b) << (8 * i) for i, b in enumerate(mac_address[::-1])])
        elif sys.version[0]=="3":
            ret_val = sum([b << (8 * i) for i, b in enumerate(mac_address[::-1])])
        else:
            msg  = "WARNING:  Cannot convert byte string to MAC Address:\n"
            msg += "    Unsupported Python version."
            print(msg)
        
    return ret_val

# End def


def buffer_to_str(buffer):
    """Convert buffer of bytes to a formatted string of byte values."""
    ret_val = "    "
    
    for i, byte in enumerate(buffer):
        try:
            ret_val += "0x{0:02X} ".format(ord(byte))
        except TypeError:
            ret_val += "0x{0:02X} ".format(byte)
            
        if (((i % 16) == 0) and (i != len(buffer))):
            ret_val += "\n    "
            
    ret_val += "\n"

    return ret_val
    
# End def


def transport_type_to_str(transport_type):
    if (transport_type == tp.TRANSPORT_TYPE_IP_UDP):
        return "IP/UDP"
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

