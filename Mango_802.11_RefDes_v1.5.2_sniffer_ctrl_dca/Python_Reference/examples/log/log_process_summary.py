"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log File Summary
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the WLAN Exp Log utilities to prase raw log data and print
a few interesting summary statistics about the log entries.

Hardware Setup:
    - None.  Parsing log data can be done completely off-line

Required Script Changes:
    - Set LOGFILE to the file name of your WLAN Exp log HDF5 file

Description:
    This script parses the log file and generates numpy arrays of each type
of log entry.  It then uses these arrays to count the number of receptions
per PHY rate and the total number of packets transmitted to each distinct
MAC address.  Finally, it optionally opens an interactive python shell to allow
you to play with the data.
------------------------------------------------------------------------------
"""
import os
import sys
import time
import numpy as np

import wlan_exp.util as wlan_exp_util

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util
import wlan_exp.log.util_sample_data as sample_data_util

#-----------------------------------------------------------------------------
# Process command line arguments
#-----------------------------------------------------------------------------

DEFAULT_LOGFILE = 'sta_two_node_two_flow_capture.hdf5'
logfile_error   = False

# Use log file given as command line argument, if present
if(len(sys.argv) != 1):
    LOGFILE = str(sys.argv[1])

    # Check if the string argument matchs a local file
    if not os.path.isfile(LOGFILE):
        # User specified non-existant file - give up and exit
        logfile_error = True

else:
    # No command line argument - check if default file name exists locally
    LOGFILE = DEFAULT_LOGFILE

    if not os.path.isfile(LOGFILE):
        # No local file specified or found - check for matching sample data file
        try:
            LOGFILE = sample_data_util.get_sample_data_file(DEFAULT_LOGFILE)
            print("Local log file not found - Using sample data file!")
        except IOError as e:
            logfile_error = True

if logfile_error:
    print("ERROR: Logfile {0} not found".format(LOGFILE))
    sys.exit()
else:
    print("Reading log file '{0}' ({1:5.1f} MB)\n".format(LOGFILE, (os.path.getsize(LOGFILE)/2**20)))

#-----------------------------------------------------------------------------
# Main script
#-----------------------------------------------------------------------------

# Get the log_data from the file
log_data = hdf_util.hdf5_to_log_data(filename=LOGFILE)

# Get the raw_log_index from the file
raw_log_index = hdf_util.hdf5_to_log_index(filename=LOGFILE)

# Describe the raw_log_index
log_util.print_log_index_summary(raw_log_index, "Raw Log Index Contents:")

# Filter log index to include all Rx entries and all Tx entries
# Merge LTG events into the non-LTG log entry types, to count all Tx/Rx events together
log_index = log_util.filter_log_index(raw_log_index,
                                      include_only=['NODE_INFO', 'TIME_INFO', 'RX_OFDM', 'TX_HIGH', 'EXP_INFO'],
                                      merge={'RX_OFDM': ['RX_OFDM', 'RX_OFDM_LTG'],
                                             'TX_HIGH': ['TX_HIGH', 'TX_HIGH_LTG']})

log_util.print_log_index_summary(log_index, "Filtered Log Index:")

# Unpack the log into numpy structured arrays
#   log_data_to_np_arrays returns a dictionary with one key-value pair per
#    entry type included in the log_index argument. The log_index keys are reused
#    as the output dictionary keys. Each output dictionary value is a numpy record array
#    Refer to wlan_exp_log.log_entries.py for the definition of each record array datatype
log_np = log_util.log_data_to_np_arrays(log_data, log_index)

exp_info = log_np['EXP_INFO']

for info in exp_info:
    print("Timestamp = {0}".format(info['timestamp']))
    print("Info Type = {0}".format(info['info_type']))
    print("Length    = {0}".format(info['length']))

###############################################################################
# Example 0: Print node info / Time info
log_node_info = log_np['NODE_INFO'][0]

print("Node Info:")
print("  Node Type    : {0}".format(wlan_exp_util.node_type_to_str(log_node_info['node_type'])))
print("  MAC Address  : {0}".format(wlan_exp_util.mac_addr_to_str(log_node_info['wlan_mac_addr'])))
print("  Serial Number: {0}".format(wlan_exp_util.sn_to_str(log_node_info['hw_generation'], log_node_info['serial_num'])))
print("  WLAN Exp Ver : {0}".format(wlan_exp_util.ver_code_to_str(log_node_info['version'])))
print("")

if(len(log_np['TIME_INFO']) > 0):
    log_time_info = log_np['TIME_INFO'][0]

    print("Experiment Started at: {0}".format(time.ctime(float(log_time_info['host_timestamp'] / 1E6))))
    print("")

###############################################################################
# Example 1: Count the number of receptions per PHY rate
#     - Since there are only loops, this example can deal with RX_OFDM being an
#       empty list and does not need a try / except.
#

# Get entry constants
TX_CONSTS = log_util.get_entry_constants('TX_HIGH')
RX_CONSTS = log_util.get_entry_constants('RX_OFDM')

# Extract all OFDM transmissions and receptions
log_tx = log_np['TX_HIGH']
log_rx = log_np['RX_OFDM']

# Print header
print("Example 1: Packets per PHY rate: (all packets)")
print("  {0:^30}    {1:^10} {2:^10}".format("Rate", "Tx", "Rx"))

# For each PHY mode, process the MCS index counts
for phy_mode in wlan_exp_util.phy_modes.keys():
    # Create index based on phy_mode
    tx_idx = (log_tx['phy_mode'] == TX_CONSTS.phy_mode[phy_mode])
    rx_idx = (log_rx['phy_mode'] == RX_CONSTS.phy_mode[phy_mode])

    # Extract arrays for each PHY mode
    tx_entries = log_tx[tx_idx]
    rx_entries = log_rx[rx_idx]

    # Extract arrays of just the MCS indexes
    tx_entries_mcs = tx_entries['mcs']
    rx_entries_mcs = rx_entries['mcs']

    # Initialize an array to count number of packets per MCS index
    #   MAC uses MCS indexes 0:7 to encode OFDM rates
    tx_entries_mcs_counts = np.bincount(tx_entries_mcs, minlength=8)
    rx_entries_mcs_counts = np.bincount(rx_entries_mcs, minlength=8)

    # Print counts per PHY rate
    for i, (tx_c, rx_c) in enumerate(zip(tx_entries_mcs_counts, rx_entries_mcs_counts)):
        try:
            rate_info = wlan_exp_util.get_rate_info(i, phy_mode)
            rate_str  = wlan_exp_util.rate_info_to_str(rate_info)
            print("  {0:30}    {1:10} {2:10}".format(rate_str, tx_c, rx_c))
        except:
            # Do nothing with unsupported PHY modes
            pass


###############################################################################
# Example 2: Calculate total number of packets and bytes transmitted to each
#            distinct MAC address for each of the MAC addresses in the header

# Skip this example if the log doesn't contain TX events
if('TX_HIGH' in log_np.keys()):
    # Extract all OFDM transmissions
    log_tx = log_np['TX_HIGH']

    # Count number of packets transmitted to each unique address in the 'addr1' field
    tx_addrs_1 = log_tx['addr1']
    tx_counts = dict()

    for addr in np.unique(tx_addrs_1):
        # Find indexes of all instances where addresses match
        #   np.squeeze here flattens the result to a 1-D array
        addr_idx = np.squeeze(tx_addrs_1 == addr)

        # Count the number of packets (True values in index array) to this address
        tx_pkts_to_addr  = np.sum(addr_idx)

        # Count the number of bytes to this address
        tx_bytes_to_addr = np.sum(log_tx['length'][addr_idx])

        # Record the results in the output dictionary
        tx_counts[addr] = (tx_pkts_to_addr, tx_bytes_to_addr)

    # Print the results
    print("\nExample 2: Tx MPDU Counts:");
    print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
        "Dest Addr",
        "# Pkts",
        "# Bytes",
        "MAC Addr Type"))

    for k in sorted(tx_counts.keys()):
        # Use the string version of the MAC address as the key for readability
        print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
            wlan_exp_util.mac_addr_to_str(k),
            tx_counts[k][0],
            tx_counts[k][1],
            wlan_exp_util.mac_addr_desc(k)))

#################################################################################################
# Example 3: Calculate total number of packets and bytes received from each distinct MAC address

# Skip this example if the log doesn't contain RX events
if('RX_OFDM' in log_np.keys()):

    # Extract all receptions
    log_rx = log_np['RX_OFDM']

    # Get RX_OFDM entry constants
    RX_CONSTS = log_util.get_entry_constants('RX_OFDM')

    # Extract only Rx entries with:
    #   - Good checksum (FCS = good)
    #   - Data / Management packets
    #
    rx_idx       = (((log_rx['flags'] & RX_CONSTS.flags.FCS_GOOD) != 0) & 
                    ((log_rx['pkt_type'] == RX_CONSTS.pkt_type.DATA) | 
                     (log_rx['pkt_type'] == RX_CONSTS.pkt_type.QOSDATA) | 
                     (log_rx['pkt_type'] == RX_CONSTS.pkt_type.NULLDATA) | 
                     (log_rx['pkt_type'] == RX_CONSTS.pkt_type.BEACON) | 
                     (log_rx['pkt_type'] == RX_CONSTS.pkt_type.PROBE_RESP)))

    rx_good_data_mgmt = log_rx[rx_idx]

    # Extract addr2 field from all good packets
    rx_addrs_2 = rx_good_data_mgmt['addr2']

    # Build a dictionary using unique MAC addresses as keys
    rx_counts = dict()
    
    for addr in np.unique(rx_addrs_2):
        # Find indexes of all instances where addresses match
        #   np.squeeze here flattens the result to a 1-D array
        addr_idx = np.squeeze(rx_addrs_2 == addr)

        # Count the number of packets (True values in index array) from this address
        rx_pkts_from_addr  = np.sum(addr_idx)

        # Count the number of bytes from this address
        try:
            # If addr_idx is greater than 1
            rx_bytes_from_addr = np.sum(rx_good_data_mgmt['length'][addr_idx])
        except:
            # If addr_idx is one 
            rx_bytes_from_addr = np.sum(rx_good_data_mgmt['length'])

        # Add the information about the address to the dictionary
        rx_counts[addr] = (rx_pkts_from_addr, rx_bytes_from_addr)

    # Print the results
    if (len(rx_counts) > 0):
        print("\nExample 3: Rx Counts (including duplicates, no control packets):");
        print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
            "Src Addr",
            "# Pkts",
            "# Bytes",
            "MAC Addr Type"))
    
        for k in sorted(rx_counts.keys()):
            # Use the string version of the MAC address as the key for readability
            print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
                wlan_exp_util.mac_addr_to_str(k),
                rx_counts[k][0],
                rx_counts[k][1],
                wlan_exp_util.mac_addr_desc(k)))
    else:
        print("\nExample 3: Rx Counts (including duplicates):");
        print("\nNo Data or Management frames recevied with good FCS.");

print('')

# Uncomment this line to open an interactive console after the script runs
#   This console will have access to all variables defined above
# wlan_exp_util.debug_here()
