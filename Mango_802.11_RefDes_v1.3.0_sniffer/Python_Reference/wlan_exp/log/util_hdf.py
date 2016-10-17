# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Log HDF5 Utilities
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
.. 1.00a ejw  1/23/14  Initial release
.. ------------------------------------------------------------------------------
.. 
.. This module provides utility functions for HDF to handle WLAN Exp log data.
.. 
.. For WLAN Exp log data manipulation, it is necessary to define a common file format
.. so that it is easy for multiple consumers, both in python and other languages, to
.. access the data.  To do this, we use HDF5 as the container format with a couple of
.. additional conventions to hold the log data as well as other pieces of information.
.. Below are the rules that we follow to create an HDF5 file that will contain WLAN
.. Exp log data:
.. 
.. wlan_exp_log_data_container (equivalent to a HDF5 group):
..    /: Root Group in HDF5 file
..        |- Attributes:
..        |      |- 'wlan_exp_log'         (1,)      bool
..        |      |- 'wlan_exp_ver'         (3,)      uint32
..        |      |- <user provided attributes in attr_dict>
..        |- Datasets:
..        |      |- 'log_data'             (1,)      voidN  (where N is the size of the data)
..        |- Groups (created if gen_index==True):
..               |- 'raw_log_index'
..                      |- Datasets:
..                         (dtype depends if largest offset in raw_log_index is < 2^32)
..                             |- <int>    (N1,)     uint32/uint64
..                             |- <int>    (N2,)     uint32/uint64
..                             |- ...
.. 
.. Naming convention:
.. 
..   log_data       -- The binary data from a WLAN Exp node's log.
.. 
..   raw_log_index  -- This is an index that has not been interpreted / filtered
..                     and corresponds 1-to-1 with what is in given log_data.
..                     The defining characteristic of a raw_log_index is that
..                     the dictionary keys are all integers (entry type IDs):
..                       { <int> : [<offsets>] }
.. 
..   log_index      -- A log_index is any index that is not a raw_log_index.  In
..                     general, this will be a interpreted / filtered version of
..                     a raw_log_index.
.. 
..   hdf5           -- A data container format that we use to store log_data,
..                     raw_log_index, and other user defined attributes.  You can
..                     find more documentation on HDF / HDF5 at:
..                         http://www.hdfgroup.org/
..                         http://www.h5py.org/
.. 
..   numpy          -- A python package that allows easy and fast manipulation of
..                     large data sets.  You can find more documentaiton on numpy at:
..                         http://www.numpy.org/
"""

__all__ = ['np_arrays_to_hdf5',
           'HDF5LogContainer',
           'hdf5_open_file',
           'hdf5_close_file',
           'log_data_to_hdf5',
           'hdf5_to_log_data',
           'hdf5_to_log_index',
           'hdf5_to_attr_dict']

import sys
from . import util as log_util


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": unicode=str

# -----------------------------------------------------------------------------
# HDF5 Log Container Class
# -----------------------------------------------------------------------------
class HDF5LogContainer(log_util.LogContainer):
    """Class to define an HDF5 log container.

    Args:
        file_handle (h5py.File()):     Handle of the HDF5 file
        name (str, optional):          Name of the HDF5 group of the log container
        compression (bool, optional):  HDF5 compression setting on the log container

    .. note:: When an HDF5LogContainer is created, the underlying HDF5 file will
        not be modified unless one of the write_* methods are called.
    """
    hdf5_group_name          = None
    compression              = None


    def __init__(self, file_handle, name=None, compression=None):
        super(HDF5LogContainer, self).__init__(file_handle)

        self.compression = compression

        if name is None:
            self.hdf5_group_name = "/"
        else:
            self.hdf5_group_name = name


    def is_valid(self):
        """Check that the HDF5 Log Container is valid.

        Returns:
            is_valid (bool):
                * True --> This is a valid HDF5 log file
                * False --> This is NOT a valid HDF5 log file
        """
        import numpy as np
        import wlan_exp.version as version

        # Check the group handle but do not create one
        group_handle = self._get_group_handle()

        if group_handle is None:
            msg  = "WARNING: Log container is not valid.\n"
            msg += "    Could not find {0} in file.".format(self.hdf5_group_name)
            print(msg)
            return False

        try:
            if group_handle.attrs['wlan_exp_log']:
                # Require two attributes named 'wlan_exp_log' and 'wlan_exp_ver'
                ver = group_handle.attrs['wlan_exp_ver']

                ver_str            = version.wlan_exp_ver_str(ver[0], ver[1], ver[2])
                ver_older_than_093 = (ver[0], ver[1], ver[2]) < (0, 9, 3)
                caller_desc = "HDF5 file '{0}' was written using version {1}".format(self.file_handle.filename, ver_str)

                status = version.wlan_exp_ver_check(major=ver[0], minor=ver[1], revision=ver[2],
                                                    caller_desc=caller_desc)
                

                if (status == version.WLAN_EXP_VERSION_NEWER and 
                        (version.wlan_exp_ver() >= (0, 9, 3)) and ver_older_than_093):
                    msg  = "The HDF5 file uses a version older than 0.93, please convert using \n"
                    msg += "the log_util_hdf5convert.py utility found the example directory in \n"
                    msg += "releases prior to 1.0."
                    print(msg)
                
                if (status == version.WLAN_EXP_VERSION_OLDER):
                    print("Please update the WLAN Exp installation to match the version on the HDF5 file.")
                            
            else:
                msg  = "WARNING: Log container is not valid.\n"
                msg += "    'wlan_exp_log' attribute indicates log container is not valid."
                print(msg)
                return False

            if group_handle['log_data']:
                # Require a dataset named 'log_data'
                if(group_handle['log_data'].dtype.kind != np.dtype(np.void).kind):
                    # Require the 'log_data' dataset to be HDF5 opaque type (numpy void type)
                    msg  = "WARNING: Log container is not valid.\n"
                    msg += "    Log Data is not valid type.  Must be an HDF5 opaque type."
                    print(msg)
                    return False
        except Exception as err:
            msg  = "WARNING: Log container is not valid.  The following error occurred:\n"
            msg += "    {0}".format(err)
            print(msg)
            return False

        return True


    def write_log_data(self, log_data, append=True):
        """Write the log data to the log container.

        Args:
            log_data (bytes):         Binary data from a WlanExpNode log
            append (bool, optional):  Append to (True) or Overwrite (False) the current log data
        """
        import numpy as np

        if not self._file_writeable():
            raise AttributeError("File {0} is not writeable.".format(self.file_handle))

        group_handle    = self._get_valid_group_handle()

        np_dt           = np.dtype('V1')
        log_data_length = len(log_data)

        # Raise an exception if the log data length is zero
        if (log_data_length == 0):
            raise AttributeError("Did not provide any log data.")

        # Get the log_data from the group data set
        ds = group_handle['log_data']

        # Set length of current data
        if append:
            curr_length = ds.shape[0]
        else:
            curr_length = 0

        # Get total length of data
        length = curr_length + log_data_length

        # Create empyt numpy container
        np_data = np.empty((log_data_length,), np_dt)

        # Redirect numpy array data pointer to the existing buffer object passed in by user
        np_data.data = log_data

        ds.resize((length,))
        ds[curr_length:length,] = np_data


    def write_log_index(self, log_index=None):
        """Write the log index to the log container.

        Args:
            log_index (dict):  Log index generated from WLAN Exp log data

        If the log index currently exists in the HDF5 file, that log index
        will be replaced with this new log index.  If log_index is provided
        then that log index will be written to the log container.  Otherwise,
        a raw log index will be generated and added to the log container.
        """
        import numpy as np

        if not self._file_writeable():
            raise AttributeError("File {0} is not writeable.".format(self.file_handle))

        index_name   = "log_index"
        group_handle = self._get_valid_group_handle()

        if log_index is None:
            log_index = self._create_raw_log_index()

            if log_index is None:
                raise AttributeError("Unable to create raw log index for group: {0}\n".format(group_handle))

        # Delete any existing 'log_index' in the group
        try:
            # Normally the try-catch would handle this error but in HDF5 1.8.9
            # exceptions are not properly thrown when using h5py, so the check
            # needs to be coded this way so that we don't get a lot of garbage
            # output.
            #
            for group in group_handle.keys():
                if (group == index_name):
                    del group_handle[index_name]
        except KeyError:
            pass

        # Write the log index to the group
        try:
            index_grp = group_handle.create_group(index_name)

            for k, v in log_index.items():
                # Check if highest-valued entry index can be represented as uint32 or requires uint64
                if (v[-1] < 2**32):
                    dtype = np.uint32
                else:
                    dtype = np.uint64

                # Group names must be strings - keys here are known to be integers (entry_type_id values)
                index_grp.create_dataset(str(k), data=np.array(v, dtype=dtype), maxshape=(None,), compression=self.compression)
        except Exception as err:
            print("ERROR:\n    {0}\n".format(err))
            raise AttributeError("Unable to add log_index to log container: {0}\n".format(group_handle))


    def write_attr_dict(self, attr_dict):
        """Add the given attribute dictionary to the opened log container.

        Args:
            attr_dict (dict):  A dictionary of user provided attributes that will be added to the HDF5 group.
        """
        import numpy as np

        if not self._file_writeable():
            raise AttributeError("File {0} is not writeable.".format(self.file_handle))

        default_attrs = ['wlan_exp_log', 'wlan_exp_ver']
        group_handle  = self._get_valid_group_handle()

        # Remove all current attributes, except default attributes
        for k in group_handle.attrs.keys():
            if k not in default_attrs:
                del group_handle.attrs[k]

        # Write the attribute dictionary to the group
        for k, v in attr_dict.items():
            try:
                if k not in default_attrs:
                    if (type(k) is str):
                        if ((type(v) is str) or (type(v) is unicode)):
                            group_handle.attrs[k] = np.string_(v)
                        else:
                            group_handle.attrs[k] = v
                    else:
                        print("WARNING: Converting '{0}' to string to add attribute.".format(k))
                        group_handle.attrs[str(k)] = v
            except KeyError:
                print("WARNING: Could not add attribute '{0}' to group {1}".format(k, group_handle))


    def get_log_data_size(self):
        """Get the current size of the log data in the log container.
        
        Returns:
            size (int):  Number of bytes of log data in the log container
        """

        group_handle = self._get_valid_group_handle()

        # Get the log_data from the group data set
        ds = group_handle['log_data']

        # Return the length of the data
        return ds.shape[0]


    def get_log_data(self):
        """Get the log data from the log container.
        
        Returns:
            log_data (bytes):  Bytes object of the log data in the container
        """
        import numpy as np

        group_handle = self._get_valid_group_handle()

        # Get the log_data from the group data set
        ds           = group_handle['log_data']
        log_data_np  = np.empty(shape=ds.shape, dtype=ds.dtype)

        # Use the h5py library's HDF5 -> numpy hooks to preserve the log_data size and void type
        ds.read_direct(log_data_np)

        # Point to the numpy array's underlying buffer to find the raw log_data to return
        log_data = bytes(log_data_np.data)

        return log_data


    def get_log_index(self, gen_index=True):
        """Get the raw log index from the log container.

        Args:
            gen_index (bool, optional):  Generate the raw log index if the log index does not
                exist in the log container.
        
        Returns:
            log_index (dict):  Log index from the log container
        """
        error        = False
        log_index    = {}
        group_handle = self._get_valid_group_handle()

        # Get the raw_log_index group from the specified group
        try:
            index_group   = group_handle["log_index"]

            for k, v in index_group.items():
                # Re-construct the raw_log_index dictionary, using integers
                #   (really entry_type IDs) as the keys and Python lists as values
                #   the [:] slice here is important - flattening the returned numpy array before
                #   listifying is *way* faster (>10x) than just v.toList()

                try:
                    log_index[int(k)] = v[:].tolist()
                except ValueError:
                    log_index[k]      = v[:].tolist()

                # Alternative to [:].toList() above - adds safetly in assuring dictionary value is
                #   Python list of ints, an requirement of downstream methods
                #   raw_log_index[int(k)] = map(int, v[:]) #fastish
        except KeyError:
            error = True

        # If there was an error getting the raw_log_index from the file and
        #   gen_index=True, then generate the raw_log_index from the log_data
        #   in the file
        if error and gen_index:
            log_index = self._create_raw_log_index()

        # If the log index is empty or None, then raise an exception
        if not log_index:
            msg  = "Unable to get log index from "
            msg += "group {0} of {1}.".format(self.hdf5_group_name, self.file_handle)
            raise AttributeError(msg)

        return log_index


    def get_attr_dict(self):
        """Get the attribute dictionary from the log container.
        
        Returns:
            attr_dict (dict):  The dictionary of user provided attributes in the log container.
        """
        import numpy as np

        attr_dict    = {}
        group_handle = self._get_valid_group_handle()

        for k, v in group_handle.attrs.items():
            try:
                if (type(v) == np.bytes_):
                    attr_dict[k] = str(v)
                else:
                    attr_dict[k] = v
            except KeyError:
                print("WARNING: Could not retreive attribute '{0}' from group {1}".format(k, group_handle))

        return attr_dict


    def trim_log_data(self):
        """Trim the log data so that it has ends on a entry boundary."""
        raise NotImplementedError


    # -------------------------------------------------------------------------
    # Internal methods for the container
    # -------------------------------------------------------------------------
    def _get_valid_group_handle(self):
        """Internal method to get a valid handle to the HDF5 group or raise an exception."""
        group_handle = self._get_group_handle()

        # Create container if group is empty
        if not group_handle.attrs.keys():
            self._create_container(group_handle)

        # Raise exception if group is not valid
        if not self.is_valid():
            raise AttributeError("Log container not valid: {0}\n".format(group_handle))

        return group_handle


    def _get_group_handle(self):
        """Internal method to get a handle to the HDF5 group."""
        group_name   = self.hdf5_group_name
        file_handle  = self.file_handle

        # Check if we are using the root group
        if (group_name == "/"):
            # Use the root group
            return file_handle

        # Check group exists in the file
        try:
            return file_handle[group_name]
        except KeyError:
            # Try to create the group
            try:
                return file_handle.create_group(group_name)
            except ValueError:
                msg  = "Cannot create group {0} ".format(self.hdf5_group_name)
                msg += "in {0}".format(self.file_handle)
                raise AttributeError(msg)

        # Could not get the group handle, return None
        return None


    def _create_container(self, group):
        """Internal method to create a valid log data container."""
        import numpy as np
        import wlan_exp.version as version

        # Add default attributes to the group
        group.attrs['wlan_exp_log'] = np.array([1], dtype=np.uint8)
        group.attrs['wlan_exp_ver'] = np.array(version.wlan_exp_ver(), dtype=np.uint32)

        # Create an empty numpy array of type 'V1' (ie one byte void)
        np_dt   = np.dtype('V1')
        np_data = np.empty((0,), np_dt)

        # Create an empty re-sizeable data set for the numpy-formatted data
        group.create_dataset("log_data", data=np_data, maxshape=(None,), compression=self.compression)


    def _create_raw_log_index(self):
        """Internal method to create a raw log index pulling data from the HDF5 file."""
        try:
            log_data       = self.get_log_data()
            raw_log_index  = log_util.gen_raw_log_index(log_data)
        except AttributeError:
            raw_log_index = None

        return raw_log_index


    def _file_writeable(self):
        """Internal method to check if the HDF5 file is writeable."""
        if (self.file_handle.mode == 'r'):
            return False
        else:
            return True

# End class()




# -----------------------------------------------------------------------------
# WLAN Exp Log HDF5 file Utilities
# -----------------------------------------------------------------------------
def hdf5_open_file(filename, readonly=False, append=False, print_warnings=True):
    """Open an HDF5 file.

    Args:
        filename (str):                   Filename of the HDF5 file to open
        readonly (bool, optional):        Open the file in read-only mode
        append (bool, optional):          Append to the data in the current file
        print_warnings (bool, optional):  Print warning messages

    Returns:
        file_handle (h5py.File()):  Handle for the HDF5 file


    Behavior of input attributes:
        +------------+------------+----------------------------------------------------------+ 
        | readonly   | append     | Behavior                                                 | 
        +============+============+==========================================================+ 
        | True       | T/F        | File opened in read-only mode                            | 
        +------------+------------+----------------------------------------------------------+ 
        | False      | True       | File opened in append mode; created if it does not exist | 
        +------------+------------+----------------------------------------------------------+ 
        | False      | False      | If file with filename exists, then a new filename is     |
        |            |            | generated using the log utilities.  The new file is then |
        |            |            | created by the h5py File method (DEFAULT)                |
        +------------+------------+----------------------------------------------------------+
    """

    import os
    import h5py

    file_handle = None

    # Get a file handle the log container file
    if readonly:
        # Open a HDF5 File Object in 'r' (Readonly) mode
        file_handle = h5py.File(filename, mode='r')
    else:
        # Determine a safe filename for the output HDF5 file
        if append:
            if os.path.isfile(filename):
                if print_warnings:
                    print("WARNING: Opening existing file {0} in append mode".format(filename))

            h5_filename = filename
        else:
            h5_filename = log_util._get_safe_filename(filename, print_warnings)

        if os.path.isfile(h5_filename):
            # Open an HDF5 File Object in 'a' (Read/Write if exists, create otherwise) mode
            file_handle = h5py.File(h5_filename, mode='a')
        else:
            # Open an HDF5 File Object in 'w' (Create file, truncate if exists) mode
            #
            # NOTE:  This is due to a bug in Anaconda where it does not throu the appropriate
            #    IOError to be caught to create a file with the 'a' mode
            file_handle = h5py.File(h5_filename, mode='w')

    return file_handle

# End def



def hdf5_close_file(file_handle):
    """Close an HDF5 file.
 
    Args:
        file_handle (h5py.File()):  Handle for the HDF5 file    
    """
    file_handle.close()

# End def



def log_data_to_hdf5(log_data, filename, attr_dict=None, gen_index=True, overwrite=False):
    """Create an HDF5 file that contains the log_data, a raw_log_index, and any
    user attributes.

    Args:
        log_data (bytes):            Binary data from a WlanExpNode log
        filename (str):              Filename of the HDF5 file to appear on disk
        attr_dict (dict, optional):  A dictionary of user provided attributes that will be added to the HDF5 group.
        gen_index (bool, optional):  Generate the 'raw_log_index' from the log_data and store it in the file.
        overwrite (bool, optional):  If True method will overwrite existing file with filename

    If the requested filename already exists and ``overwrite==True`` this
    method will replace the existing file, destroying any data in the original file.

    If the filename already exists and ``overwrite==False`` this method will print a warning,
    then create a new filename with a unique date-time suffix.
    """
    # Need to not print warnings if overwrite is True
    print_warnings = not overwrite

    # Open the file
    file_handle   = hdf5_open_file(filename, print_warnings=print_warnings)

    # Actual filename (See HDF5 File docs)
    real_filename = file_handle.filename

    # Create an HDF5 Log Container
    container     = HDF5LogContainer(file_handle)

    # Try to write the file components
    try:
        # Add the log data
        container.write_log_data(log_data)

        # Add the raw log index to the group
        #   NOTE:  Done this way to save processing time.  Since log_data is already
        #          in memory, we do not need to use the default write_log_index which
        #          pulls the log data out of the HDF5 file to create the raw log index.
        if gen_index:
            raw_log_index = log_util.gen_raw_log_index(log_data)
            container.write_log_index(raw_log_index)

        # Add the attribute dictionary to the group
        if attr_dict is not None:
            container.write_attr_dict(attr_dict)

    except AttributeError as err:
        print("Error writing log file: {0}".format(err))

    # Close the file
    hdf5_close_file(file_handle)

    # If overwrite use the os to move the temp file to a new file
    if overwrite and (real_filename != filename):
        import os
        os.remove(filename)
        os.rename(real_filename, filename)

# End log_data_to_hdf5()



def hdf5_to_log_data(filename, group_name=None):
    """Extract the log_data from an HDF5 Log Container

    Args:
        filename (str):              Name of HDF5 file to open
        group_name (str, optional):  Name of Group within the HDF5 file object 
            (defaults to "\")

    Returns:
        log_data (bytes):  Log data in the HDF5 file
    """
    log_data    = None

    # Open the file
    file_handle = hdf5_open_file(filename, readonly=True)

    # Create an HDF5 Log Container
    container   = HDF5LogContainer(file_handle, group_name)

    # Try to read the file components
    try:
        # Extract the attribute dictionary
        log_data    = container.get_log_data()

    except AttributeError as err:
        print("Error reading log file: {0}".format(err))

    # Close the file
    hdf5_close_file(file_handle)

    return log_data

# End log_data_to_hdf5()



def hdf5_to_log_index(filename, group_name=None, gen_index=True):
    """Extract the log_index from an HDF5 Log Container

    Args:
        filename (str):              Name of HDF5 file to open
        group_name (str, optional):  Name of Group within the HDF5 file object 
            (defaults to "\")
        gen_index (bool, optional):  Generate the 'raw_log_index' from the log_data and 
            store it in the file if the 'log_index' is not in the file.

    Returns:
        log_index (dict):  Either the 'log_index' from HDF5 file or a generated 
            raw_log_index from log_data in HDF5 file
    """
    log_index   = None

    # Open the file
    file_handle = hdf5_open_file(filename, readonly=True)

    # Create an HDF5 Log Container
    container   = HDF5LogContainer(file_handle, group_name)

    # Try to read the file components
    try:
        # Extract the log index
        log_index   = container.get_log_index(gen_index)

    except AttributeError as err:
        print("Error reading log file: {0}".format(err))

    # Close the file
    hdf5_close_file(file_handle)

    return log_index

# End hdf5_to_log_index()



def hdf5_to_attr_dict(filename=None, group_name=None):
    """Extract the attribute dictionary from an HDF5 Log Container.

    Args:
        filename (str):              Name of HDF5 file to open
        group_name (str, optional):  Name of Group within the HDF5 file object 
            (defaults to "\")

    Returns:
        attr_dict (dict):  The dictionary of user provided attributes in the HDF5 file
    """
    attr_dict   = None

    # Open the file
    file_handle = hdf5_open_file(filename, readonly=True)

    # Create an HDF5 Log Container
    container   = HDF5LogContainer(file_handle, group_name)

    # Try to read the file components
    try:
        # Extract the attribute dictionary
        attr_dict   = container.get_attr_dict()

    except AttributeError as err:
        print("Error reading log file: {0}".format(err))

    # Close the file
    hdf5_close_file(file_handle)

    return attr_dict

# End hdf5_to_attr_dict()




def np_arrays_to_hdf5(filename, np_log_dict, attr_dict=None, compression=None):
    """Generate an HDF5 file from numpy arrays. 
    
    Args:
        filename (str):                Name of HDF5 file to open
        np_log_dict (Numpy Array):     Numpy array to add to the HDF5 file    
        attr_dict (dict, optional):    A dictionary of user provided attributes that will be added to the HDF5 file.
        compression (bool, optional):  HDF5 compression setting on the log container
    
    The np_log_dict input must be either:
        #. A dictionary with numpy record arrays as values; each array will be a dataset in 
           the HDF5 file root group
        #. A dictionary of dictionaries like (1); each top-level value will be a group in the 
           root HDF5 group, each numpy array will be a dataset in the group.
    
    **attr_dict** is optional. If provied, values in attr_dict will be copied to HDF5
    group and dataset attributes. attr_dict values with keys matching np_log_dict keys
    will be used as dataset attributes named ``'<the_key>_INFO'``.  Attribute dictionary
    entries may have an extra value with key ``'/'``, which will be used as the value for a 
    group attribute named ``'INFO'``.
    
    **Examples:**
    ::
        # No groups - all datasets in root group
        np_log_dict = {
            'RX_OFDM':  np_array_of_rx_etries,
            'TX':       np_array_of_tx_entries
        }

        attr_dict = {
            '/':        'Data from some_log_file.bin, node serial number W3-a-00001, written on 2014-03-18',
            'RX_OFDM':  'Filtered Rx OFDM events, only good FCS receptions',
            'TX':       'Filtered Tx events, only DATA packets'
        }
    
        # Two groups, with two datasets in each group
        np_log_dict = {
            'Log_Node_A': {
                'RX_OFDM':  np_array_of_rx_etries_A,
                'TX':       np_array_of_tx_entries_A
            },
            'Log_Node_B': {
                'RX_OFDM':  np_array_of_rx_etries_B,
                'TX':       np_array_of_tx_entries_B
            }
        }

        attr_dict = {
            '/':        'Written on 2014-03-18',
            'Log_Node_A': {
                '/':        'Data from node_A_log_file.bin, node serial number W3-a-00001',
                'RX_OFDM':  'Filtered Rx OFDM events, only good FCS receptions',
                'TX':       'Filtered Tx events, only DATA packets'
            }
            'Log_Node_B': {
                '/':        'Data from node_B_log_file.bin, node serial number W3-a-00002',
                'RX_OFDM':  'Filtered Rx OFDM events, only good FCS receptions',
                'TX':       'Filtered Tx events, only DATA packets'
            }
        }
    """
    import h5py

    dk = list(np_log_dict.keys())

    h5_filename = log_util._get_safe_filename(filename)
    hf = h5py.File(h5_filename, mode='w')

    try:
        # Copy any user-supplied attributes to root group
        #   h5py uses the h5py.File handle to access the file itself and the root group
        hf.attrs['INFO'] = attr_dict['/']
    except (KeyError, TypeError):
        # TypeError - attrs dictionary does not exist
        # KeyError - attrs dictionary exists but key does not
        pass

    if type(np_log_dict[dk[0]]) is dict:
        # np_log_dict is dictionary-of-dictionaries
        # Create an HDF5 file with one group per value in np_log_dict
        #   with one dataset per value in np_log_dict[each key]
        # This is a good structure for one dictionary containing one key-value
        #   per parsed log file, where the key is the log file name and the
        #   value is another dictionary containing the log entry arrays

        for grp_k in np_log_dict.keys():
            # Create one group per log file, using log file name as group name
            grp = hf.create_group(grp_k)

            try:
                grp.attrs['INFO'] = attr_dict[grp_k]['/']
            except (KeyError, TypeError):
                pass

            for arr_k in np_log_dict[grp_k].keys():
                # Create one dataset per numpy array of log data
                ds = grp.create_dataset(arr_k, data=np_log_dict[grp_k][arr_k], compression=compression)

                try:
                    ds.attrs[arr_k + '_INFO'] = attr_dict[grp_k][arr_k]
                except (KeyError, TypeError):
                    pass

    else:
        # np_log_dict is dictionary-of-arrays
        #   Create HDF5 file with datasets in root, one per np_log_dict[each key]

        for arr_k in np_log_dict.keys():
            # Create one dataset per numpy array of log data
            ds = hf.create_dataset(arr_k, data=np_log_dict[arr_k], compression=compression)

            try:
                ds.attrs[arr_k + '_INFO'] = attr_dict[arr_k]
            except (KeyError, TypeError):
                pass
    hf.close()
    return

# End np_arrays_to_hdf5()




