"""
------------------------------------------------------------------------------
Gabriele Galiero Casay - Experiments Framework - Log File Summary
------------------------------------------------------------------------------
License:   
------------------------------------------------------------------------------
This script uses the WLAN Exp Log utilities to parse raw log data and print
a few interesting summary statistics about the log entries.

Hardware Setup:
    - None.  Parsing log data can be done completely off-line

Required Script Changes:
    - Set LOGFILE to the file name of your WLAN Exp log HDF5 file

Description:
    This script parses the log file and generates numpy arrays of each type
of log entry.  It then uses these arrays to count the number of receptions
per PHY rate and  the total number of received packets received from different
MAC addresses. It also accounts the number of users that were accessing the
channel.
------------------------------------------------------------------------------
"""
import os
import sys
import time
import numpy as np
import optparse

from operator import add

import wlan_exp.util as wlan_exp_util
from  wlan_exp.util import wlan_rates

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util
import wlan_exp.log.util_sample_data as sample_data_util

from wlan_exp.log.entry_types import log_entry_types
#from pandas import DataFrame



#-----------------------------HELPER FUNCTIONS--------------------------------#
def get_mpdu_duration(length, mcs):
    global mcs_to_dbps
    n_dbps = mcs_to_dbps[mcs]
    num_payload_bits = (16 + 8*length + 6)
    num_syms = num_payload_bits / n_dbps
    if(num_syms * n_dbps != num_payload_bits):
        num_syms+=1
    return 26 + num_syms * 4
    
def calculate_loss_and_retx(seq_nums, num_mpdus):
    num_mpdu_loss = 0
    num_mpdu_retx = 0
        
    for i in range(0,len(seq_nums)):
        if i == 0 and seq_nums[i] != 0:
            num_mpdu_loss+= 1
        elif seq_nums[i] - 1 > seq_nums[i - 1]:
            num_mpdu_loss += (seq_nums[i] - seq_nums[i - 1] - 1)
        elif seq_nums[i] == seq_nums[i - 1]:
            num_mpdu_retx += 1
    if len(seq_nums) > 0 and (seq_nums[len(seq_nums) - 1] < num_mpdus - 1):
        num_mpdu_loss+= ((num_mpdus - 1) - (seq_nums[len(seq_nums) - 1]))
    return (num_mpdu_loss, num_mpdu_retx)

def sort_retx(retx_per_mpdu):
    num_of_retx = []
    maximum_num_retx = 7
    for i in range(0,maximum_num_retx+1):
        num_of_retx.append(retx_per_mpdu.count(i))
    return num_of_retx
   
def get_retx(seq_nums, num_mpdus):
    retx_per_mpdu = []
    
    for i in range(0,num_mpdus):
        re_tx = get_retx_by_seq_num(seq_nums, i)
        retx_per_mpdu.append(re_tx)
                
    return retx_per_mpdu    
                
def get_retx_by_seq_num(list_seq, seq_num):
    retx    = 0
    done    = 0
#    print('seq_num=%d' % seq_num)
    if seq_num in list_seq:
        index   = list_seq.index(seq_num)
        next_index = index + 1
#        print('index=%d' % index)
        if next_index < len(list_seq):        
            while (next_index < len(list_seq)) and (done != 1):
#                print('next_index=%d' % next_index)
                curr_seq = list_seq[next_index]
                if curr_seq == seq_num:
                    retx += 1
                elif curr_seq > list_seq[index]:
                    done = 1
                index = next_index
                next_index += 1
#    print('retx=%d' % retx)            
    return retx
    
def get_tx_rx_mac(warp_to_comm):
    global warp_ap_mac_str, warp_sta_mac_str, comm_sta_mac_str1, comm_sta_mac_str2

    ap_mac  = warp_ap_mac_str   
    
    if warp_to_comm :
        sta_mac = comm_sta_mac_str
    else:
        sta_mac = warp_sta_mac_str
    
    return (ap_mac, sta_mac)

