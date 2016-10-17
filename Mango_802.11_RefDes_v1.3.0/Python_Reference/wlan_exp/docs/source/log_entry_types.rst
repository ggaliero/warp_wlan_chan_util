.. _log_entry_types:

.. include:: globals.rst


Log Entry
---------

The log defines a number of entry types to collect information.  The definition of the various entry types can 
be found in the :ug:`user guide <wlan_exp/log/entry_types>`.


Log Entry Class
...............
.. autoclass:: wlan_exp.log.entry_types.WlanExpLogEntryType
   :members: get_field_names, get_field_struct_formats, get_entry_type_id, append_field_defs, modify_field_def, add_gen_numpy_array_callback, deserialize, serialize


Log Entry Functions
...................

.. autofunction:: wlan_exp.log.entry_types.np_array_add_fields

