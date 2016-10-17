.. _wlan_exp_node:

.. include:: globals.rst

Node Classes
------------

The ``WlanExpNode`` class represents one node in a network of nodes running the 802.11 Reference Design. This class
is the primary interface for interacting with nodes by providing methods to send commands and read status 
of the node. This is the base class for all node types and provides the common functionality for all nodes.


Base Node Class
...............
The ``WlanExpNode`` class implements the interface to an 802.11 Reference Design node. The ``WlanExpNode`` class should
**not** be instantiated directly. Refer to the ``wlan_exp`` examples (http://warpproject.org/trac/wiki/802.11/wlan_exp/examples)
for the recommend node initialization flow which returns a properly initialized ``WlanExpNode`` instance.

The class attributes listed below should be considered read-only. The relevant attributes are given values during
the initialization process. These values represent the identity and state of the node running the 802.11 Reference
Design. Changing an attribute value does not change the corresponding state of the node, a mismatch that will distrupt
normal operation of ``wlan_exp`` tools.

.. autoclass:: wlan_exp.node.WlanExpNode


Node Sub-Classes
...........................

The AP, STA and IBSS node types are represented by dedicated subclasses of ``WlanExpNode``. The node objects in ``wlan_exp``
scripts will be instances of these subclasses. Each subclass implements methods that are specific to a given node type.

.. toctree::
    :maxdepth: 1

    node_ap.rst
    node_sta.rst
    node_ibss.rst


Common Node Methods
...................
The list below documents each node method implemented by the ``WlanExpNode`` class. These methods can be used with any
``wlan_exp`` node type (AP, STA, IBSS).

Node
````
These ``WlanExpNode`` commands are used to interact with the node and control parameters associated with the node operation.

.. automethod:: wlan_exp.node.WlanExpNode.reset_all
.. automethod:: wlan_exp.node.WlanExpNode.reset
.. automethod:: wlan_exp.node.WlanExpNode.get_wlan_mac_address
.. automethod:: wlan_exp.node.WlanExpNode.set_mac_time
.. automethod:: wlan_exp.node.WlanExpNode.get_mac_time
.. automethod:: wlan_exp.node.WlanExpNode.get_system_time
.. automethod:: wlan_exp.node.WlanExpNode.enable_beacon_mac_time_update
.. automethod:: wlan_exp.node.WlanExpNode.set_radio_channel
.. automethod:: wlan_exp.node.WlanExpNode.set_low_to_high_rx_filter
.. automethod:: wlan_exp.node.WlanExpNode.set_low_param
.. automethod:: wlan_exp.node.WlanExpNode.set_dcf_param
.. automethod:: wlan_exp.node.WlanExpNode.configure_pkt_det_min_power
.. automethod:: wlan_exp.node.WlanExpNode.set_phy_samp_rate
.. automethod:: wlan_exp.node.WlanExpNode.set_random_seed
.. automethod:: wlan_exp.node.WlanExpNode.enable_dsss
.. automethod:: wlan_exp.node.WlanExpNode.set_print_level

.. automethod:: wlan_exp.node.WlanExpNode.queue_tx_data_purge_all

.. automethod:: wlan_exp.node.WlanExpNode.send_user_command

.. automethod:: wlan_exp.node.WlanExpNode.identify
.. automethod:: wlan_exp.node.WlanExpNode.ping


Tx Power and Rate
`````````````````
These commands configure the transmit power and rate selections at the node. Powers and rates are
configured individually for various packet types. For unicast packets, Tx powers and rates can be
configured for specific destination addresses as well.

.. automethod:: wlan_exp.node.WlanExpNode.get_tx_power
.. automethod:: wlan_exp.node.WlanExpNode.set_tx_power

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_power_ctrl

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_rate_unicast
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_rate_unicast

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_rate_multicast_data
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_rate_multicast_data

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_rate_multicast_mgmt
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_rate_multicast_mgmt

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_power_unicast
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_power_unicast

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_power_multicast_data
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_power_multicast_data

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_power_multicast_mgmt
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_power_multicast_mgmt


Antenna Modes
`````````````
The WARP v3 hardware integrates two RF interfaces. These commands control which RF interface 
is used for Tx and Rx of individual packet types.

.. automethod:: wlan_exp.node.WlanExpNode.set_rx_ant_mode
.. automethod:: wlan_exp.node.WlanExpNode.get_rx_ant_mode

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_ant_mode_unicast
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_ant_mode_unicast

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_ant_mode_multicast_data
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_ant_mode_multicast_data

.. automethod:: wlan_exp.node.WlanExpNode.set_tx_ant_mode_multicast_mgmt
.. automethod:: wlan_exp.node.WlanExpNode.get_tx_ant_mode_multicast_mgmt


Association State
`````````````````
These ``WlanExpNode`` commands are used to modify / query the association state of the node.

.. automethod:: wlan_exp.node.WlanExpNode.configure_bss
.. automethod:: wlan_exp.node.WlanExpNode.get_station_info
.. automethod:: wlan_exp.node.WlanExpNode.get_bss_info
.. automethod:: wlan_exp.node.WlanExpNode.get_network_list


Tx/Rx Packet Counts
```````````````````
These ``WlanExpNode`` commands are used to to interact with the counts framework.  
Counts are kept for for each node in the station info list.  If promiscuous 
counts are enabled, then the node will keep counts for every MAC address 
overheard (whether the node is in the station info list or not).  In order to 
keep the maximum number of counts recorded on the node to a reasonable amount, 
there is a maximum number of counts implemented in the C code.  When that 
maximum is reached, then the oldest counts structure of an unassociated node 
will be overwritten.

.. automethod:: wlan_exp.node.WlanExpNode.get_txrx_counts


Local Traffic Generator (LTG)
`````````````````````````````
These ``WlanExpNode`` commands interact with the node's LTG framework. LTGs provides local traffic sources with configurable
destimations, payloads, and traffic loads. These traffic sources are ideal for running experiments without external
traffic sources connected via Ethernet.

Creating an LTG consumes memory in the node's heap. LTG creation can fail if the node's heap is full. Always
remove LTGs you no longer need using the ``ltg_remove`` or ``ltg_remove_all`` methods.

LTG traffic flows are configured with dedicated classes. See :doc:`ltg` for more information on how the payloads
and timing of LTG flows are controlled.

.. automethod:: wlan_exp.node.WlanExpNode.ltg_configure
.. automethod:: wlan_exp.node.WlanExpNode.ltg_get_status
.. automethod:: wlan_exp.node.WlanExpNode.ltg_remove
.. automethod:: wlan_exp.node.WlanExpNode.ltg_start
.. automethod:: wlan_exp.node.WlanExpNode.ltg_stop
.. automethod:: wlan_exp.node.WlanExpNode.ltg_remove_all
.. automethod:: wlan_exp.node.WlanExpNode.ltg_start_all
.. automethod:: wlan_exp.node.WlanExpNode.ltg_stop_all


Log
```
These ``WlanExpNode`` commands are used to interact with the logging framework.  
The log occupies a large portion of DRAM which is set in C code during runtime 
(see `wlan_mac_high.h <http://warpproject.org/trac/browser/ReferenceDesigns/w3_802.11/c/wlan_mac_high_framework/include/wlan_mac_high.h>`_ 
for more information on the memory map; Use ``log_get_capacity()`` to see the 
number of bytes allocated for the log).

The log has the ability to wrap to enable longer experiments.  By enabling 
wrapping and periodically pulling the logs, you can effectively log an 
experiment for an indefinite amount of time.  Also, the log can record the 
entire payload of the wireless packets.  Both the log wrapping and logging of 
full payloads is off by default and can be modified with the 
``log_configure()`` command.

.. automethod:: wlan_exp.node.WlanExpNode.log_configure
.. automethod:: wlan_exp.node.WlanExpNode.log_get
.. automethod:: wlan_exp.node.WlanExpNode.log_get_all_new
.. automethod:: wlan_exp.node.WlanExpNode.log_get_size
.. automethod:: wlan_exp.node.WlanExpNode.log_get_capacity
.. automethod:: wlan_exp.node.WlanExpNode.log_get_indexes
.. automethod:: wlan_exp.node.WlanExpNode.log_get_flags
.. automethod:: wlan_exp.node.WlanExpNode.log_is_full
.. automethod:: wlan_exp.node.WlanExpNode.log_write_exp_info
.. automethod:: wlan_exp.node.WlanExpNode.log_write_time


Network Scan
````````````
These ``WlanExpNode`` commands are used to to scan the node's environment.

.. automethod:: wlan_exp.node.WlanExpNode.set_scan_parameters
.. automethod:: wlan_exp.node.WlanExpNode.start_network_scan
.. automethod:: wlan_exp.node.WlanExpNode.stop_network_scan
.. automethod:: wlan_exp.node.WlanExpNode.is_scanning