def map_mac_to_int(mac_addr):
    mac_int = 0
    mac_str = ''
    mac = mac_addr.split(':')
    for i in range(0,len(mac)):
        mac_str += mac[i]
    mac_int = int(mac_str, 16)
    return mac_int
    
def write_list_into_file(list_elem, filename):
    try:
        store = open(filename, 'w')
        for i in range(0,len(list_elem)):
            store.write("%s\n" % str(list_elem(i)))
        store.close()
    except:
        print 'Error while writing the output files'
        
#-----------------------------------------------------------------------------#

DEFAULT_LOGFILE = 'log_cap_'
DEFAULT_CAPTURE = 0
DEFAULT_PKT_LEN = 102
DEFAULT_NUM_PKT = 1000
logfile_error   = False

#------------------------------------------------------------------------------
# Global Variables
#------------------------------------------------------------------------------

mcs_to_dbps = [24, 36, 48, 72, 96, 144, 192, 216]

# MAC address of the WARPv3 access point (AP)
warp_ap_mac_str         = '40:d8:55:04:24:b8'
# MAC address of the WARPv3 station (STA)
warp_sta_mac_str        = '40:d8:55:04:25:00'
# MAC address of the commercial station (STA)
comm_sta_mac_str        = '80:49:71:13:ba:29'

# 2.4GHz Band
comm_ap_mac_str1        = '74:44:01:4B:71:87'
# 5GHz Band
comm_ap_mac_str2        = '74:44:01:4B:71:86'

sta_mac_str             = None
ap_mac_str              = None

header                  = 36

rx_mpdu                 = list()
re_tx                   = list()
mpdu_loss               = list()
capt_mpdu                = list()
num_retx                = [0] * 7

filename1               = 'num_mpdu.txt'
filename2               = 'num_mpdu_retx.txt'
filename3               = 'num_mpdu_loss.txt'
filename4               = 'cap_percentage.txt'

#-----------------------------------------------------------------------------
# Main script
#-----------------------------------------------------------------------------

#RX_CONSTS = log_util.get_entry_constants('RX_OFDM')

parser = optparse.OptionParser()
parser.add_option('-i', action="store_true", dest="warp_to_comm", default=False)
parser.add_option('-c', action="store_true", dest="comm_to_comm", default=False)
parser.add_option('-u', action="store_true", dest="uplink", help="Uplink traffic flow", default=True)
parser.add_option('-d', action="store_false", dest="uplink", help="Downlink traffic flow")
parser.add_option('-f', action="store", dest="log_cap", help="Log file name", default=DEFAULT_CAPTURE)
parser.add_option('-l', action="store", dest="udp_pkt_len", help="UDP packet length", default=DEFAULT_PKT_LEN)
parser.add_option('-n', action="store", dest="num_mpdus", help="Number of data MPDUs generated", default=DEFAULT_NUM_PKT)
parser.add_option('-m', action="store", dest="num_exp", help="Number of data file measurements to be processed")

options, args = parser.parse_args()
warp_to_comm  = options.warp_to_comm
comm_to_comm  = options.comm_to_comm
uplink        = options.uplink
log_cap       = int(options.log_cap)
udp_pkt_len   = int(options.udp_pkt_len)
num_mpdus     = int(options.num_mpdus)
num_exp       = int(options.num_exp)
pkt_len       = udp_pkt_len + header

logfile       = DEFAULT_LOGFILE

ap_mac_str, sta_mac_str = get_tx_rx_mac(warp_to_comm)

ap_mac_int  = map_mac_to_int(ap_mac_str)
sta_mac_int = map_mac_to_int(sta_mac_str)

try:
    store1 = open(filename1, 'a+')
    store2 = open(filename2, 'a+')
    store3 = open(filename3, 'a+')
    store4 = open(filename4, 'a+')
except:
    print 'Error while opening the files to store received mpdus at the fox application level.'


