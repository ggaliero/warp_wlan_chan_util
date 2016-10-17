"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log File Anonymizer
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the WLAN Exp Log framework to anonymize a given hdf5
log file that contains data assocated with an experiment utilizing the
802.11 reference design and WARPNet.

Hardware Setup:
    - None.  Anonymizing log data can be done completely off-line

Required Script Changes:
    - None.  Script requires filename of file to be anonymized to be
      passed in on the command line.

Description:
    This script parses the log file, removes any personally identifiable
    information from the log and write the resulting log data to a new
    file.  The personally identifiable information that is removed:
        - Any MAC address that is not in the following categories:
            - Broadcast Address (ff-ff-ff-ff-ff-ff)
            - IP v4 Multicast Address (01-00-5E-xx-xx-xx)
            - IP v6 Multicast Address (33-33-xx-xx-xx-xx)
            - WARP node (40-D8-55-04-2x-xx-xx)
        - Any payloads from transmissions / receptions
        - Any WARPNet commands
        - Hostnames in the station info
------------------------------------------------------------------------------
"""
import sys
import os
import time

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util


#-----------------------------------------------------------------------------
# Global Variables
#-----------------------------------------------------------------------------

# Global flag to print performance data
print_time   = False

all_addrs    = list()
addr_idx_map = dict()



#-----------------------------------------------------------------------------
# Anonymizer Methods
#-----------------------------------------------------------------------------
def do_replace_addr(addr):
    """Determine if the MAC address should be replaced."""
    do_replace = True

    # NOTE:  This list should stay in sync with wlan_exp.util mac_addr_desc_map

    # Don't replace the broadcast address (FF-FF-FF-FF-FF-FF)
    if(addr == (0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF)):
        do_replace = False

    # Don't replace multicast IP v4 addresses (01-00-5E-xx-xx-xx)
    #   http://technet.microsoft.com/en-us/library/cc957928.aspx
    if(addr[0:3] == (0x01, 0x00, 0x5E) and (addr[4] <= 0x7F)):
        do_replace = False

    # Don't replace multicast IP v6 addresses (33-33-xx-xx-xx-xx)
    #   http://www.cavebear.com/archive/cavebear/Ethernet/multicast.html
    if(addr[0:2] == (0x33, 0x33)):
        do_replace = False

    # Don't replace Mango addresses (40-D8-55-04-2x-xx)
    if(addr[0:4] == (0x40, 0xD8, 0x55, 0x04) and ((addr[4] & 0x20) == 0x20)):
        do_replace = False

    return do_replace


def addr_to_replace(addr, byte_index, addr_idx_map):
    """Build map of all indexes for a particular address so they can all be
    replaced by the same value.
    """
    global all_addrs
    if(do_replace_addr(addr)):
        if(addr not in all_addrs):
            all_addrs.append(addr)
        if addr not in addr_idx_map.keys():
            addr_idx_map[addr] = [byte_index,]
        else:
            addr_idx_map[addr].append(byte_index)
    return


def log_anonymize(filename):
    """Anonymize the log."""
    global all_addrs

    # Get the log_data from the file
    log_bytes = bytearray(hdf_util.hdf5_to_log_data(filename=filename))

    # Get the raw_log_index from the file
    raw_log_index = hdf_util.hdf5_to_log_index(filename=filename)

    # Get the user attributes from the file
    log_attr_dict  = hdf_util.hdf5_to_attr_dict(filename=filename)


    # Generate the index of log entry locations sorted by log entry type
    #    Merge the Rx / Tx subtypes that can be processed together
    log_index      = log_util.filter_log_index(raw_log_index,
                                               merge={'RX_OFDM': ['RX_OFDM', 'RX_OFDM_LTG'],
                                                      'TX'     : ['TX', 'TX_LTG'],
                                                      'TX_LOW' : ['TX_LOW', 'TX_LOW_LTG']})

    # Re-initialize the address-byteindex map per file using the running
    #   list of known MAC addresses
    addr_idx_map = dict()
    for addr in all_addrs:
        addr_idx_map[addr] = list()

    log_util.print_log_index_summary(log_index, "Log Index Summary (merged):")


    #---------------------------------------------------------------------
    # Step 1: Build a dictionary of all MAC addresses in the log, then
    #   map each addresses to a unique anonymous address
    #   Uses tuple(bytearray slice) since bytearray isn't hashable as-is
    #
    print("Anonmyizing file step 1 ...")

    start_time = time.time()

    #----------------------------------
    # Station Info entries
    #
    try:
        print("    Anonmyizing {0} STATION_INFO entries".format(len(log_index['STATION_INFO'])))

        for idx in log_index['STATION_INFO']:
            # 6-byte address at offsets 8
                o = 8
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))

    #----------------------------------
    # Tx/Rx Statistics entries
    #
    try:
        print("    Anonmyizing {0} TXRX_STATS entries".format(len(log_index['TXRX_STATS'])))

        for idx in log_index['TXRX_STATS']:
            # 6-byte addresses at offsets 16
                o = 16
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))

    #----------------------------------
    # Rx DSSS entries
    #
    try:
        print("    Anonmyizing {0} RX_DSSS entries".format(len(log_index['RX_DSSS'])))

        for idx in log_index['RX_DSSS']:
            # 6-byte addresses at offsets 28, 34, 40
            for o in (28, 34, 40):
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))

    #----------------------------------
    # Rx OFDM entries
    #
    try:
        print("    Anonmyizing {0} RX_OFDM entries".format(len(log_index['RX_OFDM'])))

        for idx in log_index['RX_OFDM']:
            # 6-byte addresses at offsets 284, 290, 296
            for o in (284, 290, 296):
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))

    #----------------------------------
    # Tx entries
    #
    try:
        print("    Anonmyizing {0} TX entries".format(len(log_index['TX'])))

        for idx in log_index['TX']:
            # 6-byte addresses at offsets 44, 50, 56
            for o in (44, 50, 56):
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))

    #----------------------------------
    # Tx Low entries
    #
    try:
        print("    Anonmyizing {0} TX_LOW entries".format(len(log_index['TX_LOW'])))

        for idx in log_index['TX_LOW']:
            # 6-byte addresses at offsets 40, 46, 52
            for o in (40, 46, 52):
                addr_to_replace(tuple(log_bytes[idx+o:idx+o+6]), idx+o, addr_idx_map)
    except KeyError:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))


    #---------------------------------------------------------------------
    # Step 2: Enumerate actual MAC addresses and their anonymous replacements
    #
    print("Anonmyizing file step 2 ...")

    print("    Enumerate MAC addresses and their anonymous replacements")

    addr_map = dict()
    for ii,addr in enumerate(all_addrs):
        # NOTE:  Address should not have a first octet that is odd, as this
        #  indicates the address is multicast.  Hence we use 0xFE as the first octet.
        #
        # NOTE:  Due to FCS errors, the number of addresses in a log file is
        #  potentially large.  Therefore, we support 2^24 unique addresses with
        #  the anonymizer.
        #
        anon_addr = (0xFE, 0xFF, 0xFF, (ii//(256**2)), ((ii//256)%256), (ii%256))
        addr_map[addr] = anon_addr

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))


    #---------------------------------------------------------------------
    # Step 3: Replace all MAC addresses in the log
    #
    print("Anonmyizing file step 3 ...")

    print("    Replace all MAC addresses in the log")

    for old_addr in addr_idx_map.keys():
        new_addr = bytearray(addr_map[old_addr])
        for byte_idx in addr_idx_map[old_addr]:
            log_bytes[byte_idx:byte_idx+6] = new_addr

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))


    #---------------------------------------------------------------------
    # Step 4: Other annonymization steps
    #
    print("Anonmyizing file step 4 ...")

    print("    Replace STATION_INFO hostnames")

    # Station info entries contain "hostname", the DHCP client hostname field
    #   Replace these with a string version of the new anonymous MAC addr
    try:
        for idx in log_index['STATION_INFO']:
            # 6-byte MAC addr (already anonymized) at offset 8
            # 20 character ASCII string at offset 16
            addr_o = 8
            name_o = 16
            addr = log_bytes[idx+addr_o : idx+addr_o+6]

            new_name   = "AnonNode {0:02x}_{1:02x}".format(addr[4], addr[5])
            new_name   = new_name + '\x00' * (20 - len(new_name))
            log_bytes[idx+name_o : idx+name_o+20] = bytearray(new_name.encode("UTF-8"))
    except KeyError:
        pass

    print("    Remove all WN_CMD_INFO entries")

    # WARPNet Command info entries contain command arguments that could possibly
    #   contain sensitive information.  Replace with NULL entries.
    try:
        log_util.overwrite_entries_with_null_entry(log_bytes, log_index['WN_CMD_INFO'])
    except:
        pass

    print("    Remove all payloads")

    # Overwrite all payloads with zeros
    try:
        for key in log_index.keys():
            log_util.overwrite_payloads(log_bytes, log_index[key])
    except:
        pass

    if print_time:
        print("        Time = {0:.3f}s".format(time.time() - start_time))


    #---------------------------------------------------------------------
    # Write output files
    #

    # Write the modified log to a new HDF5 file
    (fn_fldr, fn_file) = os.path.split(filename)

    # Find the last '.' in the file name and classify everything after that as the <ext>
    ext_i = fn_file.rfind('.')
    if (ext_i != -1):
        # Remember the original file extension
        fn_ext  = fn_file[ext_i:]
        fn_base = fn_file[0:ext_i]
    else:
        fn_ext  = ''
        fn_base = fn_file

    newfilename = os.path.join(fn_fldr, fn_base + "_anon" + fn_ext)

    print("Writing new file {0} ...".format(newfilename))

    # Copy any user attributes to the new anonymized file
    hdf_util.log_data_to_hdf5(log_bytes, newfilename, attr_dict=log_attr_dict)

    return


#-----------------------------------------------------------------------------
# Main
#-----------------------------------------------------------------------------

if __name__ == '__main__':
    if(len(sys.argv) < 2):
        print("ERROR: must provide at least one log file input")
        sys.exit()
    else:
        for filename in sys.argv[1:]:
            # Ensure the log file actually exists; Print an error and continue to the next file.
            if(not os.path.isfile(filename)):
                print("\nERROR: File {0} not found".format(filename))
            else:
                print("\nAnonymizing file '{0}' ({1:5.1f} MB)\n".format(filename, (os.path.getsize(filename)/1E6)))
                log_anonymize(filename)

    print("\nMAC Address Mapping:")
    for ii,addr in enumerate(all_addrs):
        anon_addr = (0xFE, 0xFF, 0xFF, (ii//(256**2)), ((ii//256)%256), (ii%256))
        print("%2d: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x" %
            (ii, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
             anon_addr[0], anon_addr[1], anon_addr[2], anon_addr[3], anon_addr[4], anon_addr[5]))
