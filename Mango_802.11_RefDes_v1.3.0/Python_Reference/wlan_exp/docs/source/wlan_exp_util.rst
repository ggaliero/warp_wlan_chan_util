.. _wlan_exp_util:

.. include:: globals.rst

WLAN Exp Utilities
------------------
Common utilities for the WLAN Experiment framework.


.. automodule:: wlan_exp.util


Debug Print Levels
..................
Defined print levels that can be passed to WlanExpNode set_print_level() to control the verbosity of the 
WLAN Exp UART output of the node.

.. autoattribute:: wlan_exp.util.WLAN_EXP_PRINT_NONE
    :annotation: 

.. autoattribute:: wlan_exp.util.WLAN_EXP_PRINT_ERROR
    :annotation: 

.. autoattribute:: wlan_exp.util.WLAN_EXP_PRINT_WARNING
    :annotation: 

.. autoattribute:: wlan_exp.util.WLAN_EXP_PRINT_INFO
    :annotation: 

.. autoattribute:: wlan_exp.util.WLAN_EXP_PRINT_DEBUG
    :annotation: 


Transmit / Receive Rate Definitions
...................................
Supported transmit / receive rates for WlanExpNode 

.. autoattribute:: wlan_exp.util.wlan_rates
    :annotation: = Array of rate dictionaries 

.. autofunction:: wlan_exp.util.find_tx_rate_by_index

.. autofunction:: wlan_exp.util.tx_rate_to_str

.. autofunction:: wlan_exp.util.tx_rate_index_to_str


Channel Definitions
...................
Supported channels for WlanExpNode 

.. autoattribute:: wlan_exp.util.wlan_channel
    :annotation: = Array of channel dictionaries 

.. autofunction:: wlan_exp.util.find_channel_by_index

.. autofunction:: wlan_exp.util.find_channel_by_channel_number

.. autofunction:: wlan_exp.util.find_channel_by_freq

.. autofunction:: wlan_exp.util.channel_to_str

.. autofunction:: wlan_exp.util.channel_freq_to_str


Antenna Mode Definitions
........................
Supported antenna modes for WlanExpNode 

.. autoattribute:: wlan_exp.util.wlan_rx_ant_mode
    :annotation: = Array of receive antenna mode dictionaries 

.. autofunction:: wlan_exp.util.find_rx_ant_mode_by_index

.. autofunction:: wlan_exp.util.rx_ant_mode_to_str

.. autofunction:: wlan_exp.util.rx_ant_mode_index_to_str


.. autoattribute:: wlan_exp.util.wlan_tx_ant_mode
    :annotation: = Array of transmit antenna mode dictionaries 

.. autofunction:: wlan_exp.util.find_tx_ant_mode_by_index

.. autofunction:: wlan_exp.util.tx_ant_mode_to_str

.. autofunction:: wlan_exp.util.tx_ant_mode_index_to_str


MAC Address Definitions
.......................
Pre-defined MAC address constants

.. autoattribute:: wlan_exp.util.mac_addr_desc_map
    :annotation: = List of tuples (MAC value, mask, description) that describe various MAC address ranges

.. autoattribute:: wlan_exp.util.mac_addr_mcast_mask
    :annotation: = Multicast MAC Address Mask

.. autoattribute:: wlan_exp.util.mac_addr_local_mask
    :annotation: = Local MAC Address Mask

.. autoattribute:: wlan_exp.util.mac_addr_broadcast
    :annotation: = Broadcast MAC Address


Transmit Power Definitions
..........................

.. autofunction:: wlan_exp.util.get_node_max_tx_power

.. autofunction:: wlan_exp.util.get_node_min_tx_power



Node Utility Functions
......................

.. autofunction:: wlan_exp.util.init_nodes

.. autofunction:: wlan_exp.util.broadcast_cmd_set_time

.. autofunction:: wlan_exp.util.broadcast_cmd_write_time_to_logs

.. autofunction:: wlan_exp.util.broadcast_cmd_write_exp_info_to_logs

.. autofunction:: wlan_exp.util.filter_nodes


Misc Utility Functions
......................

.. autofunction:: wlan_exp.util.create_bss_info

.. autofunction:: wlan_exp.util.create_locally_administered_bssid

.. autofunction:: wlan_exp.util.int_to_ip

.. autofunction:: wlan_exp.util.ip_to_int

.. autofunction:: wlan_exp.util.mac_addr_to_str

.. autofunction:: wlan_exp.util.sn_to_str

.. autofunction:: wlan_exp.util.node_type_to_str

.. autofunction:: wlan_exp.util.mac_addr_desc

.. autofunction:: wlan_exp.util.debug_here