for meas in range(log_cap,num_exp + log_cap):
    logfile       = logfile + '{:03d}'.format(meas) + '.hdf5'

    if not os.path.isfile(logfile):
        logfile_error = True
    
    if logfile_error:
        print("ERROR: Logfile {0} not found".format(logfile))
        sys.exit()
    else:
        print("Reading log file '{0}' ({1:5.1f} MB)\n".format(logfile, (os.path.getsize(logfile)/2**20)))


    # Get the log data from the file
    log_data = hdf_util.hdf5_to_log_data(filename=logfile)

    # Get the raw_log_index from the file
    raw_log_index = hdf_util.hdf5_to_log_index(filename=logfile)

    # Describe the raw_log_index
    log_util.print_log_index_summary(raw_log_index, "Raw Log Index Contents:")

    # Filter log index to include all Rx entries
    # Merge LTG events into the non-LTG log entry types, so we can
    #  count all Rx events together
    log_index = log_util.filter_log_index(raw_log_index,
                                          include_only=['NODE_INFO', 'TIME_INFO', 'RX_OFDM', 'EXP_INFO'],
                                          merge={'RX_OFDM': ['RX_OFDM', 'RX_OFDM_LTG']})
                                      
    log_util.print_log_index_summary(log_index, "Filtered Log Index:")

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
    
#################################################################################################
# Example 3: Calculate total number of packets and bytes received from each distinct MAC address
# Here distinction between data, management and control packets could be carried out.

    # Skip this example if the log doesn't contain RX events
    if('RX_OFDM' in log_np.keys()):
    
        # Extract all receptions
        log_rx = log_np['RX_OFDM']
    
        # For this experiment, look at Good = 0  or Bad = 1 receptions
        rx_good_fcs = (log_rx[log_rx['fcs_result'] == log_entry_types['RX_OFDM'].consts['FCS_GOOD']] )
        rx_bad_fcs  = (log_rx[log_rx['fcs_result'] == log_entry_types['RX_OFDM'].consts['FCS_BAD']] )
        
        num_rx_good_fcs = rx_good_fcs.size
        num_rx_bad_fcs  = rx_bad_fcs.size
    
        # Extract addr2 field from all packets, src MAC address
        rx_good_addrs_2 = rx_good_fcs['addr2']
        rx_bad_addrs_2  = rx_bad_fcs['addr2']
        
        # This is to account the total number of users accessing the channel during the experiment
        num_users = np.unique(rx_good_addrs_2).size
        
        
        """Traffic analysis"""
        
        if uplink:
            rx_data_idx = (rx_good_fcs['pkt_type'] == 2)  & (rx_good_fcs['addr2'] == sta_mac_int) & (rx_good_fcs['addr1'] == ap_mac_int)
            rx_ack_idx  = (rx_good_fcs['pkt_type'] == 21) & (rx_good_fcs['addr1'] == sta_mac_int)
            rx_rts_idx  = (rx_good_fcs['pkt_type'] == 22) & (rx_good_fcs['addr1'] == ap_mac_int)
            rx_cts_idx  = (rx_good_fcs['pkt_type'] == 23) & (rx_good_fcs['addr2'] == sta_mac_int)
        else:
            rx_data_idx = (rx_good_fcs['pkt_type'] == 2)  & (rx_good_fcs['addr2'] == ap_mac_int) & (rx_good_fcs['addr1'] == sta_mac_int)
            rx_ack_idx  = (rx_good_fcs['pkt_type'] == 21) & (rx_good_fcs['addr1'] == ap_mac_int)
            rx_rts_idx  = (rx_good_fcs['pkt_type'] == 22) & (rx_good_fcs['addr1'] == sta_mac_int)
            rx_cts_idx  = (rx_good_fcs['pkt_type'] == 23) & (rx_good_fcs['addr2'] == ap_mac_int)
        
        rx_data = rx_good_fcs[rx_data_idx]
        rx_ack  = rx_good_fcs[rx_ack_idx]
        rx_rts  = rx_good_fcs[rx_rts_idx]
        rx_cts  = rx_good_fcs[rx_cts_idx]
        
        rx_data_fox_idx = (rx_data['length'] >= pkt_len) & (rx_data['length'] < 15000)
        rx_data_fox     = rx_data[rx_data_fox_idx]
        
        
        num_data_mpdu     = len(rx_data)
        num_data_fox_mpdu = len(rx_data_fox)
        num_ack_mpdu      = len(rx_ack)
        num_rts_mpdu      = len(rx_rts)
        num_cts_mpdu      = len(rx_cts)
         
        
        rx_src_addr       = rx_data_fox['addr2']
        rx_dst_addr       = rx_data_fox['addr1']
        rx_time_stamp     = rx_data_fox['timestamp']
        rx_length         = rx_data_fox['length']
        rx_power          = rx_data_fox['power']
        rx_mcs            = rx_data_fox['rate']
        rx_seq_num        = rx_data_fox['seq_num']
        rx_duration       = list()
        
        for i in range(0,num_data_fox_mpdu):
            rx_duration.append(get_mpdu_duration(rx_length[i], rx_mcs[i]))
        
        # Information data displayed
