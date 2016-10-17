# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Utilities
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
"""

import sys
import time

import wlan_exp.defaults as defaults

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None

__all__ = ['consts_dict', 'init_nodes', 'broadcast_cmd_set_mac_time', 'broadcast_cmd_write_time_to_logs',
           'filter_nodes']



# -----------------------------------------------------------------------------
# Constants Dictionary Class
# -----------------------------------------------------------------------------

class consts_dict(dict):
    """Contants Dictionary

    Sub-class of dictionary, with fields accessible as immutable properties.
    """
    def copy(self):
        return consts_dict(self)

    # Allow attribute (ie ".") notation to access contents of dictionary
    def __getattr__(self, name):
        if name in self:
            return self[name]
        else:
            raise AttributeError("No such attribute: " + name)

    # Do not allow existing attributes or items to be modified or deleted
    def __setattr__(self, name, value):
        if name in self:
            raise AttributeError("Cannot change existing entries in {0}".format(self.__class__.__name__))
        else:
            super(consts_dict, self).__setitem__(name, value)

    def __delattr__(self, name):
        pass

    def __setitem__(self, key, value):
        if key in self:
            raise AttributeError("Cannot change existing entries in {0}".format(self.__class__.__name__))
        else:
            super(consts_dict, self).__setitem__(key, value)

    def __delitem__(self, key):
        pass

# End class



# -----------------------------------------------------------------------------
# UART Print Levels
# -----------------------------------------------------------------------------

#: wlan_exp UART Print Levels:
#:
#:  * ``NONE``    - Do not print messages
#:  * ``ERROR``   - Only print error messages
#:  * ``WARNING`` - Print error and warning messages
#:  * ``INFO``    - Print error, warning and info messages
#:  * ``DEBUG``   - Print error, warning, info and debug messages
#:    
#: Use this dictionary for the ``set_print_level()`` command 
# The C counterparts are found in wlan_exp_common.h
uart_print_levels = consts_dict({
       'NONE'      :  0,
       'ERROR'     :  1,
       'WARNING'   :  2,
       'INFO'      :  3,
       'DEBUG'     :  4})



# -----------------------------------------------------------------------------
# Rate definitions
# -----------------------------------------------------------------------------

#: PHY Modes
#:
#:   * 'DSSS'  - DSSS (Rx only)
#:   * 'NONHT' - NONHT OFDM (11a/g)
#:   * 'HTMF'  - HTMF (11n)
#:
#: Use this dictionary to interpret ``phy_mode`` values encoded in Tx/Rx log entries
phy_modes = consts_dict({
       'DSSS'      :  0,
       'NONHT'     :  1,
       'HTMF'      :  2})


def get_rate_info(mcs, phy_mode, phy_samp_rate=20, short_GI=False):
    """Generate dictionary with details about a PHY rate. The returned dictionary
    has fields:
    
      * ``mcs``: the MCS index passed in the ``mcs`` argument, integer in 0 to 7
      * ``phy_mode``: the PHY mode passed in the ``phy_mode`` argument, either ``'NONHT'`` or ``'HTMF'``
      * ``desc``: string describing the rate
      * ``NDBPS``: integer number of data bits per OFDM symbol for the rate
      * ``phy_rate``: float data rate in Mbps

    Args:
        mcs (int):           Modulation and coding scheme (MCS) index
        phy_mode (str, int): PHY mode ('NONHT', 'HTMF')
        phy_samp_rate (int): PHY sampling rate (10, 20, 40)
        short_GI (bool):     Short Guard Interval (GI) (True/False)

    Returns:
        rate_info (dict):  Rate info dictionary
    """
    ret_val = dict()

    # 802.11 a/g rates - IEEE 802.11-2012 Table 18-4
    #     Clause 18 doesn't use the term "MCS", but it's a convenient
    #     way to refer to these rates.
    mod_orders_nonht        = ['BPSK', 'BPSK', 'QPSK', 'QPSK', '16-QAM', '16-QAM', '64-QAM', '64-QAM']
    code_rates_nonht        = [ '1/2',  '3/4',  '1/2',  '3/4',    '1/2',    '3/4',    '2/3',    '3/4']
    ndbps_nonht             = [    24,     36,     48,     72,       96,      144,      192,      216]
    phy_rates_nonht         = [   6.0,    9.0,   12.0,   18.0,     24.0,     36.0,     48.0,     54.0]

    # 802.11n rates - IEEE 802.11-2012 Tables 20-30 to 30-37
    mod_orders_htmf         = ['BPSK', 'QPSK', 'QPSK', '16-QAM', '16-QAM', '64-QAM', '64-QAM', '64-QAM']
    code_rates_htmf         = [ '1/2',  '1/2',  '3/4',    '1/2',    '3/4',    '2/3',    '3/4',    '5/6']
    ndbps_htmf_bw20         = [    26,     52,     78,      104,      156,      208,      234,      260]
    phy_rates_htmf_bw20_lgi = [   6.5,   13.0,   19.5,     26.0,     39.0,     52.0,     58.5,     65.0]
    phy_rates_htmf_bw20_sgi = [   7.2,   14.4,   21.7,     28.9,     43.3,     57.8,     65.0,     72.2]
    # ndbps_htmf_bw40         = [    54,    108,    162,      216,      324,      432,      486,      540]
    # phy_rates_htmf_bw40_lgi = [  13.5,   27.0,   40.5,     54.0,     81.0,    108.0,    121.5,    135.0]
    # phy_rates_htmf_bw40_sgi = [  15.0,   30.0,   45.0,     60.0,     90.0,    120.0,    135.0,    150.0]


    # Check input arguments
    if ((mcs < 0) or (mcs > 7)):
        raise AttributeError("MCS must be in [0 .. 7]")

    if (phy_mode not in ['NONHT', 'HTMF', phy_modes['NONHT'], phy_modes['HTMF']]):
        raise AttributeError("PHY mode must be in ['NONHT', 'HTMF', phy_modes['NONHT'], phy_modes['HTMF']]")
    
    if (phy_samp_rate not in [10, 20, 40]):
        raise AttributeError("PHY sample rate must be in [10, 20, 40]")
        
    # Set common values
    ret_val['mcs'] = mcs

    # Set 'NONHT' values
    if ((phy_mode == 'NONHT') or (phy_mode == phy_modes['NONHT'])):
        ret_val['phy_mode'] = 'NONHT'
        ret_val['desc']     = 'NONHT {0} {1}'.format(mod_orders_nonht[mcs], code_rates_nonht[mcs])
        ret_val['NDBPS']    = ndbps_nonht[mcs]
        ret_val['phy_rate'] = phy_rates_nonht[mcs]

    # Set 'HTMF' values
    elif ((phy_mode == 'HTMF') or (phy_mode == phy_modes['HTMF'])):
        ret_val['phy_mode'] = 'HTMF'
        ret_val['desc']     = 'HTMF {0} {1}'.format(mod_orders_htmf[mcs], code_rates_htmf[mcs])
        ret_val['NDBPS']    = ndbps_htmf_bw20[mcs]
        
        if (short_GI):
            ret_val['phy_rate'] = phy_rates_htmf_bw20_sgi[mcs]
        else:
            ret_val['phy_rate'] = phy_rates_htmf_bw20_lgi[mcs]

    # Update PHY rate for other PHY sampling rates
    ret_val['phy_rate'] = ret_val['phy_rate'] * (phy_samp_rate / 20)

    return ret_val

# End def


def rate_info_to_str(rate_info):
    """Convert dictionary returned by ``get_rate_info()`` into a printable string.

    Args:
        rate_info (dict):  Dictionary returned by ``get_rate_info()``

    Returns:
        output (str):  String representation of the rate

    Example:
        >>> import wlan_exp.util as util
        >>> r = util.get_rate_info(mcs=3, phy_mode='HTMF')
        >>> print(util.rate_info_to_str(r))
        26.0 Mbps (HTMF 16-QAM 1/2)
    
    """
    msg = ""
    if type(rate_info) is dict:
        msg += "{0:>4.1f} Mbps ({1})".format(rate_info['phy_rate'], rate_info['desc'])
    else:
        print("Invalid rate info type.  Needed dict, provided {0}.".format(type(rate_info)))
    return msg

# End def



# -----------------------------------------------------------------------------
# Channel definitions
# -----------------------------------------------------------------------------

#: List of supported channels. Each value represents a 20MHz channel in the 2.4GHz
#: or 5GHz bands. Use the ``get_channel_info`` method to lookup the actual center
#: frequency for a given channel index.
wlan_channels = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48]
#wlan_channels = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 38, 40, 44, 46, 48, 52, 54, 56, 60, 62, 64, 100, 102, 104, 108, 110, 112, 116, 118, 120, 124, 126, 128, 132, 134, 136, 140, 142, 144, 149, 151, 153, 157, 159, 161, 165, 172, 174, 175, 176, 177, 178]


def get_channel_info(channel):
    """Get channel info dictionary based on channel number

    Args:
        channel (int):  Number of 802.11 channel
            (https://en.wikipedia.org/wiki/List_of_WLAN_channels)

    Returns:
        channel_info (dict):  Channel info dictionary

    The returned dictionary has fields:

     * ``channel``: Integer channel index
     * ``freq``: Integer channel center frequency, in MHz

    Examples:
        >>> import wlan_exp.util as util
        >>> util.get_channel_info(5)
        {'freq': 2432, 'channel': 5}
    
    """
    
    channel_info = {
         1 : {'channel' :   1, 'freq': 2412},
         2 : {'channel' :   2, 'freq': 2417},
         3 : {'channel' :   3, 'freq': 2422},
         4 : {'channel' :   4, 'freq': 2427},
         5 : {'channel' :   5, 'freq': 2432},
         6 : {'channel' :   6, 'freq': 2437},
         7 : {'channel' :   7, 'freq': 2442},
         8 : {'channel' :   8, 'freq': 2447},
         9 : {'channel' :   9, 'freq': 2452},
        10 : {'channel' :  10, 'freq': 2457},
        11 : {'channel' :  11, 'freq': 2462},
        36 : {'channel' :   36, 'freq': 5180},
        38 : {'channel' :   38, 'freq': 5190},
        40 : {'channel' :   40, 'freq': 5200},
        44 : {'channel' :   44, 'freq': 5220},
        46 : {'channel' :   46, 'freq': 5230},
        48 : {'channel' :   48, 'freq': 5240},
        52 : {'channel' :   52, 'freq': 5260},
        54 : {'channel' :   54, 'freq': 5270},
        56 : {'channel' :   56, 'freq': 5280},
        60 : {'channel' :   60, 'freq': 5300},
        62 : {'channel' :   62, 'freq': 5310},
        64 : {'channel' :   64, 'freq': 5320},
        100 : {'channel' :   100, 'freq': 5500},
        102 : {'channel' :   102, 'freq': 5510},
        104 : {'channel' :   104, 'freq': 5520},
        108 : {'channel' :   108, 'freq': 5540},
        110 : {'channel' :   110, 'freq': 5550},
        112 : {'channel' :   112, 'freq': 5560},
        116 : {'channel' :   116, 'freq': 5580},
        118 : {'channel' :   118, 'freq': 5590},
        120 : {'channel' :   120, 'freq': 5600},
        124 : {'channel' :   124, 'freq': 5620},
        126 : {'channel' :   126, 'freq': 5630},
        128 : {'channel' :   128, 'freq': 5640},
        132 : {'channel' :   132, 'freq': 5660},
        134 : {'channel' :   134, 'freq': 5670},
        136 : {'channel' :   136, 'freq': 5680},
        140 : {'channel' :   140, 'freq': 5700},
        142 : {'channel' :   142, 'freq': 5710},
        144 : {'channel' :   144, 'freq': 5720},
        149 : {'channel' :   149, 'freq': 5745},
        151 : {'channel' :   151, 'freq': 5755},
        153 : {'channel' :   153, 'freq': 5765},
        157 : {'channel' :   157, 'freq': 5785},
        159 : {'channel' :   159, 'freq': 5795},
        161 : {'channel' :   161, 'freq': 5805},
        165 : {'channel' :   165, 'freq': 5825},    
        172 : {'channel' :   172, 'freq': 5860},
        174 : {'channel' :   174, 'freq': 5870},
        175 : {'channel' :   175, 'freq': 5875},
        176 : {'channel' :   176, 'freq': 5880},
        177 : {'channel' :   177, 'freq': 5885},
        178 : {'channel' :   178, 'freq': 5890}}

    # Check input arguments
    if (channel not in wlan_channels):
        raise AttributeError("Channel must be list of supported channels.")

    return channel_info[channel]

# End def


def channel_info_to_str(channel_info):
    """Convert a channel info dictionary to a string.

    Args:
        channel_info (dict):  Dictionary returned by ``get_channel_info()``

    Returns:
        output (str):  String representation of the 'channel'
    """
    msg = ""
    if type(channel_info) is dict:
        msg += "{0:4d} ({1} MHz)".format(channel_info['channel'], channel_info['freq'])
    else:
        print("Invalid channel info type.  Needed dict, provided {0}.".format(type(channel_info)))
    return msg

# End def



# -----------------------------------------------------------------------------
# Antenna Mode definitions
# -----------------------------------------------------------------------------

#: Dictionary of supported receive interfaces.
wlan_rx_ant_modes = consts_dict({
       'RF_A'           : 0x0,
       'RF_B'           : 0x1,
       'RF_C'           : 0x2,
       'RF_D'           : 0x3,
       'RF_SELDIV_AB'   : 0x4})
       
#: Dictionary of supported transmit interfaces.
wlan_tx_ant_modes = consts_dict({
       'RF_A'           : 0x0,
       'RF_B'           : 0x1})



# -----------------------------------------------------------------------------
# MAC Address definitions
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
# Node Utilities
# -----------------------------------------------------------------------------

# Node Type Dictionary
#   This variable is not valid until init_nodes has been run.
node_type_dict = None



def init_nodes(nodes_config, network_config=None, node_factory=None,
               network_reset=True, output=False):
    """Initalize wlan_exp nodes.

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
            to create nodes of a given node type
        network_reset (bool, optional):  Issue a network reset command to the nodes to
            initialize / re-initialize their network interface.
        output (bool, optional):         Print output about the nodes

    Returns:
        nodes (list of WlanExpNode):
            Initialized list of WlanExpNode / sub-classes of WlanExpNode depending on the
            hardware configuration of the WARP nodes.
    """
    global node_type_dict

    # Create a Host Configuration if there is none provided
    if network_config is None:
        import wlan_exp.transport.config as config
        network_config = config.NetworkConfiguration()

    # If node_factory is not defined, create a default WlanExpNodeFactory
    if node_factory is None:
        import wlan_exp.node as node
        node_factory = node.WlanExpNodeFactory(network_config)

    # Record the node type dictionary from the NodeFactory for later use
    node_type_dict = node_factory.get_type_dict()

    # Use the utility, init_nodes, to initialize the nodes
    import wlan_exp.transport.util as util
    return util.init_nodes(nodes_config, network_config, node_factory, network_reset, output)

# End def


def broadcast_cmd_set_mac_time(time, network_config, time_id=None):
    """Initialize the MAC time on all of the wlan_exp nodes.

    This method will iterate through all network configurations and issue a broadcast
    packet on each network that will set the MAC time on the node to 'time'.  The
    method keeps track of how long it takes to send each packet so that the time on all
    nodes is as close as possible even across networks.

    Args:
        network_config (NetworkConfiguration): One or more NetworkConfiguration objects
           that define the networks on which the set_time command will be broadcast
        time (int):              Time to which the node's MAC timestamp will be set (int microseconds)
        time_id (int, optional): Identifier used as part of the TIME_INFO log entry created by this command.
            If not specified, then a random number will be used.
    """
    import wlan_exp.cmds as cmds
    
    if type(time) not in [int, long]:
        raise AttributeError("Time must be expressed in int microseconds")
    
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
            an integer corresponding to a node type (see wlan_exp/defaults.py for node types)
            or the following strings:
            
                * **'AP'**   (equivalent to WLAN_EXP_HIGH_AP);
                * **'STA'**  (equivalent to WLAN_EXP_HIGH_STA);
                * **'IBSS'** (equivalent to WLAN_EXP_HIGH_IBSS).
                
            A value of None means that no filtering will occur for CPU High Functionality
        mac_low (str, int, optional): Filter for CPU Low functionality.  This value must be either
            an integer corresponding to a node type (see wlan_exp/defaults.py for node types)
            or the following strings:
            
                * **'DCF'**   (equivalent to WLAN_EXP_LOW_DCF);
                * **'NOMAC'** (equivalent to WLAN_EXP_LOW_NOMAC).
                
            A value of None means that no filtering will occur for CPU Low Functionality
        serial_number (str, optional):  Filters nodes by WARPv3 serial number.  This value must be of
            the form:  'W3-a-XXXXX' where XXXXX is the five digit serial number of the board.
        warn (bool, optional):          Print warnings (default value is True)

    Returns:
        nodes (list of WlanExpNode):  Filtered list of WlanExpNode / sub-classes of WlanExpNode

    If the return list of nodes is empty, then this method will issue a warning 
    if the parameter warn is True.

    Examples:
        >>> filter_nodes(nodes, mac_high='AP', mac_low='DCF')
        >>> filter_nodes(nodes, mac_high='AP')
        >>> filter_nodes(nodes, mac_high='AP', mac_low='DCF', serial_numbers=['w3-a-00001','w3-a-00002'])
    
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
        import wlan_exp.transport.util as util

        if type(serial_number) is not list:
            serial_number = [serial_number]

        tmp_serial_number = []

        for value in serial_number:
            try:
                (sn, _) = util.get_serial_number(value)
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


def check_bss_membership(nodes, verbose=False):
    """Check that each of the nodes in the input list are members of the same 
    BSS.  For a BSS to match, the 'bssid', 'ssid' and 'channel' must match.
    
    There are two acceptable patterns for the nodes argument
    
        #. 1 AP and 1+ STA and 0  IBSS ("infrastructure" network)
        #. 0 AP and 0  STA and 2+ IBSS ("ad hoc" network)
    
    In the case that nodes is 1 AP and 1+ STA and 0 IBSS, then the following
    conditions must be met in order to return True
    
        #. AP BSS must be non-null
        #. AP must have each STA in its station_info list
        #. Each STA BSS must match AP BSS
        #. Each STA must have AP as its only station_info. In the current STA
           implementation this condition is guaranteed if the STA BSS matches
           the AP BSS (previous condition)
    
    In the case that nodes is 0 AP and 0 STA and 2+ IBSS, then the following
    conditions must be met in order to return True
    
        #. BSS must match at all nodes and be non-null
    
    Args:
        nodes (list of WlanExpNode):  List of WlanExpNode / sub-classes of 
            WlanExpNode
        verbose (bool):  Print details on which nodes fail BSS membership checks
    
    Returns:
        members (bool):  Boolean indicating whether the nodes were members of the same BSS
    """
    import wlan_exp.node_ap   as node_ap
    import wlan_exp.node_sta  as node_sta
    import wlan_exp.node_ibss as node_ibss

    # Define filter methods
    is_ap   = lambda x: type(x) is node_ap.WlanExpNodeAp
    is_sta  = lambda x: type(x) is node_sta.WlanExpNodeSta
    is_ibss = lambda x: type(x) is node_ibss.WlanExpNodeIBSS

    # Define BSS info fields used by this method
    bss_info_fields = ['bssid', 'ssid', 'channel']

    # Define BSS info equality check based on bss_info_fields
    bss_cfg_eq = lambda x,y: all([x[f] == y[f] for f in bss_info_fields])

    # Define BSS info print method that only prints bss_info_fields    
    def print_bss_info(bss_info):
        msg = "BSS Info\n"
        for f in bss_info_fields:
            msg += "    {0:10s} = {1}\n".format(f, bss_info[f])
        return msg

    # Convert argument to list if it is not
    if type(nodes) is not list:
        nodes = [nodes]

    # Filter nodes by type
    ap    = list(filter(is_ap, nodes))
    stas  = list(filter(is_sta, nodes))
    ibsss = list(filter(is_ibss, nodes))


    # Check provided nodes argument
    #   (1) 1 AP and 1+ STA and 0  IBSS
    #   (2) 0 AP and 0  STA and 2+ IBSS
    if ((len(ap) == 0) and (len(stas) == 0) and (len(ibsss) == 0)):
        raise AttributeError('No wlan_exp nodes in list')

    if ((len(ibsss) > 0) and ((len(ap) > 0) or (len(stas) > 0))):
        raise AttributeError('Network cannot contain mix of IBSS and other node types')

    if (len(ap) > 1):
        raise AttributeError('{0} AP nodes in list - only 0 or 1 AP supported'.format(len(ap)))

    if ((len(ap) == 1) and (len(stas) == 0)):
        raise AttributeError('Network with 1 AP must include at least 1 STA')

    if (len(ibsss) == 1):
        raise AttributeError('Network with IBSS nodes must have 2 or more nodes')

    if ((len(ap) == 0) and (len(stas) == 1)):
        raise AttributeError('Network with STA must include 1 AP')

    msg = ''
    network_good = True

    ###################################
    # Infrastructure network (1 AP and 1+ STA and 0 IBSS)
    if (len(ap) == 1):
        # Check AP BSS info
        ap     = ap[0]
        ap_bss = ap.get_bss_info()
        
        if (ap_bss is None):
            msg += 'AP BSS is None - No BSS membership possible\n'
            network_good = False
        
        # Check AP station_infos
        if (network_good):
            for s in stas:
                if (not ap.is_associated(s)):
                    msg += '"{0}" not in AP association table\n'.format(repr(s))
                    network_good = False

        # Check STA BSS info
        if (network_good):
            for s in stas:
                sta_bss = s.get_bss_info()

                if (sta_bss is None):
                    msg += '"{0}" BSS is None - No BSS membership possible\n'.format(repr(s))
                    network_good = False
                else:
                    if (not bss_cfg_eq(ap_bss, sta_bss)):
                        msg += 'Mismatch between BSS Info:\n\n'
                        msg += '"{0}":\n{1}\n\n'.format(repr(ap), print_bss_info(ap_bss))
                        msg += '"{0}":\n{1}\n'.format(repr(s), print_bss_info(sta_bss))
                        network_good = False
        
    ###################################
    # Ad hoc network (0 AP and 0 STA and 2+ IBSS)
    elif (len(ibsss) > 1):
        # Check that all BSS infos match and are non-null
        #     - Use first node as arbitrary 'golden' config
        golden_bss = ibsss[0].get_bss_info()

        if (golden_bss is None):
            msg += '"{0}" BSS is None - No BSS membership possible\n'.format(repr(ibsss[0]))
            network_good = False
            
        if (network_good):
            for n in ibsss[1:]:
                n_bss = n.get_bss_info()

                if (n_bss is None):
                    msg += '"{0}" BSS is None - No BSS membership possible\n'.format(repr(n))
                    network_good = False
                else:
                    if (not bss_cfg_eq(golden_bss, n_bss)):
                        msg += 'Mismatch between BSS Info:\n\n'
                        msg += '"{0}":\n{1}\n\n'.format(repr(ibsss[0]), print_bss_info(golden_bss))
                        msg += '"{0}":\n{1}\n'.format(repr(n), print_bss_info(n_bss))
                        network_good = False
    else:
        # Other combination of nodes that somehow passed the nodes type checking above
        # 
        # This should be impossible with the reference AP/STA/IBSS node implementations
        # but will catch the case of a new WlanExpNode subclass that has not been added
        # to the nodes type checking
        raise AttributeError('Unreognized or invalid combination of node types')

    # Print message
    if verbose and msg:
        print(msg)

    return network_good



# -----------------------------------------------------------------------------
# Replicated Misc Utilities
# -----------------------------------------------------------------------------

#
# These utilities are replicated versions of other functions in wlan_exp.
# They are consolidated in util to ease import of wlan_exp for scripts.
#

def int_to_ip(ip_address):
    """Convert an integer to IP address string (dotted notation).

    Args:
        ip_address (int):  Unsigned 32-bit integer representation of the IP address

    Returns:
        ip_address (str):  String version of an IP address of the form W.X.Y.Z
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.int_to_ip(ip_address)

# End def


def ip_to_int(ip_address):
    """Convert IP address string (dotted notation) to an integer.

    Args:
        ip_address (str):  String version of an IP address of the form W.X.Y.Z

    Returns:
        ip_address (int):  Unsigned 32-bit integer representation of the IP address
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.ip_to_int(ip_address)

# End def


def mac_addr_to_str(mac_address):
    """Convert an integer to a colon separated MAC address string.

    Args:
        mac_address (int):  Unsigned 48-bit integer representation of the MAC address

    Returns:
        mac_address (str):  String version of an MAC address of the form XX:XX:XX:XX:XX:XX
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.mac_addr_to_str(mac_address)

# End def


def str_to_mac_addr(mac_address):
    """Convert a colon separated MAC address string to an integer.

    Args:
        mac_address (str):  String version of an MAC address of the form XX:XX:XX:XX:XX:XX

    Returns:
        mac_address (int):  Unsigned 48-bit integer representation of the MAC address
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.str_to_mac_addr(mac_address)

# End def


def mac_addr_to_byte_str(mac_address):
    """Convert an integer to a MAC address byte string.

    Args:
        mac_address (int):  Unsigned 48-bit integer representation of the MAC address

    Returns:
        mac_address (str):  Byte string version of an MAC address
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.mac_addr_to_byte_str(mac_address)

# End def


def byte_str_to_mac_addr(mac_address):
    """Convert a MAC address byte string to an integer.

    Args:
        mac_address (str):  Byte string version of an MAC address

    Returns:
        mac_address (int):  Unsigned 48-bit integer representation of the MAC address
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.byte_str_to_mac_addr(mac_address)

# End def


def buffer_to_str(buffer):
    """Convert a buffer of bytes to a formatted string.

    Args:
        buffer (bytes):  Buffer of bytes

    Returns:
        output (str):  Formatted string of the buffer byte values
    """
    import wlan_exp.transport.transport_eth_ip_udp as transport
    return transport.buffer_to_str(buffer)

# End def


def ver_code_to_str(ver_code):
    """Convert a wlan_exp version code to a string."""
    import wlan_exp.version as version
    return version.wlan_exp_ver_code_to_str(ver_code)

# End def


# -----------------------------------------------------------------------------
# Misc Utilities
# -----------------------------------------------------------------------------

def create_locally_administered_bssid(mac_address):
    """Create a locally administered BSSID.

    Set "locally administered" bit to '1' and "multicast" bit to '0'

    Args:
        mac_address (int, str):  MAC address to be used as the base for the BSSID
            either as a 48-bit integer or a colon delimited string of the form:
            XX:XX:XX:XX:XX:XX

    Returns:
        bssid (int):
            BSSID with the "locally administerd" bit set to '1' and the "multicast" bit set to '0'
    """
    if type(mac_address) is str:
        type_is_str     = True
        tmp_mac_address = str_to_mac_addr(mac_address)
    else:
        type_is_str     = False
        tmp_mac_address = mac_address

    tmp_mac_address = (tmp_mac_address | mac_addr_local_mask) & (mac_addr_broadcast - mac_addr_mcast_mask)

    if type_is_str:
        return mac_addr_to_str(tmp_mac_address)
    else:
        return tmp_mac_address

# End def


def is_locally_administered_bssid(bssid):
    """Is the BSSID a locally administered BSSID?

    Is "locally administered" bit to '1' and "multicast" bit to '0' of the BSSID?

    Args:
        bssid (int, str):  BSSID either as a 48-bit integer or a colon 
            delimited string of the form: XX:XX:XX:XX:XX:XX

    Returns:
        status (bool):

            * True      -- BSSID is locally administered BSSID
            * False     -- BSSID is not locally administered BSSID
    """
    if type(bssid) is str:
        tmp_bssid = str_to_mac_addr(bssid)
    else:
        tmp_bssid = bssid

    if (((tmp_bssid & mac_addr_local_mask) == mac_addr_local_mask) and 
        ((tmp_bssid & mac_addr_mcast_mask) == 0)):
        return True
    else:
        return False

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


def node_type_to_str(node_type, node_factory=None):
    """Convert the Node Type to a string description.

    Args:
        node_type (int):  node type ID (u32)
        node_factory (WlanExpNodeFactory): A WlanExpNodeFactory or subclass to
            create nodes of a given type

    Returns:
        node_type (str):  String representation of the node type

    By default, a dictionary of node types is built dynamically during init_nodes().
    If init_nodes() has not been run, then the method will try to create a node
    type dictionary.  If a node_factory is not provided then a default WlanExpNodeFactory
    will be used to determine the node type.  If a default WlanExpNodeFactory is used,
    then only framework node types will be known and custom node types will return:
    "Unknown node type: <value>"
    """
    global node_type_dict

    # Build a node_type_dict if it is not present
    if node_type_dict is None:

        # Build a default node_factory if it is not present
        if node_factory is None:
            import wlan_exp.node as node
            import wlan_exp.transport.config as config

            network_config = config.NetworkConfiguration()
            node_factory   = node.WlanExpNodeFactory(network_config)

        # Record the node type dictionary from the NodeFactory for later use
        node_type_dict = node_factory.get_type_dict()

    if (node_type in node_type_dict.keys()):
        return node_type_dict[node_type]['description']
    else:
        return "Unknown node type: 0x{0:8x}".format(node_type)

# End def


def mac_addr_desc(mac_addr, desc_map=None):
    """Returns a string description of a MAC address.

    This is useful when printing a table of addresses.  Custom MAC address
    descriptions can be provided via the desc_map argument.  In addition
    to the provided desc_map, the global mac_addr_desc_map that describes 
    mappings of different MAC addresses will also be used.

    Args:
        mac_address (int):  64-bit integer representing 48-bit MAC address
        desc_map (list of tuple, optional): list of tuple or tuple of the form
            (addr_mask, addr_value, descritpion)

    Returns:
        description (str):  Description of the MAC address or '' if address 
        does not match any descriptions

    The mac_address argument will be bitwise AND'd with each addr_mask, then 
    compared to addr_value. If the result is non-zero the corresponding 
    descprition will be returned.  This will only return the first description 
    in the [desc_map, mac_addr_desc_map] list.

    """
    # Cast to python int in case input is still numpy uint64
    mac_addr = int(mac_addr)

    desc_out = ''

    if(desc_map is None):
        desc_map = mac_addr_desc_map
    else:
        desc_map = list(desc_map) + mac_addr_desc_map

    for (req, mask, desc) in desc_map:
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
    tmp_nodes = [n for n in nodes if (n.node_type & 0xFFFF0000 == defaults.WLAN_EXP_80211_BASE)]

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
    """Time function to handle differences between Python 2.7 and 3.3"""
    try:
        return time.perf_counter()
    except AttributeError:
        return time.clock()

# End def


def _broadcast_time_to_nodes(time_cmd, network_config, time=0, time_id=None):
    """Internal method to issue broadcast time commands

    This method will iterate through all host interfaces and issue a
    broadcast packet on each interface that will set the time to the
    timebase.  The method keeps track of how long it takes to send
    each packet so that the time on all nodes is as close as possible
    even across interface.

    Attributes:
        time_cmd       -- NodeProcTime command to issue
        network_config -- A NetworkConfiguration object
        time           -- Optional time to broadcast to the nodes (defaults to 0)
                          as an integer number of microseconds
        time_id        -- Optional value to identify broadcast time commands across nodes
    """
    import wlan_exp.cmds as cmds

    time_factor = 6               # Time can be in # of microseconds (ie 10^(-6) seconds)

    # Need to convert time input to float so that it can be added to _time()
    if (type(time) in [int, long]):
        node_time   = float(time / (10**time_factor))   # Convert to float
    else:
        raise TypeError("Time must be an integer number of microseconds.")

    # Determine if data is being sent to multiple networks
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
            msg += "{0} to: {1}".format(network_addr, int(node_time * (10**time_factor)))
        elif (time_cmd == cmds.CMD_PARAM_TIME_ADD_TO_LOG):
            msg += "Adding current time to log for nodes on network {0}".format(network_addr)
        print(msg)

# End def


def _broadcast_cmd_to_nodes_helper(cmd, network_config):
    """Internal method to issue broadcast commands

    Attributes:
        network_config -- A NetworkConfiguration object
        cmd            -- A message.Cmd object describing the command
    """
    import wlan_exp.transport.transport_eth_ip_udp_py_broadcast as broadcast

    # Get information out of the NetworkConfiguration
    tx_buf_size  = network_config.get_param('tx_buffer_size')
    rx_buf_size  = network_config.get_param('rx_buffer_size')

    # Create broadcast transport and send message
    transport_broadcast = broadcast.TransportEthIpUdpPyBroadcast(network_config);

    transport_broadcast.transport_open(tx_buf_size, rx_buf_size)

    transport_broadcast.send(payload=cmd.serialize())

    transport_broadcast.transport_close()

# End def


