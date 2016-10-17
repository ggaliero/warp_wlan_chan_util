"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Print Tx/Rx Stats
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This module provides a simple wlan_exp example.

Hardware Setup:
  - Requires 1+ WARP v3 node running 802.11 Reference Design v1.5 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script will initialize the given nodes; extract any APs from the 
initialized nodes; then for each AP, it will get the associations and counts 
and display them.
------------------------------------------------------------------------------
"""
# Import Python modules
import time

# Import wlan_exp Framework
import wlan_exp.config as config
import wlan_exp.util as util

# Change these values to match your experiment / network setup
NETWORK              = '10.0.0.0'
USE_JUMBO_ETH_FRAMES = False
NODE_SERIAL_LIST     = ['W3-a-00001']

# TOP Level script variables
PROMISCUOUS_COUNTS   = True
CHANNEL              = 6

nodes = []


def initialize_experiment():
    """Initialize the wlan_exp experiment."""
    global nodes

    # Print initial message
    print("\nInitializing experiment\n")

    # Create an object that describes the network configuration of the host PC
    network_config = config.WlanExpNetworkConfiguration(network=NETWORK, 
                                                        jumbo_frame_support=USE_JUMBO_ETH_FRAMES)

    # Create an object that describes the WARP v3 nodes that will be used in this experiment
    nodes_config   = config.WlanExpNodesConfiguration(network_config=network_config,
                                                      serial_numbers=NODE_SERIAL_LIST)

    # Initialize the Nodes
    #   This will initialize all of the networking and gather the necessary
    #   information to control and communicate with the nodes
    nodes = util.init_nodes(nodes_config, network_config)

    # Initialize the time on all nodes to zero
    util.broadcast_cmd_set_mac_time(0, network_config)

    # Set the promiscuous counts mode
    for node in nodes:
        node.set_radio_channel(CHANNEL)
        node.reset(txrx_counts=True)


def run_experiment():
    """Run the experiment."""
    global nodes

    # Print initial message
    print("\nRunning experiment (Use Ctrl-C to exit)\n")

    while(True):

        # For each of the APs, get the counts
        for node in nodes:
            station_info = node.get_station_info()
            counts       = node.get_txrx_counts()
            print_counts(node, counts, station_info)

        print(92*"*")
        # Wait for 5 seconds
        time.sleep(5)


def print_counts(node, counts, station_info=None):
    """Helper method to print the counts."""
    print("-------------------- ----------------------------------- ----------------------------------- ")
    print("                                          Tx/Rx Counts from Node {0}".format(node.sn_str))
    print("                               Number of Packets                   Number of Bytes           ")
    print("ID                   Tx Data  Tx Mgmt  Rx Data  Rx Mgmt  Tx Data  Tx Mgmt  Rx Data  Rx Mgmt  ")
    print("-------------------- -------- -------- -------- -------- -------- -------- -------- -------- ")

    msg = ""
    for count in counts:
        count_id = count['mac_addr']

        if not station_info is None:
            for station in station_info:
                if (count['mac_addr'] == station['mac_addr']):
                    count_id  = station['host_name']
                    try:
                        count_id  = count_id.strip('\x00')
                    except:
                        pass
                    if ((count_id == '') or (count_id == b'')):
                        count_id = count['mac_addr']

        msg += "{0:<20} ".format(count_id)
        msg += "{0:8d} ".format(count['data_num_tx_packets_success'])
        msg += "{0:8d} ".format(count['mgmt_num_tx_packets_success'])
        msg += "{0:8d} ".format(count['data_num_rx_packets'])
        msg += "{0:8d} ".format(count['mgmt_num_rx_packets'])
        msg += "{0:8d} ".format(count['data_num_tx_bytes_success'])
        msg += "{0:8d} ".format(count['mgmt_num_tx_bytes_success'])
        msg += "{0:8d} ".format(count['data_num_rx_bytes'])
        msg += "{0:8d} ".format(count['mgmt_num_rx_bytes'])
        msg += "\n"
    print(msg)


def end_experiment():
    """Experiment cleanup / post processing."""
    global nodes
    print("\nEnding experiment\n")



if __name__ == '__main__':
    initialize_experiment();

    try:
        # Run the experiment
        run_experiment()
    except KeyboardInterrupt:
        pass

    end_experiment()
    print("\nExperiment Finished.")