#        print ('')
#        if uplink:
#            print ('UPLINK: STA (%s) -> AP (%s)' % (sta_mac_str, ap_mac_str))
#        else:
#            print ('DOWNLINK: AP (%s) -> STA (%s)' % (ap_mac_str, sta_mac_str))
#            
#        for i in range(0,num_data_fox_mpdu):
#            print('%03d - %d - dur=%03d - seq=%04d - len=%03d - %s -> %s - Prx: %d dBm - mcs: %d' % (i, rx_time_stamp[i], rx_duration[i] ,rx_seq_num[i], rx_length[i], wlan_exp_util.mac_addr_to_str(rx_src_addr[i]), wlan_exp_util.mac_addr_to_str(rx_dst_addr[i]), rx_power[i], rx_mcs[i]))    
#        print ('')
        
         # Re-transmissions and packets losses
        num_mpdu_loss, num_mpdu_retx = calculate_loss_and_retx(rx_seq_num, num_mpdus)  
        retx_per_mpdu = get_retx(rx_seq_num.tolist(), num_mpdus)        
        total_retx    = sort_retx(retx_per_mpdu)
        
        print(total_retx)
        print('total retx=%d' % sum(retx_per_mpdu))
        
        # Build a dictionary using unique MAC addresses as keys
        rx_counts_good = dict()
    #    rx_counts_bad  = dict()
        
        for addr in np.unique(rx_good_addrs_2):
            # Find indexes of all instances where addresses match
            #   np.squeeze here flattens the result to a 1-D array
            addr_idx = np.squeeze(rx_good_addrs_2 == addr)
    
            # Count the number of packets (True values in index array) from this address
            rx_pkts_from_addr  = np.sum(addr_idx)
    
            # Count the number of bytes from this address
            rx_bytes_from_addr = np.sum(rx_good_fcs['length'][addr_idx])
    
            # Add the information about the address to the dictionary
            rx_counts_good[addr] = (rx_pkts_from_addr, rx_bytes_from_addr)
        
        # Print the results
        print("\nExample: Rx GOOD FCS Counts (including duplicates):");
        print("Number of users: ")
        #print here number of users
        
        print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
            "Src Addr",
            "# Pkts",
            "# Bytes",
            "MAC Addr Type"))
    
        for k in sorted(rx_counts_good.keys()):
            # Use the string version of the MAC address as the key for readability
            print("{0:18}\t{1:>7}\t{2:>10}\t{3}".format(
                wlan_exp_util.mac_addr_to_str(k),
                rx_counts_good[k][0],
                rx_counts_good[k][1],
                wlan_exp_util.mac_addr_desc(k)))
                
        print ('')
        print ("---------------------Summary of MAC Addresses-------------------")
        print ("AP    MAC addr: %s" % ap_mac_str)
        print ("STA   MAC addr: %s" % sta_mac_str)
        print ("---------------------Summary of the Experiment------------------")
        print ("# Pkts Good FCS: %d" % num_rx_good_fcs)
        print ("# Pkts Bad FCS:  %d" % num_rx_bad_fcs)
        print ("# Users: %d"         % num_users)
        if uplink:
            print ("----------------------Uplink: %s -> %s----------------------" % (sta_mac_str, ap_mac_str))
        else:
            print ("---------------------Downlink: %s -> %s---------------------" % (ap_mac_str, sta_mac_str))
        print ("# Data MPDUs:           %d" % (num_data_mpdu))
        print ("# Data Fox MPDUs:       %d" % (num_data_fox_mpdu))
        print ("# Data Non-Fox MPDUs:   %d" % (num_data_mpdu - num_data_fox_mpdu))
        print ("# Re-tx Fox Data MPDUs: %d" % (num_mpdu_retx))
        print ("# Data Fox MPDUs lost:  %d" % (num_mpdu_loss))
        print ("# Ack MPDUs:            %d" % (num_ack_mpdu))
        print ("# RTS MPDUs:            %d" % (num_rts_mpdu))
        print ("# CTS MPDUs:            %d" % (num_cts_mpdu))
        
        rx_mpdu.append(num_data_fox_mpdu)
        re_tx.append(num_mpdu_retx)
        mpdu_loss.append(num_mpdu_loss)
        capt_data_mpdu_percentage = (float(num_data_fox_mpdu) / float(num_data_fox_mpdu + num_mpdu_loss)) * 100
        
        logfile = 'log_cap_'
        capt_mpdu.append(capt_data_mpdu_percentage)
