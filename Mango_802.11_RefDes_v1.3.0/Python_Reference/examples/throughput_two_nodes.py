"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Two Node Throughput
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

#-------------------------------------------------------------------------
#  Global experiment variables
#

# NOTE: change these values to match your experiment setup
NETWORK           = '10.0.0.0'
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

# Set the per-trial duration (in seconds)
TRIAL_TIME        = 10
CHANNEL = 1

#-------------------------------------------------------------------------
#  Initialization
#
print("\nInitializing experiment\n")

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
n_ap_l   = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='AP',   serial_number=NODE_SERIAL_LIST, warn=False)
n_sta_l  = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='STA',  serial_number=NODE_SERIAL_LIST, warn=False)
n_ibss_l = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='IBSS', serial_number=NODE_SERIAL_LIST, warn=False)

# Check that we have a valid setup:
#   1) AP and STA
#     or
#   2) Two IBSS nodes
if len(n_ap_l) == 1 and len(n_sta_l) == 1:
    # Setup the two nodes
    node1 = n_ap_l[0]
    node2 = n_sta_l[0]

    # Tune both nodes to the same channel
    node1.set_channel(CHANNEL)
    node2.set_channel(CHANNEL)

    # Establish the association state between nodes
    node1.add_association(node2)

elif len(n_ibss_l) == 2:
    # Setup the two nodes
    node1 = n_ibss_l[0]
    node2 = n_ibss_l[1]

    # Create the BSS_INFO describing the ad-hoc network
    bssid = wlan_exp_util.create_locally_administered_bssid(node1)
    bss_info = wlan_exp_util.create_bss_info(bssid=bssid, ssid='WARP Xput IBSS', channel=CHANNEL)

    # Add both nodes to the new IBSS
    node1.join(bss_info)
    node2.join(bss_info)

else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print("    Ensure two nodes are ready, either:\n")
    print("        1) one using the AP design, one using the STA design, or\n")
    print("        2) two using the IBSS design\n")
    sys.exit(0)

#-------------------------------------------------------------------------
#  Setup
#
#sys.exit(0)

print("\nExperimental Setup:")

# Set the rate of both nodes to 18 Mbps
rate = wlan_exp_util.wlan_rates[3]

# Put each node in a known, good state
for node in [node1, node2]:
    node.set_tx_rate_unicast(rate, curr_assoc=True, new_assoc=True)
    node.reset(log=True, txrx_stats=True, ltg=True, queue_data=True) # Do not reset associations/bss_info

    msg = ""
    if (node == node1):
        msg += "\nNode 1: \n"
    if (node == node2):
        msg += "\nNode 2: \n"

    msg += "    Description = {0}\n".format(node.description)
    msg += "    Channel     = {0}\n".format(wlan_exp_util.channel_to_str(CHANNEL))
    msg += "    Rate        = {0}\n".format(wlan_exp_util.tx_rate_to_str(rate))
    print(msg)

print("")

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not node1.is_associated(node2):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that both nodes are associated.")
    sys.exit(0)

#-------------------------------------------------------------------------
#  Run Experiments
#
print("\nRun Experiment:")

# Experiments:
#   1) Node1 -> Node2 throughput
#   2) Node2 -> Node1 throughput
#   3) Head-to-head throughput
#
#   Since this experiment is basically the same for each iteration, we have pulled out
# the main control variables so that we do not have repeated code for easier readability.
#
experiment_params = [{'node1_ltg_en' : True,  'node2_ltg_en' : False, 'desc' : 'Node 1 -> Node 2'},
                     {'node1_ltg_en' : False, 'node2_ltg_en' : True,  'desc' : 'Node 2 -> Node 1'},
                     {'node1_ltg_en' : True,  'node2_ltg_en' : True,  'desc' : 'Head-to-Head'}]


