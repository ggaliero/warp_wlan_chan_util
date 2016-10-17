"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log File Details
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the WLAN Exp Log utilities to prase raw log data and print
details about the Tx and Rx events at the two nodes.

Hardware Setup:
    - None.  Parsing log data can be done off-line

Required Script Changes:
    - Set LOGFILE to the file name of your WLAN Exp log HDF5 file

Description:
    This script parses the log file and generates numpy arrays of for all Tx
    and Rx events in the logs.
------------------------------------------------------------------------------
"""
import os
import sys
import numpy as np

import wlan_exp.util as wlan_exp_util

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util
import wlan_exp.log.util_sample_data as sample_data_util


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
log_data      = hdf_util.hdf5_to_log_data(filename=LOGFILE)

# Get the raw_log_index from the file
raw_log_index = hdf_util.hdf5_to_log_index(filename=LOGFILE)

# Describe the raw_log_index
log_util.print_log_index_summary(raw_log_index, "Log Index Contents:")

# Filter log index to include all Rx entries and all Tx entries
log_index = log_util.filter_log_index(raw_log_index,
                                      include_only=['NODE_INFO', 'RX_OFDM', 'TX_HIGH', 'TX_LOW'],
                                      merge={'RX_OFDM': ['RX_OFDM', 'RX_OFDM_LTG'],
                                             'TX_HIGH': ['TX_HIGH', 'TX_HIGH_LTG'],
                                             'TX_LOW' : ['TX_LOW', 'TX_LOW_LTG']})

log_util.print_log_index_summary(log_index, "Filtered Log Index:")

# Unpack the log into numpy structured arrays
#   log_data_to_np_arrays returns a dictionary with one key-value pair per
#    entry type included in the log_index argument. The log_index keys are reused
#    as the output dictionary keys. Each output dictionary value is a numpy record array
#    Refer to wlan_exp_log.log_entries.py for the definition of each record array datatype
log_np = log_util.log_data_to_np_arrays(log_data, log_index)


###############################################################################
# Example 1: Gather some Tx information from the log
#     - Since there are only loops, this example can deal with TX_HIGH / TX_LOW
#       being an empty list and does not need a try / except.
#

# Initialize variables
TX_CONSTS     = log_util.get_entry_constants('TX_HIGH')
log_tx_high   = log_np['TX_HIGH']
log_tx_low    = log_np['TX_LOW']
total_retrans = 0

# Print header
print("\nExample 1: Tx Information per Rate:")
print("{0:^30} {1:^32} {2:^20}".format("Rate", "# Tx Pkts", "Avg Tx time (us)"))
print("{0:^30} {1:>10} {2:>10} {3:>10} {4:>10}".format("", "CPU High", "CPU Low", "Re-trans", "CPU High"))

# For each PHY mode, process the MCS index counts
for phy_mode in wlan_exp_util.phy_modes.keys():
    # Create index based on phy_mode    
    tx_high_idx        = (log_tx_high['phy_mode'] == TX_CONSTS.phy_mode[phy_mode])
    tx_low_idx         = (log_tx_low['phy_mode'] == TX_CONSTS.phy_mode[phy_mode])    
    tx_low_src_high_only_idx = ((log_tx_low['phy_mode'] == TX_CONSTS.phy_mode[phy_mode]) & 
                          ((log_tx_low['pkt_type'] != TX_CONSTS.pkt_type.BEACON) & 
                           (log_tx_low['pkt_type'] != TX_CONSTS.pkt_type.ACK) & 
                           (log_tx_low['pkt_type'] != TX_CONSTS.pkt_type.CTS) & 
                           (log_tx_low['pkt_type'] != TX_CONSTS.pkt_type.RTS)))



    # Extract arrays for each PHY mode
    tx_high_entries = log_tx_high[tx_high_idx]
    tx_low_entries  = log_tx_low[tx_low_idx]
    tx_low_src_high_only  = log_tx_low[tx_low_src_high_only_idx]

    # Extract arrays of just the MCS indexes
    tx_high_entries_mcs = tx_high_entries['mcs']
    tx_low_entries_mcs  = tx_low_entries['mcs']
    tx_low_src_high_only_mcs  = tx_low_src_high_only['mcs']

    # Initialize an array to count number of packets per MCS index
    #   MAC uses MCS indexes 0:7 to encode OFDM rates
    tx_high_entries_mcs_counts = np.bincount(tx_high_entries_mcs, minlength=8)
    tx_low_entries_mcs_counts  = np.bincount(tx_low_entries_mcs, minlength=8)
    tx_low_src_high_only_mcs_counts  = np.bincount(tx_low_src_high_only_mcs, minlength=8)
    
    # Extract an array of just the 'time_to_done' timestamp offsets
    tx_high_done = tx_high_entries['time_to_done']

    # Calculate the average time to send a packet for each rate
    for mcs in range(0, 8):
        # Find indexes of all instances where addresses match
        #   np.squeeze here flattens the result to a 1-D array
        mcs_idx = np.squeeze(tx_high_entries_mcs == mcs)

        # Calculate the average time to send a packet
        tx_high_avg_time = 0
        
        if np.any(mcs_idx):
            tx_high_avg_time = np.average(tx_high_done[mcs_idx])

        # Calculate retransmissions
        #    Must use the counts with no control packets since control packets
        #    and beacons are transmitted without a corresponding TX_HIGH entry
        retrans = tx_low_src_high_only_mcs_counts[mcs] - tx_high_entries_mcs_counts[mcs]
        total_retrans  += retrans

        # Print info
        try:
            rate_info = wlan_exp_util.get_rate_info(mcs, phy_mode)
            rate_str  = wlan_exp_util.rate_info_to_str(rate_info)
            print("{0:30} {1:10} {2:10} {3:10} {4:>10.0f}".format(
                rate_str,
                tx_high_entries_mcs_counts[mcs],
                tx_low_entries_mcs_counts[mcs],
                retrans,
                tx_high_avg_time))
        except:
            # Do nothing with unsupported PHY modes
            pass

print("\nTotal Retransmissions: {0:d}".format(total_retrans))



###############################################################################
# Example 2: Calculate total number of packets and bytes transmitted to each
#            distinct MAC address for each of the MAC addresses in the header
#

# Skip this example if the log doesn't contain TX events
for tx in ['TX_HIGH', 'TX_LOW']:
    if(tx in log_np.keys()):
        # Extract all OFDM transmissions
        log_tx = log_np[tx]

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
        if (tx == 'TX_HIGH'):
            print("\nExample 2: Tx Counts (CPU High):");
        else:
            print("\nExample 2: Tx Counts (CPU Low - includes retransmissions):");
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
        print("\nExample 3: Rx Counts (including duplicates):");
        print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
            "Dest Addr",
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
