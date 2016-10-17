# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Utilities
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

This module provides WARPNet utility commands.

Functions (see below for more information):
    wn_init_nodes()                  -- Initialize nodes
    wn_identify_all_nodes()          -- Send the 'identify' command to all nodes
    wn_reset_network_inf_all_nodes() -- Reset the network interface for all nodes
    wn_get_serial_number()           -- Standard way to check / process serial numbers

"""

import sys
import re

from . import exception as wn_ex

__all__ = ['wn_init_nodes', 'wn_identify_all_nodes', 'wn_reset_network_inf_all_nodes', 
           'wn_get_serial_number']


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input


# -----------------------------------------------------------------------------
# WARPNet Node Utilities
# -----------------------------------------------------------------------------

def wn_init_nodes(nodes_config, network_config=None, node_factory=None, 
                  network_reset=True, output=False):
    """Initalize WARPNet nodes.

    Attributes:    
        nodes_config   -- A NodesConfiguration object describing the nodes
        network_config -- A NetworkConfiguration object describing the network configuration
        node_factory   -- A NodeFactory or subclass to create nodes of a given WARPNet type
        network_reset  -- Issue a network reset to all the nodes 
        output         -- Print output about the WARPNet nodes
    """
    nodes       = []
    error_nodes = []

    # Create a Network Configuration if there is none provided
    if network_config is None:
        from . import config as wn_config
        network_config = wn_config.NetworkConfiguration()

    host_id             = network_config.get_param('host_id')
    jumbo_frame_support = network_config.get_param('jumbo_frame_support')
    
    # Process the config to create nodes
    nodes_dict = nodes_config.get_nodes_dict()

    # If node_factory is not defined, create a default WnNodeFactory
    if node_factory is None:
        from . import node as wn_node
        node_factory = wn_node.WnNodeFactory(network_config)

    if network_reset:
        # Send a broadcast network reset command to make sure all nodes are
        # in their default state.
        wn_reset_network_inf_all_nodes(network_config=network_config)

    # Create the nodes in the dictionary
    for node_dict in nodes_dict:
        if (host_id == node_dict['node_id']):
            msg = "Host id is set to {0} and must be unique.".format(host_id)
            raise wn_ex.ConfigError(msg)

        # Set up the node_factory from the Node dictionary
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
            (_, sn_str) = wn_get_serial_number(node_dict['serial_number'], output=False)
            msg += "    {0}\n".format(sn_str)
        raise wn_ex.ConfigError(msg)

    if output:
        print("-" * 50)
        print("Initialized Nodes:")
        print("-" * 50)
        for node in nodes:
            print(node)
            print("-" * 30)
        print("-" * 50)

    return nodes

# End of wn_init_nodes()


def wn_identify_all_nodes(network_config):
    """Issues a broadcast WARPNet command: Identify.

    Attributes:
      network_config  - One or more Network configurations

    All nodes should blink their Red LEDs for 10 seconds.    
    """
    import time
    from . import cmds as wn_cmds
    from . import transport_eth_udp_py_bcast as tp_bcast

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

        transport = tp_bcast.TransportEthUdpPyBcast(network_config=network)

        transport.wn_open(tx_buf_size, rx_buf_size)

        cmd = wn_cmds.NodeIdentify(wn_cmds.CMD_PARAM_IDENTIFY_ALL_NODES)
        payload = cmd.serialize()
        transport.send(payload=payload)
        
        # Wait IDENTIFY_WAIT_TIME seconds for blink to complete since 
        #   broadcast commands cannot wait for a response.
        time.sleep(wn_cmds.CMD_PARAM_IDENTIFY_NUM_BLINKS * wn_cmds.CMD_PARAM_IDENTIFY_BLINK_TIME)
        
        transport.wn_close()

# End of wn_identify_all_nodes()


def wn_reset_network_inf_all_nodes(network_config):
    """Issues a broadcast WARPNet command: NodeResetNetwork.

    Attributes:
      network_config  - One or more Network configurations

    This will issue a broadcast network interface reset for all nodes on 
    each of the networks.
    """
    from . import cmds as wn_cmds
    from . import transport_eth_udp_py_bcast as tp_bcast

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
    
        transport = tp_bcast.TransportEthUdpPyBcast(network_config=network)

        transport.wn_open(tx_buf_size, rx_buf_size)

        cmd = wn_cmds.NodeResetNetwork(wn_cmds.CMD_PARAM_NETWORK_RESET_ALL_NODES)
        payload = cmd.serialize()
        transport.send(payload=payload)
        
        transport.wn_close()

# End of wn_identify_all_nodes()


# -----------------------------------------------------------------------------
# WARPNet Misc Utilities
# -----------------------------------------------------------------------------
def wn_get_serial_number(serial_number, output=True):
    """Function will check / convert the provided serial number to a WARPNet
    compatible format.
    
    Acceptable inputs:
        'W3-a-00001' or 'w3-a-00001' -- Succeeds quietly
        '00001' or '1' or 1          -- Succeeds with a warning
    
    Returns:
        Tuple:  (serial_number, serial_number_str)
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
    from warpnet.transport_eth_udp import mac_addr_to_str
    return mac_addr_to_str(mac_address)