#-------------------------------------------------------------------------
#  Experiment:  Compute throughput from node statistics
#
for experiment in experiment_params:

    print("\nTesting {0} throughput for rate {1} ...".format(experiment['desc'], wlan_exp_util.tx_rate_to_str(rate)))

    # Start a flow from the AP's local traffic generator (LTG) to the STA
    #    Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
    #    Start the flow immediately
    if (experiment['node1_ltg_en']):
        node1_ltg_id  = node1.ltg_configure(wlan_exp_ltg.FlowConfigCBR(dest_addr=node2.wlan_mac_address,
                                                                       payload_length=1400, 
                                                                       interval=0), auto_start=True)

    # Start a flow from the STA's local traffic generator (LTG) to the AP
    #    Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
    #    Start the flow immediately
    if (experiment['node2_ltg_en']):
        node2_ltg_id  = node2.ltg_configure(wlan_exp_ltg.FlowConfigCBR(dest_addr=node1.wlan_mac_address,
                                                                       payload_length=1400, 
                                                                       interval=0), auto_start=True)

    # Record the initial Tx/Rx stats
    #   NOTE: Since these are RX statistics, we can only see those at the opposite node.  For example, to see the
    #      packets received from Node 1 at Node 2, we need to get the TX/RX stats from Node 2 for Node 1.  This is
    #      opposite of TX statistics in which to see the packets transmitted from Node 1 to Node 2 we would need
    #      to get the TX/RX stats from Node 1 for Node 2.  In this example, since we are interested in received
    #      throughput (not transmitted throughput), we need to use the received (RX) statistics.
    node2_txrx_stats_for_node1_start = node2.stats_get_txrx(node1)
    node1_txrx_stats_for_node2_start = node1.stats_get_txrx(node2)

    # Wait for the TRIAL_TIME
    time.sleep(TRIAL_TIME)

    # Record the ending Tx/Rx stats
    node2_txrx_stats_for_node1_end = node2.stats_get_txrx(node1)
    node1_txrx_stats_for_node2_end = node1.stats_get_txrx(node2)

    # Stop the AP LTG flow and purge any remaining transmissions in the queue so that nodes are in a known, good state
    if (experiment['node1_ltg_en']):
        node1.ltg_stop(node1_ltg_id)
        node1.ltg_remove(node1_ltg_id)
        node1.queue_tx_data_purge_all()

    # Stop the STA LTG flow and purge any remaining transmissions in the queue so that nodes are in a known, good state
    if (experiment['node2_ltg_en']):
        node2.ltg_stop(node2_ltg_id)
        node2.ltg_remove(node2_ltg_id)
        node2.queue_tx_data_purge_all()

    # Compute the throughput
    # NOTE:  Timestamps are in microseconds; bits/usec == Mbits/sec
    # NOTE:  In Python 3.x, the division operator is always floating point.  In order to be compatible with all versions
    #    of python, cast operands to floats to ensure floating point division
    #
    node1_to_node2_num_bits  = float((node2_txrx_stats_for_node1_end['data_num_rx_bytes'] - node2_txrx_stats_for_node1_start['data_num_rx_bytes']) * 8)
    node1_to_node2_time_span = float(node2_txrx_stats_for_node1_end['timestamp'] - node2_txrx_stats_for_node1_start['timestamp'])
    node1_to_node2_xput      = node1_to_node2_num_bits / node1_to_node2_time_span
    print("    Node 1 -> Node 2:  Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], node1_to_node2_xput))

    node2_to_node1_num_bits  = float((node1_txrx_stats_for_node2_end['data_num_rx_bytes'] - node1_txrx_stats_for_node2_start['data_num_rx_bytes']) * 8)
    node2_to_node1_time_span = float(node1_txrx_stats_for_node2_end['timestamp'] - node1_txrx_stats_for_node2_start['timestamp'])
    node2_to_node1_xput      = node2_to_node1_num_bits / node2_to_node1_time_span
    print("    Node 2 -> Node 1:  Rate = {0:>4.1f} Mbps   Throughput = {1:>5.2f} Mbps".format(rate['rate'], node2_to_node1_xput))


