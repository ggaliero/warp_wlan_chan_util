.. _log_util_hdf:

.. include:: globals.rst


HDF5 Utilities
--------------
The logging framework uses the HDF5 standard to store log information.  You can find more documentation on HDF5 at
http://www.h5py.org/


HDF5 Container Class
....................

.. autoclass:: wlan_exp.log.util_hdf.HDF5LogContainer
   :members: is_valid, write_log_data, write_log_index, write_attr_dict, get_log_data_size, get_log_data, get_log_index, get_attr_dict


HDF5 Utility Functions
......................

These utility functions wrap the interactions with the HDF5 Container class.

.. autofunction:: wlan_exp.log.util_hdf.hdf5_open_file

.. autofunction:: wlan_exp.log.util_hdf.hdf5_close_file

.. autofunction:: wlan_exp.log.util_hdf.log_data_to_hdf5

.. autofunction:: wlan_exp.log.util_hdf.hdf5_to_log_data

.. autofunction:: wlan_exp.log.util_hdf.hdf5_to_log_index

.. autofunction:: wlan_exp.log.util_hdf.hdf5_to_attr_dict

.. autofunction:: wlan_exp.log.util_hdf.np_arrays_to_hdf5

