"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Programmable Attenuator
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and WARPnet to measure throughput between
an AP and an associated STA using the AP's local traffic generator (LTG).

Hardware Setup:
  - Requires two WARP v3 nodes
    - One node configured as AP using 802.11 Reference Design v0.95 or later
    - One node configured as STA using 802.11 Reference Design v0.95 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch
  - An Aeroflex USB 4205 Series programmable attenuator connected via USB

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script initializes two WARP v3 nodes, one AP and one STA. It assumes the STA is
already associated with the AP. The script then initiates a traffic flow from the AP to
the STA, sets the AP Tx rate and measures throughput by counting the number of bytes
received successfully at the STA. This process repeats for STA -> AP and head-to-head
traffic flows.
------------------------------------------------------------------------------
"""
import sys
import time
import wlan_exp.config as wlan_exp_config
import wlan_exp.util as wlan_exp_util
import wlan_exp.ltg as wlan_exp_ltg

import numpy as np

from wlan_exp.prog_atten import ProgAttenController

#-------------------------------------------------------------------------
#  Global experiment variables
#

# NOTE: change these values to match your experiment setup
NETWORK           = '10.0.0.0'
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

# Set the per-trial duration (in seconds)
TRIAL_TIME        = 5



#-------------------------------------------------------------------------
#  Initialization
#
print("\nInitializing experiment\n")

pa = ProgAttenController()

# Create an object that describes the network configuration of the host PC
network_config = wlan_exp_config.WlanExpNetworkConfiguration(network=NETWORK)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = wlan_exp_config.WlanExpNodesConfiguration(network_config=network_config,
                                                           serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#   This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, network_config)

# Extract the different types of nodes from the list of initialized nodes
#   NOTE:  This will work for both 'DCF' and 'NOMAC' mac_low projects
n_ap_l  = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='AP',  serial_number=NODE_SERIAL_LIST)
n_sta_l = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='STA', serial_number=NODE_SERIAL_LIST)

# Check that we have a valid AP and STA
if (((len(n_ap_l) == 1) and (len(n_sta_l) == 1))):
    # Extract the two nodes from the lists for easier referencing below
    n_ap = n_ap_l[0]
    n_sta = n_sta_l[0]
else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print(" Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)



#-------------------------------------------------------------------------
#  Setup
#
print("\nExperimental Setup:")

# Set the rate of both nodes to 18 Mbps
rate = wlan_exp_util.wlan_rates[3]

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(rate, curr_assoc=True, new_assoc=True)
    node.reset(log=True, txrx_stats=True, ltg=True, queue_data=True) # Do not reset associations/bss_info

    # Get some additional information about the experiment
    channel  = node.get_channel()

    print("\n{0}:".format(node.name))
    print("    Channel  = {0}".format(wlan_exp_util.channel_to_str(channel)))
    print("    Rate     = {0}".format(wlan_exp_util.tx_rate_to_str(rate)))

print("")

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)



#-------------------------------------------------------------------------
#  Run Experiments
#
print("\nRun Experiment:")

# Experiments:
#   1) AP -> STA throughput
#   2) STA -> AP throughput
#   3) Head-to-head throughput
#
#   Since this experiment is basically the same for each iteration, we have pulled out 
# the main control variables so that we do not have repeated code for easier readability.
#

attens = np.arange(49,56,0.5)
xputs = [0]*len(attens)

ap_ltg_id  = n_ap.ltg_configure(wlan_exp_ltg.FlowConfigCBR(dest_addr=n_sta.wlan_mac_address,
                                                                       payload_length=1400, 
                                                                       interval=0), auto_start=False)

for idx,atten in enumerate(attens):
    pa.set_atten(atten)
    time.sleep(0.100)
    
    n_ap.ltg_start(ap_ltg_id)

    # Record the initial Tx/Rx stats
    sta_rx_stats_start = n_sta.stats_get_txrx(n_ap)
    ap_rx_stats_start  = n_ap.stats_get_txrx(n_sta)
    
    # Wait for the TRIAL_TIME
    time.sleep(TRIAL_TIME)
    
    # Record the ending Tx/Rx stats
    sta_rx_stats_end = n_sta.stats_get_txrx(n_ap)
    ap_rx_stats_end  = n_ap.stats_get_txrx(n_sta)
    
    n_ap.ltg_stop(ap_ltg_id)
    n_ap.queue_tx_data_purge_all()
    
    sta_num_bits  = float((sta_rx_stats_end['data_num_rx_bytes'] - sta_rx_stats_start['data_num_rx_bytes']) * 8)
    sta_time_span = float(sta_rx_stats_end['timestamp'] - sta_rx_stats_start['timestamp'])
    sta_xput      = sta_num_bits / sta_time_span
    xputs[idx] = sta_xput
    
    print('{0} db \t {1} Mbps'.format(atten, sta_xput))

pa.close()

    