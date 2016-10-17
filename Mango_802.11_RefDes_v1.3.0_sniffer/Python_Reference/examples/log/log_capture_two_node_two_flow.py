"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Two Node Log capture
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and WARPnet to create a log
file that contains all data assocated with an experiment of head-to-head
backlogged data transfer using the local traffic generators.

Hardware Setup:
  - Requires two WARP v3 nodes
    - One node configured as AP using 802.11 Reference Design v0.95 or later
    - One node configured as STA using 802.11 Reference Design v0.95 or later
   - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script initializes two WARP v3 nodes, one AP and one STA. It assumes
the STA is already associated with the AP.  The script then initializes all of
the experiment parameters and starts a traffic flow from the AP to the STA and
the STA to the AP.  Finally, it resets the log, allows the experiment to run
and then captures all the log data after the TRIAL_TIME.
------------------------------------------------------------------------------
"""
import sys
import time

import wlan_exp.config as wlan_exp_config
import wlan_exp.util as wlan_exp_util
import wlan_exp.ltg as wlan_exp_ltg


#-----------------------------------------------------------------------------
# Top Level Script Variables
#-----------------------------------------------------------------------------
# NOTE: change these values to match your experiment setup
NETWORK           = '10.0.0.0'
NODE_SERIAL_LIST  = ['W3-a-00001', 'W3-a-00002']

AP_HDF5_FILENAME  = "ap_two_node_two_flow_capture.hdf5"
STA_HDF5_FILENAME = "sta_two_node_two_flow_capture.hdf5"

CHANNEL = 1

# Set the experiment duration (in seconds)
TRIAL_TIME        = 60

#-----------------------------------------------------------------------------
# Local Helper Utilities
#-----------------------------------------------------------------------------
def write_log_file(filename, node, exp_name):
    """Writes all the log data from the node to a HDF5 file."""

    import datetime
    import wlan_exp.log.util_hdf as hdf_util
    import wlan_exp.log.util as log_util

    data_buffer = node.log_get_all_new(log_tail_pad=0)

    try:
        print("    {0}".format(filename))

        # Get the byte log_data out of the WARPNet buffer
        data = data_buffer.get_bytes()

        # Example Attribute Dictionary for the HDF5 file
        attr_dict = {'exp_name'  : exp_name,
                     'exp_time'  : log_util.convert_datetime_to_log_time_str(datetime.datetime.utcnow()),
                     'node_desc' : node.description}

        # Write the byte Log_data to the file
        hdf_util.log_data_to_hdf5(log_data=data, filename=filename, attr_dict=attr_dict)
    except AttributeError as err:
        print("Error writing log file: {0}".format(err))

#-----------------------------------------------------------------------------
# Experiment Script
#-----------------------------------------------------------------------------
print("\nInitializing experiment\n")

# Create an object that describes the network configuration of the host PC
network_config = wlan_exp_config.WlanExpNetworkConfiguration(network=NETWORK)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = wlan_exp_config.WlanExpNodesConfiguration(network_config=network_config,
                                                           serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#   This command will fail if either WARP v3 node does not respond
nodes = wlan_exp_util.init_nodes(nodes_config, network_config)

# Set the time of all the nodes to zero
wlan_exp_util.broadcast_cmd_set_time(0.0, network_config)

# Extract the different types of nodes from the list of initialized nodes
#   NOTE:  This will work for both 'DCF' and 'NOMAC' mac_low projects
n_ap_l  = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='AP',  serial_number=NODE_SERIAL_LIST)
n_sta_l = wlan_exp_util.filter_nodes(nodes=nodes, mac_high='STA', serial_number=NODE_SERIAL_LIST)

# Check that we have a valid AP and STA
if len(n_ap_l) == 1 and len(n_sta_l) == 1:
    # Extract the two nodes from the lists for easier referencing below
    n_ap  = n_ap_l[0]
    n_sta = n_sta_l[0]

    # Establish the association state between nodes
    n_ap.add_association(n_sta)
else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print("    Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)

# Check that the nodes are associated.  Otherwise, the LTGs below will fail.
if not n_ap.is_associated(n_sta):
    print("\nERROR: Nodes are not associated.")
    print("    Ensure that the AP and the STA are associated.")
    sys.exit(0)



print("\nExperimental Setup:")

# Get the rates that we will move through during the experiment
rate = wlan_exp_util.wlan_rates[3]

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(rate, curr_assoc=True, new_assoc=True)
    node.log_configure(log_full_payloads=False)
    node.reset(log=True, txrx_stats=True, ltg=True, queue_data=True) # Do not reset associations/bss_info
    node.set_channel(CHANNEL)

# Add the current time to all the nodes
wlan_exp_util.broadcast_cmd_write_time_to_logs(network_config)


print("\nRun Experiment:")

print("\nStart LTG - AP -> STA")
# Start a flow from the AP's local traffic generator (LTG) to the STA
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#  Start the flow immediately
ap_ltg_id  = n_ap.ltg_configure(wlan_exp_ltg.FlowConfigCBR(dest_addr=n_sta.wlan_mac_address,
                                                           payload_length=1400, 
                                                           interval=0), auto_start=True)

# Let the LTG flows run at the new rate
time.sleep(TRIAL_TIME/3)


print("\nStart LTG - STA -> AP")
# Start a flow from the STA's local traffic generator (LTG) to the AP
#  Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#  Start the flow immediately
sta_ltg_id = n_sta.ltg_configure(wlan_exp_ltg.FlowConfigCBR(dest_addr=n_ap.wlan_mac_address,
                                                            payload_length=1400, 
                                                            interval=0), auto_start=True)

# Let the LTG flows run at the new rate
time.sleep(TRIAL_TIME/3)

print("\nStop  LTG - STA -> AP")

# Stop the LTG flow and purge the transmit queue so that nodes are in a known, good state
n_sta.ltg_stop(sta_ltg_id)
n_sta.queue_tx_data_purge_all()

# Let the LTG flows run at the new rate
time.sleep(TRIAL_TIME/3)

print("\nStop  LTG - AP -> STA")

# Stop the LTG flow and purge the transmit queue so that nodes are in a known, good state
n_ap.ltg_stop(ap_ltg_id)
n_ap.queue_tx_data_purge_all()

# Remove the LTGs so there are no memory leaks
n_ap.ltg_remove(ap_ltg_id)
n_sta.ltg_remove(sta_ltg_id)

# Look at the final log sizes for reference
ap_log_size  = n_ap.log_get_size()
sta_log_size = n_sta.log_get_size()

print("\nLog Sizes:  AP  = {0:10,d} bytes".format(ap_log_size))
print("            STA = {0:10,d} bytes".format(sta_log_size))

# Write Log Files for processing by other scripts
print("\nWriting Log Files...")

write_log_file(filename=STA_HDF5_FILENAME, node=n_sta, exp_name='STA: Two Node, Two Flow')
write_log_file(filename=AP_HDF5_FILENAME, node=n_ap, exp_name='AP: Two Node, Two Flow')

print("Done.")
