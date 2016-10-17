"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Log File Version Updater
------------------------------------------------------------------------------
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the WLAN Exp Log framework to update version information
in a given hdf5 log file that contains data assocated with an experiment 
utilizing the 802.11 reference design and WLAN Exp.

Hardware Setup:
    - None.  Updating log version information can be done completely off-line

Required Script Changes:
    - None.  Script requires filename of file to be anonymized to be
      passed in on the command line.

Description:
    This script parses the log file and updates any WLAN Exp / 802.11 
    version information and write the resulting log data to a new file.
------------------------------------------------------------------------------
"""
import sys
import os

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util


#-----------------------------------------------------------------------------
# Functions
#-----------------------------------------------------------------------------

def log_file_update(filename):
    """Update the log file."""
    
    #---------------------------------------------------------------------
    # Read input file
    #

    # Get the log_data from the file
    log_bytes = bytearray(hdf_util.hdf5_to_log_data(filename=filename))

    # Get the raw_log_index from the file
    raw_log_index = hdf_util.hdf5_to_log_index(filename=filename)

    # Get the user attributes from the file
    log_attr_dict  = hdf_util.hdf5_to_attr_dict(filename=filename)


    #---------------------------------------------------------------------
    # Print information about the file
    #

    log_util.print_log_index_summary(raw_log_index, "Log Index Summary:")


    #---------------------------------------------------------------------
    # Write output file
    #

    # Write the log to a new HDF5 file
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

    newfilename = os.path.join(fn_fldr, fn_base + "_update" + fn_ext)

    print("Writing new file {0} ...".format(newfilename))

    # Copy any user attributes to the new file
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
                print("\nUpdating file '{0}' ({1:5.1f} MB)\n".format(filename, (os.path.getsize(filename)/1E6)))
                log_file_update(filename)

