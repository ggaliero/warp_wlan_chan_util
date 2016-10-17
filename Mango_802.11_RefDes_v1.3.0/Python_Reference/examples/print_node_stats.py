"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Print Tx/Rx Stats
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This module provides a simple WARPNet example.

Hardware Setup:
  - Requires 1+ WARP v3 node running 802.11 Reference Design v0.895 or later
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script will initialize the given nodes; extract any APs from the initialized nodes;
then for each AP, it will get the associations and statistics and display them.
------------------------------------------------------------------------------
"""
# Import Python modules
import time

# Import WARPNet Framework
import wlan_exp.config as config

# Import WLAN Exp Framework
import wlan_exp.util as wlan_exp_util

# TOP Level script variables
NETWORK                = '10.0.0.0'
NODE_SERIAL_LIST       = ['W3-a-00001']
PROMISCUOUS_STATISTICS = True
CHANNEL = 6

nodes = []

def initialize_experiment():
    """Initialize the WLAN Exp experiment."""
    global nodes

    # Print initial message
    print("\nInitializing experiment\n")

    # Create a NetworkConfiguration
    #   This describes the netwokr configuration.
    network_config = config.WlanExpNetworkConfiguration(network=NETWORK)

    # Create a WnNodesConfiguration
    #   This describes each node to be initialized
    nodes_config   = config.WlanExpNodesConfiguration(network_config=network_config,
                                                      serial_numbers=NODE_SERIAL_LIST)

    # Initialize the Nodes
    #   This will initialize all of the networking and gather the necessary
    #   information to control and communicate with the nodes
    nodes = wlan_exp_util.init_nodes(nodes_config, network_config)

    # Initialize the time on all nodes to zero
    wlan_exp_util.broadcast_cmd_set_time(0.0, network_config)

    # Set the promiscuous statistics mode
    for node in nodes:
        node.stats_configure_txrx(promisc_stats=PROMISCUOUS_STATISTICS)
        node.reset(txrx_stats=True)
        node.set_channel(CHANNEL)
        node.set_low_to_high_rx_filter(mac_header='ALL_MPDU', fcs='GOOD')


def run_experiment():
    """WLAN Experiment."""
    global nodes

    # Print initial message
    print("\nRunning experiment\n")

    while(True):

        # For each of the APs, get the statistics
        for node in nodes:
            station_info = node.get_station_info()
            stats        = node.stats_get_txrx()
            print_stats(node, stats, station_info)

        print(92*"*")
        # Wait for 5 seconds
        time.sleep(5)


def print_stats(node, stats, station_info=None):
    """Helper method to print the statistics."""
    print("-------------------- ----------------------------------- ----------------------------------- ")
    print("                                          Tx/Rx Stats from Node {0}".format(node.sn_str))
    print("                               Number of Packets                   Number of Bytes           ")
    print("ID                   Tx Data  Tx Mgmt  Rx Data  Rx Mgmt  Tx Data  Tx Mgmt  Rx Data  Rx Mgmt  ")
    print("-------------------- -------- -------- -------- -------- -------- -------- -------- -------- ")

    msg = ""
    for stat in stats:
        stat_id = stat['mac_addr']

        hostname = False
        if not station_info is None:
            for station in station_info:
                if (stat['mac_addr'] == station['mac_addr']):
                    stat_id  = station['host_name']
                    stat_id  = stat_id.strip('\x00')
                    if (stat_id == ''):
                        stat_id  = stat['mac_addr']
                        hostname = False
                    else:
                        hostname = True

        if not hostname:
            stat_id = ''.join('{0:02X}:'.format(ord(x)) for x in stat_id)[:-1]

        msg += "{0:<20} ".format(stat_id)
        msg += "{0:8d} ".format(stat['data_num_tx_packets_success'])
        msg += "{0:8d} ".format(stat['mgmt_num_tx_packets_success'])
        msg += "{0:8d} ".format(stat['data_num_rx_packets'])
        msg += "{0:8d} ".format(stat['mgmt_num_rx_packets'])
        msg += "{0:8d} ".format(stat['data_num_tx_bytes_success'])
        msg += "{0:8d} ".format(stat['mgmt_num_tx_bytes_success'])
        msg += "{0:8d} ".format(stat['data_num_rx_bytes'])
        msg += "{0:8d} ".format(stat['mgmt_num_rx_bytes'])
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



