# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Log Entry Types
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module defines each type of log entry that may exist in the event log of
an 802.11 Reference Design Node.

The log entry definitions in this file must match the corresponding
definitions in the wlan_mac_entries.h header file in the C code
running on the node.

This module maintains a dictionary which contains a reference to each
known log entry type. This dictionary is stored in the variable
``wlan_exp_log_entry_types``. The :class:`WlanExpLogEntryType` constructor
automatically adds each log entry type definition to this dictionary. Users
may access the dictionary to view currently defined log entry types. But
user code should not modify the dictionary contents directly.


Custom Log Entry Types
----------------------
The :mod:`log_entries` module includes definitions for the log entry types
implemented in the current 802.11 Reference Design C code.

Log entry types defined here must match the corresponding entry definitions in
the node C code.  Custom entries can be defined and added to the global
dictionary by user scripts.

Log entry type definitions are instances of the :class:`WlanExpLogEntryType`
class. The :class:`WlanExpLogEntryType` constructor requires two arguments:
``name`` and ``entry_type_id``.  Both the name and entry type ID **must** be
unique relative to the existing entry types defined in :mod:`log_entries`.

To define a custom log entry type::

    import wlan_exp.log.entry_types as entry_types

    #name and entry_type_id must not collide with existing log entry type definitions
    my_entry_type = entry_types.WlanExpLogEntryType(name='MY_ENTRY', entry_type_id=999)
    my_entry_type.append_field_defs([
            ('timestamp',              'Q',      'uint64'),
            ('field_A',                'H',      'uint16'),
            ('field_B',                'H',      'uint16')])

