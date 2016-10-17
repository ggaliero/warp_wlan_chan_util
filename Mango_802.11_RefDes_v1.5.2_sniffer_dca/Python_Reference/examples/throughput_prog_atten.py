"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Programmable Attenuator
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and wlan_exp to measure throughput 
between an AP and an associated STA using the AP's local traffic generator 
(LTG) and a programmable attenuator.

Hardware Setup:
  - Requires two WARP v3 nodes
    - One node configured as AP using 802.11 Reference Design v1.5 or later
    - One node configured as STA using 802.11 Reference Design v1.5 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch
  - An Aeroflex USB 4205 Series programmable attenuator connected via USB

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script initializes two WARP v3 nodes, one AP and one STA. It will use
wlan_exp commands to set up associations between the AP and STA.  After 
initalizing the nodes, the script then sets the attenuation value, initiates a 
traffic flow from the AP to the STA, and measures throughput by counting the 
number of bytes received successfully at the STA. This process repeats for 
each attenuation value.
------------------------------------------------------------------------------
"""
import sys
import time
import wlan_exp.config as config
import wlan_exp.util as util
import wlan_exp.ltg as ltg

import numpy as np

from wlan_exp.prog_atten import ProgAttenController

#-------------------------------------------------------------------------
#  Global experiment variables
#

# Change these values to match your experiment / network setup
NETWORK              = '10.0.0.0'
USE_JUMBO_ETH_FRAMES = False
NODE_SERIAL_LIST     = ['W3-a-00001', 'W3-a-00002']

# Set the per-trial duration (in seconds)
TRIAL_TIME           = 5

# BSS parameters
SSID                 = "WARP Prog Atten Ex"
CHANNEL              = 1
BEACON_INTERVAL      = 100


#-------------------------------------------------------------------------
#  Initialization
#
print("\nInitializing experiment\n")

pa = ProgAttenController()

# Create an object that describes the network configuration of the host PC
network_config = config.WlanExpNetworkConfiguration(network=NETWORK,
                                                    jumbo_frame_support=USE_JUMBO_ETH_FRAMES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = config.WlanExpNodesConfiguration(network_config=network_config,
                                                  serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#   This command will fail if either WARP v3 node does not respond
nodes = util.init_nodes(nodes_config, network_config)

# Extract the different types of nodes from the list of initialized nodes
#     - This will work for both 'DCF' and 'NOMAC' mac_low projects
n_ap_l  = util.filter_nodes(nodes=nodes, mac_high='AP',  serial_number=NODE_SERIAL_LIST)
n_sta_l = util.filter_nodes(nodes=nodes, mac_high='STA', serial_number=NODE_SERIAL_LIST)

# Check that setup is valid
if (((len(n_ap_l) == 1) and (len(n_sta_l) == 1))):
    # Extract the two nodes from the lists for easier referencing below
    n_ap  = n_ap_l[0]
    n_sta = n_sta_l[0]
else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print(" Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)



#-------------------------------------------------------------------------
#  Setup
#
print("\nExperimental Setup:")

# Configure the AP to reject authentication requests from wireless clients
#    - Uncomment this line to block any wireless associations during the experiment
# n_ap.set_authentication_address_filter(allow='NONE')

# Set association state
#     - Configure AP BSS
#     - Create Associatiion between AP and STA
n_ap.configure_bss(ssid=SSID, channel=CHANNEL, beacon_interval=BEACON_INTERVAL)
n_ap.add_association(n_sta)

# Set the rate of both nodes to 18 Mbps (mcs = 3, phy_mode = 'NONHT')
mcs       = 3
phy_mode  = util.phy_modes['NONHT']
rate_info = util.get_rate_info(mcs, phy_mode)

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(mcs, phy_mode, curr_assoc=True, new_assoc=True)
    node.reset(log=True, txrx_counts=True, ltg=True, queue_data=True) # Do not reset associations/bss_info

    # Get some additional information about the experiment
    bss_info = node.get_bss_info()
    channel  = bss_info['channel']

    print("\n{0}:".format(node.name))
    print("    Channel  = {0}".format(util.channel_info_to_str(util.get_channel_info(channel))))
    print("    Rate     = {0}".format(util.rate_info_to_str(rate_info)))

print("")

# Check that the nodes are part of the same BSS.  Otherwise, the LTGs below will fail.
if not util.check_bss_membership([n_ap, n_sta]):
    print("\nERROR: Nodes are not part of the same BSS.")
    util.check_bss_membership([n_ap, n_sta], verbose=True)
    print("Ensure that both nodes are part of the same BSS.")
    sys.exit(0)


#-------------------------------------------------------------------------
#  Run Experiments
#
print("\nRun Experiment:")

# Experiments:
#   1) AP -> STA throughput for each attenuation value
#

attens = np.arange(49,56,0.5)
xputs = [0]*len(attens)

ap_ltg_id  = n_ap.ltg_configure(ltg.FlowConfigCBR(dest_addr=n_sta.wlan_mac_address,
                                                  payload_length=1400, 
                                                  interval=0), auto_start=False)

for idx,atten in enumerate(attens):
    pa.set_atten(atten)
    time.sleep(0.100)
    
    n_ap.ltg_start(ap_ltg_id)

    # Record the initial Tx/Rx counts
    sta_rx_counts_start = n_sta.get_txrx_counts(n_ap)
    ap_rx_counts_start  = n_ap.get_txrx_counts(n_sta)
    
    # Wait for the TRIAL_TIME
    time.sleep(TRIAL_TIME)
    
    # Record the ending Tx/Rx counts
    sta_rx_counts_end = n_sta.get_txrx_counts(n_ap)
    ap_rx_counts_end  = n_ap.get_txrx_counts(n_sta)
    
    n_ap.ltg_stop(ap_ltg_id)
    n_ap.queue_tx_data_purge_all()
    
    sta_num_bits  = float((sta_rx_counts_end['data_num_rx_bytes'] - sta_rx_counts_start['data_num_rx_bytes']) * 8)
    sta_time_span = float(sta_rx_counts_end['retrieval_timestamp'] - sta_rx_counts_start['retrieval_timestamp'])
    sta_xput      = sta_num_bits / sta_time_span
    xputs[idx] = sta_xput
    
    print('{0} db \t {1} Mbps'.format(atten, sta_xput))

pa.close()

    