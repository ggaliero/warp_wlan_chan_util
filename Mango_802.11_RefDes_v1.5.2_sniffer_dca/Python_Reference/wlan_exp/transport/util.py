# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Transport Utilities
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides transport utility commands.

Functions (see below for more information):
    init_nodes()                  -- Initialize nodes
    identify_all_nodes()          -- Send the 'identify' command to all nodes
    reset_network_inf_all_nodes() -- Reset the network interface for all nodes
    get_serial_number()           -- Standard way to check / process serial numbers

"""

import sys
import re

from . import exception as ex

__all__ = ['init_nodes', 'identify_all_nodes', 'reset_network_inf_all_nodes', 
           'get_serial_number']


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input


# -----------------------------------------------------------------------------
# Node Utilities
# -----------------------------------------------------------------------------

def init_nodes(nodes_config, network_config=None, node_factory=None, 
               network_reset=True, output=False):
    """Initalize nodes.

    Args:    
        nodes_config   (NodesConfiguration):   Describes the node configuration
        network_config (NetworkConfiguration): Describes the network configuration
        node_factory   (NodeFactory):          Factory to create nodes of a given node type
        network_reset  (bool):                 Issue a network reset to all the nodes 
        output         (bool):                 Print output about the nodes
    
    Returns:
        nodes (list of WlanExpNodes):  List of initialized nodes.
    """
    nodes       = []
    error_nodes = []

    # Create a Network Configuration if there is none provided
    if network_config is None:
        from . import config
        network_config = config.NetworkConfiguration()

    host_id             = network_config.get_param('host_id')
    jumbo_frame_support = network_config.get_param('jumbo_frame_support')
    
    # Process the config to create nodes
    nodes_dict = nodes_config.get_nodes_dict()

    # If node_factory is not defined, create a default NodeFactory
    if node_factory is None:
        from . import node
        node_factory = node.WarpNodeFactory(network_config)

    if network_reset:
        # Send a broadcast network reset command to make sure all nodes are
        # in their default state.
        reset_network_inf_all_nodes(network_config=network_config)

    # Create the nodes in the dictionary
    for node_dict in nodes_dict:
        if (host_id == node_dict['node_id']):
            msg = "Host id is set to {0} and must be unique.".format(host_id)
            raise ex.ConfigError(msg)

        # Set up the node_factory from the node dictionary
        node_factory.setup(node_dict)

        # Create the correct type of node; will return None and print a 
        #   warning if the node is not recognized
        node = node_factory.create_node(network_config, network_reset)

        if node is not None:
            node.configure_node(jumbo_frame_support)
            nodes.append(node)
        else:
            error_nodes.append(node_dict)

    if (len(nodes) != len(nodes_dict)):
        msg  = "\n\nERROR:  Was not able to initialize all nodes.  The following \n"
        msg += "nodes were not able to be initialized:\n"
        for node_dict in error_nodes:
            (_, sn_str) = get_serial_number(node_dict['serial_number'], output=False)
            msg += "    {0}\n".format(sn_str)
        raise ex.ConfigError(msg)

    if output:
        print("-" * 50)
        print("Initialized Nodes:")
        print("-" * 50)
        for node in nodes:
            print(node)
            print("-" * 30)
        print("-" * 50)

    return nodes

# End def


def identify_all_nodes(network_config):
    """Issue a broadcast command to all nodes to blink their LEDs for 10 seconds.
    
    Args:
        network_config (list of NetworkConfiguration):  One or more network 
            configurations
    """
    import time
    from . import cmds
    from . import transport_eth_ip_udp_py_broadcast as tp_broadcast

    if type(network_config) is list:
        my_network_config = network_config
    else:
        my_network_config = [network_config]

    for network in my_network_config:

        network_addr = network.get_param('network')
        tx_buf_size  = network.get_param('tx_buffer_size')
        rx_buf_size  = network.get_param('rx_buffer_size')
    
        msg  = "Identifying all nodes on network {0}.  ".format(network_addr)
        msg += "Please check the LEDs."
        print(msg)

        transport = tp_broadcast.TransportEthIpUdpPyBroadcast(network_config=network)

        transport.transport_open(tx_buf_size, rx_buf_size)

        cmd = cmds.NodeIdentify(cmds.CMD_PARAM_NODE_IDENTIFY_ALL_NODES)
        payload = cmd.serialize()
        transport.send(payload=payload)
        
        # Wait IDENTIFY_WAIT_TIME seconds for blink to complete since 
        #   broadcast commands cannot wait for a response.
        time.sleep(cmds.CMD_PARAM_NODE_IDENTIFY_NUM_BLINKS * cmds.CMD_PARAM_NODE_IDENTIFY_BLINK_TIME)
        
        transport.transport_close()

# End def


def reset_network_inf_all_nodes(network_config):
    """Reset the wlan_exp network interface of all nodes.

    This will issue a broadcast command to all nodes on each network to reset 
    the network interface informaiton (ie IP address, ports, etc.)
    
    Args:
        network_config (list of NetworkConfiguration):  One or more network 
            configurations

    """
    from . import cmds
    from . import transport_eth_ip_udp_py_broadcast as tp_broadcast

    if type(network_config) is list:
        my_network_config = network_config
    else:
        my_network_config = [network_config]

    for network in my_network_config:

        network_addr = network.get_param('network')
        tx_buf_size  = network.get_param('tx_buffer_size')
        rx_buf_size  = network.get_param('rx_buffer_size')
    
        msg  = "Resetting the network config for all nodes on network {0}.".format(network_addr)
        print(msg)
    
        transport = tp_broadcast.TransportEthIpUdpPyBroadcast(network_config=network)

        transport.transport_open(tx_buf_size, rx_buf_size)

        cmd = cmds.NodeResetNetwork(cmds.CMD_PARAM_NODE_NETWORK_RESET_ALL_NODES)
        payload = cmd.serialize()
        transport.send(payload=payload)
        
        transport.transport_close()

# End def


# -----------------------------------------------------------------------------
# Misc Utilities
# -----------------------------------------------------------------------------
def get_serial_number(serial_number, output=True):
    """Check / convert the provided serial number to a compatible format.
    
    Acceptable inputs:
        'W3-a-00001' or 'w3-a-00001' -- Succeeds quietly
        '00001' or '1' or 1          -- Succeeds with a warning
    
    Args:
        serial_number (str, int):  Serial number
        output (bool):  Should the function print any output warnings
    
    Returns:
        serial_number (tuple):  (serial_number, serial_number_str)
    """
    max_sn        = 100000
    sn_valid      = True
    ret_val       = (None, None)
    print_warning = False    
    
    
    expr = re.compile('((?P<prefix>[Ww]3-a-)|)(?P<sn>\d+)')
    m    = expr.match(str(serial_number))
    
    try:
        if m:
            sn = int(m.group('sn'))
            
            if (sn < max_sn):
                sn_str  = "W3-a-{0:05d}".format(sn)            
                
                if not m.group('prefix'):
                    print_warning = True            
                
                ret_val = (sn, sn_str)
            else:
                raise ValueError
        
        else:
            sn = int(serial_number)
            
            if (sn < max_sn):
                sn_str        = "W3-a-{0:05d}".format(sn)
                print_warning = True
                ret_val       = (sn, sn_str)
            else:
                raise ValueError
    
    except ValueError:
        sn_valid = False
    
    if not sn_valid:
        msg  = "Incorrect serial number: {0}\n".format(serial_number)
        msg += "Requires one of:\n"
        msg += "    - A string of the form: 'W3-a-XXXXX'\n"
        msg += "    - An integer that is less than 5 digits.\n"
        raise TypeError(msg)    
    
    if print_warning and output:
        msg  = "Interpreting provided serial number as "
        msg += "{0}".format(sn_str)
        print(msg)
    
    return ret_val

# End of def


def mac_addr_to_str(mac_address):
    """Convert an integer to a colon separated MAC address string."""
    from . import transport_eth_ip_udp as tp
    return tp.mac_addr_to_str(mac_address)

# End def



# -----------------------------------------------------------------------------
# Internal Methods
# -----------------------------------------------------------------------------

def _get_all_host_ip_addrs():
    """Get all host interface IP addresses."""
    import socket

    addrs = []

    # Get all the host address information
    addr_info = socket.getaddrinfo(socket.gethostname(), None)

    # Add addresses that are IPv6 and support SOCK_DGRAM or everything (0)
    #   to the list of host IP addresses
    for info in addr_info:
        if (info[0] == socket.AF_INET) and (info[1] in [0, socket.SOCK_DGRAM]):
            addrs.append(info[4][0])

    if (len(addrs) == 0):
        msg  = "WARNING: Could not find any valid interface IP addresses for host {0}\n".format(socket.gethostname())
        msg += "    Please check your network settings."
        print(msg)

    return addrs

# End def


def _get_host_ip_addr_for_network(network):
    """Get the host IP address for the given network."""
    import socket

    # Get the broadcast address of the network
    broadcast_addr = network.get_param('broadcast_address')
    
    try:
        # Create a temporary UDP socket to get the hostname
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        s.connect((broadcast_addr, 1))
        socket_name = s.getsockname()[0]
        s.close()
    except (socket.gaierror, socket.error):
        msg  = "WARNING: Could not get host IP for network {0}.".format(network.get_param('network'))
        print(msg)
        socket_name = ''

    return socket_name

# End def


def _check_network_interface(network, quiet=False):
    """Check that this network has a valid interface IP address."""
    import socket

    # Get the first three octets of the network address
    network_ip_subnet = _get_ip_address_subnet(network)
    test_ip_addr      = network_ip_subnet + ".255"
    network_addr      = network_ip_subnet + ".0"

    try:
        # Create a temporary UDP socket to get the hostname
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        s.connect((test_ip_addr, 1))
        socket_name = s.getsockname()[0]
        s.close()
    except (socket.gaierror, socket.error):
        msg  = "WARNING: Could not create temporary UDP socket.\n"
        msg += "  {0} may not work as Network address.".format(network_addr)
        print(msg)
        

    # Get the subnet of the socket
    sock_ip_subnet = _get_ip_address_subnet(socket_name)

    # Check that the socket_ip_subnet is equal to the host_ip_subnet
    if ((network != network_addr) and not quiet):
        msg  = "WARNING: Network address must be of the form 'X.Y.Z.0'.\n"
        msg += "  Provided {0}.  Using {1} instead.".format(network, network_addr)
        print(msg)
    
    # Check that the socket_ip_subnet is equal to the host_ip_subnet
    if ((network_ip_subnet != sock_ip_subnet) and not quiet):
        msg  = "WARNING: Interface IP address {0} and ".format(socket_name)
        msg += "network {0} appear to be on different subnets.\n".format(network)
        msg += "    Please check your network settings if this in not intentional."
        print(msg)

    return network_addr

# End def


def _get_ip_address_subnet(ip_address):
    """Get the subnet X.Y.Z of ip_address X.Y.Z.W"""
    expr = re.compile('\.')
    tmp = [int(n) for n in expr.split(ip_address)]
    return "{0:d}.{1:d}.{2:d}".format(tmp[0], tmp[1], tmp[2])
    
# End def


def _get_broadcast_address(ip_address):
    """Get the broadcast address X.Y.Z.255 for ip_address X.Y.Z.W"""
    ip_subnet      = _get_ip_address_subnet(ip_address)
    broadcast_addr = ip_subnet + ".255"
    return broadcast_addr
    
# End def
    

def _check_ip_address_format(ip_address):
    """Check that the string has a valid IP address format."""
    expr = re.compile('\d+\.\d+\.\d+\.\d+')

    if not expr.match(ip_address) is None:
        return True
    else:
        import warnings
        msg  = "\n'{0}' is not a valid IP address format.\n" .format(ip_address) 
        msg += "    Please use A.B.C.D where A, B, C and D are in [0, 254]"
        warnings.warn(msg)
        return False

# End def


def _check_host_id(host_id):
    """Check that the host_id is valid."""
    if (host_id >= 200) and (host_id <= 254):
        return True
    else:
        import warnings
        msg  = "\n'{0}' is not a valid Host ID.\n" .format(host_id) 
        msg += "    Valid Host IDs are integers in the range of [200,254]"
        warnings.warn(msg)
        return False

# End def


def _get_os_socket_buffer_size(req_tx_buf_size, req_rx_buf_size):
    """Get the size of the send and receive buffers from the OS given the 
    requested TX/RX buffer sizes.
    """
    import errno
    import socket
    from socket import error as socket_error
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM);

    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, req_tx_buf_size)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, req_rx_buf_size)
    except socket_error as serr:
        # Cannot set the buffer size on some hardware
        if serr.errno != errno.ENOBUFS:
            raise serr

    sock_tx_buf_size = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
    sock_rx_buf_size = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    
    return (sock_tx_buf_size, sock_rx_buf_size)

# End def
    



