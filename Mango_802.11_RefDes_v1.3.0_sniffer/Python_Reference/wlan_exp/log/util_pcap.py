# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Log PCAP Utilities
.. ------------------------------------------------------------------------------
.. Authors:   Chris Hunter (chunter [at] mangocomm.com)
..            Patrick Murphy (murphpo [at] mangocomm.com)
..            Erik Welsh (welsh [at] mangocomm.com)
.. License:   Copyright 2014-2015, Mango Communications. All rights reserved.
..            Distributed under the WARP license (http://warpproject.org/license)
.. ------------------------------------------------------------------------------
.. MODIFICATION HISTORY:
..
.. Ver   Who  Date     Changes
.. ----- ---- -------- -----------------------------------------------------
.. 1.00a ejw  03/31/14 Initial release
.. ------------------------------------------------------------------------------
.. 
.. This module provides utility functions for PCAP to handle WLAN Exp log data.
.. 
.. Naming convention:
.. 
..   log_data       -- The binary data from a WLAN Exp node's log.
..   
..   log_pcap_index -- This is an index that will be used for PCAP generation.
..                     Based on the selected event types, this index is a list
..                     of tuples of event type ids and offsets:
..                       [ (<int>, <offset>) ]
.. 
..   pcap           -- A packet capture format for capturing / processing network traffic
..                         http://en.wikipedia.org/wiki/Pcap
..                         http://wiki.wireshark.org/Development/LibpcapFileFormat
.. 
..   radio_tap      -- A packet structure to capture radio information about a wireless packet
..                         http://www.radiotap.org/
.. 
"""

__all__ = []


# -----------------------------------------------------------------------------
# WLAN Exp Log PCAP constants
# -----------------------------------------------------------------------------

# Global header for pcap:
#     typedef struct pcap_hdr_s {
#             guint32 magic_number;   /* magic number */
#             guint16 version_major;  /* major version number */
#             guint16 version_minor;  /* minor version number */
#             gint32  thiszone;       /* GMT to local correction */
#             guint32 sigfigs;        /* accuracy of timestamps */
#             guint32 snaplen;        /* max length of captured packets, in octets */
#             guint32 network;        /* data link type */
#     } pcap_hdr_t;
#
# pcap version is 2.4
# most defaults are based on descriptions:
#     wiki.wireshark.org/Development/LibpcapFileFormat
# network default (http://www.tcpdump.org/linktypes.html):
#     LINKTYPE_IEEE802_11	105	
# 

pcap_global_header_fmt = '<I H H I I I I'
pcap_global_header     = [('magic_number',  0xa1b2c3d4),
                          ('version_major', 2),
                          ('version_minor', 4),
                          ('thiszone',      0),
                          ('sigfigs',       0),
                          ('snaplen',       65535),
                          ('network',       105)]


# Generic packet header for pcap:
#     typedef struct pcaprec_hdr_s {
#             guint32 ts_sec;         /* timestamp seconds */
#             guint32 ts_usec;        /* timestamp microseconds */
#             guint32 incl_len;       /* number of octets of packet saved in file */
#             guint32 orig_len;       /* actual length of packet */
#     } pcaprec_hdr_t;

pcap_packet_header_fmt = '<I I I I'
pcap_packet_header     = [('ts_sec',   0),
                          ('ts_usec',  0),
                          ('incl_len', 0),
                          ('orig_len', 0)]


# Generic packet header for radio_tap:
#     struct ieee80211_radiotap_header {
#             u_int8_t        it_version;     /* set to 0 */
#             u_int8_t        it_pad;
#             u_int16_t       it_len;         /* entire length */
#             u_int32_t       it_present;     /* fields present */
#     } __attribute__((__packed__));

pcap_packet_header_fmt = '<B B H I'
pcap_packet_header     = [('it_version', 0),
                          ('it_pad',     0),
                          ('it_len',     0),
                          ('it_present', 0)]



# -----------------------------------------------------------------------------
# WLAN Exp Log PCAP file Utilities
# -----------------------------------------------------------------------------
def _log_data_to_pcap(log_data, log_index, filename, overwrite=False):
    """Create an PCAP file that contains the log_data for the entries in log_index 

    Args:
        log_data (bytes):            Binary data from a WlanExpNode log
        log_index (dict):            Log index generated from WLAN Exp log data
        filename (str):              Filename of the PCAP file to appear on disk
        overwrite (bool, optional):  If True method will overwrite existing file with filename
    
    If the requested filename already exists and overwrite==True this
    method will replace the existing file, destroying any data in the original file.

    If the filename already esists and overwrite==False this method will print a warning, 
    then create a new filename with a unique date-time suffix.
    
    .. note::  Currently log_data_to_pcap only supports ['RX_DSSS', 'RX_OFDM', 'RX_OFDM_LTG', 
        'TX', 'TX_LTG', 'TX_LOW', 'TX_LOW_LTG']
        entry types.  If other entry types are contained within the log_index, they will
        be ignored but a warning will be printed.    
    """
    import os
    from . import util as log_util

    # Process the inputs to generate any error
    log_pcap_index = _gen_pcap_log_index(log_index, log_data)
    
    # Determine a safe filename for the output PCAP file
    if overwrite:
        if os.path.isfile(filename):
            print("WARNING: overwriting existing file {0}".format(filename))

        pcap_filename = filename
    else:
        pcap_filename = log_util._get_safe_filename(filename)


    # Open an PCAP file in 'wb' mode    
    pf = open(pcap_filename, "wb")
    
    # Process the log data to populate the pcap file with data
    _covert_log_data_to_pcap(pf, log_data, log_pcap_index)

    # Close the file 
    pf.close()

# End log_data_to_pcap()



# -----------------------------------------------------------------------------
# Internal PCAP file Utilities
# -----------------------------------------------------------------------------
def _serialize_header(header, fmt):
    """Use the struct library to serialize the header dictionary using the given format."""
    import struct
    ret_val    = b''
    header_val = []

    for data in header:
        header_val.append(data[1])

    try:
        ret_val = struct.pack(fmt, *header_val)
    except struct.error as err:
        print("Error packing header: {0}".format(err))
    
    return ret_val

# End def



def _gen_pcap_log_index(log_index, log_data):
    """Uses a log index to create a pcap log index.

    For each supported entry in the log index, an entry is created in the
    pcap_log_index that is a list of tuples: 
        (timestamp, orig_len, [(incl_len, payload_offset), ...])

    Currently supported entry_types are:
        RX_DSSS
        RX_OFDM
        RX_OFDM_LTG
        TX
        TX_LTG
        TX_LOW
        TX_LOW_LTG

    """
    import struct
    from . import entry_types

    supported_entry_types = ['RX_DSSS', 'RX_OFDM', 'RX_OFDM_LTG', 'TX', 'TX_LTG', 'TX_LOW', 'TX_LOW_LTG']
    entry_type_offsets    = {}
    pcap_log_index        = []


    # Create a list of entry type ids to filter the index    
    for entry_type in supported_entry_types:
        try:
            entry         = entry_types.log_entry_types[entry_type]
            entry_offsets = entry.get_field_offsets()

            entry_type_offsets[entry_type] = entry_offsets

        except KeyError:
            print("Could not filter log data with event type: {0}".format(entry_type))


    # Create TX payload dictionary
    pcap_tx_payload_dict = _gen_pcap_tx_payload_dict(log_index, log_data)     

    # Create the raw pcap log index
    for entry_type in log_index.keys():
        try: 
            entry_offsets = entry_type_offsets[entry_type]

            timestamp_entry_offset = entry_offsets['timestamp']

            # Process TX entries
            if ( entry_type in ['TX', 'TX_LTG']):
                accept_entry_offset   = entry_offsets['time_to_accept']
                uniq_seq_entry_offset = entry_offsets['uniq_seq']
                
                for offset in log_index[entry_type]:
                    timestamp_offset  = offset + timestamp_entry_offset
                    accept_offset     = offset + accept_entry_offset
                    uniq_seq_offset   = offset + uniq_seq_entry_offset
                    
                    timestamp         = struct.unpack("<Q", log_data[timestamp_offset:(timestamp_offset + 8)])[0]
                    time_to_accept    = struct.unpack("<I", log_data[accept_offset:(accept_offset + 4)])[0]
                    uniq_seq          = struct.unpack("<I", log_data[uniq_seq_offset:(uniq_seq_offset + 4)])[0]

                    try:
                        payload_info  = pcap_tx_payload_dict[uniq_seq]
                        pcap_log_index.append(((timestamp + time_to_accept), payload_info[0], [(payload_info[1], payload_info[2])]))
                    except KeyError:
                        print("Could not find TX entry in payload dictionary.")


            # Process TX_LOW entries
            if ( entry_type in ['TX_LOW', 'TX_LOW_LTG']):
                uniq_seq_entry_offset = entry_offsets['uniq_seq']
                length_entry_offset   = entry_offsets['length']
                incl_len_entry_offset = entry_offsets['mac_payload_len']
                payload_entry_offset  = entry_offsets['mac_payload']
                
                for offset in log_index[entry_type]:
                    timestamp_offset  = offset + timestamp_entry_offset
                    uniq_seq_offset   = offset + uniq_seq_entry_offset
                    incl_len_offset   = offset + incl_len_entry_offset
                    payload_offset    = offset + payload_entry_offset
                    
                    timestamp         = struct.unpack("<Q", log_data[timestamp_offset:(timestamp_offset + 8)])[0]
                    uniq_seq          = struct.unpack("<I", log_data[uniq_seq_offset:(uniq_seq_offset + 4)])[0]
                    incl_len          = struct.unpack("<I", log_data[incl_len_offset:(incl_len_offset + 4)])[0]

                    # If the TX entry associated with the TX_LOW entry does not exist, then just use the 
                    # data in the TX_LOW entry.  
                    # 
                    # NOTE: For TX_LOW entries, we must copy the MAC header / LTG header from the log directly
                    # then fill in the rest of the payload with the values from the TX entry.
                    try:
                        payload_info  = pcap_tx_payload_dict[uniq_seq]
                        payload       = [(incl_len, payload_offset), (payload_info[1] - incl_len, payload_info[2] + incl_len)]

                        pcap_log_index.append((timestamp, payload_info[0], payload))

                    except KeyError:
                        length_offset = offset + length_entry_offset                        
                        orig_len      = struct.unpack("<H", log_data[length_offset:(length_offset + 2)])[0]
                        
                        pcap_log_index.append((timestamp, orig_len, [(incl_len, payload_offset)]))


            # Process RX entries
            if ( entry_type in ['RX_DSSS', 'RX_OFDM', 'RX_OFDM_LTG']):
                length_entry_offset   = entry_offsets['length']
                incl_len_entry_offset = entry_offsets['mac_payload_len']
                payload_entry_offset  = entry_offsets['mac_payload']
                
                for offset in log_index[entry_type]:
                    timestamp_offset  = offset + timestamp_entry_offset
                    length_offset     = offset + length_entry_offset
                    incl_len_offset   = offset + incl_len_entry_offset
                    payload_offset    = offset + payload_entry_offset
                    
                    timestamp         = struct.unpack("<Q", log_data[timestamp_offset:(timestamp_offset + 8)])[0]
                    orig_len          = struct.unpack("<H", log_data[length_offset:(length_offset + 2)])[0]
                    incl_len          = struct.unpack("<I", log_data[incl_len_offset:(incl_len_offset + 4)])[0]
                    
                    pcap_log_index.append((timestamp, orig_len, [(incl_len, payload_offset)]))


        except KeyError:
            print("Can not use entry type: {0} in PCAP generation.".format(entry_type))

    return pcap_log_index

# End def



def _gen_pcap_tx_payload_dict(log_index, log_data):
    """Uses a log index and log data to create a dictionary of TX payload 
    lengths and offsets.
    
    For each TX entry, there is a unique sequence number that is used by all
    TX_LOW entries to uniquely identify all transmitted packets assocaiated with
    the TX entry.  Using this unique sequence number, we will construct the
    following dictionary:
    
    { 'unique_seq' : (orig_len, incl_len, payload_offset) }

    where orig_len is the length of payload, the incl_len is the length of the 
    recorded payload and payload_offset is the offset in the log data where to 
    find the payload.
    
    This is currently supported for entry_types:
        TX
        TX_LTG

    NOTE: This function assumes that for a given log_index / log_data that all 
    uniq_seq entries are unique (ie this cannot be used for a log_index / log_data
    that contains data from multiple nodes).
    """
    import struct
    from . import entry_types

    supported_entry_types = ['TX', 'TX_LTG']
    pcap_tx_payload_dict  = {}
    entry_type_offsets    = {}    

    for entry_type in supported_entry_types:
        try:
            entry         = entry_types.log_entry_types[entry_type]
            entry_offsets = entry.get_field_offsets()

            entry_type_offsets[entry_type] = [entry_offsets['uniq_seq'],
                                              entry_offsets['length'],
                                              entry_offsets['mac_payload_len'],
                                              entry_offsets['mac_payload']]
        except KeyError:
            print("Could not filter log data with event type: {0}".format(entry_type))

    # Create the tx payload dictionary
    for entry_type in log_index.keys():
        try: 
            offsets = entry_type_offsets[entry_type]

            for offset in log_index[entry_type]:
                uniq_seq_offset   = offset + offsets[0]
                length_offset     = offset + offsets[1]
                incl_len_offset   = offset + offsets[2]
                payload_offset    = offset + offsets[3]

                # Get the unique sequence number for the TX packet
                uniq_seq     = struct.unpack("<Q", log_data[uniq_seq_offset:(uniq_seq_offset + 8)])[0]
                
                # Get the payload length
                orig_len     = struct.unpack("<H", log_data[length_offset:(length_offset + 2)])[0]

                # Get the recorded payload length
                incl_len     = struct.unpack("<I", log_data[incl_len_offset:(incl_len_offset + 4)])[0]

                # Add values to the dictionary        
                pcap_tx_payload_dict[uniq_seq] = (orig_len, incl_len, payload_offset)

        except KeyError:
            pass

    return pcap_tx_payload_dict

# End def



def _covert_log_data_to_pcap(log_file, log_data, log_pcap_index):
    """Create a PCAP file from the log data and the PCAP index.
    
    The PCAP index has the form:
        (timestamp, orig_len, [(incl_len, payload_offset), ...])

    """
    import struct
    from operator import itemgetter

    time_factor = 1000000        # Timestamps are in # of microseconds (ie 10^(-6) seconds)

    # Write the Global header to the file
    log_file.write(_serialize_header(pcap_global_header, pcap_global_header_fmt))
    
    # Sort the PCAP data by timestamp
    log_pcap_index = sorted(log_pcap_index, key=itemgetter(0))
    
    # Iterate through the index and create a pcap entry for each item
    for item in log_pcap_index:
        # Get the item data
        timestamp    = item[0]                        # Timestamp (# of microseconds)        
        orig_len     = item[1]                        # Original packet length (in bytes)

        # Get the payload
        (incl_len, payload) = _extract_payload(item[2], log_data)
        
        # Populate the packet header
        ts_sec       = int(timestamp / time_factor)
        ts_usec      = int(timestamp % time_factor)

        fmt = '<I I I I'

        # Trim the payload
        if (incl_len > orig_len):
            incl_len = orig_len
            payload  = payload[:incl_len]

        try:
            log_file.write(struct.pack(fmt, ts_sec, ts_usec, incl_len, orig_len))
            log_file.write(payload)
        except struct.error as err:
            print("Error packing packet: {0}".format(err))

# End def



def _extract_payload(offsets, log_data):
    """Extract the mac payload from the list of offsets"""
    incl_len     = 0
    payload_data = bytearray()
    
    for offset in offsets:
        incl_len     += offset[0]
        payload_data += log_data[offset[1]:(offset[1] + offset[0])]

    return (incl_len, payload_data)

# End def
















