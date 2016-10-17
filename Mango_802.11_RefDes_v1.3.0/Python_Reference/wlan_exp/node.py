# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node
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

"""
import sys

import wlan_exp.warpnet.node as wn_node
import wlan_exp.warpnet.exception as wn_ex

import wlan_exp.version as version
import wlan_exp.defaults as defaults
import wlan_exp.cmds as cmds
import wlan_exp.device as wlan_device


__all__ = ['WlanExpNode', 'WlanExpNodeFactory']


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None


# WLAN Exp Node Parameter Identifiers (Extension of WARPNet Parameter Identifiers)
#   NOTE:  The C counterparts are found in *_node.h
#
# If additional hardware parameters are needed for sub-classes of WlanExpNode, 
# please make sure that the values of these hardware parameters are not reused.
#
NODE_WLAN_EXP_DESIGN_VER               = 6
NODE_WLAN_MAC_ADDR                     = 7
NODE_WLAN_SCHEDULER_RESOLUTION         = 8


class WlanExpNode(wn_node.WnNode, wlan_device.WlanDevice):
    """Class for WLAN Experiment node.
        
    Args:
        network_config (warpnet.NetworkConfiguration): Network configuration of the node
        mac_type (int):                                CPU Low MAC type


    .. Inherited Attributes from WnNode:
        node_type (int)                     : Unique type of the WARPNet node
        node_id (int)                       : Unique identification for this node
        name (str)                          : User specified name for this node (supplied by user scripts)
        description (str)                   : String description of this node (auto-generated)
        serial_number (int)                 : Node's serial number, read from EEPROM on hardware
        fpga_dna (int)                      : Node's FPGA'a unique identification (on select hardware)
        hw_ver (int)                        : WARP hardware version of this node
        wn_ver_major (int)                  : WARPNet Major version running on this node
        wn_ver_minor (int)                  : WARPNet Minor version running on this node
        wn_ver_revision (int)               : WARPNet Revision version running on this node
        transport (warpnet.Transport)       : Node's transport object
        transport_bcast (warpnet.Transport) : Node's broadcast transport object

    .. Inherited Attributes from WlanDevice:
        device_type (int)                   : Unique type of the Wlan Device
        wlan_mac_address (int)              : Wireless MAC address of the node

    .. Module Attributes:
        wlan_scheduler_resolution (int)     : Minimum resolution (in us) of the LTG
        log_max_size (int)                  : Maximum size of event log (in bytes)
        log_total_bytes_read (int)          : Number of bytes read from the event log
        log_num_wraps (int)                 : Number of times the event log has wrapped
        log_next_read_index (int)           : Index in to event log of next read
        wlan_exp_ver_major (int)            : WLAN Exp Major version running on this node
        wlan_exp_ver_minor (int)            : WLAN Exp Minor version running on this node
        wlan_exp_ver_revision (int)         : WLAN Exp Revision version running on this node
        mac_type (int)                      : Value of the MAC type (see wlan_exp.defaults for values)
        
    """
    
    wlan_scheduler_resolution          = None
    
    log_max_size                       = None
    log_total_bytes_read               = None
    log_num_wraps                      = None
    log_next_read_index                = None

    wlan_exp_ver_major                 = None
    wlan_exp_ver_minor                 = None
    wlan_exp_ver_revision              = None

    mac_type                           = None
    
    def __init__(self, network_config=None, mac_type=None):
        super(WlanExpNode, self).__init__(network_config)
        
        (self.wlan_exp_ver_major, self.wlan_exp_ver_minor, 
                self.wlan_exp_ver_revision) = version.wlan_exp_ver()
        
        self.wlan_scheduler_resolution      = 1

        self.log_total_bytes_read           = 0
        self.log_num_wraps                  = 0
        self.log_next_read_index            = 0

        self.mac_type                       = mac_type
    

    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------


    #--------------------------------------------
    # Log Commands
    #--------------------------------------------
    def log_configure(self, log_enable=None, log_wrap_enable=None, 
                            log_full_payloads=None, log_warpnet_commands=None,
                            log_txrx_mpdu=None, log_txrx_ctrl=None):
        """Configure log with the given flags.

        By default all attributes are set to None.  Only attributes that 
        are given values will be updated on the node.  If an attribute is
        not specified, then that attribute will retain the same value on
        the node.

        Args:
            log_enable (bool):           Enable the event log (Default value on Node: TRUE)
            log_wrap_enable (bool):      Enable event log wrapping (Default value on Node: FALSE)
            log_full_payloads (bool):    Record full Tx/Rx payloads in event log (Default value on Node: FALSE)
            log_warpnet_commands (bool): Record WARPNet commands in event log (Default value on Node: FALSE)        
            log_txrx_mpdu (bool):        Enable Tx/Rx log entries for MPDU frames
            log_txrx_ctrl (bool):        Enable Tx/Rx log entries for CTRL frames
        """
        self.send_cmd(cmds.LogConfigure(log_enable, log_wrap_enable, 
                                        log_full_payloads, log_warpnet_commands,
                                        log_txrx_mpdu, log_txrx_ctrl))


    def log_get(self, size, offset=0, max_req_size=2**23):
        """Low level method to get part of the log file as a WnBuffer.
        
        Args:
            size (int):                   Number of bytes to read from the log
            offset (int, optional):       Starting byte to read from the log
            max_req_size (int, optional): Max request size that the transport will fragment
                            the request into.
        
        Returns:
            buffer (warpnet.Buffer):  Data from the log corresponding to the input parameters
        
        
        .. note:: There is no guarentee that this will return data aligned to 
           event boundaries.  Use llog_get_indexes() to get event aligned boundaries.
        
        .. note:: Log reads are not destructive.  Log entries will only be
           destroyed by a log reset or if the log wraps.
        
        .. note:: During a given log_get command, the Ethernet interface of
           the node will not be able to respond to any other Ethernet packets
           that are sent to the node.  This could cause the node to drop 
           incoming packets and cause contention among multiple log consumers.
           Therefore, for large requests, having a smaller max_req_size
           will allow the transport to fragement the command and allow the 
           node to be responsive to multiple hosts.
        
        .. note:: Some basic analysis shows that fragment sizes of 2**23 (8 MB)
           add about 2% overhead to the receive time and each command takes less
           than 1 second (~0.9 sec), which is the default WARPNet transport 
           timeout.
        """
        return self.send_cmd(cmds.LogGetEvents(size, offset), max_req_size=max_req_size)


    def log_get_all_new(self, log_tail_pad=500, max_req_size=2**23):
        """Get all "new" entries in the log.

        Args:
            log_tail_pad (int, optional): Number of bytes from the current end of the "new" entries 
                that will not be read during the call.  This is to deal with the case where the node 
                is in the process of modifying the last log entry so it my contain incomplete data 
                and should not be read.
        
        Returns:
            buffer (warpnet.Buffer): Data from the log that contains all entries since the last time the log was read.
        """
        import wlan_exp.warpnet.message as wn_message
        
        return_val = wn_message.Buffer()
        (next_index, _, num_wraps) = self.log_get_indexes()

        if ((self.log_next_read_index == 0) and (self.log_num_wraps == 0)):
            # This is the first read of the log by this python object
            if (num_wraps != 0):
                # Need to advance the num_wraps to the current num_wraps so 
                # that we don't get into a bad state with log reading.
                msg  = "\nWARNING:  On first read, the log on the node has already wrapped.\n"
                msg += "    Skipping the first {0} wraps of log data.\n".format(num_wraps)
                print(msg)
                self.log_num_wraps = num_wraps

        if (num_wraps == self.log_num_wraps):
            if (next_index > (self.log_next_read_index + log_tail_pad)):
                # Get Log data from the node
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
            if ((next_index != 0) or self.log_is_full()):
                # Get Log data from the node
                return_val = self.log_get(offset=self.log_next_read_index, 
                                          size=cmds.CMD_PARAM_LOG_GET_ALL_ENTRIES, 
                                          max_req_size=max_req_size)

                # Unfortunately, we do not know how much data should have
                # been returned from the node, but it should not be zero
                read_size  = return_val.get_buffer_size()
                if (read_size > 0):
                    self.log_next_read_index = 0
                    self.log_num_wraps       = num_wraps
                else:
                    print("WARNING:  Not able to read data from node.")                

        return return_val


    def log_get_size(self):
        """Get the size of the log (in bytes).
        
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
        """Get the capacity of the log (in bytes).
        
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
            
        .. note:: Flag values can be found in wlan_exp.cmds.CMD_PARAM_LOG_CONFIG_FLAG_*
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


    def log_enable_stream(self, port, ip_address=None, host_id=None):
        """Configure the node to stream log entries to the given port.
        
        Args:
            port (int): Port number to stream the log entries.
            ip_address (int, str, optional):  IP address to stream the entries.
            host_id (int, optional):  Host ID to be used in the WARPNet transport header for the streamed entries.
        
        .. note:: If ip_address or host_id is not provided, then ip_address and host_id of the
            WLAN Exp host will be used.
        """

        if (ip_address is None):
            import wlan_exp.warpnet.util as wn_util
            ip_address = wn_util._get_host_ip_addr_for_network(self.network_config)
            
        if (host_id is None):
            host_id = self.network_config.get_param('host_id')
        
        self.send_cmd(cmds.LogStreamEntries(1, host_id, ip_address, port))
        msg  = "{0}:".format(self.description) 
        msg += "Streaming Log Entries to {0} ({1}) with host ID {2}".format(ip_address, port, host_id)
        print(msg)


    def log_disable_stream(self):
        """Configure the node to disable log entries stream."""
        self.send_cmd(cmds.LogStreamEntries(0, 0, 0, 0))
        msg  = "{0}:".format(self.description) 
        msg += "Disable log entry stream."
        print(msg)


    def log_write_exp_info(self, info_type, message=None):
        """Write the experiment information provided to the log.
        
        Args:
            info_type (int): Type of the experiment info.  This is an arbitrary 16 bit number 
                chosen by the experimentor
            message (int, str, bytes, optional): Information to be placed in the event log.  
        
        .. note:: Message must be able to be converted to bytearray with 'UTF-8' format.
        """
        self.send_cmd(cmds.LogAddExpInfoEntry(info_type, message))


    def log_write_time(self, time_id=None):
        """Adds the current time in microseconds to the log.
        
        Args:
            time_id (int, optional):  User providied identifier to be used for the TIME_INFO
                log entry.  If none is provided, a random number will be inserted.
        """
        return self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_TIME_ADD_TO_LOG, cmds.CMD_PARAM_RSVD_TIME, time_id))


    def log_write_txrx_stats(self):
        """Write the current statistics to the log."""
        return self.send_cmd(cmds.LogAddStatsTxRx())


    #--------------------------------------------
    # Statistics Commands
    #--------------------------------------------
    def stats_configure_txrx(self, promisc_stats=None):
        """Configure statistics collection on the node.

        Args:
            promisc_stats (bool, optional): Enable promiscuous statistics collection (TRUE/False)
        
        .. note:: By default all attributes are set to None.  Only attributes that 
            are given values will be updated on the node.  If an attribute is
            not specified, then that attribute will retain the same value on the node.
        """
        self.send_cmd(cmds.StatsConfigure(promisc_stats))


    def stats_get_txrx(self, device_list=None, return_zeroed_stats_if_none=False):
        """Get the statistics from the node.
        
        Args:
            device_list (list of WlanExpNode, WlanExpNode, WlanDevice, optional): List of devices
                for which to get statistics.  See note below for more information.
            return_zeroed_stats_if_none(bool, optional):  If no statistics exist on the node for 
                the specified device(s), return a zeroed statistics dictionary with proper timestamps
                instead of None.
        
        Returns:
            staticstics_dictionary (list of dictionaries, dictionary): Statistics for the device(s) specified. 

        .. note:: If the device_list is a single device, then a single dictionary or 
            None is returned.  If the device_list is a list of devices, then a 
            list of dictionaries will be returned in the same order as the 
            devices in the list.  If any of the staistics are not there, 
            None will be inserted in the list.  If the device_list is not 
            specified, then all the statistics on the node will be returned.
        """
        ret_val = []
        if not device_list is None:
            if (type(device_list) is list):
                for device in device_list:
                    stats = self.send_cmd(cmds.StatsGetTxRx(device, return_zeroed_stats_if_none))
                    if (len(stats) == 1):
                        ret_val.append(stats)
                    else:
                        ret_val.append(None)
            else:
                ret_val = self.send_cmd(cmds.StatsGetTxRx(device_list, return_zeroed_stats_if_none))
                if (len(ret_val) == 1):
                    ret_val = ret_val[0]
                else:
                    ret_val = None
        else:
            ret_val = self.send_cmd(cmds.StatsGetTxRx())
        
        return ret_val
    

    #--------------------------------------------
    # Local Traffic Generation (LTG) Commands
    #--------------------------------------------
    def ltg_configure(self, traffic_flow, auto_start=False):
        """Configure the node for the given traffic flow.
        
        Args:
            traffic_flow (ltg.FlowConfig):  FlowConfig (or subclass of FlowConfig) that describes 
                the parameters of the LTG.
            auto_start (bool, optional): Automatically start the LTG or wait until it is explictly started.
        
        Returns:
            ID (int):  Identifier for the LTG flow        
        """
        traffic_flow.enforce_min_resolution(self.wlan_scheduler_resolution)
        return self.send_cmd(cmds.LTGConfigure(traffic_flow, auto_start))


    def ltg_get_status(self, ltg_id_list):
        """Get the status of the LTG flows.
        
        Args:
            ltg_id_list (list of int, int): List of LTG flow identifiers or single LTG flow identifier

        .. note:: If the ltg_id_list is a single ID, then a single status tuple is
            returned.  If the ltg_id_list is a list of IDs, then a list of status 
            tuples will be returned in the same order as the IDs in the list.
        
        Returns:
            status (tuple):
                #. valid (bool):          Is the LTG ID valid? (True/False)
                #. running (bool):        Is the LTG currently running? (True/False)
                #. start_timestamp (int): Timestamp when the LTG started
                #. stop_timestamp (int):  Timestamp when the LTG stopped
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
            ltg_id_list (list of int, int): List of LTG flow identifiers or single LTG flow identifier
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
            ltg_id_list (list of int, int): List of LTG flow identifiers or single LTG flow identifier        
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
            ltg_id_list (list of int, int): List of LTG flow identifiers or single LTG flow identifier        
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
          * Statistics
          * LTG
          * Queues
          * Association State
          * BSS info
        
        See the reset() command for a list of all portions of the node that will
        be reset.
        """
        status = self.reset(log=True, 
                            txrx_stats=True, 
                            ltg=True, 
                            queue_data=True, 
                            associations=True,
                            bss_info=True)

        if (status == cmds.CMD_PARAM_LTG_ERROR):
            print("LTG ERROR: Could not stop all LTGs on '{0}'".format(self.description))
    

    def reset(self, log=False, txrx_stats=False, ltg=False, queue_data=False, 
                   associations=False, bss_info=False ):
        """Resets the state of node depending on the attributes.
        
        Args:
            log (bool):          Reset the log
            txrx_stats (bool):   Reset the TX/RX Statistics
            ltg (bool):          Remove all LTGs
            queue_data (bool):   Purge all TX queue data
            associations (bool): Remove all associations
            bss_info (bool):     Remove all BSS info
        """
        flags = 0;
        
        if log:
            flags += cmds.CMD_PARAM_NODE_RESET_FLAG_LOG
            self.log_total_bytes_read = 0
            self.log_num_wraps        = 0
            self.log_next_read_index  = 0

        if txrx_stats:       flags += cmds.CMD_PARAM_NODE_RESET_FLAG_TXRX_STATS        
        if ltg:              flags += cmds.CMD_PARAM_NODE_RESET_FLAG_LTG        
        if queue_data:       flags += cmds.CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE
        if associations:     flags += cmds.CMD_PARAM_NODE_RESET_FLAG_ASSOCIATIONS
        if bss_info:         flags += cmds.CMD_PARAM_NODE_RESET_FLAG_BSS_INFO
        
        # Send the reset command
        self.send_cmd(cmds.NodeResetState(flags))


    def set_wlan_mac_address(self, mac_address=None):
        """Set the WLAN MAC Address of the node.
        
        This will allow a user to spoof the wireless MAC address of the node.  If
        the mac_address is not provided, then the node will set the wirelss MAC
        address back to the default for the node (ie the Eth A MAC address stored in 
        the EEPROM).  This will perform a "reset_all()" command on the node to 
        remove any existing state to limit any corner cases that might arise from
        changing the wireless MAC address.        
        """
        raise NotImplementedError
        
        # NOTE: We have not worked out all of the reset issues with changing the
        # underlying MAC address; leaving this not implemented for now.
        # 
        # addr = self.send_cmd(cmds.NodeProcWLANMACAddr(cmds.CMD_PARAM_WRITE, mac_address))
        # self.wlan_mac_address = addr


    def get_wlan_mac_address(self):
        """Get the WLAN MAC Address of the node.
        
        Returns:
            MAC Address (int):  Wireless Medium Access Control (MAC) address of the node.        
        """
        addr = self.send_cmd(cmds.NodeProcWLANMACAddr(cmds.CMD_PARAM_READ))

        if (addr != self.wlan_mac_address):
            import wlan_exp.util as util
            
            msg  = "WARNING:  WLAN MAC address mismatch.\n"
            msg += "    Received MAC Address:  {0}".format(util.mac_addr_to_str(addr))
            msg += "    Original MAC Address:  {0}".format(util.mac_addr_to_str(self.wlan_mac_address))
            print(msg)

        return addr


    def set_time(self, time, time_id=None):
        """Sets the time in microseconds on the node.
        
        Args:
            time (float, int):       Time to which the node's timestamp will be set (either float in sec or int in us)
            time_id (int, optional): Identifier used as part of the TIME_INFO log entry created by this command.
                If not specified, then a random number will be used.
        """
        self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_WRITE, time, time_id))
    

    def get_time(self):
        """Gets the time in microseconds from the node.
        
        Returns:
            Time (int):  Timestamp of the node in integer microseconds.        
        """
        return self.send_cmd(cmds.NodeProcTime(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_RSVD_TIME))


    def set_low_to_high_rx_filter(self, mac_header=None, fcs=None):
        """Sets the filter on the packets that are passed from the low MAC to the high MAC.
        
        Args:
            mac_header (str, optional): MAC header filter.  Values can be:
                **'MPDU_TO_ME'** -- Pass any unicast-to-me or multicast data or management packet;
                **'ALL_MPDU'**   -- Pass any data or management packet (no address filter);
                **'ALL'**        -- Pass any packet (no type or address filters)
            fcs (str, optional):        FCS status filter.  Values can be
                **'GOOD'**       -- Pass only packets with good checksum result;
                **'ALL'**        -- Pass packets with any checksum result

        .. note:: The default values on the node are mac_header='ALL_MPDU' and fcs='GOOD'

        .. note::  One thing to note is that even though all packets are passed in the 'ALL' case 
            of the mac_header filter, the high MAC does not necessarily get to decide the node's 
            response to all packets.  For example, ACKs will still be transmitted for receptions
            by the low MAC since there is not enough time in the 802.11 protocol for the high MAC 
            to decide on the response.  Default behavior like this can only be modified in the low MAC.
        """
        self.send_cmd(cmds.NodeSetLowToHighFilter(cmds.CMD_PARAM_WRITE, mac_header, fcs))        
        

    def set_channel(self, channel):
        """Sets the channel of the node.
        
        Args:
            channel (int, dict in util.wlan_channel array): Channel to set on the node
                (either the channel number as an it or an entry in the wlan_channel array)
        
        Returns:
            channel (dict):  Channel dictionary (see util.wlan_channel) that was set on the node.
        """
        return self.send_cmd(cmds.NodeProcChannel(cmds.CMD_PARAM_WRITE, channel))


    def get_channel(self):
        """Gets the current channel of the node.
        
        Returns:
            channel (dict):  Channel dictionary (see util.wlan_channel) of the current channel on the node.        
        """
        return self.send_cmd(cmds.NodeProcChannel(cmds.CMD_PARAM_READ))


    #------------------------
    # Tx Rate commands

    def set_tx_rate_unicast(self, rate, device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast packet transmit rate of the node.
        
        When using device_list or curr_assoc, this method will set the unicast data packet tx rate since
        only unicast data transmit parameters are maintained for a given assoication.  However, when using 
        new_assoc, this method will set both the default unicast data and unicast management packet tx rate.
        
        Args:
            rate (dict from util.wlan_rates):  Rate dictionary 
                (Dictionary from the wlan_rates list in wlan_exp.util)
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to set the unicast packet Tx rate to 'rate'
            curr_assoc (bool):  All current assocations will have the unicast packet Tx rate set to 'rate'
            new_assoc  (bool):  All new associations will have the unicast packet Tx rate set to 'rate'
        
        .. note:: One of device_list, curr_assoc or new_assoc must be set.  The device_list
            and curr_assoc are mutually exclusive with curr_assoc having precedence
            (ie if curr_assoc is True, then device_list will be ignored).
            
        .. note:: WLAN Exp does not differentiate between unicast management tx parameters
            and unicast data tx parameters since unicast management packets only occur when
            they will not materially affect an experiment (ie they are only sent during 
            deauthentication)
            
        .. note:: This will not affect the transmit antenna mode for control frames like ACKs that 
            will be transmitted. The rate of control packets is determined by the 802.11 standard.
        """
        self._node_set_tx_param_unicast(cmds.NodeProcTxRate, rate, 'rate', device_list, curr_assoc, new_assoc)
        

    def set_tx_rate_multicast_data(self, rate):
        """Sets the multicast data packet transmit rate for a node.

        Args:
            rate (dict from util.wlan_rates):  Rate dictionary 
                (Dictionary from the wlan_rates list in wlan_exp.util)
        """
        self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_DATA, rate))


    def set_tx_rate_multicast_mgmt(self, rate):
        """Sets the multicast management packet transmit rate for a node.

        Args:
            rate (dict from util.wlan_rates):  Rate dictionary 
                (Dictionary from the wlan_rates list in wlan_exp.util)
        """
        self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_MGMT, rate))


    def get_tx_rate_unicast(self, device_list=None, new_assoc=False):
        """Gets the unicast packet transmit rate of the node for the given device(s).
        
        This will get the unicast data packet tx rate (unicast managment packet tx rate is the same).

        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to get the unicast packet Tx rate
            new_assoc  (bool):  Get the unicast packet Tx rate for all new associations 
        
        Returns:
            rates (List of dict - Entries from the wlan_rates wlan_exp.util):  List of unicast packet Tx rate for the given devices.
        
        .. note:: If both new_assoc and device_list are specified, the return list will always have 
            the unicast packet Tx rate for all new associations as the first item in the list.
        """
        return self._node_get_tx_param_unicast(cmds.NodeProcTxRate, 'rate', device_list, new_assoc)


    def get_tx_rate_multicast_data(self):
        """Gets the current multicast data packet transmit rate for a node.

        Returns:
            rate (Dict - Entry from the wlan_rates in wlan_exp.util):  Multicast data packet transmit rate for the node
        """
        return self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_DATA))


    def get_tx_rate_multicast_mgmt(self):
        """Gets the current multicast transmit rate for a node.

        Returns:
            rate (Dict - Entry from the wlan_rates in wlan_exp.util):  Multicast management packet transmit rate for the node
        """
        return self.send_cmd(cmds.NodeProcTxRate(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_MGMT))


    #------------------------
    # Tx Antenna Mode commands

    def set_tx_ant_mode_unicast(self, ant_mode, device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast packet transmit antenna mode of the node.

        When using device_list or curr_assoc, this method will set the unicast data packet tx antenna mode since
        only unicast data transmit parameters are maintained for a given assoication.  However, when using 
        new_assoc, this method will set both the default unicast data and unicast management packet tx antenna mode.
        
        Args:
            ant_mode (dict from util.wlan_tx_ant_mode):  Antenna dictionary 
                (Dictionary from the wlan_tx_ant_mode list in wlan_exp.util)
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to set the unicast packet Tx antenna mode to 'ant_mode'
            curr_assoc (bool):  All current assocations will have the unicast packet Tx antenna mode set to 'ant_mode'
            new_assoc  (bool):  All new associations will have the unicast packet Tx antenna mode set to 'ant_mode'
        
        .. note:: One of device_list, curr_assoc or new_assoc must be set.  The device_list
            and curr_assoc are mutually exclusive with curr_assoc having precedence
            (ie if curr_assoc is True, then device_list will be ignored).
            
        .. note:: WLAN Exp does not differentiate between unicast management tx parameters
            and unicast data tx parameters since unicast management packets only occur when
            they will not materially affect an experiment (ie they are only sent during 
            deauthentication)

        .. note:: This will not affect the transmit antenna mode for control frames like ACKs that 
            will be transmitted. Control packets will be sent on whatever antenna that cause the 
            control packet to be generated (ie an ack for a reception will go out on the same antenna 
            on which the reception occurred).
        """
        self._node_set_tx_param_unicast(cmds.NodeProcTxAntMode, ant_mode, 'antenna mode',  device_list, curr_assoc, new_assoc)


    def set_tx_ant_mode_multicast_data(self, ant_mode):
        """Sets the multicast data packet transmit antenna mode.

        Args:
            ant_mode (dict from util.wlan_tx_ant_mode):  Antenna dictionary 
                (Dictionary from the wlan_tx_ant_mode list in wlan_exp.util)
        """
        self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_DATA, ant_mode))


    def set_tx_ant_mode_multicast_mgmt(self, ant_mode):
        """Sets the multicast management packet transmit antenna mode.

        Args:
            ant_mode (dict from util.wlan_tx_ant_mode):  Antenna dictionary 
                (Dictionary from the wlan_tx_ant_mode list in wlan_exp.util)
        """
        self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_MGMT, ant_mode))


    def get_tx_ant_mode_unicast(self, device_list=None, new_assoc=False):
        """Gets the unicast packet transmit antenna mode of the node for the given device(s). 

        This will get the unicast data packet Tx antenna mode (unicast managment packet Tx antenna mode is the same).

        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to get the unicast packet Tx antenna mode
            new_assoc  (bool):  Get the unicast packet Tx antenna mode for all new associations 
        
        Returns:
            ant_modes (List of dict - Entries from the wlan_tx_ant_mode in wlan_exp.util):  List of unicast packet Tx antenna mode for the given devices.
        
        .. note:: If both new_assoc and device_list are specified, the return list will always have 
            the unicast packet Tx antenna mode for all new associations as the first item in the list.
        """
        return self._node_get_tx_param_unicast(cmds.NodeProcTxAntMode, 'antenna mode', device_list, new_assoc)


    def get_tx_ant_mode_multicast_data(self):
        """Gets the current multicast data packet transmit antenna mode for a node.
        
        Returns:
            ant_mode (Dict - Entry from the wlan_tx_ant_mode in wlan_exp.util):  Multicast data packet transmit antenna mode for the node
        """
        return self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_DATA))


    def get_tx_ant_mode_multicast_mgmt(self):
        """Gets the current multicast management packet transmit antenna mode for a node.
        
        Returns:
            ant_mode (Dict - Entry from the wlan_tx_ant_mode in wlan_exp.util):  Multicast management packet transmit antenna mode for the node
        """
        return self.send_cmd(cmds.NodeProcTxAntMode(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_MGMT))


    #------------------------
    # Rx Antenna Mode commands

    def set_rx_ant_mode(self, ant_mode):
        """Sets the receive antenna mode for a node and returns the antenna mode that was set.

        Args:
            ant_mode (dict from util.wlan_rx_ant_mode):          Antenna dictionary 
                (Dictionary from the wlan_rx_ant_mode list in wlan_exp.util)
        """
        self.send_cmd(cmds.NodeProcRxAntMode(cmds.CMD_PARAM_WRITE, ant_mode))


    def get_rx_ant_mode(self):
        """Gets the current receive antenna mode for a node.
        
        Returns:
            ant_mode (dict):  Rx antenna mode dictionary
        """
        return self.send_cmd(cmds.NodeProcRxAntMode(cmds.CMD_PARAM_READ))


    #------------------------
    # Tx Power commands

    def set_tx_power_unicast(self, power, device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast packet transmit power of the node.

        When using device_list or curr_assoc, this method will set the unicast data packet tx power since
        only unicast data transmit parameters are maintained for a given assoication.  However, when using 
        new_assoc, this method will set both the default unicast data and unicast management packet tx power.
        
        Args:
            power (int):  Transmit power in dBm (a value between util.get_node_max_tx_power() and
                util.get_node_min_tx_power())
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to set the unicast packet Tx power to 'power'
            curr_assoc (bool):  All current assocations will have the unicast packet Tx power set to 'power'
            new_assoc  (bool):  All new associations will have the unicast packet Tx power set to 'power'
        
        .. note:: One of device_list, curr_assoc or new_assoc must be set.  The device_list
            and curr_assoc are mutually exclusive with curr_assoc having precedence
            (ie if curr_assoc is True, then device_list will be ignored).
            
        .. note:: WLAN Exp does not differentiate between unicast management tx parameters
            and unicast data tx parameters since unicast management packets only occur when
            they will not materially affect an experiment (ie they are only sent during 
            deauthentication)
                        
        .. note:: This will not affect the transmit power for control frames like ACKs that 
            will be transmitted. To adjust this power, use the set_tx_power_ctrl command
        """
        self._node_set_tx_param_unicast(cmds.NodeProcTxPower, power, 'power', device_list, curr_assoc, new_assoc)


    def set_tx_power_multicast_data(self, power):
        """Sets the multicast data packet transmit power.

        Args:
            power (int):  Transmit power in dBm (a value between util.get_node_max_tx_power() and
                util.get_node_min_tx_power())
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_DATA, power))


    def set_tx_power_multicast_mgmt(self, power):
        """Sets the multicast management packet transmit power.

        Args:
            power (int):  Transmit power in dBm (a value between util.get_node_max_tx_power() and
                util.get_node_min_tx_power())
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_MULTICAST_MGMT, power))


    def get_tx_power_unicast(self, device_list=None, new_assoc=False):
        """Gets the unicast packet transmit power of the node for the given device(s).

        This will get the unicast data packet Tx power (unicast managment packet Tx power is the same).

        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to get the unicast packet Tx power
            new_assoc  (bool):  Get the unicast packet Tx power for all new associations 
        
        Returns:
            tx_powers (List of int):  List of unicast packet Tx power for the given devices.
        
        .. note:: If both new_assoc and device_list are specified, the return list will always have 
            the unicast packet Tx power for all new associations as the first item in the list.
        """
        return self._node_get_tx_param_unicast(cmds.NodeProcTxPower, 'antenna mode', device_list, new_assoc)


    def get_tx_power_multicast_data(self):
        """Gets the current multicast data packet transmit power for a node.
        
        Returns:
            tx_power (int):  Multicast data packet transmit power for the node in dBm
        """
        return self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_DATA))


    def get_tx_power_multicast_mgmt(self):
        """Gets the current multicast management packet transmit power for a node.
        
        Returns:
            tx_power (int):  Multicast management packet transmit power for the node in dBm
        """
        return self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_MULTICAST_MGMT))


    def set_tx_power_ctrl(self, power):
        """Sets the control packet transmit power of the node.
        
        .. note:: Only the Tx power of the control packets can be set from WLAN Exp.  The rate
            of control packets is determined by the 802.11 standard and control packets will
            be sent on whatever antenna that cause the control packet to be generated (ie an 
            ack for a reception will go out on the same antenna on which the reception occurred).
            
        Args:
            power (int):  Transmit power in dBm (a value between util.get_node_max_tx_power() and
                util.get_node_min_tx_power())
        """
        self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_NODE_TX_POWER_LOW, power))


    def set_tx_power(self, power):
        """Sets the transmit power of the node.
        
        This command will set all transmit power fields on the node to the same value:
            * Default Unicast Management Packet Tx Power for new associations
            * Default Unicast Data Packet Tx Power for new associations
            * Default Multicast Management Packet Tx Power for new associations
            * Default Multicast Data Packet Tx Power for new associations
            * Control Packet Tx Power

        It will also update the transmit power of all current associations on the node.

        Args:
            power (int):  Transmit power in dBm (a value between util.get_node_max_tx_power() and
                util.get_node_min_tx_power())
        """
        return self.send_cmd(cmds.NodeProcTxPower(cmds.CMD_PARAM_WRITE, cmds.CMD_PARAM_NODE_TX_POWER_ALL, power))


    def get_tx_power(self):
        """Gets the current default unicast data transmit power of the node for new associations.
        
        Returns:
            tx_power (int): Current unicast data transmit power in dBm.
        """        
        return self.get_tx_power_unicast(new_assoc=True)[0]


    #------------------------
    # Other commands

    def set_dcf_rts_thresh(self, threshold):
        """Sets the RTS length threshold of the node.
        
        Args:
           threshold (int):  Value between [0, (2^32)-1] for the RTS length threshold
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_DCF_RTS_THRESH, threshold)
        
    def set_dcf_short_retry_limit(self, limit):
        """Sets the Short Retry Limit of the node.
        
        See http://warpproject.org/trac/wiki/802.11/MAC/Lower/Retransmissions for more information on 
        retransmissions
        
        Args:
           threshold (int):  Value between [0, (2^32)-1]
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_DCF_DOT11SHORTRETRY, limit)
        
    def set_dcf_long_retry_limit(self, limit):
        """Sets the Long Retry Limit of the node.
        
        See http://warpproject.org/trac/wiki/802.11/MAC/Lower/Retransmissions for more information on 
        retransmissions
        
        Args:
           threshold (int):  Value between [0, (2^32)-1]
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_DCF_DOT11LONGRETRY, limit)

    def set_phy_cs_thresh(self, threshold):
        """Sets the physical carrier sense threshold of the node.
        
        Args:
           threshold (int):  Value between [0, 1023] for the physical carrier sense threshold
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_PHYSICAL_CS_THRESH, threshold)


    def set_timestamp_offset(self, offset):
        """Sets a usec offset that will be applied to beacon timestamps.
        
        Args:
            offset (int):  Integer number of microseconds to adjust the beacon timestamps [-2^31, (2^31 - 1)].
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_TS_OFFSET, offset)      


    def set_cw_exp_min(self, cw_exp):
        """Sets the the minimum contention window:
           
        Args:
            cw_exp (int):  Sets the contention window to [0, 2^(val)] (For example, 1 --> [0,1]; 10 --> [0,1023])
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_CW_EXP_MIN, cw_exp)       


    def set_cw_exp_max(self, cw_exp):
        """Sets the the maximum contention window.

        Args:
            cw_exp (int):  Sets the contention window to [0, 2^(val)] (For example, 1 --> [0,1]; 10 --> [0,1023])
        """
        self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_CW_EXP_MAX, cw_exp)


    def configure_pkt_det_min_power(self, enable, power_level=None): 
        """Configure Minimum Power Requirement of Packet Detector.

        Args:
            enable (bool):      True/False
            power_level (int):  [-90,-30] dBm  
        """                
        if enable is False:            
            self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER, 0)
        else:
            if power_level is not None:
                if power_level >= cmds.CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM and power_level <= cmds.CMD_PARAM_NODE_MAX_MIN_PKT_DET_POWER_DBM:
                    param = (1 << 24) | ((power_level+cmds.CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM) & 0xFF)
                    self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER, param)
                else:
                    msg  = "\nPower level must be in the range of [{0},{1}]\n".format(cmds.CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM, cmds.CMD_PARAM_NODE_MAX_MIN_PKT_DET_POWER_DBM)
                    raise ValueError(msg)
            else:
                msg  = "\nPower level not specified\n"
                raise ValueError(msg)
                

    def set_random_seed(self, high_seed=None, low_seed=None, gen_random=False):
        """Sets the random number generator seed on the node.
        
        Args:
            high_seed (int, optional):   Set the random number generator seed on CPU high
            low_seed (int, optional):    Set the random number generator seed on CPU low
            gen_random (bool, optional): If high_seed or low_seed is not provided, then generate 
                a random seed for the generator.
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


    def enable_dsss(self):
        """Enables DSSS receptions on the node."""
        self.send_cmd(cmds.NodeConfigure(dsss_enable=True))
    
    
    def disable_dsss(self):
        """Disable DSSS receptions on the node."""
        self.send_cmd(cmds.NodeConfigure(dsss_enable=False))


    def set_print_level(self, level):
        """Set the WLAN Exp print level for UART output on the node.
        
        Args:
            level (int):  Printing level (defaults to WLAN_EXP_PRINT_ERROR)

        Valid print levels can be found in wlan_exp.util:  
          * WLAN_EXP_PRINT_NONE
          * WLAN_EXP_PRINT_ERROR
          * WLAN_EXP_PRINT_WARNING
          * WLAN_EXP_PRINT_INFO
          * WLAN_EXP_PRINT_DEBUG
        """
        self.send_cmd(cmds.NodeConfigure(print_level=level))


    #--------------------------------------------
    # Internal helper methods to configure node attributes
    #--------------------------------------------
    def _node_set_tx_param_unicast(self, cmd, param, param_name, 
                                         device_list=None, curr_assoc=False, new_assoc=False):
        """Sets the unicast transmit param of the node.
        
        Args:
            cmd (WnCmd)         Command to be used to set param
            param (int):        Parameter to be set
            param_name (str):   Name of parameter for error messages
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to set the Tx unicast param
            curr_assoc (bool):  All current assocations will have Tx unicast param set
            new_assoc  (bool):  All new associations will have Tx unicast param set 
        
        .. note:: One of device_list, curr_assoc or new_assoc must be set.  The device_list
            and curr_assoc are mutually exclusive with curr_assoc having precedence
            (ie if curr_assoc is True, then device_list will be ignored).
        """
        if (device_list is None) and (not curr_assoc) and (not new_assoc):
            msg  = "\nCannot set the unicast transmit {0}:\n".format(param_name)
            msg += "    Must specify either a list of devices, all current associations,\n"
            msg += "    or all new assocations on which to set the {0}.".format(param_name)
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


    def _node_get_tx_param_unicast(self, cmd, param_name, device_list=None, new_assoc=False):
        """Gets the unicast transmit param of the node.

        Args:
            cmd (WnCmd)         Command to be used to set param
            param_name (str):   Name of parameter for error messages
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices 
                or single 802.11 device for which to get the Tx unicast param
            new_assoc  (bool):  Get the Tx unicast param for all new associations 
        
        Returns:
            params (List of params):  List of Tx unicast param for the given devices.
        
        .. note:: If both new_assoc and device_list are specified, the return list will always have 
            the Tx unicast rate for all new associations as the first item in the list.
        """
        ret_val = []

        if (device_list is None) and (not new_assoc):
            msg  = "\nCannot get the unicast transmit {0}:\n".format(param_name)
            msg += "    Must specify either a list of devices or all new associations\n"
            msg += "    for which to get the {0}.".format(param_name)
            raise ValueError(msg)
        
        if new_assoc:
            val = self.send_cmd(cmd(cmds.CMD_PARAM_READ_DEFAULT, cmds.CMD_PARAM_UNICAST))
            ret_val.append(val)
            
        if (device_list is not None):
            try:
                for device in device_list:
                    val = self.send_cmd(cmd(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_UNICAST, device=device))
                    ret_val.append(val)
            except TypeError:
                val = self.send_cmd(cmd(cmds.CMD_PARAM_READ, cmds.CMD_PARAM_UNICAST, device=device_list))
                ret_val.append(val)        

        return ret_val


    def _set_low_param(self, param, values):
        """Sets any number of CPU Low parameters
 
        Args:
            param (int):          Parameter to be set
            values (list of int): Value(s) to set the parameter
        """
        
        if(values is not None):
            try:
                v0 = values[0]
            except TypeError:
                v0 = values
            
            if((type(v0) is not int) and (type(v0) is not long)) or (v0 >= 2**32):
                raise Exception('ERROR: values must be scalar or iterable of ints in [0,2^32-1]!')  
                
        try:
            vals = list(values)
        except TypeError:
            vals = [values]

        return self.send_cmd(cmds.NodeLowParam(cmds.CMD_PARAM_WRITE, param=param, values=vals))


    #--------------------------------------------
    # Internal methods to view / configure node attributes
    #     NOTE:  These methods are not safe in all cases; therefore they are not part of the public API
    #--------------------------------------------

    def _get_phy_cs_thresh(self):
        """Gets the physical carrier sense threshold of the node.
        
        Returns:
            phy_cs_thresh (int):  Value for the physical carrier sense threshold on the node
        """
        ret_val = self.send_cmd(cmds.NodeLowParam(cmds.CMD_PARAM_READ, param=cmds.CMD_PARAM_LOW_PARAM_PHYSICAL_CS_THRESH))
        
        if ret_val is not None:
            if ((ret_val[0] == 1) and (ret_val[1] == cmds.CMD_PARAM_LOW_PARAM_PHYSICAL_CS_THRESH)):
                return ret_val[2]
            else:
                print("WARNING:  CS Thresh:  Unexpected return value: {0}".format(ret_val))
                return None
        else:
            print("WARNING:  CS Thresh:  Could not get threshold.")
            return None
        

    def _get_timestamp_offset(self):
        """Gets a usec offset that will be applied to beacon timestamps.
        
        Returns:
            offset (int):  Integer number of microseconds beacon timestamps are being adjusted.
        """
        ret_val = self.send_cmd(cmds.NodeLowParam(cmds.CMD_PARAM_READ, param=cmds.CMD_PARAM_LOW_PARAM_TS_OFFSET))
        
        if ret_val is not None:
            if ((ret_val[0] == 1) and (ret_val[1] == cmds.CMD_PARAM_LOW_PARAM_TS_OFFSET)):
                return ret_val[2]
            else:
                print("WARNING:  Timestamp offset:  Unexpected return value: {0}".format(ret_val))
                return None
        else:
            print("WARNING:  Timestamp offset:  Could not get offset.")
            return None


    def _get_cw_exp_range(self):
        """Gets the contention window range.
        
        Returns:
            cw_exps (tuple):
                #. cw_exp_min (int):  Exponent of the minimum contention window
                #. cw_exp_max (int):  Exponent of the maximum contention window
        """
        cw_max = self.send_cmd(cmds.NodeLowParam(cmds.CMD_PARAM_READ, param=cmds.CMD_PARAM_LOW_PARAM_CW_EXP_MAX))
        
        if cw_max is not None:
            if ((cw_max[0] == 1) and (cw_max[1] == cmds.CMD_PARAM_LOW_PARAM_CW_EXP_MAX)):
                cw_max = cw_max[2]
            else:
                print("WARNING:  CW Max:  Unexpected return value: {0}".format(cw_max))
                cw_max = None
        else:
            print("WARNING:  CW Max:  Could not get cw exponent.")
            cw_max = None

        cw_min = self.send_cmd(cmds.NodeLowParam(cmds.CMD_PARAM_READ, param=cmds.CMD_PARAM_LOW_PARAM_CW_EXP_MIN))

        if cw_min is not None:
            if ((cw_min[0] == 1) and (cw_min[1] == cmds.CMD_PARAM_LOW_PARAM_CW_EXP_MIN)):
                cw_min = cw_min[2]
            else:
                print("WARNING:  CW Min:  Unexpected return value: {0}".format(cw_min))
                cw_min = None
        else:
            print("WARNING:  CW Min:  Could not get cw exponent.")
            cw_min = None

        return (cw_min, cw_max)


    def _set_bb_gain(self, bb_gain):
        """Sets the the baseband gain.

        Args:
            bb_gain (int):  Baseband gain setting [0,3]
        """
        if bb_gain is not None:
            if (bb_gain >= 0) and (bb_gain <=3):
                self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_BB_GAIN, bb_gain)                  
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
                self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_LINEARITY_PA, linearity_val)                  
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
            self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_AD_SCALING, [scale_int0,scale_int1,scale_srrc])                  
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
                self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_LINEARITY_VGA, linearity_val)                  
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
                self._set_low_param(cmds.CMD_PARAM_LOW_PARAM_LINEARITY_UPCONV, linearity_val)                  
            else:
                msg  = "\nBB Linearity must be in the range [0,3]\n"                
                raise ValueError(msg)                        



    #--------------------------------------------
    # Association Commands
    #   NOTE:  Some commands are implemented in sub-classes
    #--------------------------------------------
    def get_ssid(self):
        """Command to get the SSID of the node.
        
        Returns:
            ssid (str):  SSID of the node's current BSS.
        """
        return self.send_cmd(cmds.NodeGetSSID())


    def disassociate(self, device_list):
        """Remove associations of devices within the device_list from the association table
        
        Attributes:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices or single 
                802.11 device for which to disassociate
        """
        try:
            for device in device_list:
                self.send_cmd(cmds.NodeDisassociate(device))
        except TypeError:
            self.send_cmd(cmds.NodeDisassociate(device_list))


    def disassociate_all(self):
        """Remove all associations from the association table"""
        self.send_cmd(cmds.NodeDisassociate())


    def is_associated(self, device_list):
        """Is the node associated with the devices in the device list.

        For is_associated to return True, one of the following conditions must be met:
          #. If the device is a wlan_exp node, then both the node and the device must be associated.
          #. If the device is a 802.11 device, then only the node must be associated with the device, since we cannot know the state of
             the 802.11 device.
        
        Returns:
            associated (list of bool):  Boolean describing whether the given device is associated with the node.
            
        .. note:: If the device_list is a single device, then only a boolean is 
            returned.  If the device_list is a list of devices, then a list of
            booleans will be returned in the same order as the devices in the list.        
        """
        ret_val  = [] 
        ret_list = True
        my_info  = self.get_bss_info()           # Get node's BSS info
        my_bssid = my_info['bssid']

        if device_list is not None:            
            # Convert device_list to a list if it is not already one; set flag to not return a list
            if type(device_list) is not list:
                device_list = [device_list]
                ret_list    = False
            
            for device in device_list:
                try:
                    dev_info  = device.get_bss_info()
                    dev_bssid = dev_info['bssid']
                except (AttributeError, KeyError, TypeError):
                    # Cannot get bss_info from device
                    # Need to check the station info of caller to see if device is there
                    my_station_info = self.get_station_info(device)

                    if my_station_info is not None:
                        # Device is in the station info list of the caller; good enough to 
                        # say the two devices are associated.
                        dev_bssid = my_bssid
                    else:
                        # Device is not in the station info list of the caller
                        dev_bssid = None

                # If the bssids are the same, then the nodes are associated
                if (my_bssid == dev_bssid):
                    ret_val.append(True)
                else:
                    ret_val.append(False)
        else:
            ret_val = False

        # Need to return a single value and not a list
        if not ret_list:
            ret_val = ret_val[0]
        
        return ret_val


    def get_station_info(self, device_list=None):
        """Get the station info from the node.
        
        Args:
            device_list (list of WlanExpNode / WlanDevice, optional):  List of 802.11 devices or single 
                802.11 device for which to get the station info        
        
        Returns:
            station_infos (list of dict):  STATION_INFO dictionaries are defined in wlan_exp.log.entry_types

        .. note:: If the device_list is a single device, then only a station info or 
            None is returned.  If the device_list is a list of devices, then a 
            list of station infos will be returned in the same order as the 
            devices in the list.  If any of the station info are not there, 
            None will be inserted in the list.  If the device_list is not 
            specified, then all the station infos on the node will be returned.
        """
        ret_val = []
        if not device_list is None:
            if (type(device_list) is list):
                for device in device_list:
                    info = self.send_cmd(cmds.NodeGetStationInfo(device))
                    if (len(info) == 1):
                        ret_val.append(info)
                    else:
                        ret_val.append(None)
            else:
                ret_val = self.send_cmd(cmds.NodeGetStationInfo(device_list))
                if (len(ret_val) == 1):
                    ret_val = ret_val[0]
                else:
                    ret_val = None
        else:
            ret_val = self.send_cmd(cmds.NodeGetStationInfo())

        return ret_val
    

    def get_bss_info(self, bssid_list=None):
        """Get the BSS info from the node.
        
        Args:
            bssid_list (list of int, optional):  List of BSS IDs (MAC address) or single 
                BSS ID for which to get the BSS info        
        
        Returns:
            bss_infos (list of dict):  BSS_INFO dictionaries are defined in wlan_exp.log.entry_types

        .. note::  If the bssid_list is a single BSS ID, then only a single BSS info or None is 
            returned.  If the bssid_list is a list of BSS IDs, then a list of BSS infos will be 
            returned in the same order as the BSS IDs in the list.  If any of the BSS infos are 
            not there, None will be inserted in the list.  If the bssid_list is not specified, 
            then "my_bss_info" on the node will be returned.  If the the bssid_list is the string 
            "ASSOCIATED_BSS", then all of the BSS infos on the node will be returned.
        """
        ret_val = []
        if not bssid_list is None:
            if (type(bssid_list) is list):
                # Get all BSS infos in the list
                for bssid in bssid_list:
                    info = self.send_cmd(cmds.NodeGetBSSInfo(bssid))
                    if (len(info) == 1):
                        ret_val.append(info)
                    else:
                        ret_val.append(None)
            elif (type(bssid_list) is str):
                # Get all BSS infos on the node
                ret_val = self.send_cmd(cmds.NodeGetBSSInfo("ALL"))
            else:
                # Get single BSS info or return None
                ret_val = self.send_cmd(cmds.NodeGetBSSInfo(bssid_list))
                if (len(ret_val) == 1):
                    ret_val = ret_val[0]
                else:
                    ret_val = None
        else:
            # Get "my_bss_info"
            ret_val = self.send_cmd(cmds.NodeGetBSSInfo())
            if (len(ret_val) == 1):
                ret_val = ret_val[0]
            else:
                ret_val = None

        return ret_val
    


    #--------------------------------------------
    # Queue Commands
    #--------------------------------------------
    def queue_tx_data_purge_all(self):
        """Purges all data transmit queues on the node."""
        self.send_cmd(cmds.QueueTxDataPurgeAll())



    #--------------------------------------------
    # Braodcast Commands can be found in util.py
    #--------------------------------------------



    #--------------------------------------------
    # Memory Access Commands - For developer use only
    #--------------------------------------------
    def _mem_write_high(self, address, values):
        """Writes 'values' directly to CPU High memory starting at 'address'"""
        if(self._check_mem_access_args(address, values)):
            try:
                vals = list(values)
            except TypeError:
                vals = [values]

            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_WRITE, high=True, address=address, length=len(vals), values=vals))


    def _mem_read_high(self, address, length):
        """Reads 'length' values directly CPU High memory starting at 'address'"""
        if(self._check_mem_access_args(address, values=None)):
            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_READ, high=True, address=address, length=length))


    def _mem_write_low(self, address, values):
        """Writes 'values' directly to CPU High memory starting at 'address'"""
        if(self._check_mem_access_args(address, values)):
            try:
                vals = list(values)
            except TypeError:
                vals = [values]

            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_WRITE, high=False, address=address, length=len(vals), values=vals))


    def _mem_read_low(self, address, length):
        """Reads 'length' values directly CPU High memory starting at 'address'"""
        if(self._check_mem_access_args(address, values=None)):
            return self.send_cmd(cmds.NodeMemAccess(cmd=cmds.CMD_PARAM_READ, high=False, address=address, length=length))


    def _check_mem_access_args(self, address, values=None, length=None):
        if(int(address) != address) or (address >= 2**32):
            raise Exception('ERROR: address must be integer value in [0,2^32-1]!')
        
        if(values is not None):
            try:
                v0 = values[0]
            except TypeError:
                v0 = values
            
            if((type(v0) is not int) and (type(v0) is not long)) or (v0 >= 2**32):
                raise Exception('ERROR: values must be scalar or iterable of ints in [0,2^32-1]!')

        if length is not None:
            if (int(length) != length) or (length >= 320):
                raise Exception('ERROR: length must be an integer [0, 320] words (ie, 0 to 1400 bytes)!')

        return True



    #-------------------------------------------------------------------------
    # WARPNet Parameter Framework
    #   Allows for processing of hardware parameters
    #-------------------------------------------------------------------------
    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if (identifier == NODE_WLAN_EXP_DESIGN_VER):
            if (length == 1):                
                self.wlan_exp_ver_major = (values[0] & 0xFF000000) >> 24
                self.wlan_exp_ver_minor = (values[0] & 0x00FF0000) >> 16
                self.wlan_exp_ver_revision = (values[0] & 0x0000FFFF)                
                
                # Check to see if there is a version mismatch
                self.check_wlan_exp_ver()
            else:
                raise wn_ex.ParameterError("NODE_DESIGN_VER", "Incorrect length")

        elif   (identifier == NODE_WLAN_MAC_ADDR):
            if (length == 2):
                self.wlan_mac_address = ((2**32) * (values[1] & 0xFFFF) + values[0])
            else:
                raise wn_ex.ParameterError("NODE_WLAN_MAC_ADDR", "Incorrect length")

        elif   (identifier == NODE_WLAN_SCHEDULER_RESOLUTION):
            if (length == 1):
                self.wlan_scheduler_resolution = values[0]
            else:
                raise wn_ex.ParameterError("NODE_LTG_RESOLUTION", "Incorrect length")

        else:
            super(WlanExpNode, self).process_parameter(identifier, length, values)


    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def _set_node_type(self, node_type):
        """Set the node_type and keep the device_type in sync."""
        self.node_type   = node_type
        self.device_type = self.node_type
    
    
    def check_wlan_exp_ver(self):
        """Check the WLAN Exp version of the node against the current WLAN Exp version."""        
        ver_str     = version.wlan_exp_ver_str(self.wlan_exp_ver_major, 
                                               self.wlan_exp_ver_minor, 
                                               self.wlan_exp_ver_revision)

        caller_desc = "During initialization '{0}' returned version {1}".format(self.description, ver_str)
        
        status = version.wlan_exp_ver_check(major=self.wlan_exp_ver_major,
                                            minor=self.wlan_exp_ver_minor,
                                            revision=self.wlan_exp_ver_revision,
                                            caller_desc=caller_desc)
        
        if (status == version.WLAN_EXP_VERSION_NEWER):
            print("Please update the C code on the node to the proper WLAN Exp version.")
        
        if (status == version.WLAN_EXP_VERSION_OLDER):
            print("Please update the WLAN Exp installation to match the version on the node.")


# End Class WlanExpNode




class WlanExpNodeFactory(wn_node.WnNodeFactory):
    """Sub-class of WARPNet node factory used to help with node configuration 
    and setup.
        
    Attributes:
        wn_dict (dict):  Dictionary of WARPNet Node Types to class names
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
        
        .. note::  This should be overridden in each sub-class with the same overall structure 
            but a different import.  Please call the super class so that the calls will propagate 
            to catch all node types.

        .. note::  network_config is used as part of the node_class string to initialize the node.        
        """
        # NOTE:  "import wlan_exp.defaults as defaults" performed at the top of the file
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
