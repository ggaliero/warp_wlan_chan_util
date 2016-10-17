"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log File Tx Stats
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This example will process the TX statistics for a given log file.

Hardware Setup:
    - None.  Parsing log data can be done completely off-line

Required Script Changes:
    - Set LOGFILE to the file name of your WLAN Exp log HDF5 file (or pass in
        via command line argument)

------------------------------------------------------------------------------
"""
import os
import sys

import numpy as np
import matplotlib.mlab as mlab

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

# Extract just OFDM Tx events
tx_log_index  = log_util.filter_log_index(raw_log_index, include_only=['TX'],
                                          merge={'TX' : ['TX', 'TX_LTG']})

# Generate numpy array
log_np = log_util.log_data_to_np_arrays(log_data, tx_log_index)
log_tx = log_np['TX']

# Define the fields to group by
group_fields = ('addr1',)

# Define the aggregation functions
stat_calc = (
    ('num_tx',       np.mean, 'avg_num_tx'),
    ('length',       len,     'num_pkts'),
    ('length',       np.mean, 'avg_len'),
    ('length',       sum,     'tot_len'),
    ('time_to_done', np.mean, 'avg_time'))

# Calculate the aggregate statistics
tx_stats = mlab.rec_groupby(log_tx, group_fields, stat_calc)

# Display the results
print('\nTx Statistics for {0}:\n'.format(os.path.basename(LOGFILE)))

print('{0:^18} | {1:^9} | {2:^10} | {3:^14} | {4:^11} | {5:^5}'.format(
    'Dest Addr',
    'Num MPDUs',
    'Avg Length',
    'Total Tx Bytes',
    'Avg Tx Time',
    'Avg Num Tx'))

for ii in range(len(tx_stats)):
    print('{0:<18} | {1:9d} | {2:10.1f} | {3:14} | {4:11.3f} | {5:5.2f}'.format(
        wlan_exp_util.mac_addr_to_str(tx_stats['addr1'][ii]),
        tx_stats['num_pkts'][ii],
        tx_stats['avg_len'][ii],
        tx_stats['tot_len'][ii],
        tx_stats['avg_time'][ii],
        tx_stats['avg_num_tx'][ii]))

print('')

# Uncomment this line to open an interactive console after the script runs
#   This console will have access to all variables defined above
# wlan_exp_util.debug_here()