"""
import sys, os
from struct import pack, unpack, calcsize, error
import wlan_exp.util as util

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None


# Event Log Constants
#   Must match corresponding C definitions in wlan_mac_event_log.h
WLAN_EXP_LOG_DELIM = 0xACED


# Log Entry Type Constants
#   Must match corresponding C definitions in wlan_mac_entries.h
ENTRY_TYPE_NULL                   = 0
ENTRY_TYPE_NODE_INFO              = 1
ENTRY_TYPE_EXP_INFO               = 2

ENTRY_TYPE_NODE_TEMPERATURE       = 4

ENTRY_TYPE_TIME_INFO              = 6

ENTRY_TYPE_RX_OFDM                = 10
ENTRY_TYPE_RX_OFDM_LTG            = 11

ENTRY_TYPE_RX_DSSS                = 15

ENTRY_TYPE_TX_HIGH                = 20
ENTRY_TYPE_TX_HIGH_LTG            = 21

ENTRY_TYPE_TX_LOW                 = 25
ENTRY_TYPE_TX_LOW_LTG             = 26

# -----------------------------------------------------------------------------
# Log Entry Type Container
# -----------------------------------------------------------------------------

log_entry_types          = dict()

# -----------------------------------------------------------------------------
# Log Entry Type Base Class
# -----------------------------------------------------------------------------

class WlanExpLogEntryType(object):
    """Class to define a log entry type."""
    # _fields is a list of 3-tuples:
    #     (field_name, field_fmt_struct, field_fmt_np)
    _fields             = None

    entry_type_id       = None # Unique integer ID for entry type
    name                = None # Unique string name for entry type
    description         = ''   # Description of log entry type, used for generating documentation

    fields_np_dt        = None # numpy dtype object describing format
    fields_fmt_struct   = None # List of field formats, in struct module format
    _field_offsets      = None

    gen_numpy_callbacks = None

    consts              = None # Container for user-defined, entry-specific constants

    def __init__(self, name=None, entry_type_id=None):
        # Require valid name
        if name is not None:
            if(name in log_entry_types.keys()):
                print("WARNING: replacing exisitng WlanExpLogEntryType with name {0}".format(name))
            self.name = name
            log_entry_types[name] = self
        else:
            raise Exception("ERROR: new WlanExpLogEntryType instance must have valid name")

        # Entry type ID is optional
        if entry_type_id is not None:
            # entry_type_id must be int
            if type(entry_type_id) is not int:
                raise Exception("ERROR: WlanExpLogEntryType entry_type_id must be int")
            else:
                if(entry_type_id in log_entry_types.keys()):
                    print("WARNING: replacing exisitng WlanExpLogEntryType with ID {0}".format(entry_type_id))
                self.entry_type_id = entry_type_id
                log_entry_types[entry_type_id] = self

        # Initialize fields to empty lists
        self._fields             = []

        # Initialize unpack variables
        self.fields_fmt_struct   = ''

        # Initialize callbacks
        self.gen_numpy_callbacks = []

        # Initialize dictionary to contain constants specific to each entry type
        self.consts              = util.consts_dict()

        # Initialize variable that contains field names and byte offsets
        self._field_offsets      = {}


    # -------------------------------------------------------------------------
    # Accessor methods for the WlanExpLogEntryType
    # -------------------------------------------------------------------------
    def get_field_names(self):
        """Get the field names of the log entry.

        Returns:
            names (list of str):  List of string field names for the entry
        """
        return [f[0] for f in self._fields]

    def get_field_struct_formats(self):
        """Get the Python struct format of the log entry fields.

        Returns:
            formats (list of str):  List of Python struct formats for the fields in the log entry
        """
        return [f[1] for f in self._fields]

    def get_field_defs(self):          return self._fields

    def get_field_offsets(self):       return self._field_offsets

    def get_entry_type_id(self):
        """Get the ID for the entry type.

        Returns:
            ID (int):  Integer ID for the log entry
        """
        return self.entry_type_id

    def append_field_defs(self, field_info):
        """Adds fields to the definition of the log entry type.

        Args:
            field_info (list of tuple):  Each must be of the form
                ``(field_name, field_type_struct, field_type_numpy)`` where:
                
                    * ``field_name``: Name of field as string
                    * ``field_type_struct``: Field type as string, using formats specified by ``struct`` module
                    * ``field_type_numpy``: Field type as string, using formats specified by numpy ``dtype``
                    * ``field_desc``: String describing the field's meaning, used to generate wiki docs
        
        """
        if type(field_info) is list:
            self._fields.extend(field_info)
        else:
            self._fields.append(field_info)
        self._update_field_defs()


    def modify_field_def(self, name, struct_type, numpy_type, doc_str=None):
        """Modifies fields of the definition of the log entry type.

        Args:
            name (str):              Name of field to modify
            struct_type (str):       New Python struct type for the field
            numpy_type (str):        New numpy type for the field
            doc_str (str, optional): New documentation string
        """
        index = None

        for idx, f in enumerate(self._fields):
            if (f[0] == name):
                index = idx

        if index is None:
            print("WARNING:  Field {0} not found in {1}.".format(name, self.name))
            return

        if doc_str is None:
            field = (name, struct_type, numpy_type, self._fields[index][3])
        else:
            field = (name, struct_type, numpy_type, doc_str)

        self._fields[index] = field


    def add_gen_numpy_array_callback(self, callback):
        """Add callback that is run after the numpy array is generated from 
        the entry type.

        The callbacks will be executed in the order the are added to the log 
        entry
        
        Args:
            callback (function):  Function to run after the numpy array is generated
        """
        if callable(callback):
            self.gen_numpy_callbacks.append(callback)
        else:
            print("ERROR:  Callback must be callable function.")


    # -------------------------------------------------------------------------
    # Utility methods for the WlanExpLogEntryType
    # -------------------------------------------------------------------------
    def generate_numpy_array(self, log_data, byte_offsets):
        """Generate a NumPy array from the log_bytes of the given WlanExpLogEntryType instance
        at the given byte_offsets.
        """
        import numpy as np

        index_iter = [log_data[o : o + self.fields_np_dt.itemsize] for o in byte_offsets]
        np_arr = np.fromiter(index_iter, self.fields_np_dt, len(byte_offsets))

        if self.gen_numpy_callbacks:
            for callback in self.gen_numpy_callbacks:
                np_arr = callback(np_arr)

        return np_arr


    def generate_entry_doc(self, fmt='wiki'):
        """Generate wiki documentation."""
        import textwrap
        import wlan_exp.log.util as log_util

        field_descs = list()
        for f in self._fields:
            # Field tuple is (name, struct_type, np_type, (optional)desc)
            #   Construct new tuple of (name, np_type, desc ('' if not defined))
            try:
                field_descs.append( (f[0], f[2], f[3]))
            except IndexError:
                # Field missing description; use empty string
                field_descs.append( (f[0], f[2], ''))

        def doc_fields_table(field_list, fmt='wiki'):
            """field_list must be iterable of 3-tuples:
            0: Field name (string)
            1: Field data type (string, preferably the numpy datatype string)
            2: Field description (string)
            """

            doc_str = ''

            if fmt == 'wiki':
                doc_str += '{{{#!th align=center\nField Name\n}}}\n'
                doc_str += '{{{#!th align=center\nData Type\n}}}\n'
                doc_str += '{{{#!th align=center\nDescription\n}}}\n'
                doc_str += '|----------------\n'

                for fd in field_list:
                    import re
                    # Wiki-ify some string formats:
                    #   Line breaks in descriptions must be explicit "[[BR]]"
                    #   Braces with numeric contents need escape (![) to disable Trac interpretting as changeset number
                    fd_desc = fd[2]
                    fd_desc = re.sub('(\[[\d:,]+\])', '!\\1', fd_desc) # do this first, so other wiki tags inserted below aren't escaped
                    #fd_desc = fd_desc.replace('\n', '[[BR]]')

                    try:
                        consts = self.consts[fd[0]]
                        fd_desc += '\n\nConstants defined for this field:\n'
                        for c in consts.keys():
                            fd_desc += ' * {{{{{{\'{0}\': {1}}}}}}}\n'.format(c, consts[c])
                    except:
                        # Field didn't have constants defined
                        pass

                    doc_str += '{{{{{{#!td align=left\n{0}\n}}}}}}\n'.format(fd[0])
                    doc_str += '{{{{{{#!td align=center\n{0}\n}}}}}}\n'.format(fd[1])
                    doc_str += '{{{{{{#!td align=left\n{0}\n}}}}}}\n'.format(fd_desc)
                    doc_str += '|----------------\n'

            elif fmt == 'txt':
                doc_str += 'Field Name\t\t\t| Data Type\t| Description\n'
                doc_str += '---------------------------------------------------------------------------------------------------------------------\n'

                for fd in field_descs:
                    doc_str += '{0:30}\t| {1:10}\t| {2}\n'.format(fd[0], fd[1], fd[2])

                doc_str += '---------------------------------------------------------------------------------------------------------------------\n'

            return doc_str

        if fmt == 'wiki':
            # Construct the Trac-wiki-style documentation string for this entry type
            doc_str = '\n----\n\n'
            doc_str += '=== Entry Type {0} ===\n'.format(self.name)
            doc_str += self.description + '\n\n'
            doc_str += 'Entry type ID: {0}\n\n'.format(self.entry_type_id)

            doc_str += doc_fields_table(field_descs, fmt='wiki')

        elif fmt == 'txt':
            # Construct plain text version of documentation string for this entry type
            doc_str = '---------------------------------------------------------------------------------------------------------------------\n'
            doc_str += 'Entry Type {0}\n'.format(self.name)
            doc_str += 'Entry type ID: {0}\n\n'.format(self.entry_type_id)
            doc_str += textwrap.fill(self.description) + '\n\n'

            doc_str += doc_fields_table(field_descs, fmt='txt')

        # Check each post-numpy array generation callback to any documentation to include
        #   The callbacks can define "virtual" fields - convenience fields appended to the numpy
        #   array that are calculated from the "real" fields
        for cb in self.gen_numpy_callbacks:
            field_descs = []
            try:
                (cb_doc_str, cb_doc_fields) = cb(np_arr_orig=None, docs_only=True)

                for f in cb_doc_fields:
                    field_descs.append((f[0], f[1], f[2]))

                doc_str += '\n\n'
                doc_str += cb_doc_str + '\n\n'
                doc_str += doc_fields_table(field_descs, fmt=fmt)

            except (TypeError, IndexError):
                # Callback didn't implement suitable 'docs_only' output; fail quietly and return
                #   print('Error generating callback field docs for {0}\n'.format(cb))
                pass

        return doc_str


    def _entry_as_string(self, buf):
        """Generate a string representation of the entry from a buffer. 
        
        This method should only be used for debugging log data parsing and log 
        index generation, not for general creation of text log files.

        This method does not work correctly on RX_OFDM entries due to the way 
        the channel estimates are defined.  The channel_est, mac_payload_len, 
        and mac_payload will all be zeros.
        """
        entry_size = calcsize(self.fields_fmt_struct)
        entry      = self.deserialize(buf[0:entry_size])[0]

        str_out = self.name + ': '

        for k in entry.keys():
            s = entry[k]
            if((type(s) is int) or (type(s) is long)):
                str_out += "\n    {0:30s} = {1:20d} (0x{1:16x})".format(k, s)
            elif(type(s) is str):
                s = map(ord, list(entry[k]))
                str_out += "\n    {0:30s} = [".format(k)
                for x in s:
                    str_out += "{0:d}, ".format(x)
                str_out += "\b\b]"

        str_out += "\n"

        return str_out


    def _entry_as_byte_array_string(self, buf):
        """Generate a string representation of the entry from a buffer as an array of bytes. This
        method should only be used for debugging log data parsing and log index generation, not
        for general creation of text log files.
        """
        entry_size = calcsize(self.fields_fmt_struct)
        entry      = self.deserialize(buf[0:entry_size])[0]

        str_out = self.name + ': '

        line_num = 0

        for byte in range(entry_size):
            if (byte % 16) == 0:
                str_out  += "\n{0:>8d}: ".format(line_num)
                line_num += 16
            str_out += "0x{0:02x} ".format(ord(buf[byte]))

        str_out += "\n"

        print(entry)

        return str_out


    def deserialize(self, buf):
        """Unpacks one or more raw log entries of the same type into a list of dictionaries

        Args:
            buf (bytearray): Array of raw log data containing 1 or more log entries of the same type.

        Returns:
            entries (List of dict):
                Each dictionary in the list has one value per field in the log entry definition using
                the field names as keys.
        """
        from collections import OrderedDict

        ret_val    = []
        buf_size   = len(buf)
        entry_size = calcsize(self.fields_fmt_struct)
        index      = 0

        while (index < buf_size):
            try:
                dataTuple = unpack(self.fields_fmt_struct, buf[index:index+entry_size])
                all_names = self.get_field_names()
                all_fmts  = self.get_field_struct_formats()

                # Filter out names for fields ignored during unpacking
                names = [n for (n,f) in zip(all_names, all_fmts) if 'x' not in f]

                # Use OrderedDict to preserve user-specified field order
                ret_val.append(OrderedDict(zip(names, dataTuple)))

            except error as err:
                print("Error unpacking {0} buffer with len {1}: {2}".format(self.name, len(buf), err))

            index += entry_size

        return ret_val


    def serialize(self, entries):
        """Packs one or more list of dictionaries into a buffer of log entries of the same type

        Args:
            entry_list (dictionary):  Array of dictionaries for 1 or more log entries of the same type

        Returns:
            data (bytearray):  Bytearray of packed binary data
        """
        length     = 1
        ret_val    = ""
        entry_size = calcsize(self.fields_fmt_struct)

        # Convert entries to a list if it is not already one
        if type(entries) is not list:
            entries = [entries]

        # Pack each of the entries into a single data buffer
        for entry in entries:
            fields     = []
            tmp_values = []
            used_field = []

            for field in self._fields:
                if 'x' not in field[1]:
                    fields.append(field[0])
                    try:
                        tmp_values.append(entry[field[0]])
                        used_field.append(True)
                    except KeyError:
                        tmp_values.append(0)
                        used_field.append(False)

            if (False):
                print("Serialize Entry:")
                print(fields)
                print(used_field)
                print(tmp_values)

            ret_val += pack(self.fields_fmt_struct, *tmp_values)

        if (entry_size * length) != len(ret_val):
            msg  = "WARNING: Sizes do not match.\n"
            msg += "    Expected  {0} bytes".format(entry_size * length)
            msg += "    Buffer is {0} bytes".format(len(ret_val))
            print(msg)

        return ret_val


    # -------------------------------------------------------------------------
    # Internal methods for the WlanExpLogEntryType
    # -------------------------------------------------------------------------
    def _update_field_defs(self):
        """Internal method to update fields."""
        import numpy as np

        # Update the fields format used by struct unpack/calcsize
        self.fields_fmt_struct = ' '.join(self.get_field_struct_formats())

        # Update the numpy dtype definition
        # fields_np_dt is a numpy dtype, built using a dictionary of names/formats/offsets:
        #     {'names':[field_names], 'formats':[field_formats], 'offsets':[field_offsets]}
        # We specify each field's byte offset explicitly. Byte offsets for fields in _fields
        # are inferred, assuming tight C-struct-type packing (same assumption as struct.unpack)

        get_np_size = (lambda f: np.dtype(f).itemsize)

        # Compute the offset of each real field, inferred by the sizes of all previous fields
        #   This loop must look at all real fields, even ignored/padding fields
        sizes   = list(map(get_np_size, [f[2] for f in self._fields]))
        offsets = [sum(sizes[0:i]) for i in range(len(sizes))]

        np_fields = self._fields

        # numpy processing ignores the same fields ignored by struct.unpack
        # !!!BAD!!! Doing this filtering breaks HDF5 export
        # offsets = [o for (o,f) in zip(offsets_all, self._fields) if 'x' not in f[1]]
        # np_fields = [f for f in self._fields if 'x' not in f[1]]

        names   =  [f[0] for f in np_fields]
        formats =  [f[2] for f in np_fields]

        self.fields_np_dt = np.dtype({'names':names, 'formats':formats, 'offsets':offsets})

        # Update the field offsets
        self._field_offsets = dict(zip(names, offsets))

        # Check our definitions of struct vs numpy are in sync
        struct_size = calcsize(self.fields_fmt_struct)
        np_size     = self.fields_np_dt.itemsize

        if (struct_size != np_size):
            msg  = "WARNING:  Definitions of struct {0} do not match.\n".format(self.name)
            msg += "    Struct size = {0}    Numpy size = {1}".format(struct_size, np_size)
            print(msg)

    def __eq__(self, other):
        """WlanExpLogEntryType are equal if their names are equal."""
        if type(other) is str:
            return self.name == other
        else:
            return (isinstance(other, self.__class__) and (self.name == other.name))

    def __lt__(self, other):
        """WlanExpLogEntryType are less than if their names are less than."""
        if type(other) is str:
            return self.name < other
        else:
            return (isinstance(other, self.__class__) and (self.name < other.name))

    def __gt__(self, other):
        """WlanExpLogEntryType are greater than if their names are greater than."""
        if type(other) is str:
            return self.name > other
        else:
            return (isinstance(other, self.__class__) and (self.name > other.name))

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.name)

    def __repr__(self):
        return self.name

# End class


# -----------------------------------------------------------------------------
# Log Entry Type Functions
# -----------------------------------------------------------------------------

def np_array_add_txrx_ltg_fields(np_arr_orig, docs_only=False):
    """Add 'virtual' fields to TX/RX LTG packets."""
    return np_array_add_fields(np_arr_orig, mac_addr=True, ltg=True, docs_only=docs_only)

# End def


def np_array_add_txrx_fields(np_arr_orig, docs_only=False):
    """Add 'virtual' fields to TX/RX packets."""
    return np_array_add_fields(np_arr_orig, mac_addr=True, ltg=False, docs_only=docs_only)

# End def


def np_array_add_fields(np_arr_orig, mac_addr=False, ltg=False, docs_only=False):
    """Add 'virtual' fields to the numpy array.

    This is an example of a gen_numpy_array_callback
    
    Args:
        np_arr_orig (Numpy Array):  Numpy array to extend
        mac_addr (bool, optional):  Add MAC address information to the numpy array?
        ltg (bool, optional):       Add LTG information ot the numpy array?
        docs_only (bool):           Only generate documentation strings for virtual fields?
            If this is True, then only documentation strings are returned.

    Returns:
        np_array (Numpy Array):     Updated numpy array

    Extend the default np_arr with convenience fields for:
        * MAC header addresses
        * LTG packet information

    IMPORTANT: np_arr uses the original bytearray as its underlying data
    We must operate on a copy to avoid clobbering log entries adjacent to the
    Tx or Rx entries being extended.
    """
    import numpy as np

    names   = ()
    formats = ()
    descs   = ()

    # Add the MAC address fields
    if mac_addr:
        names   += ('addr1', 'addr2', 'addr3', 'mac_seq')
        formats += ('uint64', 'uint64', 'uint64', 'uint16')
        descs   += ('MAC Header Address 1', 'MAC Header Address 2', 'MAC Header Address 3', 'MAC Header Sequence Number')

    # Add the LTG fields
    if ltg:
        names   += ('ltg_uniq_seq', 'ltg_flow_id')
        formats += ('uint64', 'uint64')
        descs   += ('Unique sequence number for LTG packet', 'LTG Flow ID, calculated as:\n  16LSB: LTG instance ID\n  48MSB: Destination MAC address')

    # If there are no fields to add, just return the original array
    if not names:
        return np_arr_orig

    if docs_only:
        ret_str  = 'The following fields are populated when the log entry is part of a numpy array generated via the {{{generate_numpy_array}}} method. '
        ret_str += 'These fields are calculated from the underlying bytes in the raw log entries and are stored in more convenient formats tha the raw '
        ret_str += 'log fields. For example, these MAC address fields are 48-bit values stored in 64-bit integers. These integer addresses are much easier '
        ret_str += 'to use when filtering Tx/Rx log entries using numpy and pandas.'

        ret_list = zip(names, formats, descs)

        return (ret_str, ret_list)

    # Return if array is None to not raise an Attribute Exception
    if np_arr_orig is None:
        return np_arr_orig

    # Create a new numpy dtype with additional fields
    dt_new = extend_np_dt(np_arr_orig.dtype, {'names': names, 'formats': formats})

    # Initialize the output array (same shape, new dtype)
    np_arr_out = np.zeros(np_arr_orig.shape, dtype=dt_new)

    # Copy data from the base numpy array into the output array
    for f in np_arr_orig.dtype.names:
        # TODO: maybe don't copy fields that are ignored in the struct format?
        #   problem is non-TxRx entries would still have these fields in their numpy versions
        np_arr_out[f] = np_arr_orig[f]

    # Extract the MAC payload (the payload is at a minimum a 24-entry uint8 array)
    mac_hdrs = np_arr_orig['mac_payload']

    # Populate the MAC Address fields
    if mac_addr:
        # Helper array of powers of 2
        #   this array arranges bytes such that they match other u64 representations of MAC addresses
        #   elsewhere in the framework
        addr_conv_arr = np.uint64(2)**np.array(range(40, -1, -8), dtype='uint64')

        # Compute values for address-as-int fields using numpy's dot-product routine
        #  MAC header offsets here select the 3 6-byte address fields and 1 3-byte sequence number
        #  Each field is conditioned on the MAC payload length exceeding the bytes needed to compute the field
        #   This check handles the case of logging code being changed to record short/no payloads or
        #    for short packet Tx/Rx, such as ACKs, which do not contain all 3 address and sequence number fields

        arr_filt = np_arr_out['mac_payload_len'] >= 10
        np_arr_out['addr1'][arr_filt] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[arr_filt,  4:10]))

        arr_filt = np_arr_out['mac_payload_len'] >= 16
        np_arr_out['addr2'][arr_filt] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[arr_filt, 10:16]))

        arr_filt = np_arr_out['mac_payload_len'] >= 22
        np_arr_out['addr3'][arr_filt] = np.dot(addr_conv_arr, np.transpose(mac_hdrs[arr_filt, 16:22]))

        arr_filt = np_arr_out['mac_payload_len'] >= 24
        np_arr_out['mac_seq'][arr_filt] = np.dot(mac_hdrs[arr_filt, 22:24], [1, 256]) // 16

    # Populate the LTG fields
    if ltg:
        # Helper array of powers of 2
        #   this array arranges bytes such that they match other u64 representations of MAC addresses
        #   elsewhere in the framework
        uniq_seq_conv_arr = np.uint64(2)**np.array(range(0, 64, 8), dtype='uint64')
        flow_id_conv_arr  = np.uint64(2)**np.array(range(0, 32, 8), dtype='uint64')

        # Compute the LTG unique sequence number from the bytes in the LTG mac payload
        np_arr_out['ltg_uniq_seq'] = np.dot(uniq_seq_conv_arr, np.transpose(mac_hdrs[:, 32:40]))

        # Compute the LTG flow ID from the bytes in the LTG mac payload and the transmitting address (ie 'addr2') if present.
        #     flow_id[63:16] = Transmitting address
        #     flow_id[15: 0] = LTG ID from LTG mac payload
        try:
            np_arr_out['ltg_flow_id']  = (np_arr_out['addr2'] << 16) + (np.dot(flow_id_conv_arr, np.transpose(mac_hdrs[:, 40:44])) & 0xFFFF)
        except KeyError:
            np_arr_out['ltg_flow_id']  = np.dot(flow_id_conv_arr, np.transpose(mac_hdrs[:, 40:44]))

    return np_arr_out

# End def


def extend_np_dt(dt_orig, new_fields=None):
    """Extends a numpy dtype object with additional fields.

    Args:
        dt_orig (Numpy DataType):  Original Numpy data type
        new_fields (dict):         Dictionary with keys 'names' and 'formats', same as when specifying
            new dtype objects. The return dtype will *not* contain byte offset values for existing or
            new fields, even if exisiting fields had specified offsets. Thus the original dtype should
            be used to interpret raw data buffers before the extended dtype is used to add new fields.
    """
    import numpy as np
    from collections import OrderedDict

    if(type(dt_orig) is not np.dtype):
        raise Exception("ERROR: extend_np_dt requires valid numpy dtype as input")
    else:
        # Use ordered dictionary to preserve original field order (not required, just convenient)
        dt_ext = OrderedDict()

        # Extract the names/formats/offsets dictionary for the base dtype
        #   dt.fields returns dictionary with field names as keys and
        #   values of (dtype, offset). The dtype objects in the first tuple field
        #   can be used as values in the 'formats' entry when creating a new dtype
        # This approach will preserve the types and dimensions of scalar and non-scalar fields
        dt_ext['names'] = list(dt_orig.names)
        dt_ext['formats'] = [dt_orig.fields[f][0] for f in dt_orig.names]

        if(type(new_fields) is dict):
            # Add new fields to the extended dtype
            dt_ext['names'].extend(new_fields['names'])
            dt_ext['formats'].extend(new_fields['formats'])
        elif type(new_fields) is not None:
            raise Exception("ERROR: new_fields argument must be dictionary with keys 'names' and 'formats'")

        # Construct and return the new numpy dtype object
        dt_new = np.dtype(dt_ext)

        return dt_new

# End def


# -----------------------------------------------------------------------------
# Log Entry Type Definitions
# -----------------------------------------------------------------------------

# The code below runs when this module is imported to define each log entry type implemented
#  in the reference design. However the append_field_defs method depends on numpy, which is
#  not avaiable on the warpproject.org server, so this code will break autodoc as-is
# The server sets an environment variable BUILDING_DOCS_ON_SERVER when it runs post-svn-commit
#  Skip all code below if this variable is set
if not os.environ.get('BUILDING_DOCS_ON_SERVER', False):

    # -----------------------------------------------------------------------------
    # Common Log Entry Contants
    # -----------------------------------------------------------------------------

    # Packet Type
    #     Matches definition in wlan_mac_802_11_defs.h
    common_pkt_type = util.consts_dict({
        'BEACON'            :  0x80,
        'PROBE_RESP'        :  0x50,
        'ACK'               :  0xD4,        
        'CTS'               :  0xC4,
        'RTS'               :  0xB4,
        'DATA'              :  0x08,        
        'QOSDATA'           :  0x88,
        'NULLDATA'          :  0x48,})
        

    # -----------------------------------------------------------------------------
    # NULL Log Entry Instance
    # -----------------------------------------------------------------------------

    # The NULL entry is used to "remove" an existing entry within the log.
    #   By replacing the current entry type with the NULL entry type and zeroing
    #   out all data following the header, this can effectively remove an entry
    #   without changing the memory footprint of the log.  NULL entries will
    #   be filtered and never show up in the raw_log_index.

    entry_null = WlanExpLogEntryType(name='NULL', entry_type_id=ENTRY_TYPE_NULL)


    # -----------------------------------------------------------------------------
    # Virtual Log Entry Instances
    # -----------------------------------------------------------------------------

    ###########################################################################
    # Rx Common
    #
    entry_rx_common = WlanExpLogEntryType(name='RX_ALL', entry_type_id=None)

    entry_rx_common.description  = 'These log entries will only be created for packets that are passed to the high-level MAC code in CPU High. If '
    entry_rx_common.description += 'the low-level MAC filter drops the packet, it will not be logged. For full "monitor mode" ensure the low-level '
    entry_rx_common.description += 'MAC filter is configured to pass all receptions up to CPU High.'

    entry_rx_common.append_field_defs([
        ('timestamp',              'Q',      'uint64',  'Value of MAC Time in microseconds at PHY RX_START'),
        ('timestamp_frac',         'B',      'uint8',   'Fractional timestamp (units of 6.25ns)'),
        ('phy_samp_rate',          'B',      'uint8',   'PHY Sampling Rate Mode'),
        ('length',                 'H',      'uint16',  'Length of payload in bytes'),
        ('cfo_est',                'i',      'int32',   'Time-domain CFO estimate from Rx PHY; Fix32_31 value, CFO as fraction of sampling frequency'),
        ('mcs',                    'B',      'uint8',   'MCS index, in [0:7]'),
        ('phy_mode',               'B',      'uint8',   'PHY mode'),
        ('ant_mode',               'B',      'uint8',   'Antenna mode: [1,2,3,4] for SISO Rx on RF [A,B,C,D]'),
        ('power',                  'b',      'int8',    'Rx power in dBm'),
        ('padding0',               'x',      'uint8',   ''),
        ('pkt_type',               'B',      'uint8',   'Packet type, (first frame control byte of 802.11 header)'),
        ('channel',                'B',      'uint8',   'Channel (center frequency) index'),
        ('padding1',               'x',      'uint8',   ''),
        ('rf_gain',                'B',      'uint8',   'AGC RF gain setting: [1,2,3] for [0,15,30]dB gain'),
        ('bb_gain',                'B',      'uint8',   'AGC BB gain setting: [0:31] for approx [0:63]dB gain'),
        ('flags',                  'H',      'uint16',  '1-bit flags'),
        ('seq_num',                'H',      'uint16',  'Seq'),
        ('pad',                    '2x',     '2uint8',  'Padding')])

    entry_rx_common.consts = util.consts_dict({
        'ant_mode'      : util.consts_dict({
                'RF_A': 0x01,
                'RF_B': 0x02,
                'RF_C': 0x03,
                'RF_D': 0x04,
            }),
        'pkt_type'      : common_pkt_type,
        'phy_mode'      : util.phy_modes,
        'flags'         : util.consts_dict({
            'FCS_GOOD'                 : 0x0001,
            'DUPLICATE'                : 0x0002,
            'UNEXPECTED_RESPONSE'      : 0x0004,
            'LTG_PYLD'                 : 0x0040,
            'LTG'                      : 0x0080
        })
    })


    ###########################################################################
    # Tx CPU High Common
    #
    entry_tx_common = WlanExpLogEntryType(name='TX_ALL', entry_type_id=None)

    entry_tx_common.description  = 'Tx events in CPU High, logged for each MPDU frame created and enqueued in CPU High. See TX_LOW for log entries of '
    entry_tx_common.description += 'actual Tx events, including re-transmissions. The time values in this log entry can be used to determine time in queue '
    entry_tx_common.description += '(time_to_accept), time taken by CPU Low for all Tx attempts (time_to_done) and total time from creation to completion '
    entry_tx_common.description += '(time_to_accept+time_to_done).'

    entry_tx_common.append_field_defs([
        ('timestamp',              'Q',      'uint64',  'Value of MAC Time in microseconds when packet was created, immediately before it was enqueued'),
        ('time_to_accept',         'I',      'uint32',  'Time duration in microseconds between packet creation and packet acceptance by CPU Low'),
        ('time_to_done',           'I',      'uint32',  'Time duration in microseconds between packet acceptance by CPU Low and Tx completion in CPU Low'),
        ('uniq_seq',               'Q',      'uint64',  'Unique sequence number for Tx packet; 12 LSB of this used for 802.11 MAC header sequence number'),
        ('mcs',                    'B',      'uint8',   'MCS index in [0:7]'),
        ('phy_mode',               'B',      'uint8',   'PHY mode'),
        ('ant_mode',               'B',      'uint8',   'PHY antenna mode in [0x10, 0x20, 0x30, 0x40]'),
        ('tx_power',               'b',      'int8',    'Tx power in dBm'),
        ('num_tx',                 'B',      'uint8',   'Number of Tx attempts that were made for this packet'),
        ('padding0',               'B',      'uint8',   'Padding'),
        ('length',                 'H',      'uint16',  'Length in bytes of MPDU; includes MAC header, payload and FCS'),
        ('padding1',               'x',      'uint8',   ''),
        ('pkt_type',               'B',      'uint8',   'Packet type, (first frame control byte of 802.11 header)'),
        ('queue_id',               'H',      'uint16',  'Tx queue ID from which the packet was retrieved'),
        ('queue_occupancy',        'H',      'uint16',  'Occupancy of the Tx queue at the time the packet was created (value includes itself)'),
        ('flags',                  'H',      'uint16',  '1-bit flags')])

    entry_tx_common.consts = util.consts_dict({
        'ant_mode'      : util.consts_dict({
                'RF_A': 0x10,
                'RF_B': 0x20,
                'RF_C': 0x30,
                'RF_D': 0x40,
            }),
        'phy_mode'   : util.phy_modes,
        'pkt_type'   : common_pkt_type,
        'flags'      : util.consts_dict({
            'SUCCESSFUL'     : 0x0001,
            'LTG_PYLD'       : 0x0040,
            'LTG'            : 0x0080
        })
    })


    ###########################################################################
    # Tx CPU Low Common
    #
    entry_tx_low_common = WlanExpLogEntryType(name='TX_LOW_ALL', entry_type_id=None)

    entry_tx_low_common.description  = 'Record of actual PHY transmission. At least one TX_LOW will be logged for every TX entry. Multiple TX_LOW entries '
    entry_tx_low_common.description += 'may be created for the same TX entry if the low-level MAC re-transmitted the frame. The uniq_seq fields can be match '
    entry_tx_low_common.description += 'between TX and TX_LOW entries to find records common to the same MPUD.'

    entry_tx_low_common.append_field_defs([
        ('timestamp',              'Q',      'uint64',  'Value of MAC Time in microseconds when packet transmission actually started (PHY TX_START time)'),
        ('uniq_seq',               'Q',      'uint64',  'Unique sequence number of original MPDU'),
        ('mcs',                    'B',      'uint8',   'MCS index in [0:7]'),
        ('phy_mode',               'B',      'uint8',   'PHY mode'),
        ('ant_mode',               'B',      'uint8',   'PHY antenna mode in [0x10, 0x20, 0x30, 0x40]'),
        ('tx_power',               'b',      'int8',    'Tx power in dBm'),
        ('tx_count',               'B',      'uint8',   'Transmission index for this attempt, starting at 0 (0 = initial Tx, 1+ = subsequent attempts)'),
        ('channel',                'B',      'uint8',   'Channel (center frequency) index'),
        ('length',                 'H',      'uint16',  'Length in bytes of MPDU; includes MAC header, payload and FCS'),
        ('num_slots',              'h',      'int16',   'Number of backoff slots allotted prior to this transmission; may not have been used for initial Tx (tx_count==0); A value of -1 in this field means no backoff occured'),
        ('cw',                     'H',      'uint16',  'Contention window value at time of this Tx'),
        ('pkt_type',               'B',      'uint8',   'Packet type, (first frame control byte of 802.11 header)'),
        ('flags',                  'B',      'uint8',   '1-bit Flags'),
        ('timestamp_frac',         'B',      'uint8',   'Fractional timestamp (units of 6.25ns)'),
        ('phy_samp_rate',          'B',      'uint8',   'PHY Sampling Rate Mode')])

    entry_tx_low_common.consts = util.consts_dict({
        'ant_mode'      : util.consts_dict({
                'RF_A': 0x10,
                'RF_B': 0x20,
                'RF_C': 0x30,
                'RF_D': 0x40,
            }),
        'phy_mode'      : util.phy_modes,
        'pkt_type'      : common_pkt_type,
        'flags'         : util.consts_dict({
            'RECEIVED_RESPONSE'          : 0x01,
            'LTG'                        : 0x40,
            'LTG_PYLD'                   : 0x80
        })
    })


    # -----------------------------------------------------------------------------
    # Log Entry Type Instances
    # -----------------------------------------------------------------------------

    ###########################################################################
    # Node Info
    #
    entry_node_info = WlanExpLogEntryType(name='NODE_INFO', entry_type_id=ENTRY_TYPE_NODE_INFO)

    entry_node_info.description = 'Details about the node hardware and its configuration. Node info values are static after boot.'

    entry_node_info.append_field_defs([
        ('timestamp',                  'Q',  'uint64',  'Value of MAC Time in microseconds when log entry created'),
        ('node_type',                  'I',  'uint32',  'Code identifying applications in CPU High and CPU Low'),
        ('node_id',                    'I',  'uint32',  'Node ID, as set during wlan_exp init'),
        ('hw_generation',              'I',  'uint32',  'WARP hardware generation; always 3 for WARP v3'),
        ('serial_num',                 'I',  'uint32',  'Serial number of WARP board'),
        ('fpga_dna',                   'Q',  'uint64',  'DNA value of node FPGA'),
        ('version',                    'I',  'uint32',  'wlan_exp version, as packed values [(u8)major (u8)minor (u16)rev]'),
        ('wlan_scheduler_resolution',  'I',  'uint32',  'Minimum interval in microseconds of the WLAN scheduler'),
        ('wlan_mac_addr',              'Q',  'uint64',  'Node MAC address, 6 bytes in lower 48-bits of u64'),
        ('wlan_max_tx_power_dbm',      'i',  'int32',   'Maximum transmit power'),
        ('wlan_min_tx_power_dbm',      'i',  'int32',   'Minimum transmit power')])

    entry_node_info.consts = util.consts_dict({
        'node_type': util.consts_dict({
            'AP_DCF':     0x00010101,
            'STA_DCF':    0x00010201,
            'IBSS_DCF':   0x00010301,
            'AP_NOMAC':   0x00010102,
            'STA_NOMAC':  0x00010202,
            'IBSS_NOMAC': 0x00010302,
            })
        })

    ###########################################################################
    # Experiment Info header - actual exp_info contains a "message" field that
    #   follows this header. Since the message is variable length it is not described
    #   in the fields list below. Full exp_info entries (header + message) must be extracted
    #   directly by a user script.
    #
    entry_exp_info_hdr = WlanExpLogEntryType(name='EXP_INFO', entry_type_id=ENTRY_TYPE_EXP_INFO)

    entry_exp_info_hdr.description = 'Header for generic experiment info entries created by the user application. '
    entry_exp_info_hdr.description += 'The payload of the EXP_INFO entry is not described by the Python entry type. User '
    entry_exp_info_hdr.description += 'code must access the payload in the binary log data directly.'

    entry_exp_info_hdr.append_field_defs([
        ('timestamp',              'Q',      'uint64',  'Value of MAC Time in microseconds when log entry created'),
        ('info_type',              'H',      'uint16',  'Exp info type - arbitrary value supplied by application'),
        ('info_len',               'H',      'uint16',  'Exp info length - length (in byte) of info_payload'),
        ('info_payload',           'I',      'uint32',  'Exp info payload')])


    ###########################################################################
    # Time Info
    #
    entry_time_info = WlanExpLogEntryType(name='TIME_INFO', entry_type_id=ENTRY_TYPE_TIME_INFO)

    entry_time_info.description  = 'Record of a time base event at the node. This log entry is used to enable parsing of log data '
    entry_time_info.description += 'recorded before and after changes to the node\'s microsecond MAC timer. This entry also allows a wlan_exp controler to '
    entry_time_info.description += 'write the current host time to the node log without affecting the node\'s MAC timer value. This enables adjustment '
    entry_time_info.description += 'of log entry timestamps to real timestamps in post-proessing.'

    entry_time_info.append_field_defs([
        ('timestamp',              'Q',      'uint64', 'Value of MAC Time in microseconds when log entry created (before any time change is applied)'),
        ('time_id',                'I',      'uint32', 'Random ID value included in wlan_exp TIME_INFO command; used to find common entries across nodes'),
        ('reason',                 'I',      'uint32', 'Reason code for TIME_INFO log entry creation'),
        ('mac_timestamp',          'Q',      'uint64', 'New value of MAC Time in microseconds '),
        ('system_timestamp',       'Q',      'uint64', 'Value of System Time in microseconds'),
        ('host_timestamp',         'Q',      'uint64', 'Host time in microseconds-since-epoch; 0xFFFFFFFFFFFFFFFF if unknown')])

    entry_time_info.consts = util.consts_dict({
        'reason'        : util.consts_dict({
            'SYSTEM'                   : 0x00000000,
            'WLAN_EXP_SET_TIME'        : 0x00000001,
            'WLAN_EXP_ADD_LOG'         : 0x00000002
        })
    })


    ###########################################################################
    # Temperature
    #
    entry_node_temperature = WlanExpLogEntryType(name='NODE_TEMPERATURE', entry_type_id=ENTRY_TYPE_NODE_TEMPERATURE)

    entry_node_temperature.description  = 'Record of the FPGA system monitor die temperature. This entry is only created when directed by a wlan_exp '
    entry_node_temperature.description += 'command. Temperature values are stored as 32-bit unsigned integers. To convert to degrees Celcius, apply '
    entry_node_temperature.description += '(((float)temp_u32)/(65536.0*0.00198421639)) - 273.15'

    entry_node_temperature.append_field_defs([
        ('timestamp',              'Q',      'uint64', 'Value of MAC Time in microseconds when log entry created'),
        ('node_id',                'I',      'uint32', 'wlan_exp node ID'),
        ('serial_num',             'I',      'uint32', 'Node serial number'),
        ('temp_current',           'I',      'uint32', 'Current FPGA die temperature (deg C)'),
        ('temp_min',               'I',      'uint32', 'Minimum FPGA die temperature (deg C) since FPGA configuration or sysmon reset'),
        ('temp_max',               'I',      'uint32', 'Maximum FPGA die temperature (deg C) since FPGA configuration or sysmon reset')])


    ###########################################################################
    # Receive OFDM
    #
    entry_rx_ofdm = WlanExpLogEntryType(name='RX_OFDM', entry_type_id=ENTRY_TYPE_RX_OFDM)

    entry_rx_ofdm.description  = 'Rx events from OFDM PHY. ' + entry_rx_common.description

    entry_rx_ofdm.append_field_defs(entry_rx_common.get_field_defs())
    entry_rx_ofdm.append_field_defs([
        ('chan_est',               '256B',   '(64,2)i2',    'OFDM Rx channel estimates, packed as [(uint16)I (uint16)Q] values, one per subcarrier'),
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])

    entry_rx_ofdm.add_gen_numpy_array_callback(np_array_add_txrx_fields)

    entry_rx_ofdm.consts = entry_rx_common.consts.copy()


    ###########################################################################
    # Receive OFDM LTG packet
    #
    entry_rx_ofdm_ltg = WlanExpLogEntryType(name='RX_OFDM_LTG', entry_type_id=ENTRY_TYPE_RX_OFDM_LTG)

    entry_rx_ofdm_ltg.description  = 'LTG ' + entry_rx_ofdm.description

    entry_rx_ofdm_ltg.append_field_defs(entry_rx_common.get_field_defs())
    entry_rx_ofdm_ltg.append_field_defs([
        ('chan_est',               '256B',   '(64,2)i2',    'OFDM Rx channel estimates, packed as [(uint16)I (uint16)Q] values, one per subcarrier'),
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '44s',    '44uint8',     'First 44 bytes of MAC payload: the 802.11 MAC header, LLC header, Packet ID, LTG ID')])

    entry_rx_ofdm_ltg.add_gen_numpy_array_callback(np_array_add_txrx_ltg_fields)

    entry_rx_ofdm_ltg.consts = entry_rx_common.consts.copy()


    ###########################################################################
    # Receive DSSS
    #
    entry_rx_dsss = WlanExpLogEntryType(name='RX_DSSS', entry_type_id=ENTRY_TYPE_RX_DSSS)

    entry_rx_dsss.description  = 'Rx events from DSSS PHY. ' + entry_rx_common.description

    entry_rx_dsss.append_field_defs(entry_rx_common.get_field_defs())
    entry_rx_dsss.append_field_defs([
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])

    entry_rx_dsss.add_gen_numpy_array_callback(np_array_add_txrx_fields)

    entry_rx_dsss.consts = entry_rx_common.consts.copy()


    ###########################################################################
    # Transmit from CPU High
    #
    entry_tx_high = WlanExpLogEntryType(name='TX_HIGH', entry_type_id=ENTRY_TYPE_TX_HIGH)

    entry_tx_high.description  = entry_tx_common.description

    entry_tx_high.append_field_defs(entry_tx_common.get_field_defs())
    entry_tx_high.append_field_defs([
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])

    entry_tx_high.add_gen_numpy_array_callback(np_array_add_txrx_fields)

    entry_tx_high.consts = entry_tx_common.consts.copy()


    ###########################################################################
    # Transmit from CPU High LTG packet
    #
    entry_tx_high_ltg = WlanExpLogEntryType(name='TX_HIGH_LTG', entry_type_id=ENTRY_TYPE_TX_HIGH_LTG)

    entry_tx_high_ltg.description  = entry_tx_common.description

    entry_tx_high_ltg.append_field_defs(entry_tx_common.get_field_defs())
    entry_tx_high_ltg.append_field_defs([
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '44s',    '44uint8',     'First 44 bytes of MAC payload: the 802.11 MAC header, LLC header, Packet ID, LTG ID')])

    entry_tx_high_ltg.add_gen_numpy_array_callback(np_array_add_txrx_ltg_fields)

    entry_tx_high_ltg.consts = entry_tx_common.consts.copy()


    ###########################################################################
    # Transmit from CPU Low
    #
    entry_tx_low = WlanExpLogEntryType(name='TX_LOW', entry_type_id=ENTRY_TYPE_TX_LOW)

    entry_tx_low.description  = entry_tx_low_common.description

    entry_tx_low.append_field_defs(entry_tx_low_common.get_field_defs())
    entry_tx_low.append_field_defs([
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '24s',    '24uint8',     'First 24 bytes of MAC payload, typically the 802.11 MAC header')])

    entry_tx_low.add_gen_numpy_array_callback(np_array_add_txrx_fields)

    entry_tx_low.consts = entry_tx_low_common.consts.copy()


    ###########################################################################
    # Transmit from CPU Low LTG packet
    #
    entry_tx_low_ltg = WlanExpLogEntryType(name='TX_LOW_LTG', entry_type_id=ENTRY_TYPE_TX_LOW_LTG)

    entry_tx_low_ltg.description  = entry_tx_low_common.description

    entry_tx_low_ltg.append_field_defs(entry_tx_low_common.get_field_defs())
    entry_tx_low_ltg.append_field_defs([
        ('mac_payload_len',        'I',      'uint32',      'Length in bytes of MAC payload recorded in log for this packet'),
        ('mac_payload',            '44s',    '44uint8',     'First 44 bytes of MAC payload: the 802.11 MAC header, LLC header, Packet ID, LTG ID')])

    entry_tx_low_ltg.add_gen_numpy_array_callback(np_array_add_txrx_ltg_fields)

    entry_tx_low_ltg.consts = entry_tx_low_common.consts.copy()

