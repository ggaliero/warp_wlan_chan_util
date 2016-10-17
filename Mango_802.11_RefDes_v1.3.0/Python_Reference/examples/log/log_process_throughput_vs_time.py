"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log Throughput vs Time
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the WLAN Exp Log utilities to parse raw log data and
plot the throughput vs time using the pandas framework.

Hardware Setup:
    - None.  Parsing log data can be done completely off-line

Required Script Changes:
    - Set *_LOGFILE to the file name of your WLAN Exp log HDF5 file

------------------------------------------------------------------------------
"""
import os
import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util
import wlan_exp.log.util_sample_data as sample_data_util

#-----------------------------------------------------------------------------
# Process filenames
#-----------------------------------------------------------------------------

DEFAULT_AP_LOGFILE  = 'ap_two_node_two_flow_capture.hdf5'
DEFAULT_STA_LOGFILE = 'sta_two_node_two_flow_capture.hdf5'

logfile_error   = False

# Use log file given as command line argument, if present
if(len(sys.argv) != 1):
    LOGFILE_AP = str(sys.argv[1])
    LOGFILE_STA = str(sys.argv[2])

    # Check if the string argument matchs a local file
    if not (os.path.isfile(LOGFILE_AP) and os.path.isfile(LOGFILE_STA)):
        # User specified non-existant files - give up and exit
        logfile_error = True
else:
    # No command line arguments - check if default files exists locally
    LOGFILE_AP = DEFAULT_AP_LOGFILE
    LOGFILE_STA = DEFAULT_STA_LOGFILE

    if not (os.path.isfile(LOGFILE_AP) and os.path.isfile(LOGFILE_STA)):
        # No local files specified or found - check for matching sample data file
        try:
            LOGFILE_AP = sample_data_util.get_sample_data_file(DEFAULT_AP_LOGFILE)
            LOGFILE_STA = sample_data_util.get_sample_data_file(DEFAULT_STA_LOGFILE)
            print("Local log files not found - Using sample data files!")

        except IOError as e:
            logfile_error = True

if logfile_error:
    print("ERROR: Log files {0} and {1} not found".format(LOGFILE_AP, LOGFILE_STA))
    sys.exit()
else:
    print("Reading log files:")
    print(  "'{0}' ({1:5.1f} MB)".format(LOGFILE_AP, (os.path.getsize(LOGFILE_AP)/2**20)))
    print(  "'{0}' ({1:5.1f} MB)".format(LOGFILE_STA, (os.path.getsize(LOGFILE_AP)/2**20)))

#-----------------------------------------------------------------------------
# Main script
#-----------------------------------------------------------------------------
exit_script = False

# Extract the log data and index from the log files
log_data_ap       = hdf_util.hdf5_to_log_data(filename=LOGFILE_AP)
raw_log_index_ap  = hdf_util.hdf5_to_log_index(filename=LOGFILE_AP)

log_data_sta      = hdf_util.hdf5_to_log_data(filename=LOGFILE_STA)
raw_log_index_sta = hdf_util.hdf5_to_log_index(filename=LOGFILE_STA)

# Generate indexes with just Tx and Rx events
entries_filt  = ['NODE_INFO', 'RX_OFDM', 'TX', 'TX_LOW']
entries_merge = {'RX_OFDM': ['RX_OFDM', 'RX_OFDM_LTG'],
                 'TX'     : ['TX', 'TX_LTG'],
                 'TX_LOW' : ['TX_LOW', 'TX_LOW_LTG']}

log_index_txrx_ap  = log_util.filter_log_index(raw_log_index_ap,  include_only=entries_filt, merge=entries_merge)
log_index_txrx_sta = log_util.filter_log_index(raw_log_index_sta, include_only=entries_filt, merge=entries_merge)

# Generate numpy arrays
log_np_ap  = log_util.log_data_to_np_arrays(log_data_ap,  log_index_txrx_ap)
log_np_sta = log_util.log_data_to_np_arrays(log_data_sta, log_index_txrx_sta)

# Extract tne NODE_INFO's and determine each node's MAC address
try:
    addr_ap  = log_np_ap['NODE_INFO']['wlan_mac_addr']
except:
    print("ERROR: Log for AP did not contain a NODE_INFO.  Cannot determine MAC Address of AP.\n")
    exit_script = True

try:
    addr_sta = log_np_sta['NODE_INFO']['wlan_mac_addr']
except:
    print("ERROR: Log for STA did not contain a NODE_INFO.  Cannot determine MAC Address of STA.\n")
    exit_script = True

# Extract Tx entry arrays
try:
    tx_ap  = log_np_ap['TX']
except:
    print("ERROR: Log for AP did not contain any transmissions.\n")
    exit_script = True

try:
    tx_sta = log_np_sta['TX']
except:
    print("ERROR: Log for STA did not contain any transmissions.\n")
    exit_script = True

# Extract Rx entry arrays
try:
    rx_ap  = log_np_ap['RX_OFDM']
except:
    print("ERROR: Log for AP did not contain any receptions.\n")
    exit_script = True

try:
    rx_sta = log_np_sta['RX_OFDM']
except:
    print("ERROR: Log for STA did not contain any receptions.\n")
    exit_script = True


# Exit the script if necessary
if exit_script:
    print("Too many errors to continue.  Exiting...")
    sys.exit(0)

print('AP  Rx: {0:10d}, AP  Tx: {1:10d}'.format(len(rx_ap), len(tx_ap)))
print('STA Rx: {0:10d}, STA Tx: {1:10d}'.format(len(rx_sta), len(tx_sta)))


# Resample docs: http://stackoverflow.com/questions/17001389/pandas-resample-documentation
rs_interval = 1 #msec
rolling_winow = 1000 #samples

# Select non-duplicate packets from partner node
rx_ap_idx      = (rx_ap['addr2'] == addr_sta) & ((rx_ap['flags'] & 0x1) == 0) & (rx_ap['fcs_result'] == 0) & ( (rx_ap['pkt_type'] == 2) | (rx_ap['pkt_type'] == 3) )
rx_ap_from_sta = rx_ap[rx_ap_idx]

if (len(rx_ap_from_sta) == 0):
    print("WARNING:  No packets received at AP from STA.")

rx_ap_t        = rx_ap_from_sta['timestamp']
rx_ap_len      = rx_ap_from_sta['length']

# Select non-duplicate packets from partner node
rx_sta_idx     = (rx_sta['addr2'] == addr_ap) & ((rx_sta['flags'] & 0x1) == 0)  & (rx_sta['fcs_result'] == 0) & ( (rx_sta['pkt_type'] == 2) | (rx_sta['pkt_type'] == 3) )
rx_sta_from_ap = rx_sta[rx_sta_idx]

if (len(rx_sta_from_ap) == 0):
    print("WARNING:  No packets received at STA from AP.")

rx_sta_t       = rx_sta_from_ap['timestamp']
rx_sta_len     = rx_sta_from_ap['length']

# Convert to Pandas series
rx_ap_t_pd    = pd.to_datetime(rx_ap_t, unit='us')
rx_ap_len_pd  = pd.Series(rx_ap_len, index=rx_ap_t_pd)

rx_sta_t_pd   = pd.to_datetime(rx_sta_t, unit='us')
rx_sta_len_pd = pd.Series(rx_sta_len, index=rx_sta_t_pd)

# Resample
rx_ap_len_rs  = rx_ap_len_pd.resample(('%dL' % rs_interval), how='sum').fillna(value=0)
rx_sta_len_rs = rx_sta_len_pd.resample(('%dL' % rs_interval), how='sum').fillna(value=0)

# Merge the indexes
t_idx = rx_ap_len_rs.index.union(rx_sta_len_rs.index)

if (len(t_idx) == 0):
    print("ERROR:  No throughput to plot.")
    print("    Please check that there is data between the AP and STA.  Exiting...")
    sys.exit(0)

# Reindex both Series to the common index, filling 0 in empty slots
rx_ap_len_rs  = rx_ap_len_rs.reindex(t_idx, fill_value=0)
rx_sta_len_rs = rx_sta_len_rs.reindex(t_idx, fill_value=0)

# Compute rolling means
rx_xput_ap_r  = pd.rolling_mean(rx_ap_len_rs, window=rolling_winow, min_periods=1)
rx_xput_sta_r = pd.rolling_mean(rx_sta_len_rs, window=rolling_winow, min_periods=1)

# Set NaN values to 0 (no packets == zero throughput)
rx_xput_ap_r  = rx_xput_ap_r.fillna(value=0)
rx_xput_sta_r = rx_xput_sta_r.fillna(value=0)

# Create x axis values
t_sec = t_idx.astype('int64') / 1.0E9
plt_t = np.linspace(0, (max(t_sec) - min(t_sec)), len(t_sec))

# Rescale xputs to bits/sec
plt_xput_ap  = rx_xput_ap_r  * (1.0e-6 * 8.0 * (1.0/(rs_interval * 1e-3)))
plt_xput_sta = rx_xput_sta_r * (1.0e-6 * 8.0 * (1.0/(rs_interval * 1e-3)))


# Create figure to plot data
plt.close('all')
plt.figure(1)
plt.clf()

plt.plot(plt_t, plt_xput_ap, 'r', label='STA -> AP Flow')
plt.plot(plt_t, plt_xput_sta, 'b', label='AP -> STA Flow')
plt.plot(plt_t, plt_xput_ap + plt_xput_sta, 'g', label='Sum of Flows')

plt.xlim(min(plt_t), max(plt_t))

plt.grid('on')

plt.legend(loc='lower center')
plt.xlabel('Time (usec)')
plt.ylabel('Throughput (Mb/sec)')

plt.savefig('Two_Node_Througput_vs_Time.png')