# End def



# -----------------------------------------------------------------------------
# WARPNet Setup Utilities
# -----------------------------------------------------------------------------
def wn_nodes_setup(ini_file=None):
    """Create / Modify WARPNet Nodes ini file from user input."""    
    nodes_config = None

    print("-" * 70)
    print("WARPNet Nodes Setup:")
    print("   NOTE:  Many values are populated automatically to ease setup.  Excessive")
    print("          editing using this menu can result in the 'Current Nodes' displayed") 
    print("          being out of sync with the final nodes configuration.  The final")
    print("          nodes configuration will be printed on exit.  Please check that")
    print("          and re-run WARPNet Nodes Setup if necessary or manually edit the")
    print("          nodes_config ini file.")
    print("-" * 70)

    # base_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    # config_file = os.path.normpath(os.path.join(base_dir, "../", "nodes_config.ini"))

    from . import config as wn_config
    nodes_config = wn_config.NodesConfiguration(ini_file=ini_file)

    # Actions Menu    
    actions = {0 : "Add node",
               1 : "Remove node",
               2 : "Change Node ID",
               3 : "Disable / Enable node",
               4 : "Change IP address",
               5 : "Quit (default)"}

    nodes_done = False

    # Process actions until either a Ctrl-C or a quit action
    try:
        while not nodes_done:
            nodes = nodes_config.print_nodes()
            
            print("Actions:")
            for idx in range(len(actions.keys())):
                print("    [{0}] {1}".format(idx, actions[idx]))
            
            action = _select_from_table("Selection : ", actions)
            print("-" * 50)
            
            if (action is None) or (action == 5):
                nodes_done = True
            elif (action == 0):
                serial_num = _add_node_from_user()
                if serial_num is not None:
                    nodes_config.add_node(serial_num)
                    print("    Adding node: {0}".format(serial_num))
            elif (action == 1):
                node = _select_from_table("Select node : ", nodes)
                if node is not None:
                    nodes_config.remove_node(nodes[node])
                    print("    Removing node: {0}".format(nodes[node]))
            elif (action == 2):
                node = _select_from_table("Select node : ", nodes)
                if node is not None:
                    tmp_node_id = _get_node_id_from_user(nodes[node])
                    if tmp_node_id is not None:
                        nodes_config.set_param(nodes[node], 'node_id', tmp_node_id)
                        print("    Setting node: {0} to ID {1}".format(nodes[node], tmp_node_id))
                    else:
                        nodes_config.remove_param(nodes[node], 'node_id')
                        print("    Setting node: {0} to IP address {1}".format(nodes[node], 
                                nodes_config.get_param(nodes[node], 'node_id')))
            elif (action == 3):
                node = _select_from_table("Select node : ", nodes)
                if node is not None:
                    if (nodes_config.get_param(nodes[node], 'use_node')):
                        nodes_config.set_param(nodes[node], 'use_node', False)
                        print("    Disabling node: {0}".format(nodes[node]))
                    else:
                        nodes_config.set_param(nodes[node], 'use_node', True)
                        print("    Enabling node: {0}".format(nodes[node]))
            elif (action == 4):
                node = _select_from_table("Select node : ", nodes)
                if node is not None:
                    ip_addr = _get_ip_address_from_user(nodes[node])
                    if ip_addr is not None:
                        nodes_config.set_param(nodes[node], 'ip_address', ip_addr)
                        print("    Setting node: {0} to IP address {1}".format(nodes[node], ip_addr))
                    else:
                        nodes_config.remove_param(nodes[node], 'ip_address')
                        print("    Setting node: {0} to IP address {1}".format(nodes[node], 
                                nodes_config.get_param(nodes[node], 'ip_address')))

            print("-" * 50)
            
    except KeyboardInterrupt:
        pass

    nodes_config.save_config(output=True)
    print("-" * 70)
    print("Final Nodes Configuration:")
    print("-" * 70)

    # Print final configuration
    nodes_config = wn_config.NodesConfiguration(ini_file=ini_file)
    nodes_config.print_nodes()

    print("-" * 70)
    print("Nodes Configuration Complete.")
    print("-" * 70)
    print("\n")

# End of wn_nodes_setup()



# -----------------------------------------------------------------------------
# Internal Methods
# -----------------------------------------------------------------------------

def _add_node_from_user():
    """Internal method to add a node based on info from the user."""
    serial_num = None
    serial_num_done = False

    while not serial_num_done:
        temp = raw_input("WARP Node Serial Number (last 5 digits or enter to end): ")
        if temp is not '':
            serial_num = "W3-a-{0:05d}".format(int(temp))
            message = "    Is {0} Correct? [Y/n]: ".format(serial_num)
            confirmation = _get_confirmation_from_user(message)
            if (confirmation == 'y'):
                serial_num_done = True
            else:
                serial_num = None
        else:
            break

    return serial_num

