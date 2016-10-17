.. _wlan_exp_util:

.. include:: globals.rst

WLAN Exp Utilities
------------------
Common utilities for the WLAN Experiment framework.


UART Print Levels
.................
Defined UART print levels to control the verbosity of the wlan_exp UART output.

.. autodata:: wlan_exp.util.uart_print_levels


Transmit / Receive Rate Definitions
...................................
Supported transmit / receive rates for WlanExpNode 

.. autodata:: wlan_exp.util.phy_modes

.. autofunction:: wlan_exp.util.get_rate_info

.. autofunction:: wlan_exp.util.rate_info_to_str


Channel Definitions
...................
Supported channels for WlanExpNode 

.. autodata:: wlan_exp.util.wlan_channels

.. autofunction:: wlan_exp.util.get_channel_info

.. autofunction:: wlan_exp.util.channel_info_to_str


Antenna Mode Definitions
........................
Supported antenna modes for WlanExpNode 

.. autoattribute:: wlan_exp.util.wlan_rx_ant_modes
    :annotation: = Dictionary of receive antenna modes


.. autoattribute:: wlan_exp.util.wlan_tx_ant_modes
    :annotation: = Dictionary of transmit antenna modes


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


Node Utility Functions
......................

.. autofunction:: wlan_exp.util.init_nodes

.. autofunction:: wlan_exp.util.broadcast_cmd_set_mac_time

.. autofunction:: wlan_exp.util.broadcast_cmd_write_time_to_logs

.. autofunction:: wlan_exp.util.broadcast_cmd_write_exp_info_to_logs

.. autofunction:: wlan_exp.util.filter_nodes

.. autofunction:: wlan_exp.util.check_bss_membership


Misc Utility Functions
......................

.. autofunction:: wlan_exp.util.create_locally_administered_bssid

.. autofunction:: wlan_exp.util.int_to_ip

.. autofunction:: wlan_exp.util.ip_to_int

.. autofunction:: wlan_exp.util.mac_addr_to_str

.. autofunction:: wlan_exp.util.str_to_mac_addr

.. autofunction:: wlan_exp.util.mac_addr_to_byte_str

.. autofunction:: wlan_exp.util.byte_str_to_mac_addr

.. autofunction:: wlan_exp.util.sn_to_str

.. autofunction:: wlan_exp.util.node_type_to_str

.. autofunction:: wlan_exp.util.mac_addr_desc

.. autofunction:: wlan_exp.util.debug_here