#        num_retx = map(add, num_retx, total_retx)
#
#num_retx_percentage = [(i/num_exp)*100 for i in num_retx]
        
        store1.write("%s\n" % num_data_fox_mpdu)
        store2.write("%s\n" % num_mpdu_retx)
        store3.write("%s\n" % num_mpdu_loss)
        store4.write("%s\n" % capt_data_mpdu_percentage)

print rx_mpdu

store1.close()
store2.close()
store3.close()
store4.close()

#write_list_into_file(rx_mpdu, 'num_mpdu.txt')
#write_list_into_file(re_tx, 'num_mpdu_retx.txt')
#write_list_into_file(mpdu_loss, 'num_mpdu_loss.txt')
#write_list_into_file(capt_mpdu, 'capture_percentage.txt')
#write_list_into_file(num_retx_percentage, 'retx_percentage.txt')

        
#        # Total number of MPDUs received from STA1, with GOOD FCS
#        total_uplink_mpdus1 = 0
#        mac_addr_uplink_idx1 = np.squeeze(rx_good_addrs_2 == mac_sta_int1)
#        total_uplink_mpdus1 = np.sum(mac_addr_uplink_idx1)    
            
        
       
    #    total_retx_uplink1 = sort_retx(retx_per_mpdu_uplink1)
    #    print(total_retx_uplink1)
    #    print('total retx=%d' % sum(retx_per_mpdu_uplink1)
        
        
    #    if num_rx_bad_fcs > 0:
    #        # Maybe this is not very useful. Just in case we want to check if there was a BAD FCS
    #        # from a desired MPDU
    #        # Creates a RX counts for BAD FCS packets
    #        for addr in np.unique(rx_bad_addrs_2):
    #            # Find indexes of all instances where addresses match
    #            #   np.squeeze here flattens the result to a 1-D array
    #            addr_idx_bad = np.squeeze(rx_bad_addrs_2 == addr)
    #            
    #            # Count the number of packets (True values in index array) from this address
    #            rx_pkts_from_addr_bad = np.sum(addr_idx_bad)
    #            
    #            # Count the number of bytes from this address
    #            rx_bytes_from_addr_bad = np.sum(rx_bad_fcs['length'][addr_idx_bad])
    #            
    #            # Add the information about the address to the dictionary
    #            rx_counts_bad[addr] = (rx_pkts_from_addr_bad, rx_bytes_from_addr_bad)
                                             
                                             