# End of _add_node_from_user()


def _get_node_id_from_user(msg):
    """Internal method to change a node's ID based on info from the user."""
    node_id = None
    node_id_done = False

    while not node_id_done:
        print("-" * 30)
        temp = raw_input("Enter ID for node {0}: ".format(msg))
        if temp is not '':
            if (int(temp) < 254) and (int(temp) >= 0):
                print("    Setting Node ID to {0}".format(temp))
                node_id = int(temp)
                node_id_done = True
            else:
                print("    '{0}' is not a valid Node ID.".format(temp))
                print("    Valid Node IDs are integers in the range of [0,254).")
        else:
            break
    
    return node_id

# End of _get_ip_address_from_user()


def _get_ip_address_from_user(msg):
    """Internal method to change a node's IP address based on info from 
    the user.
    """
    ip_address = None
    ip_address_done = False

    while not ip_address_done:
        print("-" * 30)
        temp = raw_input("Enter IP address for {0}: ".format(msg))
        if temp is not '':
            if _check_ip_address_format(temp):
                ip_address = temp
                ip_address_done = True
        else:
            break
    
    return ip_address

# End of _get_ip_address_from_user()


def _select_from_table(select_msg, table):
    return_val = None
    selection_done = False

    while not selection_done:
        temp = raw_input(select_msg)
        if temp is not '':
            if (not int(temp) in table.keys()):
                print("{0} is an invalid Selection.  Please choose again.".format(temp))
            else:
                return_val = int(temp)
                selection_done = True            
        else:
            selection_done = True

    return return_val

# End of _select_from_table()


def _get_confirmation_from_user(message):
    """Get confirmation from the user.  Default return value is 'y'."""
    confirmation = False
    
    while not confirmation:            
        selection = raw_input(message).lower()
        if selection is not '':
            if (selection == 'y') or (selection == 'n'):
                confirmation = True
            else:
                print("    '{0}' is not a valid selection.".format(selection))
                print("    Please select [y] or [n].")
        else:
            selection = 'y'
            confirmation = True
    
    return selection

# End of _get_confirmation_from_user()


def _get_all_host_ip_addrs():
    """Get all host interface IP addresses."""
    import socket

    addrs = []

    # Get all the host address information
    addr_info = socket.getaddrinfo(socket.gethostname(), None)

    # For addresses that are IPv6 and support SOCK_DGRAM or everything
    #   we need to add them to the list of host IP addresses
    for info in addr_info:
        if (info[0] == socket.AF_INET) and (info[1] in [0, socket.SOCK_DGRAM]):
            addrs.append(info[4][0])

    if (len(addrs) == 0):
        msg  = "WARNING: Could not find any valid interface IP addresses for host {0}\n".format(socket.gethostname())
        msg += "    Please check your network settings."
        print(msg)

    return addrs

# End of _get_all_host_ip_addrs()


def _get_host_ip_addr_for_network(network):
    """Get the host IP address for the given network."""
    import socket

    # Get the broadcast address of the network
    bcast_addr = network.get_param('bcast_address')
    
    try:
        # Create a temporary UDP socket to get the hostname
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        s.connect((bcast_addr, 1))
        socket_name = s.getsockname()[0]
        s.close()
    except (socket.gaierror, socket.error):
        msg  = "WARNING: Could not get host IP for network {0}.".format(network.get_param('network'))
        print(msg)
        socket_name = ''

    return socket_name

# End of _get_host_ip_addr_for_network()


def _check_network_interface(network, quiet=False):
    """Check that this is a network has a valid interface IP address."""
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

# End of _check_network_interface()


def _get_ip_address_subnet(ip_address):
    """Get the subnet X.Y.Z of ip_address X.Y.Z.W"""
    expr = re.compile('\.')
    tmp = [int(n) for n in expr.split(ip_address)]
    return "{0:d}.{1:d}.{2:d}".format(tmp[0], tmp[1], tmp[2])
    
# End of _get_ip_address_subnet()


def _get_bcast_address(ip_address):
    """Get the broadcast address X.Y.Z.255 for ip_address X.Y.Z.W"""
    ip_subnet  = _get_ip_address_subnet(ip_address)
    bcast_addr = ip_subnet + ".255"
    return bcast_addr
    
# End of _get_bcast_address()
    

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

# End of _check_ip_address_format()


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

# End of _check_host_id()


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
        # On some HW we cannot set the buffer size
        if serr.errno != errno.ENOBUFS:
            raise serr

    sock_tx_buf_size = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
    sock_rx_buf_size = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    
    return (sock_tx_buf_size, sock_rx_buf_size)

# End of _get_os_socket_buffer_size()
    



