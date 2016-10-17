"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log File Summary
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
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
from  wlan_exp.util import wlan_rates

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util
import wlan_exp.log.util_sample_data as sample_data_util

from wlan_exp.log.entry_types import log_entry_types


#-----------------------------------------------------------------------------
# Process command line arguments
#-----------------------------------------------------------------------------

DEFAULT_LOGFILE = 'ap_two_node_two_flow_capture.hdf5'
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
# Merge LTG events into the non-LTG log entry types, so we can
#  count all Tx/Rx events together
log_index = log_util.filter_log_index(raw_log_index,
                                      include_only=['NODE_INFO', 'TIME_INFO', 'RX_OFDM', 'TX', 'EXP_INFO'],
                                      merge={'RX_OFDM': ['RX_OFDM', 'RX_OFDM_LTG'],
                                             'TX'     : ['TX', 'TX_LTG']})

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
print("  WLAN Exp Ver : {0}".format(wlan_exp_util.ver_code_to_str(log_node_info['wlan_exp_ver'])))
print("")

if(len(log_np['TIME_INFO']) > 0):
    log_time_info = log_np['TIME_INFO'][0]

    print("Experiment Started at: {0}".format(time.ctime(float(log_time_info['abs_time'] / 1E6))))
    print("")

###############################################################################
# Example 1: Count the number of receptions per PHY rate
#   NOTE:  Since there are only loops, this example can deal with RX_OFDM being an
#          empty list and does not need a try / except.
#

# Extract all OFDM transmissions and receptions
log_tx = log_np['TX']
log_rx = log_np['RX_OFDM']

# Extract arrays of just the rates
tx_rates = log_tx['rate']
rx_rates = log_rx['rate']

# Initialize an array to count number of Rx per PHY rate
#   MAC uses rate_indexes 1:8 to encode OFDM rates
tx_rate_counts = np.bincount(tx_rates, minlength=9)
tx_rate_counts = tx_rate_counts[1:9] #only rate values 1:8 are valid

rx_rate_counts = np.bincount(rx_rates, minlength=9)
rx_rate_counts = rx_rate_counts[1:9] #only rate values 1:8 are valid

print("Example 1: Pkts per Rate:")
print("  Rate       {0:7} {1:7}".format("Tx","Rx"))
for i,(tx_c,rx_c) in enumerate(zip(tx_rate_counts, rx_rate_counts)):
    print(" {0:2d} Mbps: {1:7} {2:7}".format( int(wlan_rates[i]['rate']), tx_c, rx_c))


###############################################################################
# Example 2: Calculate total number of packets and bytes transmitted to each
#            distinct MAC address for each of the MAC addresses in the header

# Skip this example if the log doesn't contain TX events
if('TX' in log_np.keys()):
    # Extract all OFDM transmissions
    log_tx = log_np['TX']

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

    # For this experiment, only look at Good = 0  or Bad = 1 receptions
    # Extract only Rx entries with good checksum (FCS = good)
    rx_good_fcs = log_rx[log_rx['fcs_result'] == log_entry_types['RX_OFDM'].consts['FCS_GOOD']]

    # Extract addr2 field from all good packets
    rx_addrs_2 = rx_good_fcs['addr2']

    # Build a dictionary using unique MAC addresses as keys
    rx_counts = dict()
    for addr in np.unique(rx_addrs_2):
        # Find indexes of all instances where addresses match
        #   np.squeeze here flattens the result to a 1-D array
        addr_idx = np.squeeze(rx_addrs_2 == addr)

        # Count the number of packets (True values in index array) from this address
        rx_pkts_from_addr  = np.sum(addr_idx)

        # Count the number of bytes from this address
        rx_bytes_from_addr = np.sum(rx_good_fcs['length'][addr_idx])

        # Add the information about the address to the dictionary
        rx_counts[addr] = (rx_pkts_from_addr, rx_bytes_from_addr)

    # Print the results
    print("\nExample 3: Rx Counts (including duplicates):");
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

print('')

# Uncomment this line to open an interactive console after the script runs
#   This console will have access to all variables defined above
# wlan_exp_util.debug_here()
