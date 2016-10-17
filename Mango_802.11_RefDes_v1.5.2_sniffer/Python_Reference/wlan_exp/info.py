# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework 
    - Information Struct classes
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides class definitions for information classes.

Classes (see below for more information):
    Info()            -- Base class for information classes
    StationInfo()     -- Base class for station information
    BSSInfo()         -- Base class for basic service set (BSS) information
    CountsInfo()      -- Base class for information on node counts

"""
import struct
import sys

import wlan_exp.util as util

__all__ = ['StationInfo', 'BSSInfo', 'TxRxCounts']

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None


#-------------------------------------------------------------------------
# Field definitions for Information structures
#
# The info_field_defs is a dictionary of field defintions whose key is the
# field name and whose value is an ordered list of tuples of the form:
#    (field name, struct format, numpy format, description)
#
# where:
#     field name     - Text name of the field
#     struct format  - Python struct format identifier
#     numpy format   - Numpy format identifier
#     description    - Text description of the field
#
# These tuples can be extended by sub-classes, but the first four fields are
# required in order to efficiently serialize / deserialize information objects
# for communication over the transport.
#
# These definitions match the corresponding definitions in the 802.11 
# Reference Design Experiments Framework in C.
#
#-------------------------------------------------------------------------
info_field_defs = {
    'STATION_INFO' : [
        ('mac_addr',                       '6s',     '6uint8',  'MAC address of station'),
        ('id',                             'H',      'uint16',  'Identification Index for this station'),
        ('host_name',                      '20s',    '20uint8', 'String hostname (19 chars max), taken from DHCP DISCOVER packets'),
        ('flags',                          'I',      'uint32',  'Station flags'),
        ('latest_rx_timestamp',            'Q',      'uint64',  'Value of System Time in microseconds of last successful Rx from device'),
        ('latest_txrx_timestamp',          'Q',      'uint64',  'Value of System Time in microseconds of last successful Rx from device or Tx to device'),
        ('latest_rx_seq',                  'H',      'uint16',  'Sequence number of last packet received from device'),
        ('padding',                        '6s',     '6uint8',   ''),
        ('tx_phy_mcs',                     'B',      'uint8',   'Current PHY MCS index in [0:7] for new transmissions to device'),
        ('tx_phy_mode',                    'B',      'uint8',   'Current PHY mode for new transmissions to deivce'),
        ('tx_phy_antenna_mode',            'B',      'uint8',   'Current PHY antenna mode in [1:4] for new transmissions to device'),
        ('tx_phy_power',                   'b',      'int8',    'Current Tx power in dBm for new transmissions to device'),
        ('tx_mac_flags',                   'B',      'uint8',   'Flags for Tx MAC config for new transmissions to device'),
        ('padding2',                       '3x',     '3uint8',  '')],

    'BSS_INFO' : [
        ('bssid',                       '6s',     '6uint8',  'BSS ID'),
        ('channel',                     'B',      'uint8',   'Primary channel'),
        ('channel_type',                'B',      'uint8',   'Channel Type'),
        ('latest_beacon_rx_time',       'Q',      'uint64',  'Value of System Time in microseconds of last beacon Rx'),
        ('ssid',                        '33s',    '33uint8', 'SSID (32 chars max)'),
        ('latest_beacon_rx_power',      'b',      'int8',    'Last observed beacon Rx Power (dBm)'),
        ('padding0',                    '2x',     'uint16',  ''),
        ('capabilities',                'H',      'uint16',  'Supported capabilities of the BSS'),
        ('beacon_interval',             'H',      'uint16',  'Beacon interval - In time units of 1024 us')],

    'BSS_CONFIG' : [
        ('update_mask',                 'I',      'uint32',  'Bit mask indicating which fields were updated'),    
        ('bssid',                       '6s',     '6uint8',  'BSS ID'),
        ('beacon_interval',             'H',      'uint16',  'Beacon interval - In time units of 1024 us'),
        ('ssid',                        '33s',    '33uint8', 'SSID (32 chars max)'),
        ('channel',                     'B',      'uint8',   'Primary channel'),
        ('channel_type',                'B',      'uint8',   'Channel Type'),
        ('ht_capable',                  'B',      'uint8',   'Support for HTMF Tx/Rx')],

    'TXRX_COUNTS' : [
        ('retrieval_timestamp',         'Q',      'uint64',  'Value of System Time in microseconds when structure retrieved from the node'),
        ('mac_addr',                    '6s',     '6uint8',  'MAC address of remote node whose counts are recorded here'),
        ('padding',                     'H',      'uint16',  ''),
        ('data_num_rx_bytes',           'Q',      'uint64',  'Number of non-duplicate bytes received in DATA packets from remote node'),
        ('data_num_rx_bytes_total',     'Q',      'uint64',  'Total number of bytes received in DATA packets from remote node'),
        ('data_num_tx_bytes_success',   'Q',      'uint64',  'Total number of bytes successfully transmitted in DATA packets to remote node'),
        ('data_num_tx_bytes_total',     'Q',      'uint64',  'Total number of bytes transmitted (successfully or not) in DATA packets to remote node'),
        ('data_num_rx_packets',         'I',      'uint32',  'Number of non-duplicate DATA packets received from remote node'),
        ('data_num_rx_packets_total',   'I',      'uint32',  'Total number of DATA packets received from remote node'),
        ('data_num_tx_packets_success', 'I',      'uint32',  'Total number of DATA packets successfully transmitted to remote node'),
        ('data_num_tx_packets_total',   'I',      'uint32',  'Total number of DATA packets transmitted (successfully or not) to remote node'),
        ('data_num_tx_attempts',        'Q',      'uint64',  'Total number of attempts of DATA packets to remote node (includes re-transmissions)'),
        ('mgmt_num_rx_bytes',           'Q',      'uint64',  'Number of bytes non-duplicate received in management packets from remote node'),
        ('mgmt_num_rx_bytes_total',     'Q',      'uint64',  'Total number of bytes received in management packets from remote node'),
        ('mgmt_num_tx_bytes_success',   'Q',      'uint64',  'Total number of bytes successfully transmitted in management packets to remote node'),
        ('mgmt_num_tx_bytes_total',     'Q',      'uint64',  'Total number of bytes transmitted (successfully or not) in management packets to remote node'),
        ('mgmt_num_rx_packets',         'I',      'uint32',  'Number of non-duplicate management packets received from remote node'),
        ('mgmt_num_rx_packets_total',   'I',      'uint32',  'Total number of management packets received from remote node'),
        ('mgmt_num_tx_packets_success', 'I',      'uint32',  'Total number of management packets successfully transmitted to remote node'),
        ('mgmt_num_tx_packets_total',   'I',      'uint32',  'Total number of management packets transmitted (successfully or not) to remote node'),
        ('mgmt_num_tx_attempts',        'Q',      'uint64',  'Total number of attempts management packets to remote node (includes re-transmissions)')]
}


info_consts_defs = {
    'STATION_INFO' : util.consts_dict({
        'flags'         : util.consts_dict({
            'KEEP'                     : 0x00000001,
            'DISABLE_ASSOC_CHECK'      : 0x00000002,
            'DOZE'                     : 0x00000004,
            'HT_CAPABLE'               : 0x00000008
        }),
        'tx_phy_mode'   : util.phy_modes,
        'tx_mac_flags'  : util.consts_dict()
    }),

    'BSS_INFO'     : util.consts_dict({
        'channel_type'  : util.consts_dict({
            'BW20'                     : 0x0000,
            'BW40_SEC_BELOW'           : 0x0001,
            'BW40_SEC_ABOVE'           : 0x0002,
        }),
        'capabilities'  : util.consts_dict({
            'ESS'                      : 0x0001,
            'IBSS'                     : 0x0002,
            'HT_CAPABLE'               : 0x0004,
            'PRIVACY'                  : 0x0010,
        })
    }),

    'BSS_CONFIG'   : util.consts_dict({
        'update_mask'  : util.consts_dict({
            'BSSID'                    : 0x00000001,
            'CHANNEL'                  : 0x00000002,
            'SSID'                     : 0x00000004,
            'BEACON_INTERVAL'          : 0x00000008,
            'HT_CAPABLE'               : 0x00000010
        }),
        'channel_type'  : util.consts_dict({
            'BW20'                     : 0x0000,
            'BW40_SEC_BELOW'           : 0x0001,
            'BW40_SEC_ABOVE'           : 0x0002,
        })
    }),
    
    'TXRX_COUNTS'  : util.consts_dict()
}



# -----------------------------------------------------------------------------
# Information Structure Base Class
# -----------------------------------------------------------------------------

class InfoStruct(dict):
    """Base class for structured information classes

    This class provides the basic methods for setting / accessing information
    within the info struct object.  It also provides the base methods for
    serializing / deserializing the object for communication by the transport.

    An InfoStruct object represents structured data that is passed to/from the
    MAC C code. In the C code these objects are represented by struct defintions.
    The fields defined for an InfoStruct object must match the fields in the
    corresponding C struct. Conceptually these InfoStruct objects are very similar
    to log entries; they even share the log entry syntax for defining fields. By
    defining InfoStruct objects here, however, wlan_exp scripts can control and
    retrieve parameters encoded in non-log structs on the node without relying
    on any of wlan_exp's logging framework. The primary benefit of this separation
    is removing the numpy dependency when dealing with the non-log version of
    the info structures described here.
    """
    _field_name         = None         # Internal name for the info_field_defs entry
    _fields_struct_fmt  = None         # Internal string of field formats, in struct module format
    _consts             = None         # Internal container for user-defined, type-specific constants

    def __init__(self, field_name):
        super(InfoStruct, self).__init__()

        if(field_name not in info_field_defs.keys()):
            msg  = "Field name {0} does not exist in info_field_defs.".format(field_name)
            raise AttributeError(msg)
            
        if(field_name not in info_consts_defs.keys()):
            msg  = "Field name {0} does not exist in info_consts_defs.".format(field_name)
            raise AttributeError(msg)

        # Initialize variables
        self._field_name         = field_name
        self._fields_struct_fmt  = ''
        self._consts             = info_consts_defs[field_name]

        # Add and initialize all the fields in the info_field_defs
        for field in info_field_defs[field_name]:
            if 'x' not in field[1]:
                self[field[0]] = None

        # Update the meta-data about the fields
        self._update_field_defs()


    # -------------------------------------------------------------------------
    # Accessor methods for the Info Type
    # -------------------------------------------------------------------------
    def get_field_names(self):
        """Get the field names.

        Returns:
            names (list of str):  List of string field names for the entry
        """
        if(self._field_name in info_field_defs.keys()):
            return [f[0] for f in info_field_defs[self._field_name]]
        else:
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)


    def get_field_struct_formats(self):
        """Get the Python struct format of the fields.

        Returns:
            formats (list of str):  List of Python struct formats for the fields
        """
        if(self._field_name in info_field_defs.keys()):
            return [f[1] for f in info_field_defs[self._field_name]]
        else:
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)


    def get_field_defs(self):
        """Get the field definitions.

        Returns:
            fields (list of tuple):  List of tuples that describe each field
        """
        if(self._field_name in info_field_defs.keys()):            
            return info_field_defs[self._field_name]
        else:
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)


    def get_consts(self):
        """Get all constants defined in the info struct object as a dictionary

        Returns:
            values (dict):  All constant values in the object
        """
        return self._consts.copy()


    def get_const(self, name):
        """Get a constant defined in the info struct object

        Returns:
            value (int or str):  Value associated with the constant
        """
        if (name in self._consts.keys()):
            return self._consts[name]
        else:
            msg  = "Constant {0} does not exist ".format(name)
            msg += "in {0}".format(self.__class__.__name__)
            raise AttributeError(msg)


    def sizeof(self):
        """Return the size of the object when being transmitted / received by
        the transport
        """
        return struct.calcsize(self._fields_struct_fmt)


    # -------------------------------------------------------------------------
    # Utility methods for the InfoStruct object
    # -------------------------------------------------------------------------
    def serialize(self):
        """Packs object into a data buffer

        Returns:
            data (bytearray):  Bytearray of packed binary data
        """
        # Pack the object into a single data buffer
        ret_val    = b''
        fields     = []
        field_type = []
        tmp_values = []
        used_field = []

        if(self._field_name not in info_field_defs.keys()):
            msg  = "Field name {0} does not exist in info_feild_defs.".format(self._field_name)
            raise AttributeError(msg)

        for field in info_field_defs[self._field_name]:
            if 'x' not in field[1]:
                fields.append(field[0])
                try:
                    tmp_values.append(self[field[0]])
                    field_type.append(type(self[field[0]]))
                    used_field.append(True)
                except KeyError:
                    tmp_values.append(0)
                    field_type.append(None)
                    used_field.append(False)

        try:
            ret_val += struct.pack(self._fields_struct_fmt, *tmp_values)
        except struct.error as err:
            print("Error serializing structure:\n\t{0}".format(err))
            print("Serialize Structure:")
            print(fields)
            print(field_type)
            print(used_field)
            print(tmp_values)
            print(self._fields_struct_fmt)
            raise RuntimeError("See above print statements to debug error.")
            

        if (self.sizeof()) != len(ret_val):
            msg  = "WARNING: Sizes do not match.\n"
            msg += "    Expected  {0} bytes".format(self.sizeof())
            msg += "    Buffer is {0} bytes".format(len(ret_val))
            print(msg)

        return ret_val


    def deserialize(self, buf):
        """Unpacks a buffer of data into the object

        Args:
            buf (bytearray): Array of bytes containing the values of an information object

        Returns:
             (Info Object):  Each Info object in the list has been filled in with the corresponding
                data from the buffer.
        """
        all_names   = self.get_field_names()
        all_fmts    = self.get_field_struct_formats()
        object_size = self.sizeof()

        try:
            data_tuple = struct.unpack(self._fields_struct_fmt, buf[0:object_size])

            # Filter out names for fields ignored during unpacking
            names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

            # Populate object with data
            for i, name in enumerate(names):
                self[name] = data_tuple[i]

        except struct.error as err:
            print("Error unpacking buffer with len {0}: {1}".format(len(buf), err))
            


    # -------------------------------------------------------------------------
    # Internal methods for the InfoStruct object
    # -------------------------------------------------------------------------
    def _update_field_defs(self):
        """Internal method to update meta-data about the fields."""
        # Update the fields format used by struct unpack/calcsize
        self._fields_struct_fmt = ' '.join(self.get_field_struct_formats())


    def __str__(self):
        """Pretty print info struct object"""
        msg = "{0}\n".format(self.__class__.__name__)

        for field in info_field_defs[self._field_name]:
            if 'x' not in field[1]:
                msg += "    {0:30s} = {1}\n".format(field[0], self[field[0]])

        return msg

    # Allow attribute (ie ".") notation to access contents of dictionary
    def __getattr__(self, name):
        if name in self:
            return self[name]
        else:
            super(InfoStruct, self).__getattr__(name)
    
    def __setattr__(self, name, value):
        if name in self:
            self[name] = value
        else:
            super(InfoStruct, self).__setattr__(name, value)


# End Class



# -----------------------------------------------------------------------------
# TX/RX Counts Class
# -----------------------------------------------------------------------------

class TxRxCounts(InfoStruct):
    """Class for TX/RX counts."""

    def __init__(self):
        super(TxRxCounts, self).__init__(field_name='TXRX_COUNTS')

        # To populate the TxRxCounts with information, use the
        # deserialize() function on a proper buffer of data


    def serialize(self):
        # serialize() is currently not supported for TxRxCounts.  This
        # is due to the fact that TxRxCounts information should only come
        # directly from the node and should not be set to the node.
        #
        print("Error:  serialize() is not supported for TxRxCounts.")
        raise NotImplementedError

        # If serialize() needs to be supported in future version for
        # TxRxCounts, below is the code to use:
        #
        # # Convert MAC address to byte string for transmit
        # mac_addr_tmp     = self['mac_addr']
        # self['mac_addr'] = util.str_to_mac_addr(self['mac_addr'])
        # self['mac_addr'] = util.mac_addr_to_byte_str(self['mac_addr'])
        #
        # ret_val = super(TxRxCounts, self).serialize()
        #
        # # Revert MAC address to a colon delimited string
        # self['mac_addr'] = mac_addr_tmp
        #
        # return ret_val


    def deserialize(self, buf):
        super(TxRxCounts, self).deserialize(buf)

        if (False):
            msg = "TX/RX Counts Data Buffer:\n"
            msg += util.buffer_to_str(buf)
            print(msg)

        # Clean up the values
        #   - Convert the MAC Address to a colon delimited string
        self['mac_addr'] = util.byte_str_to_mac_addr(self['mac_addr'])
        #self['mac_addr'] = util.mac_addr_to_str(self['mac_addr'])


# End Class



# -----------------------------------------------------------------------------
# Station Info Class
# -----------------------------------------------------------------------------

class StationInfo(InfoStruct):
    """Class for Station Information."""

    def __init__(self):
        super(StationInfo, self).__init__(field_name='STATION_INFO')

        # To populate the TxRxCounts with information, use the
        # deserialize() function on a proper buffer of data


    def serialize(self):
        # serialize() is currently not supported for StationInfo.  This
        # is due to the fact that StationInfo information should only come
        # directly from the node and should not be set to the node.
        #
        print("Error:  serialize() is not supported for StationInfo.")
        raise NotImplementedError

        # If serialize() needs to be supported in future version for
        # StationInfo, below is the code to use:
        #
        # # Convert MAC address to byte string for transmit
        # mac_addr_tmp     = self['mac_addr']
        # self['mac_addr'] = util.str_to_mac_addr(self['mac_addr'])
        # self['mac_addr'] = util.mac_addr_to_byte_str(self['mac_addr'])
        #
        # ret_val = super(StationInfo, self).serialize()
        #
        # # Revert MAC address to a colon delimited string
        # self['mac_addr'] = mac_addr_tmp
        #
        # return ret_val


    def deserialize(self, buf):
        super(StationInfo, self).deserialize(buf)

        if (False):
            msg = "Station Info Data buffer:\n"
            msg += util.buffer_to_str(buf)
            print(msg)

        # Clean up the values
        #   - Remove extra characters in the SSID
        #   - Convert the MAC Address to a colon delimited string
        if (self['host_name'][0] == '\x00'):
            self['host_name'] = '\x00'
        else:
            import ctypes
            self['host_name'] = ctypes.c_char_p(self['host_name']).value

        self['mac_addr'] = util.byte_str_to_mac_addr(self['mac_addr'])
        #self['mac_addr'] = util.mac_addr_to_str(self['mac_addr'])


# End Class



# -----------------------------------------------------------------------------
# BSS Info Class
# -----------------------------------------------------------------------------

class BSSInfo(InfoStruct):
    """Class for Basic Service Set (BSS) Information

    Attributes:
        bssid (int, str):  40-bit ID of the BSS either as a integer or colon delimited
            string of the form:  XX:XX:XX:XX:XX:XX
        ssid (str):   SSID string (Must be 32 characters or less)
        channel (int): Channel number on which the BSS operates
        ibss_status (bool, optional): Status of the
            BSS:
                * **True**  --> Capabilities field = 0x2 (BSS_INFO is for IBSS)
                * **False** --> Capabilities field = 0x1 (BSS_INFO is for BSS)
        beacon_interval (int): Integer number of beacon Time Units in [1, 65534]
            (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds)
    """
    def __init__(self, bssid=None, ssid=None, channel=None, ibss_status=None, beacon_interval=None):
        super(BSSInfo, self).__init__(field_name='BSS_INFO')
        
        # Only initialize the BSSInfo() if one of the fields is provided.
        init_fields = False
        
        if ((bssid is not None) or (ssid is not None) or (channel is not None) or 
            (ibss_status is not None) or (beacon_interval is not None)):
            init_fields = True        
        
        # Default values used if initializing fields but value not provided:
        #     bssid           - No default value - Error
        #     ssid            - ""
        #     channel         - No default value - Error
        #     ibss_status     - False
        #     beacon_interval - 100
        #
        # This is done so there is no issue during serialization when instantiating 
        # a BSSInfo() with initialized fields.
        #
        if init_fields:
            # Set default values for fields not set by this method
            self['timestamp']                  = 0
            self['latest_beacon_rx_time']      = 0
            self['state']                      = self._consts.state.UNAUTHENTICATED
            self['latest_beacon_rx_power']     = 0
            self['flags']                      = 0

            # Set SSID
            if ssid is not None:
                # Check SSID
                if type(ssid) is not str:
                    raise ValueError("The SSID must be a string.")

                if len(ssid) > 32:
                    ssid = ssid[:32]
                    print("WARNING:  SSID must be 32 characters or less.  Truncating to {0}".format(ssid))

                try:
                    self['ssid']         = bytes(ssid, "UTF8")
                except:
                    self['ssid']         = ssid
            else:
                self['ssid'] = bytes()

            # Set Channel
            if channel is not None:
                # Check Channel
                #   - Make sure it is a valid channel
                if channel not in util.wlan_channels:
                    msg  = "The channel must be a valid channel number.  See util.py wlan_channels."
                    raise ValueError(msg)
    
                self['channel']      = channel
                self['channel_type'] = self._consts.channel_type.BW20
            else:
                raise AttributeError("Channel must be provided when initializing BSSInfo() fields")
                

            # Set the beacon interval
            if beacon_interval is not None:
                # Check beacon interval
                if type(beacon_interval) is not int:
                    beacon_interval = int(beacon_interval)
                    print("WARNING:  Beacon interval must be an interger number of time units.  Rounding to {0}".format(beacon_interval))

                if not ((beacon_interval > 0) and (beacon_interval < (2**16 - 1))):
                    msg  = "The beacon interval must be in [1, 65534] (ie 16-bit positive integer)."
                    raise ValueError(msg)

                self['beacon_interval'] = beacon_interval
            else:
                self['beacon_interval'] = 100               


            # Set the BSSID
            if bssid is not None:
                if ibss_status is None:
                    ibss_status = False
                else:
                    # Check IBSS status value provided
                    if type(ibss_status) is not bool:
                        raise ValueError("The ibss_status must be a boolean.")

                # Set BSSID, capabilities
                #   - If this is an IBSS, then set local bit to '1' and mcast bit to '0'
                #   - Set the appropriate capabilities (The 802.11 reference design only supports short timeslots (ie short = 9us))
                if ibss_status:
                    self['bssid']        = util.create_locally_administered_bssid(bssid)
                    self['capabilities'] = (self._consts.capabilities.IBSS | self._consts.capabilities.SHORT_TIMESLOT)
                else:
                    self['bssid']        = bssid
                    self['capabilities'] = (self._consts.capabilities.ESS | self._consts.capabilities.SHORT_TIMESLOT)

                # Convert BSSID to colon delimited string for internal storage
                if type(bssid) in [int, long]:
                    self['bssid']        = util.mac_addr_to_str(self['bssid'])
            else:
                raise AttributeError("BSSID must be provided when initializing BSSInfo() fields")


    def serialize(self):
        # Convert bssid to byte string for transmit
        bssid_tmp     = self['bssid']
        self['bssid'] = util.str_to_mac_addr(self['bssid'])
        self['bssid'] = util.mac_addr_to_byte_str(self['bssid'])

        ret_val = super(BSSInfo, self).serialize()

        # Revert bssid to colon delimited string
        self['bssid'] = bssid_tmp

        return ret_val


    def deserialize(self, buf):
        super(BSSInfo, self).deserialize(buf)

        if (False):
            msg  = "BSS Info Data buffer:\n"
            msg += util.buffer_to_str(buf)
            print(msg)

        # Clean up the values
        #   - Remove extra characters in the SSID
        #   - Convert the BSS ID to a colon delimited string for storage
        #
        #   A BSSID is a 48-bit integer and can be treated like a MAC
        #     address in the wlan_exp framework (ie all the MAC address
        #     utility functions can be used on it.)
        #
        import ctypes            
        self['ssid']  = ctypes.c_char_p(self['ssid']).value
        # If the SSID is not a string already (which happens in Python3)
        #   then decode the bytes class assuming a UTF-8 encoding
        if type(self['ssid']) is not str:
            self['ssid'] = self['ssid'].decode('utf-8')
                    
        self['bssid'] = util.byte_str_to_mac_addr(self['bssid'])
        self['bssid'] = util.mac_addr_to_str(self['bssid'])


# End Class



# -----------------------------------------------------------------------------
# BSS Info Class
# -----------------------------------------------------------------------------

class BSSConfig(InfoStruct):
    """Class for Basic Service Set (BSS) Configuration Information

    Attributes:
        bssid (int):  48-bit ID of the BSS either as a integer; A value of 
            None will remove current BSS on the node (similar to 
            node.reset(bss=True)); A value of False will not update the current 
            bssid
        ssid (str):  SSID string (Must be 32 characters or less); A value of 
            None will not update the current SSID
        channel (int): Channel on which the BSS operates; A value of None will
            not update the current channel
        beacon_interval (int): Integer number of beacon Time Units in [10, 65534]
            (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
            A value of None will disable beacons;  A value of False will not 
            update the current beacon interval
        ht_capable (bool):  Does the node support HTMF Tx/Rx.  A value of None 
            will not update the current value of HT capable.
            
    """
    def __init__(self, bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):                       
        super(BSSConfig, self).__init__(field_name='BSS_CONFIG')

        # Default values used if value not provided:
        #     bssid           - 00:00:00:00:00:00
        #     ssid            - ""
        #     channel         - 0
        #     beacon_interval - 0xFFFF
        #     ht_capable      - 0xFF

        # Initialize update mask
        self['update_mask'] = 0

        # Set the BSSID field
        if bssid is not False:
            if bssid is not None:
                self['bssid'] = bssid
    
                # Convert BSSID to colon delimited string for internal storage
                if type(bssid) in [int, long]:
                    self['bssid']        = util.mac_addr_to_str(self['bssid'])
                    
            else:
                self['bssid'] = "00:00:00:00:00:00"
                
            # Set update mask
            self['update_mask'] |= self._consts.update_mask.BSSID
        else:
            # Remove current BSS on the node
            self['bssid'] = "00:00:00:00:00:00"
        
        # Set SSID field
        if ssid is not None:
            # Check SSID            
            
            if type(ssid) is not str:
                raise ValueError("The SSID type was {0}".format(type(ssid)))

            if len(ssid) > 32:
                ssid = ssid[:32]
                print("WARNING:  SSID must be 32 characters or less.  Truncating to {0}".format(ssid))

            try:
                self['ssid']         = bytes(ssid, "UTF8")
            except:
                self['ssid']         = ssid
            
            # Set update mask
            self['update_mask'] |= self._consts.update_mask.SSID
        else:
            self['ssid'] = bytes()

        # Set Channel field
        if channel is not None:
            # Check Channel
            #   - Make sure it is a valid channel; only store channel
            if channel not in util.wlan_channels:
                msg  = "The channel must be a valid channel number.  See util.py wlan_channels."
                raise ValueError(msg)

            self['channel']    = channel
            
            # Set update mask
            self['update_mask'] |= self._consts.update_mask.CHANNEL
        else:
            self['channel'] = 0

        self['channel_type'] = self._consts.channel_type.BW20
        
        # Set the beacon interval field
        if beacon_interval is not False:
            if beacon_interval is not None:
                # Check beacon interval
                if type(beacon_interval) is not int:
                    beacon_interval = int(beacon_interval)
                    print("WARNING:  Beacon interval must be an interger number of time units.  Rounding to {0}".format(beacon_interval))
    
                if not ((beacon_interval > 9) and (beacon_interval < (2**16 - 1))):
                    msg  = "The beacon interval must be in [10, 65534] (ie 16-bit positive integer)."
                    raise ValueError(msg)
    
                self['beacon_interval'] = beacon_interval
            else:
                # Disable beacons
                self['beacon_interval'] = 0                
            
            # Set update mask
            self['update_mask'] |= self._consts.update_mask.BEACON_INTERVAL
        else:
            self['beacon_interval'] = 0xFFFF


        # Set the HT capable field
        if ht_capable is not None:
            # Check HT capable
            if type(ht_capable) is not bool:
                msg  = "ht_capable must be a boolean."
                raise ValueError(msg)

            self['ht_capable'] = ht_capable
            
            # Set update mask
            self['update_mask'] |= self._consts.update_mask.HT_CAPABLE
        else:
            self['ht_capable'] = 0xFF
        

    def serialize(self):
        # Convert bssid to byte string for transmit
        bssid_tmp     = self['bssid']
        self['bssid'] = util.str_to_mac_addr(self['bssid'])
        self['bssid'] = util.mac_addr_to_byte_str(self['bssid'])

        ret_val = super(BSSConfig, self).serialize()

        # Revert bssid to colon delimited string
        self['bssid'] = bssid_tmp

        return ret_val


    def deserialize(self, buf):
        raise NotImplementedError("")


# End Class



# -----------------------------------------------------------------------------
# Misc Utilities
# -----------------------------------------------------------------------------
def deserialize_info_buffer(buffer, buffer_class):
    """Unpacks a buffer of data into a list of objects

    Args:
        buf (bytearray): Array of bytes containing 1 or more objects of the same type

    Returns:
        information objects (List of Info Objects):
            Each Info object in the list has been filled in with the corresponding
            data from the buffer.
    """
    ret_val     = []
    buffer_size = len(buffer)
    index       = 0
    object_size = 1

    try:
        tmp_obj     = eval(buffer_class, globals(), locals())
        object_size = tmp_obj.sizeof()
    except:
        print("ERROR:  Cannot create information object of class: {0}".format(buffer_class))

    while (index < buffer_size):
        # try:
        tmp_obj = eval(buffer_class, globals(), locals())
        tmp_obj.deserialize(buffer[index:index+object_size])
        # except:
        #     tmp_obj = None

        ret_val.append(tmp_obj)

        index += object_size

    return ret_val

# End def

