# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Node Classes
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

"""
import sys

import wlan_exp.transport.node as node
import wlan_exp.transport.exception as ex

import wlan_exp.version as version
import wlan_exp.defaults as defaults
import wlan_exp.cmds as cmds
import wlan_exp.device as wlan_device


__all__ = ['WlanExpNode', 'WlanExpNodeFactory']

# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None

# Node Parameter Identifiers
#   Values here must match the C counterparts in *_node.h
#
# If additional hardware parameters are needed for sub-classes of WlanExpNode,
# please make sure that the values of these hardware parameters are not reused.
#
NODE_PARAM_ID_WLAN_EXP_VERSION                  = 5
NODE_PARAM_ID_WLAN_SCHEDULER_RESOLUTION         = 6
NODE_PARAM_ID_WLAN_MAC_ADDR                     = 7
NODE_PARAM_ID_WLAN_MAX_TX_POWER_DBM             = 8
NODE_PARAM_ID_WLAN_MIN_TX_POWER_DBM             = 9


class WlanExpNode(node.WarpNode, wlan_device.WlanDevice):
    """Class for 802.11 Reference Design Experiments Framwork node.

    Args:
        network_config (transport.NetworkConfiguration) : Network configuration of the node


    Attributes:
        node_type (int): Unique type of the node
        node_id (int): Unique identification for this node
        name (str): User specified name for this node (supplied by user scripts)
        description (str): String description of this node (auto-generated)
        serial_number (int): Node's serial number, read from EEPROM on hardware
        fpga_dna (int): Node's FPGA'a unique identification (on select hardware)
        hw_ver (int): WARP hardware version of this node
        transport (transport.Transport): Node's transport object
        transport_broadcast (transport.Transport): Node's broadcast transport object

        device_type (int): Unique type of the ``WlanDevice`` (inherited from ``WlanDevice``)
        wlan_mac_address (int): Wireless MAC address of the node (inherited from ``WlanDevice``)
        ht_capable (bool): Indicates if device has PHY capable of HT (802.11n) rates 
            (inherited from ``WlanDevice``)

        scheduler_resolution (int): Minimum resolution (in us) of the scheduler.  This
            is also the minimum time between LTGs.
        log_max_size (int): Maximum size of event log (in bytes)
        log_total_bytes_read (int): Number of bytes read from the event log
        log_num_wraps (int): Number of times the event log has wrapped
        log_next_read_index (int): Index in to event log of next read
        wlan_exp_ver_major (int): ``wlan_exp`` Major version running on this node
        wlan_exp_ver_minor (int): ``wlan_exp`` Minor version running on this node
        wlan_exp_ver_revision (int): ``wlan_exp`` Revision version running on this node
        max_tx_power_dbm(int): Maximum transmit power of the node (in dBm)
        min_tx_power_dbm(int): Minimum transmit power of the node (in dBm)

    """

    scheduler_resolution               = None

    log_max_size                       = None
    log_total_bytes_read               = None
    log_num_wraps                      = None
    log_next_read_index                = None

    wlan_exp_ver_major                 = None
    wlan_exp_ver_minor                 = None
    wlan_exp_ver_revision              = None

    max_tx_power_dbm                   = None
    min_tx_power_dbm                   = None

    def __init__(self, network_config=None):
        super(WlanExpNode, self).__init__(network_config)

        (self.wlan_exp_ver_major, self.wlan_exp_ver_minor,
                self.wlan_exp_ver_revision) = version.wlan_exp_ver()

        self.scheduler_resolution           = 1

        self.log_total_bytes_read           = 0
        self.log_num_wraps                  = 0
        self.log_next_read_index            = 0

        # As of v1.5 all 802.11 Ref Design nodes are HT capable
        self.ht_capable = True

    #-------------------------------------------------------------------------
    # Node Commands
    #-------------------------------------------------------------------------


    #--------------------------------------------
    # Log Commands
    #--------------------------------------------
    def log_configure(self, log_enable=None, log_wrap_enable=None,
                            log_full_payloads=None, log_txrx_mpdu=None,  
                            log_txrx_ctrl=None):
        """Configure log with the given flags.

        By default all attributes are set to None.  Only attributes that
        are given values will be updated on the node.  If an attribute is
        not specified, then that attribute will retain the same value on
        the node.

        Args:
            log_enable (bool):        Enable the event log 
                (Default value on Node: TRUE)
            log_wrap_enable (bool):   Enable event log wrapping 
                (Default value on Node: FALSE)
            log_full_payloads (bool): Record full Tx/Rx payloads in event log 
                (Default value on Node: FALSE)
            log_txrx_mpdu (bool):     Enable Tx/Rx log entries for MPDU frames
                (Default value on Node: TRUE)
            log_txrx_ctrl (bool):     Enable Tx/Rx log entries for CTRL frames
                (Default value on Node: TRUE)
        """
        self.send_cmd(cmds.LogConfigure(log_enable, log_wrap_enable,
                                        log_full_payloads, log_txrx_mpdu, 
                                        log_txrx_ctrl))


    def log_get(self, size, offset=0, max_req_size=2**23):
        """Low level method to retrieve log data from a wlan_exp node.

        Args:
            size (int):                   Number of bytes to read from the log
            offset (int, optional):       Starting byte to read from the log
            max_req_size (int, optional): Max request size that the transport 
                will fragment the request into.

        Returns:
            buffer (transport.Buffer):  
                Data from the log corresponding to the input parameters

        There is no guarentee that this will return data aligned to event
        boundaries.  Use ``log_get_indexes()`` to get event aligned boundaries.

        Log reads are not destructive. Log entries will only be destroyed by a 
        log reset or if the log wraps.

        During a given ``log_get()`` command, the ETH_B Ethernet interface of
        the node will not be able to respond to any other Ethernet packets
        that are sent to the node.  This could cause the node to drop
        incoming wlan_exp packets from other wlan_exp instances accessing
        the node. Therefore, for large requests, having a smaller 
        ``max_req_size`` will allow the transport to fragement the command and 
        allow the node to be responsive to multiple hosts.

        Some basic analysis shows that fragment sizes of 2**23 (8 MB)
        add about 2% overhead to the receive time and each command takes less
        than 1 second (~0.9 sec), which is the default transport timeout.
        """
        cmd_size = size
        log_size = self.log_get_size()

        if ((size + offset) > log_size):
            cmd_size = log_size - offset

            print("WARNING:  Trying to get {0} bytes from the log at offset {1}".format(size, offset))
            print("    while log only has {0} bytes.".format(log_size))
            print("    Truncating command to {0} bytes at offset {1}.".format(cmd_size, offset))

        return self.send_cmd(cmds.LogGetEvents(cmd_size, offset), max_req_size=max_req_size)


    def log_get_all_new(self, log_tail_pad=500, max_req_size=2**23):
        """Get all "new" entries in the log.
        
        In order to make reading log data easier, the ``log_get_all_new()``
        command was created.  It utilizes the ``log_get_indexex()`` and 
        ``log_get()`` commmands to understand the state of the log and to 
        retrieve data from the log.  The Python node object also maintains 
        internal state about what log data has been read from the node to 
        determine if there is "new" data in the log.  Since this state data is 
        maintained in Python and not on the node itself, it is possible for
        multiple host scripts to read log data using ``log_get_all_new()``. The 
        internal state maintained for ``log_get_all_new()`` is only reset when 
        using the node reset method (i.e. ``node.reset(log=True)``).
        
        When a node is adding entries to its log, it will allocate the 
        memory for an entry from the log and then fill in the contents.  This 
        means at any given time, the last log entry may have incomplete 
        information.  Currently, the log maintains the index where the next 
        log entry will be recorded (i.e. the ``next_index``), which does not 
        provide any state about whether the last log entry has been completed.
        Therefore, in order to not retrieve incomplete information, 
        ``log_get_all_new()`` has an argument ``log_tail_pad=N`` to not read 
        the last N "new" bytes of log data.  
        
        Unfortunately, this means that using a ``log_tail_pad`` other than 
        zero will result in return data that is not aligned to a log entry
        boundary.  This should not be an issue if the goal is to periodically
        read the log data from the node and store it in a file (as seen in 
        the ``log_capture_continuous.py`` example).  However, this can be an 
        issue if trying to periodically read the log and process the log data 
        directly.  In this case, ``log_tail_pad`` must be zero and the code 
        has to assume the last entry is invalid.  This has not come up very 
        much, but please let us know if this is an issue via the forums.        

        Args:
            log_tail_pad (int, optional): Number of bytes from the current end 
                of the "new" entries that will not be read during the call.  
                This is to deal with the case where the node is in the process 
                of modifying the last log entry so it my contain incomplete 
                data and should not be read.

        Returns:
            buffer (transport.Buffer): 
                Data from the log that contains all entries since the last 
                time the log was read.
        """
        import wlan_exp.transport.message as message

        return_val = message.Buffer()

        # Check if the log is full to interpret the indexes correctly
        if (self.log_is_full()):
            log_size  = self.log_get_size()
            read_size = log_size - self.log_next_read_index - log_tail_pad

            # Read the data from the node
            if (read_size > 0):
                return_val = self.log_get(offset=self.log_next_read_index,
                                          size=read_size,
                                          max_req_size=max_req_size)

                # Only increment index by how much was actually read
                read_size  = return_val.get_buffer_size()

                if (read_size > 0):
                    self.log_next_read_index += read_size
                else:
                    print("WARNING:  Not able to read data from node.")
            else:
                print("WARNING:  No new data on node.")

        # Log is not full
        else:
            (next_index, _, num_wraps) = self.log_get_indexes()

            if ((self.log_next_read_index == 0) and (self.log_num_wraps == 0)):
                # This is the first read of the log by this python object
                if (num_wraps != 0):
                    # Need to advance the num_wraps to the current num_wraps so
                    # that the code does not get into a bad state with log reading.
                    msg  = "\nWARNING:  On first read, the log on the node has already wrapped.\n"
                    msg += "    Skipping the first {0} wraps of log data.\n".format(num_wraps)
                    print(msg)
                    self.log_num_wraps = num_wraps

            # Check if log has wrapped
            if (num_wraps == self.log_num_wraps):
                # Since log has not wrapped, then read to the (next_index - log_tail_pad)
                if (next_index > (self.log_next_read_index + log_tail_pad)):

                    return_val = self.log_get(offset=self.log_next_read_index,
                                              size=(next_index - self.log_next_read_index - log_tail_pad),
                                              max_req_size=max_req_size)

                    # Only increment index by how much was actually read
                    read_size  = return_val.get_buffer_size()
                    if (read_size > 0):
                        self.log_next_read_index += read_size
                    else:
                        print("WARNING:  Not able to read data from node.")
                else:
                    print("WARNING:  No new data on node.")
            else:
                # Log has wrapped.  Get all the entries on the old wrap
                if (next_index != 0):

                    return_val = self.log_get(offset=self.log_next_read_index,
                                              size=cmds.CMD_PARAM_LOG_GET_ALL_ENTRIES,
                                              max_req_size=max_req_size)

                    # The amount of data returned from the node should not be zero
                    read_size  = return_val.get_buffer_size()
                    if (read_size > 0):
                        self.log_next_read_index = 0
                        self.log_num_wraps       = num_wraps
                    else:
                        print("WARNING:  Not able to read data from node.")
                else:
                    print("WARNING:  No new data on node.")

        return return_val


    def log_get_size(self):
        """Get the size of the node's current log (in bytes).

        Returns:
            num_bytes (int): Number of bytes in the log
        """
        (capacity, size)  = self.send_cmd(cmds.LogGetCapacity())

        # Check the maximum size of the log and update the node state
        if self.log_max_size is None:
            self.log_max_size = capacity
        else:
            if (self.log_max_size != capacity):
                msg  = "EVENT LOG WARNING:  Log capacity changed.\n"
                msg += "    Went from {0} bytes to ".format(self.log_max_size)
                msg += "{0} bytes.\n".format(capacity)
                print(msg)
                self.log_max_size = capacity

        return size


    def log_get_capacity(self):
        """Get the total capacity of the node's log memory allocation (in bytes).

        Returns:
            capacity (int): Number of byte allocated for the log.
        """
        return self.log_max_size


    def log_get_indexes(self):
        """Get the indexes that describe the state of the event log.

        Returns:
            indexes (tuple):
            
                #. oldest_index (int): Log index of the oldest event in the log
                #. next_index (int):   Log index where the next event will be recorded
                #. num_wraps (int):    Number of times the log has wrapped
        
        """
        (next_index, oldest_index, num_wraps, _) = self.send_cmd(cmds.LogGetStatus())

        # Check that the log is in a good state
        if ((num_wraps < self.log_num_wraps) or
            ((num_wraps == self.log_num_wraps) and
             (next_index < self.log_next_read_index))):
            msg  = "\n!!! Event Log Corrupted.  Please reset the log. !!!\n"
            print(msg)

        return (next_index, oldest_index, num_wraps)


    def log_get_flags(self):
        """Get the flags that describe the event log configuration.

        Returns:
            flags (int): Integer describing the configuration of the event log.
            
                * ``0x0001`` - Logging enabled
                * ``0x0002`` - Log wrapping enabled
                * ``0x0004`` - Full payload logging enabled
                * ``0x0008`` - Tx/Rx log entries for MPDU frames enabled
                * ``0x0010`` - Tx/Rx log entries for CTRL frames enabled
                
        """
        (_, _, _, flags) = self.send_cmd(cmds.LogGetStatus())

        return flags


    def log_is_full(self):
        """Return whether the log is full or not.

        Returns:
            status (bool): True if the log is full; False if the log is not full.
        """
        (next_index, oldest_index, num_wraps, flags) = self.send_cmd(cmds.LogGetStatus())

        if (((flags & cmds.CMD_PARAM_LOG_CONFIG_FLAG_WRAP) != cmds.CMD_PARAM_LOG_CONFIG_FLAG_WRAP) and
            ((next_index == 0) and (oldest_index == 0) and (num_wraps == (self.log_num_wraps + 1)))):
            return True
        else:
            return False


    def log_write_exp_info(self, info_type, message=None):
        """Write the experiment information provided to the log.

        Args:
            info_type (int): Type of the experiment info.  This is an arbitrary 
                16 bit number chosen by the experimentor
            message (int, str, bytes, optional): Information to be placed in 
                the event log.

        Message must be able to be converted to bytearray with 'UTF-8' format.
        """
        self.send_cmd(cmds.LogAddExpInfoEntry(info_type, message))


    def log_write_time(self, time_id=None):
        """Adds the current time in microseconds to the log.

        Args:
            time_id (int, optional):  User providied identifier to be used for 
                the TIME_INFO log entry.  If none is provided, a random number 
                will be inserted.
        """
        return self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_TIME_ADD_TO_LOG, cmds.CMD_PARAM_RSVD_TIME, time_id))



    #--------------------------------------------
    # Counts Commands
    #--------------------------------------------
    def counts_get_txrx(self, *args, **kwargs):
        """DEPRECATED: Old name for current get_txrx_counts method
        """       
        print("WARNING: the counts_get_txrx() method has been renamed get_txrx_counts!\n")
        print(" The old method will be removed in a future release. Please update your script\n")
        print(" to use get_txrx_counts\n")
        return self.get_txrx_counts(*args, **kwargs)

    def get_txrx_counts(self, device_list=None, return_zeroed_counts_if_none=False):
        """Get the Tx/Rx counts data structurs from the node.

        Args:
            device_list (list of WlanExpNode, WlanExpNode, WlanDevice, optional): 
                List of devices for which to get counts.  See note below for 
                more information.
            return_zeroed_counts_if_none(bool, optional):  If no counts exist 
                on the node for the specified device(s), return a zeroed counts 
                dictionary with proper timestamps instead of None.

        Returns:
            counts_dictionary (list of TxRxCounts, TxRxCounts): 
                TxRxCounts() for the device(s) specified.


        The TxRxCounts() structure returned by this method can be accessed like
        a dictionary and has the following fields:
        
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | Field                       | Description                                                                                         |
            +=============================+=====================================================================================================+
            | retrieval_timestamp         |  Value of System Time in microseconds when structure retrieved from the node                        |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mac_addr                    |  MAC address of remote node whose statics are recorded here                                         |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_rx_bytes           |  Total number of bytes received in DATA packets from remote node (only non-duplicates)              |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_rx_bytes_total     |  Total number of bytes received in DATA packets from remote node (including duplicates)             |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_tx_bytes_success   |  Total number of bytes successfully transmitted in DATA packets to remote node                      |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_tx_bytes_total     |  Total number of bytes transmitted (successfully or not) in DATA packets to remote node             |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_rx_packets         |  Total number of DATA packets received from remote node                                             |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_tx_packets_success |  Total number of DATA packets successfully transmitted to remote node                               |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_tx_packets_total   |  Total number of DATA packets transmitted (successfully or not) to remote node                      |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_tx_attempts        |  Total number of low-level attempts of DATA packets to remote node (includes re-transmissions)      |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_rx_bytes           |  Total number of bytes received in management packets from remote node (only non-duplicates)        |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_rx_bytes_total     |  Total number of bytes received in management packets from remote node (including duplicates)       |            
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_bytes_success   |  Total number of bytes successfully transmitted in management packets to remote node                |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_bytes_total     |  Total number of bytes transmitted (successfully or not) in management packets to remote node       |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_rx_packets         |  Total number of management packets received from remote node (only non-duplicates)                 |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_rx_packets_total   |  Total number of management packets received from remote node (including duplicates)                |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_packets_success |  Total number of management packets successfully transmitted to remote node                         |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_packets_total   |  Total number of management packets transmitted (successfully or not) to remote node                |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_attempts        |  Total number of low-level attempts of management packets to remote node (includes re-transmissions)|
            +-----------------------------+-----------------------------------------------------------------------------------------------------+


        If the ``device_list`` is a single device, then a single dictionary or 
        None is returned.  If the ``device_list`` is a list of devices, then a
        list of dictionaries will be returned in the same order as the devices 
        in the list.  If any of the counts are not there, None will be inserted 
        in the list.  If the ``device_list`` is not specified, then all the 
        counts on the node will be returned.
        """
        ret_val = []
        if not device_list is None:
            if (type(device_list) is list):
                for device in device_list:
                    counts = self.send_cmd(cmds.CountsGetTxRx(device, return_zeroed_counts_if_none))
                    if (len(counts) == 1):
                        ret_val.append(counts)
                    else:
                        ret_val.append(None)
            else:
                ret_val = self.send_cmd(cmds.CountsGetTxRx(device_list, return_zeroed_counts_if_none))
                if (len(ret_val) == 1):
                    ret_val = ret_val[0]
                else:
                    ret_val = None
        else:
            ret_val = self.send_cmd(cmds.CountsGetTxRx())

        return ret_val



    #--------------------------------------------
    # Local Traffic Generation (LTG) Commands
    #--------------------------------------------
    def ltg_configure(self, traffic_flow, auto_start=False):
        """Configure the node for the given traffic flow.

        Args:
            traffic_flow (ltg.FlowConfig):  FlowConfig (or subclass of 
                FlowConfig) that describes the parameters of the LTG.
            auto_start (bool, optional): Automatically start the LTG or wait 
                until it is explictly started.

        Returns:
            ID (int):  Identifier for the LTG flow

        The ``trafic_flow`` argument configures the behavior of the LTG, 
        including the traffic's destination, packet payloads, and packet 
        generation timing. The reference code implements three flow 
        configurations by default:

         - ``FlowConfigCBR()``: a constant bit rate (CBR) source targeted to a 
           specifc destination address. Packet lengths and the packet 
           creation interval are constant.
         - ``FlowConfigAllAssocCBR()``: a CBR source that targets traffic to 
           all nodes in the station info list.
         - ``FlowConfigRandomRandom()``: a source that targets traffic to a 
           specific destination address, where each packet has a random 
           length and packets are created at random intervals.

        Refer to the :doc:`ltg` documentation for details on these flow configs 
        and the underlying classes that can be used to build custom flow 
        configurations.

        Examples:
            The code below illustrates a few LTG configuration examples. ``n1``
            and ``n2`` here are a wlan_exp node objects and the LTG traffic in
            each flow will go from ``n1`` to ``n2``.
            ::

                import wlan_exp.ltg as ltg
                
                # Configure a CBR LTG addressed to a single node
                ltg_id = n1.ltg_configure(ltg.FlowConfigCBR(dest_addr=n2.wlan_mac_address, payload_length=40, interval=0.01), auto_start=True)

                # Configure a backlogged traffic source with constant packet size
                ltg_id = n1.ltg_configure(ltg.FlowConfigCBR(dest_addr=n2.wlan_mac_address, payload_length=1000, interval=0), auto_start=True)

                # Configure a random traffic source
                ltg_id = n1.ltg_configure(ltg.FlowConfigRandomRandom(dest_addr=n2.wlan_mac_address,
                                               min_payload_length=100,
                                               max_payload_length=500,
                                               min_interval=0,
                                               max_interval=0.1),
                                            auto_start=True)
        
        """
        traffic_flow.enforce_min_resolution(self.scheduler_resolution)
        return self.send_cmd(cmds.LTGConfigure(traffic_flow, auto_start))


    def ltg_get_status(self, ltg_id_list):
        """Get the status of the LTG flows.

        Args:
            ltg_id_list (list of int, int): List of LTG flow identifiers or 
                single LTG flow identifier

        If the ltg_id_list is a single ID, then a single status tuple is
        returned.  If the ltg_id_list is a list of IDs, then a list of status
        tuples will be returned in the same order as the IDs in the list.

        Returns:
            status (tuple):
            
                #. valid (bool): Is the LTG ID valid? (True/False)
                #. running (bool): Is the LTG currently running? (True/False)
                #. start_timestamp (int): Timestamp when the LTG started
                #. stop_timestamp (int): Timestamp when the LTG stopped
            
        """
        ret_val = []
        if (type(ltg_id_list) is list):
            for ltg_id in ltg_id_list:
                result = self.send_cmd(cmds.LTGStatus(ltg_id))
                if not result[0]:
                    self._print_ltg_error(cmds.CMD_PARAM_LTG_ERROR, "get status for LTG {0}".format(ltg_id))
                ret_val.append(result)
        else:
            result = self.send_cmd(cmds.LTGStatus(ltg_id_list))
            if not result[0]:
                self._print_ltg_error(cmds.CMD_PARAM_LTG_ERROR, "get status for LTG {0}".format(ltg_id_list))
            ret_val.append(result)

        return ret_val


    def ltg_remove(self, ltg_id_list):
        """Remove the LTG flows.

        Args:
            ltg_id_list (list of int, int): List of LTG flow identifiers or 
                single LTG flow identifier
        """
        if (type(ltg_id_list) is list):
            for ltg_id in ltg_id_list:
                status = self.send_cmd(cmds.LTGRemove(ltg_id))
                self._print_ltg_error(status, "remove LTG {0}".format(ltg_id))
        else:
            status = self.send_cmd(cmds.LTGRemove(ltg_id_list))
            self._print_ltg_error(status, "remove LTG {0}".format(ltg_id_list))


    def ltg_start(self, ltg_id_list):
        """Start the LTG flow.

        Args:
            ltg_id_list (list of int, int): List of LTG flow identifiers or 
                single LTG flow identifier
        """
        if (type(ltg_id_list) is list):
            for ltg_id in ltg_id_list:
                status = self.send_cmd(cmds.LTGStart(ltg_id))
                self._print_ltg_error(status, "start LTG {0}".format(ltg_id))
        else:
            status = self.send_cmd(cmds.LTGStart(ltg_id_list))
            self._print_ltg_error(status, "start LTG {0}".format(ltg_id_list))


    def ltg_stop(self, ltg_id_list):
        """Stop the LTG flow to the given nodes.

        Args:
            ltg_id_list (list of int, int): List of LTG flow identifiers or 
                single LTG flow identifier
        """
        if (type(ltg_id_list) is list):
            for ltg_id in ltg_id_list:
                status = self.send_cmd(cmds.LTGStop(ltg_id))
                self._print_ltg_error(status, "stop LTG {0}".format(ltg_id))
        else:
            status = self.send_cmd(cmds.LTGStop(ltg_id_list))
            self._print_ltg_error(status, "stop LTG {0}".format(ltg_id_list))


    def ltg_remove_all(self):
        """Stops and removes all LTG flows on the node."""
        status = self.send_cmd(cmds.LTGRemove())
        self._print_ltg_error(status, "remove all LTGs")


    def ltg_start_all(self):
        """Start all LTG flows on the node."""
        status = self.send_cmd(cmds.LTGStart())
        self._print_ltg_error(status, "start all LTGs")


    def ltg_stop_all(self):
        """Stop all LTG flows on the node."""
        status = self.send_cmd(cmds.LTGStop())
        self._print_ltg_error(status, "stop all LTGs")


    def _print_ltg_error(self, status, msg):
        """Print an LTG error message.

        Args:
            status (int): Status of LTG command
            msg (str):    Message to print as part of LTG Error
        """
        if (status == cmds.CMD_PARAM_LTG_ERROR):
            print("LTG ERROR: Could not {0} on '{1}'".format(msg, self.description))



    #--------------------------------------------
    # Configure Node Attribute Commands
    #--------------------------------------------
    def reset_all(self):
        """Resets all portions of a node.

        This includes:
        
          * Log
          * Counts
          * Local Traffic Generators (LTGs)
          * Queues
          * BSS (i.e. all network state)
          * Observed Networks

        See the reset() command for a list of all portions of the node that 
        will be reset.
        """
        status = self.reset(log=True,
                            txrx_counts=True,
                            ltg=True,
                            queue_data=True,
                            bss=True,
                            network_list=True)

        if (status == cmds.CMD_PARAM_LTG_ERROR):
            print("LTG ERROR: Could not stop all LTGs on '{0}'".format(self.description))


    def reset(self, log=False, txrx_counts=False, ltg=False, queue_data=False,
                    bss=False, network_list=False ):
        """Resets the state of node depending on the attributes.

        Args:
            log (bool):          Reset the log data.  This will reset both the 
                log data on the node as well as the local variables used to 
                track log reads for ``log_get_all_new()``.  This will 
                not alter any log settings set by ``log_configure()``.
            txrx_counts (bool):  Reset the TX/RX Counts.  This will zero out 
                all of the packet / byte counts as well as the 
                ``latest_txrx_timestamp`` for all nodes in the station info 
                list.  It will also remove all promiscuous counts.
            ltg (bool):          Remove all LTGs.  This will stop and remove
                any LTGs that are on the node.
            queue_data (bool):   Purge all TX queue data.  This will discard
                all currently enqueued packets awaiting transmission at the 
                time the command is received.  This will not discard packets
                already submitted to the lower-level MAC for transmission.
                Also, this will not stop additional packets from sources 
                such as LTGs from being enqueued.
            bss (bool):          Reset network state.  This will nullify
                the node's active BSS and removes all entries from the 
                station info list.  However, it will not remove the BSS from 
                the network list.  This will produce the following OTA
                transmissions:
                
                  * For AP, a deauthentication frame to each associated station
                  * For STA, a disassociation frame to its AP
                  * For IBSS, nothing. 
                    
            network_list (bool): Remove all BSS entries from the network list,
                excluding the BSS entry for the node's current network (if any)            
        """
        flags = 0;

        if log:
            flags += cmds.CMD_PARAM_NODE_RESET_FLAG_LOG
            self.log_total_bytes_read = 0
            self.log_num_wraps        = 0
            self.log_next_read_index  = 0

        if txrx_counts:      flags += cmds.CMD_PARAM_NODE_RESET_FLAG_TXRX_COUNTS
        if ltg:              flags += cmds.CMD_PARAM_NODE_RESET_FLAG_LTG
        if queue_data:       flags += cmds.CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE
        if bss:              flags += cmds.CMD_PARAM_NODE_RESET_FLAG_BSS
        if network_list:     flags += cmds.CMD_PARAM_NODE_RESET_FLAG_NETWORK_LIST

        # Send the reset command
        self.send_cmd(cmds.NodeResetState(flags))


    def get_wlan_mac_address(self):
        """Get the WLAN MAC Address of the node.

        Returns:
            MAC Address (int):  
                Wireless Medium Access Control (MAC) address of the node.
        """
        addr = self.send_cmd(cmds.NodeProcWLANMACAddr(cmds.CMD_PARAM_READ))

        if (addr != self.wlan_mac_address):
            import wlan_exp.util as util

            msg  = "WARNING:  WLAN MAC address mismatch.\n"
            msg += "    Received MAC Address:  {0}".format(util.mac_addr_to_str(addr))
            msg += "    Original MAC Address:  {0}".format(util.mac_addr_to_str(self.wlan_mac_address))
            print(msg)

        return addr


    def set_mac_time(self, time, time_id=None):
        """Sets the MAC time on the node.

        Args:
            time (int):              Time to which the node's MAC time will 
                be set (int in microseconds)
            time_id (int, optional): Identifier used as part of the TIME_INFO 
                log entry created by this command.  If not specified, then a 
                random number will be used.
        """
        if type(time) not in [int, long]:
            raise AttributeError("Time must be expressed in int microseconds")
        
        self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_WRITE, time, time_id))


    def get_mac_time(self):
        """Gets the MAC time from the node.

        Returns:
            time (int):  Timestamp of the MAC time of the node in int microseconds
        """
        node_time = self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_RSVD_TIME))

        return node_time[0]


    def get_system_time(self):
        """Gets the system time from the node.

        Returns:
            Time (int):  Timestamp of the System time of the node in int microseconds
        """
        node_time = self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_RSVD_TIME))

        return node_time[1]


    def enable_beacon_mac_time_update(self, enable):
        """Enable / Disable MAC time update from beacons
        
        Args:
            enable (bool):  ``True`` enables and ``False`` disables MAC time 
                updates from received beacons
        """
        self.send_cmd(cmds.NodeConfigure(beacon_mac_time_update=enable))


    def set_radio_channel(self, channel):
        """Re-tune the node's RF interfaces to a new center frequency. 
        
        This method directly controls the RF hardware. This can be disruptive 
        to MAC operation if the node is currently associated with other nodes 
        in a BSS which manages its own channel state.
        
        This method will immediately change the center frequency of the node's 
        RF interfaces.  As a result this method can only be safely used on a 
        node which is not currently a member of a BSS. To change the center 
        frequency of nodes participating in a BSS use the 
        ``node.configure_bss(channel=N)`` method on every node in the BSS.
        
        Args:
            channel (int):  Channel index for new center frequency. Must be a 
                valid channel index.  See ``wlan_channels`` in util.py for the 
                list of supported channel indexes.
        """
        import wlan_exp.util as util
        
        if channel in util.wlan_channels:
            self.send_cmd(cmds.NodeProcChannel(cmds.CMD_PARAM_WRITE, channel))
        else:
            raise AttributeError("Channel must be in util.py wlan_channels")


    def set_low_to_high_rx_filter(self, mac_header=None, fcs=None):
        """Configures the filter that controls which received packets are 
        passed from CPU Low to CPU High.
        
        The filter will always pass received packets where the destination 
        address matches the node's MAC address. The filter can be configured 
        to drop or pass other packets. This filter effectively controls which 
        packets are written to the node's log.

        Args:
            mac_header (str, optional): MAC header filter.  Values can be:
               
               - ``'MPDU_TO_ME'`` -- Pass all unicast-to-me or multicast data or management packet
               - ``'ALL_MPDU'``   -- Pass all data and management packets; no destination address filter
               - ``'ALL'``        -- Pass all packets; no packet type or address filters
            
            fcs (str, optional): FCS status filter.  Values can be:

               - ``'GOOD'``       -- Pass only packets with good checksum result
               - ``'ALL'``        -- Pass packets with any checksum result

        At boot the filter defaults to ``mac_header='ALL'`` and ``fcs='ALL'``.
        """
        self.send_cmd(cmds.NodeSetLowToHighFilter(cmds.CMD_PARAM_WRITE, mac_header, fcs))


    #------------------------
    # Tx Rate commands

    def set_tx_rate_unicast(self, mcs, phy_mode, device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast packet transmit rate (mcs, phy_mode) of the node.

        When using ``device_list`` or ``curr_assoc``, this method will set the 
        unicast data packet tx rate since only unicast data transmit parameters 
        are maintained for a given station info.  However, when using 
        ``new_assoc``, this method will set both the default unicast data and 
        unicast management packet tx rate.

        Args:
            mcs (int): Modulation and coding scheme (MCS) index (in [0 .. 7])
            phy_mode (str, int): PHY mode.  Must be one of:

                * ``'NONHT'``: Use 802.11 (a/g) rates
                * ``'HTMF'``: Use 802.11 (n) rates

            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to set the 
                unicast packet Tx rate to the rate (mcs, phy_mdoe)
            curr_assoc (bool):  All current station infos will have the unicast 
                packet Tx rate set to the rate (mcs, phy_mode)
            new_assoc  (bool):  All new station infos will have the unicast 
                packet Tx rate set to the rate (mcs, phy_mode)

        One of ``device_list``, ``curr_assoc`` or ``new_assoc`` must be set.  
        The ``device_list`` and ``curr_assoc`` are mutually exclusive with 
        ``curr_assoc`` having precedence (ie if ``curr_assoc`` is True, then 
        ``device_list`` will be ignored).

        The MAC code does not differentiate between unicast management tx 
        parameters and unicast data tx parameters since unicast management 
        packets only occur when they will not materially affect an experiment 
        (ie they are only sent during deauthentication)

        This will not affect the transmit antenna mode for control frames like 
        ACKs that will be transmitted. The rate of control packets is 
        determined by the 802.11 standard.
        """
        if self._check_allowed_rate(mcs=mcs, phy_mode=phy_mode):
            self._node_set_tx_param_unicast(cmds.NodeProcTxRate, 'rate', (mcs, phy_mode), device_list, curr_assoc, new_assoc)
        else:
            self._check_allowed_rate(mcs=mcs, phy_mode=phy_mode, verbose=True)
            raise AttributeError("Tx rate, (mcs, phy_mode) tuple, not supported by the design. See above error message.")


    def set_tx_rate_multicast_data(self, mcs, phy_mode):
        """Sets the multicast data packet transmit rate for a node.

        Args:
            mcs (int): Modulation and coding scheme (MCS) index (in [0 .. 7])
            phy_mode (str, int): PHY mode.  Must be one of:

                * ``'NONHT'``: Use 802.11 (a/g) rates
                * ``'HTMF'``: Use 802.11 (n) rates

        """
        if self._check_allowed_rate(mcs=mcs, phy_mode=phy_mode):
            self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_DATA, (mcs, phy_mode)))
        else:
            self._check_allowed_rate(mcs=mcs, phy_mode=phy_mode, verbose=True)
            raise AttributeError("Tx rate, (mcs, phy_mode) tuple, not supported by the design. See above error message.")


    def set_tx_rate_multicast_mgmt(self, mcs, phy_mode):
        """Sets the multicast management packet transmit rate for a node.

        Args:
            mcs (int): Modulation and coding scheme (MCS) index (in [0 .. 7])
            phy_mode (str, int): PHY mode.  Must be one of:

                * ``'NONHT'``: Use 802.11 (a/g) rates
                * ``'HTMF'``: Use 802.11 (n) rates

        """
        if self._check_allowed_rate(mcs=mcs, phy_mode=phy_mode):
            self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_MGMT, (mcs, phy_mode)))
        else:
            self._check_allowed_rate(mcs=mcs, phy_mode=phy_mode, verbose=True)
            raise AttributeError("Tx rate, (mcs, phy_mode) tuple, not supported by the design. See above error message.")


    def get_tx_rate_unicast(self, device_list=None, new_assoc=False):
        """Gets the unicast packet transmit rate of the node for the given 
        device(s).

        This will get the unicast data packet tx rate (unicast managment 
        packet tx rate is the same).

        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to get the 
                unicast packet Tx rate
            new_assoc  (bool):  Get the unicast packet Tx rate for all new 
                station infos

        Returns:
            rates (List of tuple):  
                List of unicast packet Tx rate tuples, (mcs, phy_mode), for 
                the given devices.

        If both ``new_assoc`` and ``device_list`` are specified, the return 
        list will always have the unicast packet Tx rate for all new 
        station infos as the first item in the list.
        """
        return self._node_get_tx_param_unicast(cmds.NodeProcTxRate, 'rate', None, device_list, new_assoc)


    def get_tx_rate_multicast_data(self):
        """Gets the current multicast data packet transmit rate for a node.

        Returns:
            rate (tuple):  
                Multicast data packet transmit rate tuple, (mcs, phy_mode), 
                for the node
        """
        return self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_DATA))


    def get_tx_rate_multicast_mgmt(self):
        """Gets the current multicast transmit rate for a node.

        Returns:
            rate (tuple):  
                Multicast management packet transmit rate tuple, (mcs, 
                phy_mode), for the node
        """
        return self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_MGMT))


    #------------------------
    # Tx Antenna Mode commands

    def set_tx_ant_mode_unicast(self, ant_mode, device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast packet transmit antenna mode of the node.

        When using ``device_list`` or ``curr_assoc``, this method will set the 
        unicast data packet tx antenna mode since only unicast data transmit 
        parameters are maintained for a given staion info.  However, when 
        using ``new_assoc``, this method will set both the default unicast data 
        and unicast management packet tx antenna mode.

        Args:
            ant_mode (str):  Antenna mode; must be one of:

                * ``'RF_A'``: transmit on RF A interface
                * ``'RF_B'``: transmit on RF B interface

            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to set the 
                unicast packet Tx antenna mode to 'ant_mode'
            curr_assoc (bool):  All current statoin infos will have the unicast 
                packet Tx antenna mode set to 'ant_mode'
            new_assoc  (bool):  All new station infos will have the unicast 
                packet Tx antenna mode set to 'ant_mode'

        One of ``device_list``, ``curr_assoc`` or ``new_assoc`` must be set.  
        The ``device_list`` and ``curr_assoc`` are mutually exclusive with 
        ``curr_assoc`` having precedence. If ``curr_assoc`` is provided 
        ``device_list`` will be ignored.

        The MAC code does not differentiate between unicast management tx 
        parameters and unicast data tx parameters since unicast management 
        packets only occur when they will not materially affect an experiment 
        (ie they are only sent during deauthentication)

        This will not affect the transmit antenna mode for control frames like 
        ACKs that will be transmitted. Control packets will be sent on whatever 
        antenna that cause the control packet to be generated (ie an ack for a 
        reception will go out on the same antenna on which the reception 
        occurred).
        """
        if ant_mode is None:
            raise AttributeError("Invalid ant_mode: {0}".format(ant_mode))
            
        self._node_set_tx_param_unicast(cmds.NodeProcTxAntMode, 'antenna mode', ant_mode, device_list, curr_assoc, new_assoc)


    def set_tx_ant_mode_multicast_data(self, ant_mode):
        """Sets the multicast data packet transmit antenna mode.

        Args:
            ant_mode (str):  Antenna mode; must be one of:

                * ``'RF_A'``: transmit on RF A interface
                * ``'RF_B'``: transmit on RF B interface

        """
        if ant_mode is None:
            raise AttributeError("Invalid ant_mode: {0}".format(ant_mode))
            
        self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_DATA, ant_mode))


    def set_tx_ant_mode_multicast_mgmt(self, ant_mode):
        """Sets the multicast management packet transmit antenna mode.

        Args:
            ant_mode (str):  Antenna mode; must be one of:

                * ``'RF_A'``: transmit on RF A interface
                * ``'RF_B'``: transmit on RF B interface

        """
        if ant_mode is None:
            raise AttributeError("Invalid ant_mode: {0}".format(ant_mode))
            
        self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_MGMT, ant_mode))


    def get_tx_ant_mode_unicast(self, device_list=None, new_assoc=False):
        """Gets the unicast packet transmit antenna mode of the node for the 
        given device(s).

        This will get the unicast data packet Tx antenna mode (unicast 
        managment packet Tx antenna mode is the same).

        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to get the 
                unicast packet Tx antenna mode
            new_assoc  (bool):  Get the unicast packet Tx antenna mode for all 
                new station infos

        Returns:
            ant_modes (List of str):  
                List of unicast packet Tx antenna mode for the given devices; 
                each antenna mode must be one of:

                  * ``'RF_A'``: transmit on RF A interface
                  * ``'RF_B'``: transmit on RF B interface

        If both ``new_assoc`` and ``device_list`` are specified, the return 
        list will always have the unicast packet Tx antenna mode for all new 
        station infos as the first item in the list.
        """
        return self._node_get_tx_param_unicast(cmds.NodeProcTxAntMode, 'antenna mode', None, device_list, new_assoc)


    def get_tx_ant_mode_multicast_data(self):
        """Gets the current multicast data packet transmit antenna mode for 
        a node.

        Returns:
            ant_mode (str):  
                Multicast data packet transmit antenna mode for the node; 
                must be one of:

                  * ``'RF_A'``: transmit on RF A interface
                  * ``'RF_B'``: transmit on RF B interface

        """
        return self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_DATA))


    def get_tx_ant_mode_multicast_mgmt(self):
        """Gets the current multicast management packet transmit antenna mode for a node.

        Returns:
            ant_mode (str):  
                Multicast management packet transmit antenna mode for the node;
                must be one of:

                  * ``'RF_A'``: transmit on RF A interface
                  * ``'RF_B'``: transmit on RF B interface

        """
        return self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_MGMT))


    def set_tx_ant_mode(self, ant_mode):
        """Sets the transmit antenna mode of the node.

        This command will set all transmit antenna mode fields on the node to the same value:
        
            * Default Unicast Management Packet Tx Antenna mode for new station infos
            * Default Unicast Data Packet Tx Tx Antenna mode for new station infos
            * Default Multicast Management Packet Tx Antenna mode for new station infos
            * Default Multicast Data Packet Tx Antenna mode for new station infos

        It will also update the transmit antenna mode of all current station infos on the node.

        Args:
            ant_mode (str):  Antenna mode; must be one of:

                * ``'RF_A'``: transmit on RF A interface
                * ``'RF_B'``: transmit on RF B interface

        """
        if ant_mode is None:
            raise AttributeError("Invalid ant_mode: {0}".format(ant_mode))
            
        self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_NODE_TX_ANT_ALL, ant_mode))


    def get_tx_ant_mode(self):
        """Gets the current default unicast data transmit antenna mode of the 
        node for new station infos.

        Returns:
            ant_mode (str):  
                Current unicast packet Tx antenna mode; must be one of:

                  * ``'RF_A'``: transmit on RF A interface
                  * ``'RF_B'``: transmit on RF B interface

        """
        return self.get_tx_ant_mode_unicast(new_assoc=True)[0]


    #------------------------
    # Rx Antenna Mode commands

    def set_rx_ant_mode(self, ant_mode):
        """Sets the receive antenna mode for a node.

        Args:
            ant_mode (str):  Antenna mode; must be one of:

                * ``'RF_A'``: receive on RF A interface
                * ``'RF_B'``: receive on RF B interface

        """
        if ant_mode is None:
            raise AttributeError("Invalid ant_mode: {0}".format(ant_mode))
            
        self.send_cmd(cmds.NodeProcRxAntMode(cmds.CMD_PARAM_WRITE, ant_mode))


    def get_rx_ant_mode(self):
        """Gets the current receive antenna mode for a node.

        Returns:
            ant_mode (str):  
                Rx Antenna mode; must be one of:

                  * ``'RF_A'``: receive on RF A interface
                  * ``'RF_B'``: receive on RF B interface

        """
        return self.send_cmd(cmds.NodeProcRxAntMode(cmds.CMD_PARAM_READ))


    #------------------------
    # Tx Power commands

    def set_tx_power_unicast(self, power, device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast packet transmit power of the node.

        When using ``device_list`` or ``curr_assoc``, this method will set the 
        unicast data packet tx power since only unicast data transmit 
        parameters are maintained for a given station info.  However, when 
        using ``new_assoc``, this method will set both the default unicast 
        data and unicast management packet tx power.

        Args:
            power (int):  Transmit power in dBm (a value between 
                ``node.max_tx_power_dbm`` and ``node.min_tx_power_dbm``)
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to set the 
                unicast packet Tx power to 'power'
            curr_assoc (bool):  All current station infos will have the unicast 
                packet Tx power set to 'power'
            new_assoc  (bool):  All new station infos will have the unicast 
                packet Tx power set to 'power'

        One of ``device_list``, ``curr_assoc`` or ``new_assoc`` must be set.  
        The ``device_list`` and ``curr_assoc`` are mutually exclusive with 
        ``curr_assoc`` having precedence (ie if ``curr_assoc`` is True, then 
        ``device_list`` will be ignored).

        The MAC code does not differentiate between unicast management tx 
        parameters and unicast data tx parameters since unicast management 
        packets only occur when they will not materially affect an experiment 
        (ie they are only sent during deauthentication)

        This will not affect the transmit power for control frames like ACKs 
        that will be transmitted. To adjust this power, use the 
        ``set_tx_power_ctrl`` command
        """
        self._node_set_tx_param_unicast(cmds.NodeProcTxPower, 'tx power', 
                                        (power, self.max_tx_power_dbm, self.min_tx_power_dbm), 
                                        device_list, curr_assoc, new_assoc)


    def set_tx_power_multicast_data(self, power):
        """Sets the multicast data packet transmit power.

        Args:
            power (int):  Transmit power in dBm (a value between 
                ``node.max_tx_power_dbm`` and ``node.min_tx_power_dbm``)
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_DATA, 
                                           (power, self.max_tx_power_dbm, self.min_tx_power_dbm)))


    def set_tx_power_multicast_mgmt(self, power):
        """Sets the multicast management packet transmit power.

        Args:
            power (int):  Transmit power in dBm (a value between 
                ``node.max_tx_power_dbm`` and ``node.min_tx_power_dbm``)
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_MGMT, 
                                           (power, self.max_tx_power_dbm, self.min_tx_power_dbm)))


    def get_tx_power_unicast(self, device_list=None, new_assoc=False):
        """Gets the unicast packet transmit power of the node for the given 
        device(s).

        This will get the unicast data packet Tx power (unicast managment 
        packet Tx power is the same).

        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to get the 
                unicast packet Tx power
            new_assoc  (bool):  Get the unicast packet Tx power for all new 
                station infos

        Returns:
            tx_powers (List of int):  
                List of unicast packet Tx power for the given devices.

        If both ``new_assoc`` and ``device_list`` are specified, the return 
        list will always have the unicast packet Tx power for all new 
        station infos as the first item in the list.
        """
        return self._node_get_tx_param_unicast(cmds.NodeProcTxPower, 'tx power', 
                                               (0, self.max_tx_power_dbm, self.min_tx_power_dbm), 
                                               device_list, new_assoc)


    def get_tx_power_multicast_data(self):
        """Gets the current multicast data packet transmit power for a node.

        Returns:
            tx_power (int):  
                Multicast data packet transmit power for the node in dBm
        """
        return self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_DATA,
                                                  (0, self.max_tx_power_dbm, self.min_tx_power_dbm)))


    def get_tx_power_multicast_mgmt(self):
        """Gets the current multicast management packet transmit power for a node.

        Returns:
            tx_power (int):  
                Multicast management packet transmit power for the node in dBm
        """
        return self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_MGMT, 
                                                  (0, self.max_tx_power_dbm, self.min_tx_power_dbm)))


    def set_tx_power_ctrl(self, power):
        """Sets the control packet transmit power of the node.

        Only the Tx power of the control packets can be set via wlan_exp.  The 
        rate of control packets is determined by the 802.11 standard and 
        control packets will be sent on whatever antenna that cause the control 
        packet to be generated (ie an ack for a reception will go out on the 
        same antenna on which the reception occurred).

        Args:
            power (int):  Transmit power in dBm (a value between 
                ``node.max_tx_power_dbm`` and ``node.min_tx_power_dbm``)
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_NODE_TX_POWER_LOW, 
                                           (power, self.max_tx_power_dbm, self.min_tx_power_dbm)))


    def set_tx_power(self, power):
        """Sets the transmit power of the node.

        This command will set all transmit power fields on the node to the same 
        value:
        
            * Default Unicast Management Packet Tx Power for new station infos
            * Default Unicast Data Packet Tx Power for new station infos
            * Default Multicast Management Packet Tx Power for new station infos
            * Default Multicast Data Packet Tx Power for new station infos
            * Control Packet Tx Power

        It will also update the transmit power of all current station infos
        that are part of the BSS on the node.

        Args:
            power (int):  Transmit power in dBm (a value between 
                ``node.max_tx_power_dbm`` and ``node.min_tx_power_dbm``)
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_NODE_TX_POWER_ALL, 
                                           (power, self.max_tx_power_dbm, self.min_tx_power_dbm)))


    def get_tx_power(self):
        """Gets the current default unicast data transmit power of the node 
        for new station infos.

        Returns:
            tx_power (int): Current unicast data transmit power in dBm.
        """
        return self.get_tx_power_unicast(new_assoc=True)[0]


    #------------------------
    # Other commands

    def set_low_param(self, param_id, param_values):
        """Set a CPU Low parameter

        This command provides a generic data pipe to set parameters in CPU Low.  
        Currently supported parameters are defined in cmds.py and use the 
        naming convention:  CMD_PARAM_LOW_PARAM_*   In some cases, additional 
        commands have been added to the node that utilize ``set_low_param()``
        in order to add error checking.

        See http://warpproject.org/trac/wiki/802.11/wlan_exp/Extending
        for more information about how to extend ``set_low_param()`` to 
        control additional parameters.

        Args:
            param_id (int):              ID of parameter to be set
            param_values (list of int):  Value(s) to set the parameter
        """
        if(param_values is not None):
            try:
                v0 = param_values[0]
            except TypeError:
                v0 = param_values

            if((type(v0) is not int) and (type(v0) is not long)) or (v0 >= 2**32):
                raise Exception('ERROR: parameter values must be scalar or iterable of ints in [0,2^32-1]!')

        try:
            values = list(param_values)
        except TypeError:
            values = [param_values]

        self.send_cmd(cmds.NodeLowParam(cmds.CMD_PARAM_WRITE, param_id=param_id, param_values=values))


    def set_dcf_param(self, param_name, param_val):
        """Configures parameters of the DCF MAC in CPU Low. 
        
        These parameters are write-only. These parameters only affect nodes 
        running the DCF in CPU Low. Other MAC implementations should ignore 
        these parameter updates.

        Args:
            param_name (str): Name of the param to change (see table below)
            param_val (int): Value to set for param_name (see table below)

        This method currently implements the following parameters:

        .. list-table::
            :header-rows: 1
            :widths: 15 20 60

            * - Name
              - Valid Values
              - Description

            * - ``'rts_thresh'``
              - [0 .. 65535]
              - Threshold in bytes for maximum length 
                packet which will not trigger RTS handshake

            * - ``'short_retry_limit'`` and
                ``'long_retry_limit'``
              - [0 .. 10]
              - DCF retry limits, controls maximum number of retransmissions for short and long packets
                See `retransmissions <http://warpproject.org/trac/wiki/802.11/MAC/Lower/Retransmissions>`_.
                for more details.

            * - ``'phy_cs_thresh'``
              - [0 .. 1023]
              - Physical carrier sensing threshold, in units of digital RSSI. Set to max (1023) 
                to effectively disable physical carrier sensing

            * - ``'cw_exp_min'`` and
                ``'cw_exp_max'``
              - [1 .. 16]
              - Contention window exponent bounds. Contention window is set to random number in [0, (2^CW - 1)], 
                where CW is bounded by [cw_exp_min, cw_exp_max]


        """
        if type(param_name) is not str:
            raise AttributeError("param_name must be a str.  Provided {0}.".format(type(param_name)))
        
        if type(param_val) is not int:
            raise AttributeError("param_val must be an int.  Provided {0}.".format(type(param_val)))
        
        # Process paramater 
        if   (param_name == 'rts_thresh'):
            if ((param_val < 0) or (param_val > 65535)):
                raise AttributeError("'rts_thresh' must be in [0 .. 65535].")
            
            self._check_cpu_low_type(low_type=defaults.WLAN_EXP_LOW_DCF, command_name="set_dcf_param('rts_thresh')")

            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_DCF_RTS_THRESH, param_values=param_val)
            
        elif (param_name == 'short_retry_limit'):
            if ((param_val < 0) or (param_val > 10)):
                raise AttributeError("'short_retry_limit' must be in [0 .. 10].")
                
            self._check_cpu_low_type(low_type=defaults.WLAN_EXP_LOW_DCF, command_name="set_dcf_param('short_retry_limit')")
    
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_DCF_DOT11SHORTRETRY, param_values=param_val)
            
        elif (param_name == 'long_retry_limit'):
            if ((param_val < 0) or (param_val > 10)):
                raise AttributeError("'long_retry_limit' must be in [0 .. 10].")
                
            self._check_cpu_low_type(low_type=defaults.WLAN_EXP_LOW_DCF, command_name="set_dcf_param('long_retry_limit')")
    
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_DCF_DOT11LONGRETRY, param_values=param_val)
            
        elif (param_name == 'phy_cs_thresh'):
            if ((param_val < 0) or (param_val > 1023)):
                raise AttributeError("'phy_cs_thresh' must be in [0 .. 1023].")
            
            self._check_cpu_low_type(low_type=defaults.WLAN_EXP_LOW_DCF, command_name="set_dcf_param('phy_cs_thresh')")
    
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_DCF_PHYSICAL_CS_THRESH, param_values=param_val)
            
        elif (param_name == 'cw_exp_min'):
            if ((param_val < 1) or (param_val > 16)):
                raise AttributeError("'cw_exp_min' must be in [1 .. 16].")
                
            self._check_cpu_low_type(low_type=defaults.WLAN_EXP_LOW_DCF, command_name="set_dcf_param('cw_exp_min')")
    
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MIN, param_values=param_val)
            
        elif (param_name == 'cw_exp_max'):
            if ((param_val < 1) or (param_val > 16)):
                raise AttributeError("'cw_exp_max' must be in [1 .. 16].")
                
            self._check_cpu_low_type(low_type=defaults.WLAN_EXP_LOW_DCF, command_name="set_dcf_param('cw_exp_max')")
    
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MAX, param_values=param_val)
            
        else:
            msg  = "param_name must be one of the following strings:\n"
            msg += "    'rts_thresh', 'short_retry_limit', 'long_retry_limit', \n"
            msg += "    'phy_cs_thresh', 'cw_exp_min', 'cw_exp_max' \n"
            msg += "Provided '{0}'".format(param_name)
            raise AttributeError(msg)


    def configure_pkt_det_min_power(self, enable, power_level=None):
        """Configure Minimum Power Requirement of Packet Detector.

        Args:
            enable (bool):      True/False
            power_level (int):  [-90,-30] dBm
        """
        if enable is False:
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER, param_values=0)
        else:
            if power_level is not None:
                if power_level >= cmds.CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM and power_level <= cmds.CMD_PARAM_NODE_MAX_MIN_PKT_DET_POWER_DBM:
                    param = (1 << 24) | ((power_level+cmds.CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM) & 0xFF)
                    self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER, param_values=param)
                else:
                    msg  = "\nPower level must be in the range of [{0},{1}]\n".format(cmds.CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM, cmds.CMD_PARAM_NODE_MAX_MIN_PKT_DET_POWER_DBM)
                    raise ValueError(msg)
            else:
                msg  = "\nPower level not specified\n"
                raise ValueError(msg)


    def set_phy_samp_rate(self, phy_samp_rate):
        """Sets the PHY sample rate (in MSps)

        Args:
            phy_samp_rate (int):    
                PHY sample rate in MSps (10, 20, 40).  Default is 20 MSps.
        """
        if (phy_samp_rate not in [10, 20, 40]):
            raise AttributeError("'phy_samp_rate' must be in [10, 20, 40].")
            
        self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_PHY_SAMPLE_RATE, param_values=phy_samp_rate)


    def set_random_seed(self, high_seed=None, low_seed=None, gen_random=False):
        """Sets the random number generator seed on the node.

        Args:
            high_seed (int, optional):   Set the random number generator seed 
                on CPU high
            low_seed (int, optional):    Set the random number generator seed 
                on CPU low
            gen_random (bool, optional): If high_seed or low_seed is not 
                provided, then generate a random seed for the generator.
        """
        import random
        max_seed = 2**32 - 1
        min_seed = 0

        if (high_seed is None):
            if gen_random:
                high_seed = random.randint(min_seed, max_seed)
        else:
            if (high_seed > max_seed) or (high_seed < min_seed):
                msg  = "Seed must be an integer between [{0}, {1}]".format(min_seed, max_seed)
                raise AttributeError(msg)

        if (low_seed is None):
            if gen_random:
                low_seed  = random.randint(min_seed, max_seed)
        else:
            if (low_seed > max_seed) or (low_seed < min_seed):
                msg  = "Seed must be an integer between [{0}, {1}]".format(min_seed, max_seed)
                raise AttributeError(msg)

        self.send_cmd(cmds.NodeProcRandomSeed(cmds.CMD_PARAM_WRITE, high_seed, low_seed))


    def enable_dsss(self, enable):
        """Enable / Disable DSSS receptions on the node

        By default DSSS receptions are enabled on the node when possible 
        (ie PHY sample rate is 20 MHz and Channel is in 2.4 GHz band)
        
        Args:
            enable (bool):  
            
              * True - enable DSSS receptions on the node
              * False - disable DSSS receptions on the node
        """
        self.send_cmd(cmds.NodeConfigure(dsss_enable=enable))


    def set_print_level(self, level):
        """Set the verbosity of the wlan_exp output to the node's UART.

        Args:
            level (int):  Printing level (defaults to ``WARNING``)

        Valid print levels can be found in ``wlan_exp.util.uart_print_levels``:
        
          * ``NONE``    - Do not print messages
          * ``ERROR``   - Only print error messages
          * ``WARNING`` - Print error and warning messages
          * ``INFO``    - Print error, warning and info messages
          * ``DEBUG``   - Print error, warning, info and debug messages
        """
        import wlan_exp.util as util
        
        valid_levels = ['NONE', 'ERROR', 'WARNING', 'INFO', 'DEBUG',
                        util.uart_print_levels['NONE'], 
                        util.uart_print_levels['ERROR'],
                        util.uart_print_levels['WARNING'],
                        util.uart_print_levels['INFO'],
                        util.uart_print_levels['DEBUG']]
        
        if (level in valid_levels):
            self.send_cmd(cmds.NodeConfigure(print_level=level))
        else:
            msg  = "\nInvalid print level {0}.  Print level must be one of: \n".format(level)
            msg += "    ['NONE', 'ERROR', 'WARNING', 'INFO', 'DEBUG', \n"
            msg += "     uart_print_levels['NONE'], uart_print_levels['ERROR'],\n"
            msg += "     uart_print_levels['WARNING'], uart_print_levels['INFO'],\n"
            msg += "     uart_print_levels['DEBUG']]"
            raise ValueError(msg)
            


    #--------------------------------------------
    # Internal methods to view / configure node attributes
    #--------------------------------------------
    def _check_allowed_rate(self, mcs, phy_mode, verbose=False):
        """Check that rate parameters are allowed

        Args:
            mcs (int):           Modulation and coding scheme (MCS) index
            phy_mode (str, int): PHY mode (from util.phy_modes)

        Returns:
            valid (bool):  Are all parameters valid?
        """
        return self._check_supported_rate(mcs, phy_mode, verbose)


    def _check_supported_rate(self, mcs, phy_mode, verbose=False):
        """Checks that the selected rate parameters are supported by the
        current MAC and PHY implementation. This method only checks if a
        rate can be used in hardware. The application-specific method
        _check_allowed_rate() should be used to confirm a selected rate is
        both supported and allowed given the currnet MAC and network state.

        Args:
            mcs (int): Modulation and coding scheme (MCS) index (in [0 .. 7])
            phy_mode (str, int): PHY mode.  Must be one of:

                * ``'NONHT'``: Use 802.11 (a/g) rates
                * ``'HTMF'``: Use 802.11 (n) rates

        Returns:
            rate_suppored (bool):  True if the specified rate is supported
        """
        import wlan_exp.util as util

        rate_ok = True

        if ((mcs < 0) or (mcs > 7)):
            if (verbose):
                print("Invalid MCS {0}. MCS must be integer in [0 .. 7]".format(mcs))
            rate_ok = False

        if (phy_mode not in ['NONHT', 'HTMF', util.phy_modes['NONHT'], util.phy_modes['HTMF']]):
            if (verbose):
                print("Invalid PHY mode {0}. PHY mode must be one of ['NONHT', 'HTMF', phy_modes['NONHT'], phy_modes['HTMF']]".format(phy_mode))
            rate_ok = False

        return rate_ok


    def _node_set_tx_param_unicast(self, cmd, param_name, param,
                                         device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast transmit param of the node.

        Args:
            cmd (Cmd):          Command to be used to set param
            param_name (str):   Name of parameter for error messages
            param (int):        Parameter to be set
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to set the Tx 
                unicast param
            curr_assoc (bool):  All current station infos will have Tx unicast 
                param set
            new_assoc  (bool):  All new staion infos will have Tx unicast 
                param set

        One of ``device_list``, ``curr_assoc`` or ``new_assoc`` must be set.  
        The ``device_list`` and ``curr_assoc`` are mutually exclusive with 
        ``curr_assoc`` having precedence (ie if ``curr_assoc`` is True, then 
        ``device_list`` will be ignored).
        """
        if (device_list is None) and (not curr_assoc) and (not new_assoc):
            msg  = "\nCannot set the unicast transmit {0}:\n".format(param_name)
            msg += "    Must specify either a list of devices, all current station infos,\n"
            msg += "    or all new station infos on which to set the {0}.".format(param_name)
            raise ValueError(msg)

        if new_assoc:
            self.send_cmd(cmd(cmds.CMD_PARAM_WRITE_DEFAULT, cmds.CMD_PARAM_UNICAST, param))

        if curr_assoc:
            self.send_cmd(cmd(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_UNICAST, param))
        else:
            if (device_list is not None):
                try:
                    for device in device_list:
                        self.send_cmd(cmd(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_UNICAST, param, device))
                except TypeError:
                    self.send_cmd(cmd(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_UNICAST, param, device_list))


    def _node_get_tx_param_unicast(self, cmd, param_name, param=None, device_list=None, new_assoc=False):
        """Gets the unicast transmit param of the node.

        Args:
            cmd (Cmd):          Command to be used to set param
            param_name (str):   Name of parameter for error messages
            param (int):        Optional parameter to pass information to cmd
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 
                802.11 devices or single 802.11 device for which to get the Tx 
                unicast param
            new_assoc  (bool):  Get the Tx unicast param for all new station infos

        Returns:
            params (List of params):  
                List of Tx unicast param for the given devices.

        If both ``new_assoc`` and ``device_list`` are specified, the return 
        list will always have the Tx unicast rate for all new station infos as 
        the first item in the list.
        """
        ret_val = []

        if (device_list is None) and (not new_assoc):
            msg  = "\nCannot get the unicast transmit {0}:\n".format(param_name)
            msg += "    Must specify either a list of devices or all new station infos\n"
            msg += "    for which to get the {0}.".format(param_name)
            raise ValueError(msg)

        if new_assoc:
            val = self.send_cmd(cmd(cmds.CMD_PARAM_READ_DEFAULT, cmds.CMD_PARAM_UNICAST, param))
            ret_val.append(val)

        if (device_list is not None):
            try:
                for device in device_list:
                    val = self.send_cmd(cmd(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_UNICAST, param, device=device))
                    ret_val.append(val)
            except TypeError:
                val = self.send_cmd(cmd(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_UNICAST, param, device=device_list))
                ret_val.append(val)

        return ret_val


    def _set_bb_gain(self, bb_gain):
        """Sets the the baseband gain.

        Args:
            bb_gain (int):  Baseband gain setting [0,3]
        """
        if bb_gain is not None:
            if (bb_gain >= 0) and (bb_gain <=3):
                self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_BB_GAIN, param_values=bb_gain)
            else:
                msg  = "\nBB Gain must be in the range [0,3]\n"
                raise ValueError(msg)


    def _set_linearity_pa(self, linearity_val):
        """Sets the the PA linearity.

        Args:
            linearity_val (int):  Linearity setting [0,3]
        """
        if linearity_val is not None:
            if (linearity_val >= 0) and (linearity_val <=3):
                self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_LINEARITY_PA, param_values=linearity_val)
            else:
                msg  = "\nBB Linearity must be in the range [0,3]\n"
                raise ValueError(msg)


    def _set_interp_filt_scaling(self, scale_int0=0x10, scale_int1=0x10, scale_srrc=0x10):
        """Sets the the DAC scaling at the output of each stage of interpolations.

        Args:
            scale_int0 (int):  Scaling Stage 0    [0,31]
            scale_int1 (int):  Scaling Stage 0    [0,31]
            scale_srrc (int):  Scaling Stage SRRC [0,31]
        """

        if (scale_int0 >= 0) and (scale_int0 <=31) and (scale_int1 >= 0) and (scale_int1 <=31) and (scale_srrc >= 0) and (scale_srrc <=31):
            self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_AD_SCALING, param_values=[scale_int0, scale_int1, scale_srrc])
        else:
            msg  = "\nInterp scalings must be in the range [0,31]\n"
            raise ValueError(msg)


    def _set_linearity_vga(self, linearity_val):
        """Sets the the VGA linearity.

        Args:
            linearity_val (int):  Linearity setting [0,3]
        """
        if linearity_val is not None:
            if (linearity_val >= 0) and (linearity_val <=3):
                self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_LINEARITY_VGA, param_values=linearity_val)
            else:
                msg  = "\nBB Linearity must be in the range [0,3]\n"
                raise ValueError(msg)


    def _set_linearity_upconv(self, linearity_val):
        """Sets the the upconversion linearity.

        Args:
            linearity_val (int):  Linearity setting [0,3]
        """
        if linearity_val is not None:
            if (linearity_val >= 0) and (linearity_val <=3):
                self.set_low_param(param_id=cmds.CMD_PARAM_LOW_PARAM_LINEARITY_UPCONV, param_values=linearity_val)
            else:
                msg  = "\nBB Linearity must be in the range [0,3]\n"
                raise ValueError(msg)



    #--------------------------------------------
    # Scan Commands
    #--------------------------------------------
    def set_scan_parameters(self, time_per_channel=None, num_probe_tx_per_channel=None, channel_list=None, ssid=None):
        """Set the paramters of the wireless network scan state machine.
        
        Args:
            time_per_channel (float, optional): Time (in float sec) to spend 
                on each channel.  A value of None will not modify the current
                time per channel
            num_probe_tx_per_channel (int, optional):   Number of probe requests 
                transmitted while on each channel.  A value of None will not 
                modify the current number of probe requests per channel.
            channel_list (list of int optional): Channel(s) to scan; A value 
                of None will not modify the current channel list.
            ssid (str, optional):  SSID to scan for (as part of probe request);
                A value of None will not modify the current SSID.
        
        Setting ``num_probe_tx_per_chan`` to a non-zero value will enable 
        active scanning. The node will transmit broadcast Probe Request packets 
        on every channel and will gather network information from received 
        Probe Response and Beacon packets. Setting ``num_probe_tx_per_chan=0`` 
        will enable passive scanning. In this mode the node will not transmit 
        any Probe Request packets and network information will be gathered only 
        from received Beacon packets.

        The blank SSID (``ssid=""``) is interpretted as a wildcard and will 
        solicit Probe Response packets from networks with any SSID. "Closed" 
        networks do not respond to the wildcard SSID. These networks will still 
        be discovered via Beacon receptions.

        If the channel list / SSID is not specified, then it will not be 
        updated on the node (ie it will use the current channel list / SSID)

        """
        # Check time_per_channel
        if time_per_channel is not None:
            try:
                time_per_channel = float(time_per_channel)
            except:
                raise AttributeError("time_per_channel must be expressable as a float.")
                
        # Check channel_list
        if channel_list is not None:
            tmp_chan_list = []
            
            if type(channel_list) is str:
                # Process pre-defined strings
                import wlan_exp.util as util

                if   (channel_list == 'ALL'):
                    tmp_chan_list = util.wlan_channels
                elif (channel_list == 'ALL_2.4GHZ'):
                    tmp_chan_list = [x for x in util.wlan_channels if (x <= 14)]
                elif (channel_list == 'ALL_5GHZ'):
                    tmp_chan_list = [x for x in util.wlan_channels if (x > 14)]
                else:
                    msg  = "\n    String '{0}' not recognized.".format(channel_list)
                    msg += "\n    Please use 'ALL', 'ALL_2.4GHZ', 'ALL_5GHZ' or either an int or list of channels"
                    raise AttributeError(msg)

            elif type(channel_list) is int:
                # Proess scalar integer
                tmp_chan_list.append(channel_list)
                
            else:
                # Proess interables
                try:
                    for channel in channel_list:
                        tmp_chan_list.append(channel)
                except:
                    msg  = "\n    Unable to process channel_list."
                    msg += "\n    Please use 'ALL', 'ALL_2.4GHZ', 'ALL_5GHZ' or either an int or list of channels"
                    raise AttributeError(msg)

            if tmp_chan_list:
                channel_list = tmp_chan_list
            else:
                msg  = "\n    channel_list is empty."
                msg += "\n    Please use 'ALL', 'ALL_2.4GHZ', 'ALL_5GHZ' or either an int or list of channels"
                raise AttributeError(msg)
                
        self.send_cmd(cmds.NodeProcScanParam(cmds.CMD_PARAM_WRITE, time_per_channel, num_probe_tx_per_channel, channel_list, ssid))
    
    
    def start_network_scan(self):
        """Starts the wireless network scan state machine at the node. 
        
        During a scan, the node cycles through a set of channels and transmits 
        periodic Probe Request frames on each channel.  Information about 
        available wireless networks is extracted from received Probe Responses 
        and Beacon frames. The network scan results can be queried any time 
        using the ``node.get_network_list()`` method.

        Network scans can only be run by unassociated nodes. An associated 
        node must first reset its BSS state before starting a scan.

        The network scan state machine can be stopped with 
        ``node.stop_network_scan()``. The scan state machine will also be 
        stopped automatically if the node is configured with a new non-null 
        BSS state.
            
        Example:
        ::

            # Ensure node has null BSS state
            n.configure_bss(None)

            # Start the scan state machine; scan will use default scan params
            #  Use n.set_scan_parameters() to customize scan behavior
            n.start_network_scan()

            # Wait 5 seconds, retrieve the list of discovered networks
            time.sleep(5)
            networks = n.get_network_list()

            # Stop the scan state machine
            n.stop_network_scan()

        """
        self.send_cmd(cmds.NodeProcScan(enable=True))
    
    
    def stop_network_scan(self):
        """Stops the wireless network scan state machine."""
        self.send_cmd(cmds.NodeProcScan(enable=False))


    def is_scanning(self):
        """Queries if the node's wireless network scanning state machine is 
        currently running.
        
        Returns:
            status (bool):

                * True      -- Scan state machine is running
                * False     -- Scan state machine is not running
        """
        return self.send_cmd(cmds.NodeProcScan())



    #--------------------------------------------
    # Association Commands
    #--------------------------------------------
    def configure_bss(self):
        """Configure the Basic Service Set (BSS) information of the node
        
        Each node is either a member of no BSS (colloquially "unassociated") 
        or a member of one BSS.  A node requires a minimum valid bss_info to 
        be a member of a BSS.  Based on the node type, there is a minimum 
        set of fields needed for a valid bss_info.  
        
        This method must be overloaded by sub-classes.
        
        See http://warpproject.org/trac/wiki/802.11/wlan_exp/bss for more 
        information about BSSes.
        """
        raise NotImplementedError()

    def get_station_info(self, device_list=None):
        
        ret_val = self.get_bss_members()
        return ret_val
        
    def get_bss_members(self):
        """Get the BSS members from the node.

        The StationInfo() returned by this method can be accessed like a 
        dictionary and has the following fields:
        
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | Field                       | Description                                                                                        |
            +=============================+====================================================================================================+
            | mac_addr                    |  MAC address of station                                                                            |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | id                          |  Identification Index for this station                                                             |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | host_name                   |  String hostname (19 chars max), taken from DHCP DISCOVER packets                                  |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | flags                       |  Station flags.  Value containts 1 bit fields:                                                     |
            |                             |      * 0x0001 - 'KEEP'                                                                             |
            |                             |      * 0x0002 - 'DISABLE_ASSOC_CHECK'                                                              |
            |                             |      * 0x0004 - 'DOZE'                                                                             |
            |                             |      * 0x0008 - 'HT_CAPABLE'                                                                       |
            |                             |                                                                                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_rx_timestamp         |  Value of System Time in microseconds of last successful Rx from device                            |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_txrx_timestamp       |  Value of System Time in microseconds of last Tx or successful Rx from device                      |            
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_rx_seq               |  Sequence number of last packet received from device                                               |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_mcs                  |  Current PHY MCS index in [0:7] for new transmissions to device                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_mode                 |  Current PHY mode for new transmissions to deivce                                                  |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_antenna_mode         |  Current PHY antenna mode in [1:4] for new transmissions to device                                 |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_power                |  Current Tx power in dBm for new transmissions to device                                           |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_mac_flags                |  Flags for Tx MAC config for new transmissions to device.  Value contains 1 bit flags:             |
            |                             |                                                                                                    |
            |                             |      * None defined                                                                                |
            |                             |                                                                                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
        
        Returns:
            station_infos (list of StationInfo):  
                List of StationInfo() BSS members known to the node

        """
        ret_val = self.send_cmd(cmds.NodeGetBSSMembers())

        return ret_val


    def get_station_info_list(self):
        """Get the all Station Infos from node.

        The StationInfo() returned by this method can be accessed like a 
        dictionary and has the following fields:
        
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | Field                       | Description                                                                                        |
            +=============================+====================================================================================================+
            | mac_addr                    |  MAC address of station                                                                            |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | id                          |  Identification Index for this station                                                             |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | host_name                   |  String hostname (19 chars max), taken from DHCP DISCOVER packets                                  |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | flags                       |  Station flags.  Value containts 1 bit fields:                                                     |
            |                             |      * 0x0001 - 'KEEP'                                                                             |
            |                             |      * 0x0002 - 'DISABLE_ASSOC_CHECK'                                                              |
            |                             |      * 0x0004 - 'DOZE'                                                                             |
            |                             |      * 0x0008 - 'HT_CAPABLE'                                                                       |
            |                             |                                                                                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_rx_timestamp         |  Value of System Time in microseconds of last successful Rx from device                            |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_txrx_timestamp       |  Value of System Time in microseconds of last Tx or successful Rx from device                      |            
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_rx_seq               |  Sequence number of last packet received from device                                               |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_mcs                  |  Current PHY MCS index in [0:7] for new transmissions to device                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_mode                 |  Current PHY mode for new transmissions to deivce                                                  |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_antenna_mode         |  Current PHY antenna mode in [1:4] for new transmissions to device                                 |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_phy_power                |  Current Tx power in dBm for new transmissions to device                                           |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | tx_mac_flags                |  Flags for Tx MAC config for new transmissions to device.  Value contains 1 bit flags:             |
            |                             |                                                                                                    |
            |                             |      * None defined                                                                                |
            |                             |                                                                                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
        
        Returns:
            station_infos (list of StationInfo):  
                List of all StationInfo() known to the node

        """
        ret_val = self.send_cmd(cmds.NodeGetStationInfoList())

        return ret_val

    def get_bss_info(self):
        """Get the node's BSS info 

        The BSSInfo() returned by this method can be accessed like a 
        dictionary and has the following fields:
        
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | Field                       | Description                                                                                        |
            +=============================+====================================================================================================+
            | bssid                       |  BSS ID: 48-bit MAC address                                                                        |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | ssid                        |  SSID (32 chars max)                                                                               |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | channel                     |  Primary channel.  In util.wlan_channels = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48]     |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | channel_type                |  Channel Type.  Value is one of:                                                                   |
            |                             |                                                                                                    |
            |                             |      * 0x00 - 'BW20'                                                                               |
            |                             |      * 0x01 - 'BW40_SEC_BELOW'                                                                     |
            |                             |      * 0x02 - 'BW40_SEC_ABOVE'                                                                     |
            |                             |                                                                                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_beacon_rx_time       |  Value of System Time in microseconds of last beacon Rx                                            |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | latest_beacon_rx_power      |  Last observed beacon Rx Power (dBm)                                                               |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | capabilities                |  Supported capabilities of the BSS.  Value contains 1 bit fields:                                  |
            |                             |                                                                                                    |
            |                             |      * 0x0001 - 'ESS'                                                                              |
            |                             |      * 0x0002 - 'IBSS'                                                                             |
            |                             |      * 0x0004 - 'HT_CAPABLE'                                                                       |
            |                             |      * 0x0010 - 'PRIVACY'                                                                          |
            |                             |                                                                                                    |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            | beacon_interval             |  Beacon interval - In time units of 1024 us'                                                       |
            +-----------------------------+----------------------------------------------------------------------------------------------------+
            
        Returns:
            bss_info (BSSInfo):  
                BSS Info of node (can be None if unassociated)
        """
        ret_val = self.send_cmd(cmds.NodeGetBSSInfo())
        
        if (len(ret_val) == 1):
            ret_val = ret_val[0]
        else:
            ret_val = None

        return ret_val


    def get_network_list(self):
        """Get a list of known networks (BSSInfo()s) on the node

        Returns:
            networks (list of BSSInfo):  
                List of BSSInfo() that are known to the node 
        """
        return self.send_cmd(cmds.NodeGetBSSInfo("ALL"))



    #--------------------------------------------
    # Queue Commands
    #--------------------------------------------
    def queue_tx_data_purge_all(self):
        """Purges all data transmit queues on the node.
        
        This will discard all currently enqueued packets awaiting transmission 
        at the time the command is received.  This will not discard packets
        already submitted to the lower-level MAC for transmission.  Also, this 
        will not stop additional packets from sources such as LTGs from being 
        enqueued.
        
        This command is equivalent to ``reset(queue_data=True)``.
        """
        self.send_cmd(cmds.QueueTxDataPurgeAll())



    #--------------------------------------------
    # Braodcast Commands can be found in util.py
    #--------------------------------------------



    #--------------------------------------------
    # Node User Commands
    #--------------------------------------------
    def send_user_command(self, cmd_id, args=None):
        """Send User defined command to the node

        See documentation on how-to extend wlan_exp:
        http://warpproject.org/trac/wiki/802.11/wlan_exp/Extending

        Args:
            cmd_id (u32):  User-defined Command ID
            args (u32, list of u32):  Scalar or list of u32 command arguments 
                to send to the node

        Returns:
            resp_args (list of u32):  
                List of u32 response arguments received from the node
        """
        if cmd_id is None:
            raise AttributeError("Command ID must be defined for a user command")

        ret_val = self.send_cmd(cmds.UserSendCmd(cmd_id=cmd_id, args=args))

        if ret_val is not None:
            return ret_val
        else:
            return []



    #--------------------------------------------
    # Memory Access Commands - For developer use only
    #--------------------------------------------
    def _mem_write_high(self, address, values):
        """Writes 'values' to CPU High memory starting at 'address'

        Args:
            address (int):         Address must be in [0 .. (2^32 - 1)]
            values (list of int):  Each value must be in [0 .. (2^32 - 1)]
        """
        # Convert scalar values to a list for processing
        if (type(values) is not list):
            values = [values]

        if (self._check_mem_access_args(address, values)):
            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_WRITE, high=True, address=address, length=len(values), values=values))


    def _mem_read_high(self, address, length):
        """Reads 'length' values from CPU High memory starting at 'address'

        Args:
            address (int):  Address must be in [0 .. (2^32 - 1)]
            length (int):   Length must be in [1 .. 320] (ie fit in a 1400 byte packet)

        Returns:
            values (list of u32):  List of u32 values received from the node
        """
        if (self._check_mem_access_args(address, values=None)):
            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_READ, high=True, address=address, length=length))


    def _mem_write_low(self, address, values):
        """Writes 'values' to CPU Low memory starting at 'address'

        Args:
            address (int):         Address must be in [0 .. (2^32 - 1)]
            values (list of int):  Each value must be in [0 .. (2^32 - 1)]
        """
        # Convert scalar values to a list for processing
        if (type(values) is not list):
            values = [values]

        if (self._check_mem_access_args(address, values)):
            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_WRITE, high=False, address=address, length=len(values), values=values))


    def _mem_read_low(self, address, length):
        """Reads 'length' values from CPU Low memory starting at 'address'

        Args:
            address (int):  Address must be in [0 .. (2^32 - 1)]
            length (int):   Length must be in [1 .. 320] (ie fit in a 1400 byte packet)

        Returns:
            values (list of u32):  List of u32 values received from the node
        """
        if (self._check_mem_access_args(address, values=None)):
            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_READ, high=False, address=address, length=length))


    def _check_mem_access_args(self, address, values=None, length=None):
        """Check memory access variables

        Args:
            address (int):         Address must be in [0 .. (2^32 - 1)]
            values (list of int):  Each value must be in [0 .. (2^32 - 1)]
            length (int):          Length must be in [1 .. 320] (ie fit in a 1400 byte packet)

        Returns:
            valid (bool):  Are all arguments valid?
        """
        if ((int(address) != address) or (address >= 2**32) or (address < 0)):
            raise Exception('ERROR: address must be integer value in [0 .. (2^32 - 1)]!')

        if (values is not None):
            if (type(values) is not list):
                values = [values]

            error = False

            for value in values:
                if (((type(value) is not int) and (type(value) is not long)) or
                    (value >= 2**32) or (value < 0)):
                    error = True

            if (error):
                raise Exception('ERROR: values must be scalar or iterable of ints in [0 .. (2^32 - 1)]!')

        if length is not None:
            if ((int(length) != length) or (length > 320) or (length <= 0)):
                raise Exception('ERROR: length must be an integer [1 .. 320] words (ie, 4 to 1400 bytes)!')

        return True


    def _eeprom_write(self, address, values):
        """Writes 'values' to EEPROM starting at 'address'

        Args:
            address (int):         Address must be in [0 .. 15999]
            values (list of int):  Each value must be in [0 .. 255]
        """
        # Convert scalar values to a list for processing
        if (type(values) is not list):
            values = [values]

        if (self._check_eeprom_access_args(address=address, values=values, length=len(values))):
            if(address >= 16000):
                raise Exception('ERROR: EEPROM addresses [16000 .. 16383] are read only!')
            else:
                return self.send_cmd(cmds.NodeEEPROMAccess(cmd=cmds.CMD_PARAM_WRITE, address=address, length=len(values), values=values))


    def _eeprom_read(self, address, length):
        """Reads 'length' values from EEPROM starting at 'address'

        Args:
            address (int):  Address must be in [0 .. 16383]
            length (int):   Length must be in [1 .. 320] (ie fit in a 1400 byte packet)

        Returns:
            values (list of u8):  List of u8 values received from the node
        """
        if (self._check_eeprom_access_args(address=address, length=length)):
            return self.send_cmd(cmds.NodeEEPROMAccess(cmd=cmds.CMD_PARAM_READ, address=address, length=length))


    def _check_eeprom_access_args(self, address, values=None, length=None):
        """Check EEPROM access variables

        Args:
            address (int):         Address must be in [0 .. 16383]
            values (list of int):  Each value must be in [0 .. 255]
            length (int):          Length must be in [1 .. 320] (ie fit in a 1400 byte packet)

        Returns:
            valid (bool):  Are all arguments valid?
        """
        if ((int(address) != address) or (address >= 16384) or (address < 0)):
            raise Exception('ERROR: address must be integer value in [0 .. 16383]!')

        if (values is not None):
            if (type(values) is not list):
                values = [values]

            error = False

            for value in values:
                if (((type(value) is not int) and (type(value) is not long)) or
                    (value >= 2**8) or (value < 0)):
                    error = True

            if (error):
                raise Exception('ERROR: values must be scalar or iterable of ints in [0 .. 255]!')

        if length is not None:
            if ((int(length) != length) or (length > 320) or (length <= 0)):
                raise Exception('ERROR: length must be an integer [1 .. 320] words (ie, 4 to 1400 bytes)!')

        return True



    #-------------------------------------------------------------------------
    # Parameter Framework
    #   Allows for processing of hardware parameters
    #-------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if (identifier == NODE_PARAM_ID_WLAN_EXP_VERSION):
            if (length == 1):
                self.wlan_exp_ver_major = (values[0] & 0xFF000000) >> 24
                self.wlan_exp_ver_minor = (values[0] & 0x00FF0000) >> 16
                self.wlan_exp_ver_revision = (values[0] & 0x0000FFFF)

                # Check to see if there is a version mismatch
                self.check_wlan_exp_ver()
            else:
                raise ex.ParameterError("NODE_DESIGN_VER", "Incorrect length")

        elif   (identifier == NODE_PARAM_ID_WLAN_MAC_ADDR):
            if (length == 2):
                self.wlan_mac_address = ((2**32) * (values[1] & 0xFFFF) + values[0])
            else:
                raise ex.ParameterError("NODE_WLAN_MAC_ADDR", "Incorrect length")

        elif   (identifier == NODE_PARAM_ID_WLAN_SCHEDULER_RESOLUTION):
            if (length == 1):
                self.scheduler_resolution = values[0]
            else:
                raise ex.ParameterError("NODE_LTG_RESOLUTION", "Incorrect length")

        elif   (identifier == NODE_PARAM_ID_WLAN_MAX_TX_POWER_DBM):
            if (length == 1):
                # Power is an int transmited as a uint
                if (values[0] & 0x80000000):
                    self.max_tx_power_dbm = values[0] - 2**32
                else:
                    self.max_tx_power_dbm = values[0]
            else:
                raise ex.ParameterError("MAX_TX_POWER_DBM", "Incorrect length")

        elif   (identifier == NODE_PARAM_ID_WLAN_MIN_TX_POWER_DBM):
            if (length == 1):
                # Power is an int transmited as a uint
                if (values[0] & 0x80000000):
                    self.min_tx_power_dbm = values[0] - 2**32
                else:
                    self.min_tx_power_dbm = values[0]
            else:
                raise ex.ParameterError("MIN_TX_POWER_DBM", "Incorrect length")

        else:
            super(WlanExpNode, self).process_parameter(identifier, length, values)


    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def _set_node_type(self, node_type):
        """Set the node_type and keep the device_type in sync."""
        self.node_type   = node_type
        self.device_type = self.node_type

    def _get_node_type_high(self):
        """Get the node type of CPU High

        See defaults.py for a list of CPU High node types
        """
        return (self.node_type & defaults.WLAN_EXP_HIGH_MASK)

    def _get_node_type_low(self):
        """Get the node type of CPU Low

        See defaults.py for a list of CPU Low node types
        """
        return (self.node_type & defaults.WLAN_EXP_LOW_MASK)

    def _check_cpu_high_type(self, high_type, command_name, raise_error=False):
        """Check the node CPU High type against the high_type argument

        Args:
            high_type (int):    Node type for CPU High (see defaults.py)
            command_name (str): Name of command calling function
            raise_error (bool): Raise an exception?
        """
        node_high_type = self._get_node_type_high()

        if (node_high_type != high_type):
            msg  = "WARNING:  CPU High Type mismatch.\n"
            msg += "    Command \'{0}()\' ".format(command_name)

            try:
                msg += "expects {0}, ".format(defaults.WLAN_EXP_HIGH_TYPES[high_type])
            except:
                msg += "expects UNKNOWN TYPE, "

            try:
                msg += "node reports {0}\n".format(defaults.WLAN_EXP_HIGH_TYPES[node_high_type])
            except:
                msg += "reports UNKNOWN TYPE\n"

            msg += "    Command may have unintended effects.\n"

            if (raise_error):
                raise TypeError(msg)
            else:
                print(msg)

    def _check_cpu_low_type(self, low_type, command_name, raise_error=False):
        """Check the node CPU Low type against the low_type argument

        Args:
            low_type (int):     Node type for CPU Low (see defaults.py)
            command_name (str): Name of command calling function
            raise_error (bool): Raise an exception?
        """
        node_low_type = self._get_node_type_low()

        if (node_low_type != low_type):
            msg  = "WARNING:  CPU Low Type mismatch:\n"
            msg += "    Command \'{0}\' ".format(command_name)

            try:
                msg += "expects {0}, ".format(defaults.WLAN_EXP_LOW_TYPES[low_type])
            except KeyError:
                msg += "expects UNKNOWN TYPE, "

            try:
                msg += "node reports {0}\n".format(defaults.WLAN_EXP_LOW_TYPES[node_low_type])
            except KeyError:
                msg += "reports UNKNOWN TYPE\n"

            msg += "    Command may have unintended effects.\n"

            if (raise_error):
                raise TypeError(msg)
            else:
                print(msg)

    def check_wlan_exp_ver(self):
        """Check the wlan_exp version of the node against the current wlan_exp 
        version.
        """
        ver_str     = version.wlan_exp_ver_str(self.wlan_exp_ver_major,
                                               self.wlan_exp_ver_minor,
                                               self.wlan_exp_ver_revision)

        caller_desc = "During initialization '{0}' returned version {1}".format(self.description, ver_str)

        status = version.wlan_exp_ver_check(major=self.wlan_exp_ver_major,
                                            minor=self.wlan_exp_ver_minor,
                                            revision=self.wlan_exp_ver_revision,
                                            caller_desc=caller_desc)

        if (status == version.WLAN_EXP_VERSION_NEWER):
            print("Please update the C code on the node to the proper wlan_exp version.")

        if (status == version.WLAN_EXP_VERSION_OLDER):
            print("Please update the wlan_exp installation to match the version on the node.")


# End Class WlanExpNode




class WlanExpNodeFactory(node.WarpNodeFactory):
    """Sub-class of WarpNodeFactory used to help with node configuration
    and setup.
    """
    def __init__(self, network_config=None):
        super(WlanExpNodeFactory, self).__init__(network_config)

        # Add default classes to the factory
        self.node_add_class(defaults.WLAN_EXP_AP_DCF_TYPE,
                            defaults.WLAN_EXP_AP_DCF_CLASS_INST,
                            defaults.WLAN_EXP_AP_DCF_DESCRIPTION)

        self.node_add_class(defaults.WLAN_EXP_STA_DCF_TYPE,
                            defaults.WLAN_EXP_STA_DCF_CLASS_INST,
                            defaults.WLAN_EXP_STA_DCF_DESCRIPTION)

        self.node_add_class(defaults.WLAN_EXP_IBSS_DCF_TYPE,
                            defaults.WLAN_EXP_IBSS_DCF_CLASS_INST,
                            defaults.WLAN_EXP_IBSS_DCF_DESCRIPTION)

        self.node_add_class(defaults.WLAN_EXP_AP_NOMAC_TYPE,
                            defaults.WLAN_EXP_AP_NOMAC_CLASS_INST,
                            defaults.WLAN_EXP_AP_NOMAC_DESCRIPTION)

        self.node_add_class(defaults.WLAN_EXP_STA_NOMAC_TYPE,
                            defaults.WLAN_EXP_STA_NOMAC_CLASS_INST,
                            defaults.WLAN_EXP_STA_NOMAC_DESCRIPTION)

        self.node_add_class(defaults.WLAN_EXP_IBSS_NOMAC_TYPE,
                            defaults.WLAN_EXP_IBSS_NOMAC_CLASS_INST,
                            defaults.WLAN_EXP_IBSS_NOMAC_DESCRIPTION)


    def node_eval_class(self, node_class, network_config):
        """Evaluate the node_class string to create a node.

        This should be overridden in each sub-class with the same overall 
        structure but a different import.  Please call the super class so that 
        the calls will propagate to catch all node types.

        network_config is used as part of the node_class string to initialize 
        the node.
        """
        # "import wlan_exp.defaults as defaults" performed at the top of the file
        import wlan_exp.node_ap as node_ap
        import wlan_exp.node_sta as node_sta
        import wlan_exp.node_ibss as node_ibss

        node = None

        try:
            node = eval(node_class, globals(), locals())
        except:
            # We will catch all errors that might occur when trying to create
            # the node since this class might not be able to create the node.
            pass

        if node is None:
            return super(WlanExpNodeFactory, self).node_eval_class(node_class,
                                                                   network_config)
        else:
            return node


# End Class WlanExpNodeFactory
