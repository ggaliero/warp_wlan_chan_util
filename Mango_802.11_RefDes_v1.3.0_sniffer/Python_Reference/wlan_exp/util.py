# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Utilities
.. ------------------------------------------------------------------------------
.. Authors:   Chris Hunter (chunter [at] mangocomm.com)
..            Patrick Murphy (murphpo [at] mangocomm.com)
..            Erik Welsh (welsh [at] mangocomm.com)
.. License:   Copyright 2014-2015, Mango Communications. All rights reserved.
..            Distributed under the WARP license (http://warpproject.org/license)
.. ------------------------------------------------------------------------------
.. MODIFICATION HISTORY:
..
.. Ver   Who  Date     Changes
.. ----- ---- -------- -----------------------------------------------------
.. 1.00a ejw  1/23/14  Initial release
.. ------------------------------------------------------------------------------

"""

import sys
import time

import wlan_exp.defaults as defaults


__all__ = ['init_nodes', 'broadcast_cmd_set_time', 'broadcast_cmd_write_time_to_logs',
           'filter_nodes']



# -----------------------------------------------------------------------------
# WLAN Exp Node Print Levels
# -----------------------------------------------------------------------------

#   NOTE:  The C counterparts are found in wlan_exp_common.h
WLAN_EXP_PRINT_NONE               = 0
WLAN_EXP_PRINT_ERROR              = 1
WLAN_EXP_PRINT_WARNING            = 2
WLAN_EXP_PRINT_INFO               = 3
WLAN_EXP_PRINT_DEBUG              = 4



# -----------------------------------------------------------------------------
# WLAN Exp Rate definitions
# -----------------------------------------------------------------------------
wlan_rates = [{'index' :  0, 'rate' :  6.0, 'desc' : 'BPSK 1/2',   'NDBPS': 24},
              {'index' :  1, 'rate' :  9.0, 'desc' : 'BPSK 3/4',   'NDBPS': 36},
              {'index' :  2, 'rate' : 12.0, 'desc' : 'QPSK 1/2',   'NDBPS': 48},
              {'index' :  3, 'rate' : 18.0, 'desc' : 'QPSK 3/4',   'NDBPS': 72},
              {'index' :  4, 'rate' : 24.0, 'desc' : '16-QAM 1/2', 'NDBPS': 96},
              {'index' :  5, 'rate' : 36.0, 'desc' : '16-QAM 3/4', 'NDBPS': 144},
              {'index' :  6, 'rate' : 48.0, 'desc' : '64-QAM 2/3', 'NDBPS': 192},
              {'index' :  7, 'rate' : 54.0, 'desc' : '64-QAM 3/4', 'NDBPS': 216}]


def find_tx_rate_by_index(index):
    """Return the wlan_rates entry for the given index.
    
    Args:
        index (int):  Index into wlan_rate array.    
    
    Returns:
        rate (dict):  Rate dictionary corresponding to the index    
    """
    return _find_param_by_index(index, wlan_rates, 'tx rate')

# End def


def tx_rate_to_str(rate):
    """Convert a wlan_rates entry to a string.
    
    Args:
        rate (dict):  Rate dictionary 
    
    Returns:
        output (str):  String representation of the 'rate'
    """
    msg = ""
    if type(rate) is dict:
        msg += "{0:2.1f} Mbps ({1})".format(rate['rate'], rate['desc'])
    else:
        print("Invalid Tx rate type.  Needed dict, provided {0}.".format(type(rate)))
    return msg

# End def


def tx_rate_index_to_str(index):
    """Convert a wlan_rates entry index to a string.
    
    Args:
        index (int):  Index into wlan_rate array.
    
    Returns:
        output (str):  String representation of the 'rate' corresponding to the index
    """
    rate = find_tx_rate_by_index(index)
    return tx_rate_to_str(rate)

# End def



# -----------------------------------------------------------------------------
# WLAN Exp Channel definitions
# -----------------------------------------------------------------------------
wlan_channel = [{'index' :   1, 'channel' :   1, 'freq': 2412, 'desc' : '2.4 GHz Band'},
                {'index' :   2, 'channel' :   2, 'freq': 2417, 'desc' : '2.4 GHz Band'},
                {'index' :   3, 'channel' :   3, 'freq': 2422, 'desc' : '2.4 GHz Band'},
                {'index' :   4, 'channel' :   4, 'freq': 2427, 'desc' : '2.4 GHz Band'},
                {'index' :   5, 'channel' :   5, 'freq': 2432, 'desc' : '2.4 GHz Band'},
                {'index' :   6, 'channel' :   6, 'freq': 2437, 'desc' : '2.4 GHz Band'},
                {'index' :   7, 'channel' :   7, 'freq': 2442, 'desc' : '2.4 GHz Band'},
                {'index' :   8, 'channel' :   8, 'freq': 2447, 'desc' : '2.4 GHz Band'},
                {'index' :   9, 'channel' :   9, 'freq': 2452, 'desc' : '2.4 GHz Band'},
                {'index' :  10, 'channel' :  10, 'freq': 2457, 'desc' : '2.4 GHz Band'},
                {'index' :  11, 'channel' :  11, 'freq': 2462, 'desc' : '2.4 GHz Band'},
                {'index' :  36, 'channel' :  36, 'freq': 5180, 'desc' : '5 GHz Band'},
                {'index' :  40, 'channel' :  40, 'freq': 5200, 'desc' : '5 GHz Band'},
                {'index' :  44, 'channel' :  44, 'freq': 5220, 'desc' : '5 GHz Band'},
                {'index' :  48, 'channel' :  48, 'freq': 5240, 'desc' : '5 GHz Band'}]


def find_channel_by_index(index):
    """Return the wlan_channel entry for the given index.
    
    Args:
        index (int):  Index into wlan_rate array.    
    
    Returns:
        channel (dict):  Channel dictionary corresponding to the index    
    """
    return _find_param_by_index(index, wlan_channel, 'channel')

# End def


def find_channel_by_channel_number(channel_number):
    """Return the wlan_channel entry for the given channel number.
    
    Args:
        channel_number (int):  Number of 802.11 channel
            (https://en.wikipedia.org/wiki/List_of_WLAN_channels)
    
    Returns:
        channel (dict):  Channel dictionary corresponding to the channel number
    """
    return _find_param_by_name('channel', channel_number, wlan_channel, 'channel')

# End def


def find_channel_by_freq(freq):
    """Return the wlan_channel entry for the given frequency.

    Args:
        freq (int):  Frequency (in MHz) of the channel
            (https://en.wikipedia.org/wiki/List_of_WLAN_channels)
    
    Returns:
        channel (dict):  Channel dictionary corresponding to the frequency
    """
    return _find_param_by_name('freq', freq, wlan_channel, 'channel')

# End def


def channel_to_str(channel):
    """Convert a wlan_channel entry to a string.

    Args:
        channel (dict):  Channel dictionary 
    
    Returns:
        output (str):  String representation of the 'channel'    
    """
    msg = ""

    if type(channel) is int:
        tmp_chan = find_channel_by_channel_number(channel)
    elif type(channel) is dict:
        tmp_chan = channel
    else:
        print("Invalid channel type.  Needed int or dict, provided {0}.".format(type(channel)))
        return msg

    msg += "{0:4d} ({1} MHz)".format(tmp_chan['channel'], tmp_chan['freq'])

    return msg

# End def


def channel_freq_to_str(freq):
    """Convert a channel frequency to a channel string.

    Args:
        freq (int):  Frequency (in MHz) of the channel
            (https://en.wikipedia.org/wiki/List_of_WLAN_channels)
    
    Returns:
        output (str):  String representation of the channel corresponding to 'freq'
    """
    channel = find_channel_by_freq(freq)
    return channel_to_str(channel)

# End def



# -----------------------------------------------------------------------------
# WLAN Exp Antenna Mode definitions
# -----------------------------------------------------------------------------
wlan_rx_ant_mode = [{'index' :  0x00, 'desc' : 'Rx SISO Antenna A'},
                    {'index' :  0x01, 'desc' : 'Rx SISO Antenna B'},
                    {'index' :  0x02, 'desc' : 'Rx SISO Antenna C'},
                    {'index' :  0x03, 'desc' : 'Rx SISO Antenna D'},
                    {'index' :  0x04, 'desc' : 'Rx SISO Selection Diversity 2 Antennas'}]

wlan_tx_ant_mode = [{'index' :  0x10, 'desc' : 'Tx SISO Antenna A'},
                    {'index' :  0x20, 'desc' : 'Tx SISO Antenna B'}]


def find_rx_ant_mode_by_index(index):
    """Return the wlan_rx_ant_mode entry for the given index.
    
    Args:
        index (int):  Index into wlan_rx_ant_mode array.    
    
    Returns:
        ant_mode (dict):  Antenna mode dictionary corresponding to the index    
    """
    return _find_param_by_index(index, wlan_rx_ant_mode, 'rx antenna mode')

# End def


def rx_ant_mode_to_str(ant_mode):
    """Convert a wlan_rx_ant_mode entry to a string.

    Args:
        ant_mode (dict):  Antenna Mode dictionary 
    
    Returns:
        output (str):  String representation of the 'ant_mode'
    """
    msg = ""
    if type(ant_mode) is dict:
        msg += "{0}".format(ant_mode['desc'])
    else:
        print("Invalid Rx antenna mode type.  Needed dict, provided {0}.".format(type(ant_mode)))
    return msg

# End def


def rx_ant_mode_index_to_str(index):
    """Convert a wlan_rx_ant_mode entry index to a string.

    Args:
        index (int):  Index into wlan_rx_ant_mode array.
    
    Returns:
        output (str):  String representation of the antenna mode corresponding to 'index'
    """
    ant_mode = find_rx_ant_mode_by_index(index)
    return rx_ant_mode_to_str(ant_mode)

# End def


def find_tx_ant_mode_by_index(index):
    """Return the wlan_tx_ant_mode entry for the given index.
    
    Args:
        index (int):  Index into wlan_tx_ant_mode array.    
    
    Returns:
        ant_mode (dict):  Antenna mode dictionary corresponding to the index    
    """
    return _find_param_by_index(index, wlan_tx_ant_mode, 'tx antenna mode')

# End def


def tx_ant_mode_to_str(ant_mode):
    """Convert a wlan_tx_ant_mode entry to a string.

    Args:
        ant_mode (dict):  Antenna Mode dictionary 
    
    Returns:
        output (str):  String representation of the 'ant_mode'
    """
    msg = ""
    if type(ant_mode) is dict:
        msg += "{0}".format(ant_mode['desc'])
    else:
        print("Invalid Tx antenna mode type.  Needed dict, provided {0}.".format(type(ant_mode)))
    return msg

# End def


def tx_ant_mode_index_to_str(index):
    """Convert a wlan_tx_ant_mode entry index to a string.

    Args:
        index (int):  Index into wlan_tx_ant_mode array.
    
    Returns:
        output (str):  String representation of the antenna mode corresponding to 'index'
    """
    ant_mode = find_tx_ant_mode_by_index(index)
    return tx_ant_mode_to_str(ant_mode)

# End def



# -----------------------------------------------------------------------------
# WLAN Exp MAC Address definitions
# -----------------------------------------------------------------------------

# MAC Description Map
#   List of tuples:  (MAC value, mask, description) describe various MAC addresses
#
# IP -> MAC multicast references:
#     http://technet.microsoft.com/en-us/library/cc957928.aspx
#     http://en.wikipedia.org/wiki/Multicast_address#Ethernet
#     http://www.cavebear.com/archive/cavebear/Ethernet/multicast.html

mac_addr_desc_map = [(0xFFFFFFFFFFFF, 0xFFFFFFFFFFFF, 'Broadcast'),
                     (0x01005E000000, 0xFFFFFF800000, 'IP v4 Multicast'),
                     (0x333300000000, 0xFFFF00000000, 'IP v6 Multicast'),
                     (0xFEFFFF000000, 0xFFFFFF000000, 'Anonymized Device'),
                     (0xFFFFFFFF0000, 0xFFFFFFFF0000, 'Anonymized Device'),
                     (0x40D855042000, 0xFFFFFFFFF000, 'Mango WARP Hardware')]


# MAC bit definitions
#   - Reference: http://standards.ieee.org/develop/regauth/tut/macgrp.pdf
mac_addr_mcast_mask = 0x010000000000
mac_addr_local_mask = 0x020000000000
mac_addr_broadcast  = 0xFFFFFFFFFFFF



# -----------------------------------------------------------------------------
# WLAN Exp Power definitions
# -----------------------------------------------------------------------------
def get_node_max_tx_power():
    """Get the maximum supported transmit power of the node."""
    import wlan_exp.cmds as cmds
    return cmds.CMD_PARAM_NODE_TX_POWER_MAX_DBM

# End def


def get_node_min_tx_power():
    """Get the minimum supported transmit power of the node."""
    import wlan_exp.cmds as cmds
    return cmds.CMD_PARAM_NODE_TX_POWER_MIN_DBM

# End def



# -----------------------------------------------------------------------------
# WLAN Exp Node Utilities
# -----------------------------------------------------------------------------

# WARPNet Type Dictionary
#   This variable is not valid until init_nodes has been run.
warpnet_type_dict = None



def init_nodes(nodes_config, network_config=None, node_factory=None,
               network_reset=True, output=False):
    """Initalize WLAN Exp nodes.
    
    The init_nodes function serves two purposes:  1) To initialize the node for
    participation in the experiment and 2) To retrieve all necessary information 
    from the node to provide a valid python WlanExpNode object to be used in 
    the experiment script.
    
    When a WARP node is first configured from a bitstream, its network interface is
    set to a default value such that it is part of the defalt subnet 10.0.0 but does not 
    have a valid IP address for communication with the host.  As part of the init_nodes 
    process, if network_reset is True, the host will reset the network configuration
    on the node and configure the node with a valid IP address.  If the network settings 
    of a node have already been configured and are known to the python experiment script
    a priori, then it is not necessary to issue a network reset to reset the network
    settings on the node.  This can be extremely useful when trying to interact with a 
    node via multiple python experiment scripts at the same time.    

    Args:
        nodes_config (NodesConfiguration):  A NodesConfiguration describing the nodes
            in the network.
        network_config (NetworkConfiguration, optional): A NetworkConfiguration object 
            describing the network configuration
        node_factory (WlanExpNodeFactory, optional):  A WlanExpNodeFactory or subclass 
            to create nodes of a given WARPNet type
        network_reset (bool, optional):  Issue a network reset command to the nodes to 
            initialize / re-initialize their network interface.
        output (bool, optional):         Print output about the WARPNet nodes
    
    Returns:
        nodes (list of WlanExpNode):  
            Initialized list of WlanExpNode / sub-classes of WlanExpNode depending on the 
            hardware configuration of the WARP nodes.
    """
    global warpnet_type_dict

    # Create a Host Configuration if there is none provided
    if network_config is None:
        import wlan_exp.warpnet.config as wn_config
        network_config = wn_config.NetworkConfiguration()

    # If node_factory is not defined, create a default WlanExpNodeFactory
    if node_factory is None:
        import wlan_exp.node as node
        node_factory = node.WlanExpNodeFactory(network_config)

    # Record the WARPNet type dictionary from the NodeFactory for later use
    warpnet_type_dict = node_factory.get_wn_type_dict()

    # Use the WARPNet utility, wn_init_nodes, to initialize the nodes
    import wlan_exp.warpnet.util as wn_util
    return wn_util.wn_init_nodes(nodes_config, network_config, node_factory, network_reset, output)

# End def


def broadcast_cmd_set_time(time, network_config, time_id=None):
    """Initialize the timebase on all of the WLAN Exp nodes.

    This method will iterate through all network configurations and issue a broadcast 
    packet on each network that will set the timebase on the node to 'time'.  The 
    method keeps track of how long it takes to send each packet so that the time on all 
    nodes is as close as possible even across networks.

    Args:
        network_config (NetworkConfiguration): One or more NetworkConfiguration objects
           that define the networks on which the set_time command will be broadcast
        time (float, int):       Time to which the node's timestamp will be set (either float in sec or int in us)
        time_id (int, optional): Identifier used as part of the TIME_INFO log entry created by this command.
            If not specified, then a random number will be used.
    """
    import wlan_exp.cmds as cmds
    _broadcast_time_to_nodes(time_cmd=cmds.CMD_PARAM_WRITE, network_config=network_config, time=time, time_id=time_id)

# End def


def broadcast_cmd_write_time_to_logs(network_config, time_id=None):
    """Add the current host time to the log on each node.

    This method will iterate through all network configurations and issue a broadcast 
    packet on each network that will add the current time to the log. The method 
    keeps track of how long it takes to send each packet so that the time on all 
    nodes is as close as possible even across networks.

    Args:
        network_config (NetworkConfiguration): One or more NetworkConfiguration objects
           that define the networks on which the log_write_time command will be broadcast
        time_id (int, optional): Identifier used as part of the TIME_INFO log entry created by this command.
            If not specified, then a random number will be used.
    """
    import wlan_exp.cmds as cmds
    _broadcast_time_to_nodes(time_cmd=cmds.CMD_PARAM_TIME_ADD_TO_LOG, network_config=network_config, time_id=time_id)

# End def


def broadcast_cmd_write_exp_info_to_logs(network_config, info_type, message=None):
    """Add the EXP INFO log entry to the log on each node.

    This method will iterate through all network configurations and issue a broadcast 
    packet on each network that will add the EXP_INFO log entry to the log

    Args:
        network_config (NetworkConfiguration): One or more NetworkConfiguration objects
           that define the networks on which the log_write_exp_info command will be broadcast
        info_type (int): Type of the experiment info.  This is an arbitrary 16 bit number 
            chosen by the experimentor
        message (int, str, bytes, optional): Information to be placed in the event log.  
    """
    import wlan_exp.cmds as cmds

    if type(network_config) is list:
        configs = network_config
    else:
        configs = [network_config]

    for config in configs:
        _broadcast_cmd_to_nodes_helper(cmds.LogAddExpInfoEntry(info_type, message), config)

# End def


def filter_nodes(nodes, mac_high=None, mac_low=None, serial_number=None, warn=True):
    """Return a list of nodes that match all the values for the given filter parameters.

    Each of these filter parameters can be a single value or a list of values.

    Args:
        nodes (list of WlanExpNode):  List of WlanExpNode / sub-classes of WlanExpNode
        mac_high (str, int, optional):  Filter for CPU High functionality.  This value must be either
            an integer corresponding to a WARPNet type (see wlan_exp/defaults.py for WARPNet types) 
            or the following strings:
                * **'AP'**   (equivalent to WLAN_EXP_HIGH_AP); 
                * **'STA'**  (equivalent to WLAN_EXP_HIGH_STA);
                * **'IBSS'** (equivalent to WLAN_EXP_HIGH_IBSS).
            A value of None means that no filtering will occur for CPU High Functionality
        mac_low (str, int, optional): Filter for CPU Low functionality.  This value must be either
            an integer corresponding to a WARPNet type (see wlan_exp/defaults.py for WARPNet types) 
            or the following strings:
                * **'DCF'**   (equivalent to WLAN_EXP_LOW_DCF);
                * **'NOMAC'** (equivalent to WLAN_EXP_LOW_NOMAC).
            A value of None means that no filtering will occur for CPU Low Functionality
        serial_number (str, optional):  Filters nodes by WARPv3 serial number.  This value must be of 
            the form:  'W3-a-XXXXX' where XXXXX is the five digit serial number of the board.
        warn (bool, optional):          Print warnings (default value is True)

    Returns:
        nodes (list of WlanExpNode):  Filtered list of WlanExpNode / sub-classes of WlanExpNode
    

    **Examples**
    :: 
        filter_nodes(nodes, mac_high='AP', mac_low='DCF') --> Only AP DCF nodes
        filter_nodes(nodes, mac_high='AP')                --> AP nodes where low can be DCF/NOMAC
        filter_nodes(nodes, mac_high='ap', mac_low='dcf', serial_numbers=['w3-a-00001','w3-a-00002'])
            --> Find AP DCF nodes with serial numbers 'W3-a-00001' and 'W3-a-00002'

    .. note::  If the list is empty, then this method will issue a warning if the parameter warn is True.
    """
    ret_nodes         = []
    tmp_mac_high      = None
    tmp_mac_low       = None
    tmp_serial_number = None

    # Create MAC High Filter
    if mac_high is not None:
        if type(mac_high) is not list:
            mac_high = [mac_high]

        tmp_mac_high = []

        for value in mac_high:
            if type(value) is str:
                if (value.lower() == 'ap'):
                    tmp_mac_high.append(defaults.WLAN_EXP_HIGH_AP)
                elif (value.lower() == 'sta'):
                    tmp_mac_high.append(defaults.WLAN_EXP_HIGH_STA)
                elif (value.lower() == 'ibss'):
                    tmp_mac_high.append(defaults.WLAN_EXP_HIGH_IBSS)
                else:
                    msg  = "Unknown mac_high filter value: {0}\n".format(value)
                    msg += "    Must be either 'AP', 'STA', or 'IBSS'"
                    print(msg)

            if type(value) is int:
                tmp_mac_high.append(value)

    # Create MAC Low Filter
    if mac_low is not None:
        if type(mac_low) is not list:
            mac_low = [mac_low]

        tmp_mac_low = []

        for value in mac_low:
            if type(value) is str:
                if (value.lower() == 'dcf'):
                    tmp_mac_low.append(defaults.WLAN_EXP_LOW_DCF)
                elif (value.lower() == 'nomac'):
                    tmp_mac_low.append(defaults.WLAN_EXP_LOW_NOMAC)
                else:
                    msg  = "Unknown mac_low filter value: {0}\n".format(value)
                    msg += "    Must be either 'DCF' or 'NOMAC'"
                    print(msg)

            if type(value) is int:
                tmp_mac_low.append(value)

    # Create Serial Number Filter
    if serial_number is not None:
        import wlan_exp.warpnet.util as wn_util

        if type(serial_number) is not list:
            serial_number = [serial_number]

        tmp_serial_number = []

        for value in serial_number:
            try:
                (sn, _) = wn_util.wn_get_serial_number(value)
                tmp_serial_number.append(sn)
            except TypeError as err:
                print(err)

    ret_nodes = _get_nodes_by_type(nodes, tmp_mac_high, tmp_mac_low)
    ret_nodes = _get_nodes_by_sn(ret_nodes, tmp_serial_number)

    if ((len(ret_nodes) == 0) and warn):
        import warnings
        msg  = "\nNo nodes match filter: \n"
        msg += "    mac_high = {0}\n".format(mac_high)
        msg += "    mac_high = {0}\n".format(mac_high)
        warnings.warn(msg)

    return ret_nodes

# End def



# -----------------------------------------------------------------------------
# WLAN Exp Misc Utilities
# -----------------------------------------------------------------------------

def create_bss_info(bssid, ssid, channel, ibss_status=False, beacon_interval=100):
    """Create a Basic Service Set (BSS) information structure.

    This method will create a dictionary that contains all necessary information
    for a BSS for the device.  This is the same structure that is used by the
    BSS_INFO log entry.

    Args:
        ssid (str):  SSID string (Must be 32 characters or less)
        channel (int, dict in util.wlan_channel array): Channel on which the BSS operates
            (either the channel number as an it or an entry in the wlan_channel array)
        bssid (int):  40-bit ID of the BSS (Uses current wlan_mac_address if not specified)  
        ibss_status (bool, optional): Status of the 
            BSS: 
                * **True**  --> Capabilities field = 0x2 (BSS_INFO is for IBSS)
                * **False** --> Capabilities field = 0x1 (BSS_INFO is for BSS)
        beacon_interval (int): Integer number of beacon Time Units in [1, 65535]
            (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds)
    
    Returns:
        bss_info (dict):  BSS_INFO dictionary (defined in wlan_exp.log.entry_types)
    """
    channel_error = False
    bss_dict      = {}

    # Set default values for fields not set by this method
    bss_dict['timestamp']       = 0
    bss_dict['last_timestamp']  = 0
    bss_dict['flags']           = 0
    bss_dict['state']           = 5              # BSS_STATE_OWNED
    bss_dict['num_basic_rates'] = 0
    bss_dict['basic_rates']     = bytes()

    # Check SSID
    if type(ssid) is not str:
        raise ValueError("The SSID must be a string.")

    if len(ssid) > 32:
        ssid = ssid[:32]
        print("WARNING:  SSID must be 32 characters or less.  Trucating to {0}".format(ssid))

    bss_dict['ssid'] = ssid

    # Check Channel
    #   - Make sure it is a valid channel; only store channel
    if type(channel) is int:
        channel = find_channel_by_channel_number(channel)

        if channel is None: channel_error = True

    elif type(channel) is dict:
        pass
    else:
        channel_error = True

    if not channel_error:
        bss_dict['chan_num'] = channel['channel']
    else:
        msg  = "The channel must either be a valid channel number or a wlan_exp.util.wlan_channel entry."
        raise ValueError(msg)

    # Check beacon interval
    if type(beacon_interval) is not int:
        beacon_interval = int(beacon_interval)
        print("WARNING:  Beacon interval must be an interger number of time units rounding to {0}".format(beacon_interval))

    if not ((beacon_interval > 0) and (beacon_interval < 2**16)):
        msg  = "The beacon interval must be in [1, 65535] (ie 16-bit positive integer)."
        raise ValueError(msg)

    bss_dict['beacon_interval'] = beacon_interval

    # Check IBSS status
    if type(ibss_status) is not bool:
        raise ValueError("The ibss_status must be a boolean.")

    # Set BSSID, capabilities
    #   - If this is an IBSS, then set local bit to '1' and mcast bit to '0'
    if ibss_status:
        bss_dict['bssid_int']    = (bssid | mac_addr_local_mask) & (mac_addr_broadcast - mac_addr_mcast_mask)
        bss_dict['capabilities'] = 0x2
    else:
        bss_dict['bssid_int']    = bssid
        bss_dict['capabilities'] = 0x1

    # Convert bssid_int to string for bssid
    bss_dict['bssid'] = ''.join([chr((bss_dict['bssid_int'] >> ((6 - i - 1) * 8)) % 256) for i in range(6)])

    return bss_dict

# End def


def create_locally_administered_bssid(node):
    """Create a locally administered BSSID based on the wireless MAC address of the node.
    
    Args:
        node (WlanExpNode):  WlanExpNode or sub-class of WlanExpNode
    
    Returns:
        bssid (int):  
            BSSID with the "locally administerd" bit set to '1' and the "multicast" bit set to '0'    
    """

    return (node.wlan_mac_address | mac_addr_local_mask) & (mac_addr_broadcast - mac_addr_mcast_mask)

# End def


def int_to_ip(ip_address):
    """Convert an integer to IP address string (dotted notation).

    Args:
        ip_address (int):  Unsigned 32-bit integer representation of the IP address

    Returns:
        ip_address (str):  String version of an IP address of the form W.X.Y.Z        
    """
    import wlan_exp.warpnet.transport_eth_udp as tport
    return tport.int_to_ip(ip_address)

# End def


def ip_to_int(ip_address):
    """Convert IP address string (dotted notation) to an integer.

    Args:
        ip_address (str):  String version of an IP address of the form W.X.Y.Z        

    Returns:
        ip_address (int):  Unsigned 32-bit integer representation of the IP address
    """
    import wlan_exp.warpnet.transport_eth_udp as tport
    return tport.ip_to_int(ip_address)

# End def


def mac_addr_to_str(mac_address):
    """Convert an integer to a colon separated MAC address string.

    Args:
        mac_address (int):  Unsigned 40-bit integer representation of the MAC address    

    Returns:
        mac_address (str):  String version of an MAC address of the form XX:XX:XX:XX:XX:XX        
    """
    import wlan_exp.warpnet.transport_eth_udp as tport
    return tport.mac_addr_to_str(mac_address)

# End def


def sn_to_str(hw_generation, serial_number):
    """Convert serial number to a string for a given hardware generation.

    Args:
        hw_generation (int):  WARP hardware generation (currently only '3' is supported)
        serial_number (int):  Integer representation of the WARP serial number

    Returns:
        serial_number (str):  String representation of the WARP serial number (of the form 'W3-a-XXXXX')    
    """
    if(hw_generation == 3):
        return ('W3-a-{0:05d}'.format(int(serial_number)))
    else:
        print("ERROR:  Not valid Hardware Generation: {0}".format(hw_generation))

# End def


def ver_code_to_str(ver_code):
    """Convert a WLAN Exp version code to a string."""
    import wlan_exp.version as version
    return version.wlan_exp_ver_code_to_str(ver_code)

# End def


def node_type_to_str(node_type, node_factory=None):
    """Convert the Node WARPNet Type to a string description.

    Args:
        node_type (int):  WARPNet node type code (u32)
        node_factory (WlanExpNodeFactory): A WlanExpNodeFactory or subclass to 
            create nodes of a given WARPNet type
    
    Returns:
        node_type (str):  String representation of the WARPNet node type

    By default, a dictionary of WARPNet types is built dynamically during init_nodes().  
    If init_nodes() has not been run, then the method will try to create a WARPNet 
    type dictionary.  If a node_factory is not provided then a default WlanExpNodeFactory 
    will be used to determine the WARPNet type.  If a default WlanExpNodeFactory is used, 
    then only framework WARPNet types will be known and custom WARPNet types will return:  
    "Unknown WARPNet type: <value>"
    """
    global warpnet_type_dict

    # Build a warpnet_type_dict if it is not present
    if warpnet_type_dict is None:

        # Build a default node_factory if it is not present
        if node_factory is None:
            import wlan_exp.node as node
            import wlan_exp.warpnet.config as wn_config

            network_config = wn_config.NetworkConfiguration()
            node_factory   = node.WlanExpNodeFactory(network_config)

        # Record the WARPNet type dictionary from the NodeFactory for later use
        warpnet_type_dict = node_factory.get_wn_type_dict()

    if (node_type in warpnet_type_dict.keys()):
        return warpnet_type_dict[node_type]['description']
    else:
        return "Unknown WARPNet type: 0x{0:8x}".format(node_type)

# End def


def mac_addr_desc(mac_addr, desc_map=None):
    """Returns a string description of a MAC address.

    Args:
        mac_address (int):  Unsigned 40-bit integer representation of the MAC address        
        desc_map (list of tuple, optional): list of tuple or tuple of the form
            (addr_mask, addr_value, descritpion)

    Returns:
        description (str):  
            Description of the MAC address or '' if address does not match any descriptions        

    This is useful when printing a table of addresses.  Custom MAC address
    descriptions can be provided via the desc_map argument.  In addition
    to the provided desc_map, the global mac_addr_desc_map that describes mappings
    of different MAC addresses will also be used.

    .. note:: The mac_addr argument will be bitwise AND'd with each addr_mask, then compared to
    addr_value. If the result is non-zero the corresponding descprition will be returned.  This
    will only return the first description in the [desc_map, mac_addr_desc_map] list.
    
    **Example**
    ::
        desc_map = [ (0x000102030405, 0xFFFFFFFFFFFF, 'My Custom MAC Addr'),
                     (0x000203040506, 0xFFFFFFFFFFFF, 'My Other MAC Addr') ]
    """
    # Cast to python int in case input is still numpy uint64
    mac_addr = int(mac_addr)

    desc_out = ''

    if(desc_map is None):
        desc_map = mac_addr_desc_map
    else:
        desc_map = list(desc_map) + mac_addr_desc_map

    for (mask, req, desc) in desc_map:
        if( (mac_addr & mask) == req):
            desc_out += desc
            break

    return desc_out

# End def


# Excellent util function for dropping into interactive Python shell
#   From http://vjethava.blogspot.com/2010/11/matlabs-keyboard-command-in-python.html
def debug_here(banner=None):
    """Function that mimics the matlab keyboard command for interactive debbug.
    
    Args:
        banner (str):  Banner message to be displayed before the interactive prompt    
    """
    import code
    # Use exception trick to pick up the current frame
    try:
        raise None
    except TypeError:
        frame = sys.exc_info()[2].tb_frame.f_back

    print("# Use quit() or Ctrl-D to exit")

    # evaluate commands in current namespace
    namespace = frame.f_globals.copy()
    namespace.update(frame.f_locals)

    try:
        code.interact(banner=banner, local=namespace)
    except SystemExit:
        return

# End def


# -----------------------------------------------------------------------------
# Internal Methods
# -----------------------------------------------------------------------------
def _get_nodes_by_type(nodes, mac_high=None, mac_low=None):
    """Returns all nodes in the list that have the given node_type."""

    # Check that these nodes are 802.11 nodes
    tmp_nodes = [n for n in nodes if (n.node_type & 0xFFFF0000 == defaults.WLAN_EXP_BASE)]

    # Filter the 8 bits for MAC High node type
    if mac_high is not None:
        if type(mac_high) is not list:
            mac_high = [mac_high]
        tmp_nodes = [n for n in tmp_nodes if ((n.node_type & 0x0000FF00) in mac_high)]

    # Filter the 8 bits for MAC Low node type
    if mac_low is not None:
        if type(mac_low) is not list:
            mac_low = [mac_low]
        tmp_nodes = [n for n in tmp_nodes if ((n.node_type & 0x000000FF) in mac_low)]

    return tmp_nodes

# End def


def _get_nodes_by_sn(nodes, serial_number=None):
    """Returns all nodes in the list that have the given serial number."""

    if serial_number is not None:
        if type(serial_number) is not list:
            serial_number = [serial_number]
        nodes = [n for n in nodes if (n.serial_number in serial_number)]

    return nodes

# End def


def _time():
    """WLAN Exp time function to handle differences between Python 2.7 and 3.3"""
    try:
        return time.perf_counter()
    except AttributeError:
        return time.clock()

# End def


def _broadcast_time_to_nodes(time_cmd, network_config, time=0.0, time_id=None):
    """Internal method to issue broadcast time commands

    This method will iterate through all host interfaces and issue a
    broadcast packet on each interface that will set the time to the
    timebase.  The method keeps track of how long it takes to send
    each packet so that the time on all nodes is as close as possible
    even across interface.

    Attributes:
        time_cmd       -- NodeProcTime command to issue
        network_config -- A NetworkConfiguration object
        time           -- Optional time to broadcast to the nodes (defaults to 0.0)
                          either as an integer number of microseconds or a floating
                          point number in seconds
        time_id        -- Optional value to identify broadcast time commands across nodes
    """
    import wlan_exp.cmds as cmds

    time_factor = 6               # Time can be in # of microseconds (ie 10^(-6) seconds)

    # Need to convert time input to float so that it can be added to _time()
    if   (type(time) is float):
        node_time   = time
    elif (type(time) is int):
        node_time   = float(time / (10**time_factor))   # Convert to float
    else:
        raise TypeError("Time must be either a float or int")

    # Determine if we are sending to multiple networks
    if type(network_config) is list:
        configs = network_config
    else:
        configs = [network_config]

    # Send command to each network
    for idx, config in enumerate(configs):
        network_addr    = config.get_param('network')

        if (idx == 0):
            node_time   = node_time
            start_time  = _time()
        else:
            node_time   = node_time + (_time() - start_time)

        cmd             = cmds.NodeProcTime(time_cmd, node_time, time_id);

        _broadcast_cmd_to_nodes_helper(cmd, network_config)

        msg = ""
        if (time_cmd == cmds.CMD_PARAM_WRITE):
            msg += "Initializing the time of all nodes on network "
            msg += "{0} to: {1}".format(network_addr, node_time)
        elif (time_cmd == cmds.CMD_PARAM_TIME_ADD_TO_LOG):
            msg += "Adding current time to log for nodes on network {0}".format(network_addr)
        print(msg)

# End def


def _broadcast_cmd_to_nodes_helper(cmd, network_config):
    """Internal method to issue broadcast commands

    Attributes:
        network_config -- A NetworkConfiguration object
        cmd            -- A wn_message.Cmd object describing the command
    """
    import wlan_exp.warpnet.transport_eth_udp_py_bcast as bcast

    # Get information out of the NetworkConfiguration
    tx_buf_size  = network_config.get_param('tx_buffer_size')
    rx_buf_size  = network_config.get_param('rx_buffer_size')

    # Create broadcast transport and send message
    transport_bcast = bcast.TransportEthUdpPyBcast(network_config);

    transport_bcast.wn_open(tx_buf_size, rx_buf_size)

    transport_bcast.send(payload=cmd.serialize())

    transport_bcast.wn_close()

# End def



def _find_param_by_index(index, param_list, param_name):
    """Return the entry for the given index."""
    ret_val = None

    if type(index) is int:
        for param in param_list:
            if (param['index'] == index):
                ret_val = param
                break

    if ret_val is None:
        print("Unknown {0} index: {1}".format(param_name, index))

    return ret_val

# End def


def _find_param_by_name(name, value, param_list, param_name):
    """Return the entry for the given name."""
    ret_val = None

    for param in param_list:
        if (param[name] == value):
            ret_val = param
            break

    if ret_val is None:
        print("Unknown {0} '{1}': {2}".format(param_name, name, value))

    return ret_val

# End def





