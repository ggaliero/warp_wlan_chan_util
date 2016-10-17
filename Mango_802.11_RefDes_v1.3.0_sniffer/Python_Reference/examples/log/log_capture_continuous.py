"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design - Experiments Framework - Continuous Log capture
------------------------------------------------------------------------------
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
This script uses the 802.11 ref design and WARPnet to create a log
file that contains all data assocated with an interactive experiment.

Hardware Setup:
  - Requires one WARP v3 node configured as either an AP or STA
  - PC NIC and ETH B on WARP v3 nodes connected to common Ethernet switch

Required Script Changes:
  - Set NETWORK to the IP address of your host PC NIC network (eg X.Y.Z.0 for IP X.Y.Z.W)
  - Set NODE_SERIAL_LIST to the serial number of your WARP node

Description:
  This script initializes one WARP v3 node.  It will periodically log the
Tx/Rx statistics and update information on the screen about the log.  The
script will also read the log data every LOG_READ_TIME seconds, write it
to the hdf5 file, HDF5_FILENAME, and continue until MAX_LOG_SIZE is reached
or the user ends the experiment.
------------------------------------------------------------------------------
"""
import sys
import time
import datetime
import threading

import wlan_exp.util as wlan_exp_util
import wlan_exp.config as wlan_exp_config

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input


#-----------------------------------------------------------------------------
# Experiment Variables
#-----------------------------------------------------------------------------
# Network / Node information
NETWORK            = '10.0.0.0'
NODE_SERIAL_LIST   = ['W3-a-00001']

# HDF5 File to log information
HDF5_FILENAME      = 'log_continuous_capture.hdf5'

# Interval for printing
PRINT_TIME         = 1

# Logging variables
LOG_READ_TIME      = 30
MAX_LOG_SIZE       = 2**30             # Max size is 1GB


#-----------------------------------------------------------------------------
# Global Variables
#-----------------------------------------------------------------------------
network_config     = None
nodes              = []
node               = None
exp_done           = False
input_done         = False
timeout            = 0.1

h5_file            = None
log_container      = None

attr_dict          = {}


#-----------------------------------------------------------------------------
# Local Helper Utilities
#-----------------------------------------------------------------------------
def add_data_to_log(log_tail_pad=500):
    """Adds data to the log."""
    global node
    global log_container

    wn_buffer = node.log_get_all_new(log_tail_pad=log_tail_pad)
    data      = wn_buffer.get_bytes()

    # Write Log Files for processing by other scripts
    print("\nWriting {0:15,d} bytes of data to log file {1}...".format(len(data), h5_file.filename))
    log_container.write_log_data(data)

# End def


def get_log_size_str(nodes):
    """Gets the log size str for each node."""

    msg  = "Log Size:"

    for node in nodes:
        log_size  = node.log_get_size()
        msg += "    {0:10s}  = {1:10d} bytes".format(node.name, log_size)

    return msg

# End def


def get_exp_duration_str(start_time):
    """Gets the duration str of the experiment since start_time."""
    return "Duration:  {0:8.0f} sec".format(time.time() - start_time)

# End def


def print_node_state(start_time):
    """Print the current state of the node."""

    msg  = "\r"
    msg += get_exp_duration_str(start_time)
    msg += " " * 5
    msg += get_log_size_str(nodes)
    msg += " " * 5

    sys.stdout.write(msg)
    sys.stdout.flush()

# End def



#-----------------------------------------------------------------------------
# Experiment Script
#-----------------------------------------------------------------------------
def init_experiment():
    """Initialize WLAN Experiment."""
    global network_config, nodes, attr_dict

    print("\nInitializing experiment\n")

    # Log attributes about the experiment
    attr_dict['exp_start_time'] = log_util.convert_datetime_to_log_time_str(datetime.datetime.utcnow())

    # Create an object that describes the network configuration of the host PC
    network_config = wlan_exp_config.WlanExpNetworkConfiguration(network=NETWORK)

    # Create an object that describes the WARP v3 nodes that will be used in this experiment
    nodes_config   = wlan_exp_config.WlanExpNodesConfiguration(network_config=network_config,
                                                               serial_numbers=NODE_SERIAL_LIST)

    # Initialize the Nodes
    #   This command will fail if either WARP v3 node does not respond
    nodes = wlan_exp_util.init_nodes(nodes_config, network_config)

    # Do not set the node into a known state.  This example will just read
    # what the node currently has in the log.  However, the below code could
    # be used to initialize all nodes into a default state:
    #
    # Set each node into the default state
    # for tmp_node in nodes:
    #     # Issue a reset command to stop current operation / initialize components
    #     tmp_node.reset(log=True, txrx_stats=True, ltg=True, queue_data=True) # Do not reset associations/bss_info
    #
    #     # Configure the log
    #     tmp_node.log_configure(log_full_payloads=True)

# End def


def setup_experiment():
    """Setup WLAN Experiment."""
    global node

    # Check that we have one node
    if (len(nodes) == 1):
        # Extract the node from the list for easier referencing below
        node = nodes[0]
    else:
        print("ERROR: Node configurations did not match requirements of script.\n")
        return

# End def


def run_experiment():
    """WLAN Experiment."""
    global network_config, node, log_container, exp_done, input_done

    print("\nRun Experiment:\n")
    print("Log data will be retrieved every {0} seconds\n".format(LOG_READ_TIME))
    print("Use 'q' or Ctrl-C to end the experiment.\n")
    print("  NOTE:  In IPython, press return to see status update.\n")

    # Add the current time to all the nodes
    wlan_exp_util.broadcast_cmd_write_time_to_logs(network_config)

    # Write Statistics to log
    node.log_write_txrx_stats()

    # Get the start time
    start_time = time.time()
    last_print = time.time()
    last_read  = time.time()

    # Print the current state of the node
    print_node_state(start_time)

    while not exp_done:
        loop_time = time.time()

        if ((loop_time - last_print) > PRINT_TIME):
            # Print the current state of the node
            print_node_state(start_time)

            # Write Statistics to log
            node.log_write_txrx_stats()

            # Set the last_print time
            last_print = time.time()

        if ((loop_time - last_read) > LOG_READ_TIME):
            # Write the data to the log
            add_data_to_log()

            # Set the last_read time
            last_read  = time.time()

            # Log size stop condition
            if (log_container.get_log_data_size() > MAX_LOG_SIZE):
                print("\n!!! Reached Max Log Size.  Ending experiment. !!!\n")
                input_done = True
                exp_done   = True

# End def


def end_experiment():
    """Experiment cleanup / post processing."""
    global node, log_container
    print("\nEnding experiment\n")

    # Get the last of the data
    add_data_to_log(log_tail_pad=0)

    # Create the log index
    log_container.write_log_index()

    # Get the end time as an attribute
    attr_dict['exp_end_time'] = log_util.convert_datetime_to_log_time_str(datetime.datetime.utcnow())

    # Add the attribute dictionary to the log file
    log_container.write_attr_dict(attr_dict)

    # Print final log size
    log_size = log_container.get_log_data_size()

    print("Final log size:  {0:15,d} bytes".format(log_size))

    # Clost the Log file for processing by other scripts
    hdf_util.hdf5_close_file(h5_file)

    print("Done.")
    return

# End def


#-----------------------------------------------------------------------------
# Main Function
#-----------------------------------------------------------------------------
if __name__ == '__main__':

    # Use log file given as command line argument, if present
    if(len(sys.argv) == 1):
        #No filename on command line
        LOGFILE = HDF5_FILENAME
    else:
        LOGFILE = str(sys.argv[1])

    # Create Log Container
    h5_file       = hdf_util.hdf5_open_file(LOGFILE)
    log_container = hdf_util.HDF5LogContainer(h5_file)

    # Log attributes about the experiment
    attr_dict['exp_name'] = 'Interactive Capture, Continuous Log Read'

    # Create thread for experiment
    exp_thread = threading.Thread(target=run_experiment)
    exp_thread.daemon = True

    try:
        # Initialize the experiment
        init_experiment()

        # Setup the experiment
        setup_experiment()

        # Start the experiment loop thread
        exp_thread.start()

        # See if there is any input from the user
        while not input_done:
            sys.stdout.flush()
            temp = raw_input("")

            if temp is not '':
                user_input = temp.strip()
                user_input = user_input.upper()

                if ((user_input == 'Q') or (user_input == 'QUIT') or (user_input == 'EXIT')):
                    input_done = True
                    exp_done   = True

        # Wait for all threads
        exp_thread.join()
        sys.stdout.flush()

        # End the experiment
        end_experiment()

    except KeyboardInterrupt:
        exp_done   = True
        input_done = True

        # If there is a keyboard interrupt, then end the experiment
        end_experiment()

    print("\nExperiment Finished.")
