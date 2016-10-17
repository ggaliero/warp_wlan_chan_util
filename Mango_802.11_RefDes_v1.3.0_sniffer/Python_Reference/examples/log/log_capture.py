"""
------------------------------------------------------------------------------
Gabriele Galiero Casay - Experiments Framework - Continuous Log capture
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
import optparse
import paramiko

import wlan_exp.util as wlan_exp_util
import wlan_exp.config as wlan_exp_config

import wlan_exp.log.util as log_util
import wlan_exp.log.util_hdf as hdf_util

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": raw_input=input

#--------------HELPER FUNCTIONS-------------------------#
#def get_mac_channel_index(wlan_mac_chan):
#    if wlan_mac_chan > 0 and wlan_mac_chan < 12:
#        return wl

#-----------------------------------------------------------------------------
# Experiment Variables
#-----------------------------------------------------------------------------
# Network / Node information
NETWORK            = '10.0.0.0'
NODE_SERIAL_LIST   = ['W3-a-00610']

# HDF5 File to log information
DEFAULT_LOGFILE      = 'log_capture'

# Experiment iterations
DEFAULT_EXP_ITER     = 10 

# Interval for printing
PRINT_TIME         = 1

# Logging variables
LOG_READ_TIME      = 30
MAX_LOG_SIZE       = 2**30             # Max size is 1GB, 2**30

DEFAULT_CHANNEL    = 48
DEFAULT_START      = 0

WLAN_MAC_CHANNELS  = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48]

#-----------------------------------------------------------------------------
# Global Variables
#-----------------------------------------------------------------------------
hosts              = ['terror.verkstad.net', 'phantom.verkstad.net', 'dread.verkstad.net']   # Available host names
user               = 'demo'             # User name
password           = 'pw4demo'          # User name password
warp_ap_ip1        = '10.10.129.13'
warp_ap_ip2        = '10.10.130.13'
comm_ap_ip         = '192.168.0.1'
warp_sta_ip        = '10.10.129.100'
comm_sta_ip        = '10.10.130.100'

# Fox Traffic Application Parameters
num_frames         = 2000
idt                = 30
header             = 36
frame_len          = 150
udp_len            = frame_len - header

network_config     = None
nodes              = []
threads            = []
node               = None
exp_done           = False
input_done         = False
timeout            = 0.1
time_exp           = None
wlan_mac_chan      = None
store              = None
rx_mpdu_app        = []
ssh_list           = []

h5_file            = None
log_container      = None
#file_start         = 25

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

def get_tx_rx_ip(uplink, warp_to_comm):
    global warp_ap_ip1, warp_ap_ip2, warp_sta_ip, comm_ap_ip, comm_sta_ip

    sta_ip = None
    ap_ip  = None
    tx_ip  = None
    rx_ip  = None
    
    if comm_to_comm:
        sta_ip = comm_sta_ip
        ap_ip  = comm_ap_ip
    elif warp_to_comm:
        sta_ip = comm_sta_ip
        ap_ip  = warp_ap_ip2
    elif (not comm_to_comm) and (not warp_to_comm):
        sta_ip = warp_sta_ip
        ap_ip  = warp_ap_ip1
    
    if uplink:
        tx_ip = sta_ip
        rx_ip = ap_ip
    else:
        tx_ip = ap_ip
        rx_ip = sta_ip
    return (tx_ip, rx_ip)
    
def get_tx_rx_host(uplink, comm_to_comm, warp_to_comm):
    global hosts
    
    tx_host = None
    rx_host = None
    
    if comm_to_comm:
        if uplink:
            tx_host = hosts[0]
            rx_host = hosts[2]
        else:
            tx_host = hosts[2]
            rx_host = hosts[0]
    else:
        if uplink:
            tx_host = hosts[0]
            rx_host = hosts[1]
        else:
            tx_host = hosts[1]
            rx_host = hosts[0]
    return tx_host, rx_host

def run_listener(host,user,p,time_len):
    global ssh_list, rx_mpdu_app, store
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        ssh.connect(host, username=user, password=p)
        ssh_list.append(ssh)
#        listener_command = "/usr/local/bin/rtp_src -f -w /usr/home/demo/zapme -l " + str(time_len+3)
#        read_lines_command = "/usr/bin/wc -l /usr/home/demo/zapme"
        listener_command = "/usr/local/bin/rtp_src -f -w gabri -l " + str(time_len+3)
        read_lines_command = "/usr/bin/wc -l gabri"
        stdin1, stdout1, stderr1 = ssh.exec_command(listener_command)
        type(stdin1)
        stdout1.readlines()
        stdin2, stdout2, stderr2 = ssh.exec_command(read_lines_command)
        for line in stdout2.read().splitlines():
            #print('.' + line)
            for elem in line.split(' '):
                if elem != '' and elem != 'gabri': 
                    rx_mpdu_app.append(elem)
                    store.write("%s\n" % elem)
#        line = stdout2.read().splitlines()
#        line_elem = line.split(' ')
#        rx_mpdu_app.append(line_elem[0])            
        ssh.close()  
    except:
        print 'Error while running the traffic listener'

def run_traffic(host,user,p,ip):
    global num_frames, udp_len, idt, ssh_list
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        ssh.connect(host, username=user, password=p)
        ssh_list.append(ssh)
        #command2 = "/usr/local/bin/rtp_src -h >> foobar.txt"
        command =  "/usr/local/bin/rtp_src -f -d " + ip + " -s " + str(udp_len) + " -c " + str(idt) + " -n " + str(num_frames)
        stdin, stdout, stderr = ssh.exec_command(command)
        type(stdin)
        stdout.readlines()
        ssh.close()
    except:
        print 'Error when running the traffic generator.'
    
def set_channel(mac_channel):
    if mac_channel not in WLAN_MAC_CHANNELS:
        return WLAN_MAC_CHANNELS[12]
    return mac_channel

def reset_exp_var():
    global network_config, node, nodes, log_container, exp_done, input_done, time_exp
    
    network_config = None
    node = None
    nodes = []
    log_container = None
    exp_done = False
    input_done = False
    #time_exp = None

def write_list_into_file(list_elem, filename):
    try:
        store = open(filename, 'w')
        for i in range(0,len(list_elem)):
            store.write("%s\n" % list_elem[i])
        store.close()
    except:
        print 'Error while writing the output files'


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
    for tmp_node in nodes:
    #     # Issue a reset command to stop current operation / initialize components
         tmp_node.reset(log=True, txrx_stats=True, ltg=True, queue_data=True) # Do not reset associations/bss_info
    #
    #     # Configure the log
         tmp_node.log_configure(log_full_payloads=True)

# End def


def setup_experiment():
    """Setup WLAN Experiment."""
    global node, wlan_mac_chan

    # Check that we have one node
    if (len(nodes) == 1):
        # Extract the node from the list for easier referencing below
        node = nodes[0]
        #node.set_low_to_high_rx_filter(mac_header='ALL_MPDU',fcs='GOOD') # gabri
        node.set_low_to_high_rx_filter(mac_header='ALL',fcs='ALL')
        node.set_channel(wlan_mac_chan)
    else:
        print("ERROR: Node configurations did not match requirements of script.\n")
        return

# End def


def run_experiment():
    """WLAN Experiment."""
    global network_config, node, log_container, exp_done, input_done, time_exp

    print("\nRun Experiment:\n")
    print("Log data will be retrieved every {0} seconds\n".format(LOG_READ_TIME))
    print("The experiment lasts {0} seconds.\n".format(time_exp))
#    print("Use 'q' or Ctrl-C to end the experiment.\n")
#    print("  NOTE:  In IPython, press return to see status update.\n")

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
        if((loop_time - start_time) > (time_exp + 3)):
            exp_done = True

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
    
    
    parser = optparse.OptionParser()
    parser.add_option('-i', action="store_true", dest="warp_to_comm", default=False)
    parser.add_option('-c', action="store_true", dest="comm_to_comm", default=False)
    parser.add_option('-u', action="store_true", dest="uplink", help="Uplink traffic flow", default=True)
    parser.add_option('-d', action="store_false", dest="uplink", help="Downlink traffic flow")
    parser.add_option('--ch', action="store", dest="channel", help="WLAN Channel", default=DEFAULT_CHANNEL)
    parser.add_option('-n', action="store", dest="num_exp", help="Number of experiment iterations", default=DEFAULT_EXP_ITER)
    parser.add_option('-s', action="store", dest="start", help="Measurmerent index start", default=DEFAULT_START)
    
    options, args    = parser.parse_args()
    warp_to_comm     = options.warp_to_comm
    comm_to_comm     = options.comm_to_comm
    uplink           = options.uplink      
    num_exp          = int(options.num_exp)
    wlan_mac_chan    = set_channel(int(options.channel))
    time_exp         = int((num_frames * idt) / 1000)
    file_start       = int(options.start)
   
    # Assign the traffic flow, hosts and IPs
    tx_ip, rx_ip = get_tx_rx_ip(uplink, warp_to_comm)   
    tx_host, rx_host = get_tx_rx_host(uplink, comm_to_comm, warp_to_comm)
   
    # Print information of the experiment
    print ''
    if comm_to_comm:
        print "Commercial to Commercial Device Communication"
    elif warp_to_comm:
        print "WARP to Commercial Device Communication"
    else:
        print "WARP to WARP Device Communication"
    print ''
    if uplink:
        print "Traffic Flow: Uplink"
    else:
        print "Traffic Flow: Downlink"
    
    print('Tx IP: %s --> Rx IP: %s' % (tx_ip, rx_ip))
    print ''
    print('Tx Host name: %s' % tx_host)
    print('Rx Host name: %s' % rx_host)
    
    try:
        filename = 'num_mpdu_app.txt'
        store = open(filename, 'a+')
    except:
        print 'Error while opening the file to store received mpdus at the fox application level.'
    
#    i = file_start
#    while ((i < (num_exp + file_start)) and (not exp_done)):
    for i in range(file_start, num_exp + file_start):
        # Create file name
        logfile = 'log_cap_' + '{:03d}'.format(i) + '.hdf5'

        # Create Log Container
        h5_file       = hdf_util.hdf5_open_file(logfile)
        log_container = hdf_util.HDF5LogContainer(h5_file)
        
        # Log attributes about the experiment
        attr_dict['exp_name'] = 'Interactive Capture, Continuous Log Read'

        # Create thread for the traffic listener    
        listener_thread = threading.Thread(target=run_listener, args=(rx_host,user,password,time_exp))    
        #listener_thread.daemon = True
    
        # Create thread for experiment
        exp_thread = threading.Thread(target=run_experiment)
        #exp_thread.daemon = True    
    
        # Create thread for the traffic generation
        traffic_thread = threading.Thread(target=run_traffic, args=(tx_host,user,password,rx_ip))
        #traffic_thread.daemon = True

    
        try:
        
            print("\nExperiment %d initialized." % (i + 1))
            # Initialize the experiment
            init_experiment()

            # Setup the experiment
            setup_experiment()
        
            # Start listener thread
            listener_thread.start()
            time.sleep(1)

            # Start the experiment loop thread
            exp_thread.start()
            time.sleep(1)
        
            # Start traffic thread
            traffic_thread.start()
        
            #time.sleep(time_exp + 5)

#        # See if there is any input from the user
#        while not input_done:
#            sys.stdout.flush()
#            temp = raw_input("")
#
#            if temp is not '':
#                user_input = temp.strip()
#                user_input = user_input.upper()
#
#                if ((user_input == 'Q') or (user_input == 'QUIT') or (user_input == 'EXIT')):
#                    input_done = True
#                    exp_done   = True

            # Wait for all threads
            traffic_thread.join()
            listener_thread.join()
            exp_thread.join()
            sys.stdout.flush()

            # End the experiment
            end_experiment()
#            i = i + 1          
            print("\nExperiment %d Finished." % (i + 1))
            
            reset_exp_var()

            
        except KeyboardInterrupt:
            exp_done   = True
            input_done = True
            break

            # If there is a keyboard interrupt, then end the experiment
            end_experiment()
#        i = i + 1
    print rx_mpdu_app
        
    print("\nExperiment Finished.")
