"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Identify Nodes Example
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script blinks the right hex display of all nodes to ensure broadcast
communication on the network.

Hardware Setup:
  - Requires one or more WARP v3 nodes configured with the 802.11 Reference
      Design v0.81 or later.
  - PC NIC and ETH B on WARP v3 nodes connected to common gigbit Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)

Description:
  This script will cause all nodes on each of the network to blink their hex
displays.  If a node has not been initialized with an IP address (ie, it has
just booted and and has not had its network configured via init_nodes()), then
it will respond to all broadcast packets that are seen regardless of the
network.  This is useful to test connectivity regardless of further network
segmentation during the experiment.
------------------------------------------------------------------------------
"""
import wlan_exp.warpnet.config as wn_config
import wlan_exp.warpnet.util as wn_util

# NOTE: change these values to match your experiment setup
NETWORK = '10.0.0.0'

# Create the network configuration
network_config = wn_config.NetworkConfiguration(network=NETWORK)

# Issue identify all command on the network
wn_util.wn_identify_all_nodes(network_config)
