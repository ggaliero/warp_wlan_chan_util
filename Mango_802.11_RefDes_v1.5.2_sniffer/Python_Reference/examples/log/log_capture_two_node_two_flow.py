"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Two Node Log capture
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and wlan_exp to create a log
file that contains all data assocated with an experiment of head-to-head
backlogged data transfer using the local traffic generators.

Hardware Setup:
  - Requires two WARP v3 nodes
    - One node configured as AP using 802.11 Reference Design v1.5 or later
    - One node configured as STA using 802.11 Reference Design v1.5 or later
   - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial numbers of your WARP nodes

Description:
  This script initializes two WARP v3 nodes, one AP and one STA. It usees 
wlan_exp commands to form any needed associations.  After initializing all of
the experiment parameters, the script starts a traffic flow from the AP to 
the STA and while that flow is running will start and stop a traffic flow from 
the STA to the AP.  Finally, it resets the log, allows the experiment to run
and then captures all the log data after the TRIAL_TIME.
------------------------------------------------------------------------------
"""
import sys
import time

import wlan_exp.config as config
import wlan_exp.util as util
import wlan_exp.ltg as ltg


#-----------------------------------------------------------------------------
# Top Level Script Variables
#-----------------------------------------------------------------------------
# Change these values to match your experiment / network setup
NETWORK              = '10.0.0.0'
USE_JUMBO_ETH_FRAMES = False
NODE_SERIAL_LIST     = ['W3-a-00001', 'W3-a-00002']

AP_HDF5_FILENAME     = "ap_two_node_two_flow_capture.hdf5"
STA_HDF5_FILENAME    = "sta_two_node_two_flow_capture.hdf5"

# BSS parameters
SSID                 = "WARP Log 2 Node 2 Flow Ex"
CHANNEL              = 1
BEACON_INTERVAL      = 100

# Set the experiment duration (in seconds)
TRIAL_TIME           = 60

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

        # Get the byte log_data out of the Buffer
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
network_config = config.WlanExpNetworkConfiguration(network=NETWORK,
                                                    jumbo_frame_support=USE_JUMBO_ETH_FRAMES)

# Create an object that describes the WARP v3 nodes that will be used in this experiment
nodes_config   = config.WlanExpNodesConfiguration(network_config=network_config,
                                                  serial_numbers=NODE_SERIAL_LIST)

# Initialize the Nodes
#   This command will fail if either WARP v3 node does not respond
nodes = util.init_nodes(nodes_config, network_config)

# Set the time of all the nodes to zero
util.broadcast_cmd_set_mac_time(0, network_config)

# Extract the different types of nodes from the list of initialized nodes
#     - This will work for both 'DCF' and 'NOMAC' mac_low projects
n_ap_l  = util.filter_nodes(nodes=nodes, mac_high='AP',  serial_number=NODE_SERIAL_LIST)
n_sta_l = util.filter_nodes(nodes=nodes, mac_high='STA', serial_number=NODE_SERIAL_LIST)

# Check that setup is valid
if len(n_ap_l) == 1 and len(n_sta_l) == 1:
    # Extract the two nodes from the lists for easier referencing below
    n_ap  = n_ap_l[0]
    n_sta = n_sta_l[0]

    # Configure the AP to reject authentication requests from wireless clients
    #     - Uncomment this line to block any wireless associations during the experiment
    # n_ap.set_authentication_address_filter(allow='NONE')

    # Configure AP BSS
    n_ap.configure_bss(ssid=SSID, channel=CHANNEL, beacon_interval=BEACON_INTERVAL)

    # Establish the association state between nodes
    #     - This will change the STA to the appropriate channel
    n_ap.add_association(n_sta)
else:
    print("ERROR: Node configurations did not match requirements of script.\n")
    print("    Ensure two nodes are ready, one using the AP design, one using the STA design\n")
    sys.exit(0)

# Check that the nodes are part of the same BSS.  Otherwise, the LTGs below will fail.
if not util.check_bss_membership([n_ap, n_sta]):
    print("\nERROR: Nodes are not part of the same BSS.")
    util.check_bss_membership([n_ap, n_sta], verbose=True)
    print("Ensure that both nodes are part of the same BSS.")
    sys.exit(0)


print("\nExperimental Setup:")

# Set the rate of both nodes to 26 Mbps (mcs = 3, phy_mode = 'HTMF')
mcs       = 3
phy_mode  = util.phy_modes['HTMF']
rate_info = util.get_rate_info(mcs, phy_mode)

# Put each node in a known, good state
for node in nodes:
    node.set_tx_rate_unicast(mcs, phy_mode, curr_assoc=True, new_assoc=True)
    node.log_configure(log_full_payloads=False)
    node.reset(log=True, txrx_counts=True, ltg=True, queue_data=True) # Do not reset associations/bss_info
    node.configure_bss(channel=CHANNEL)

# Add the current time to all the nodes
util.broadcast_cmd_write_time_to_logs(network_config)


print("\nRun Experiment:")

print("\nStart LTG - AP -> STA")
# Start a flow from the AP's local traffic generator (LTG) to the STA
#     - Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#     - Start the flow immediately
ap_ltg_id  = n_ap.ltg_configure(ltg.FlowConfigCBR(dest_addr=n_sta.wlan_mac_address,
                                                  payload_length=1400, 
                                                  interval=0), auto_start=True)

# Let the LTG flows run at the new rate
time.sleep(TRIAL_TIME/3)


print("\nStart LTG - STA -> AP")
# Start a flow from the STA's local traffic generator (LTG) to the AP
#     - Set the flow to 1400 byte payloads, fully backlogged (0 usec between new pkts), run forever
#     - Start the flow immediately
sta_ltg_id = n_sta.ltg_configure(ltg.FlowConfigCBR(dest_addr=n_ap.wlan_mac_address,
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
