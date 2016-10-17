# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node - IBSS Node
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

import wlan_exp.node as node
import wlan_exp.cmds as cmds


__all__ = ['WlanExpNodeIBSS']


class WlanExpNodeIBSS(node.WlanExpNode):
    """802.11 IBSS functionality for a WLAN Experiment node.
    
    Args:
        network_config (warpnet.NetworkConfiguration): Network configuration of the node
        mac_type (int):                                CPU Low MAC type
    """

    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the IBSS
    #-------------------------------------------------------------------------
    def ibss_configure(self, beacon_ts_update=None, beacon_transmit=None ):
        """Configure the IBSS behavior.

        By default all attributes are set to None.  Only attributes that 
        are given values will be updated on the node.  If an attribute is
        not specified, then that attribute will retain the same value on
        the node.

        Allowed values of the (beacon_ts_update, beacon_transmit) are: 
          * (True, True)
          * (True, False)
          * (False, False)
        
        (ie you must update your timestamps if you want to send beacons)

        Args:
            beacon_ts_update (bool): Enable timestamp updates from beacons (Default value on Node: TRUE)
            beacon_transmit (bool):  Enable beacon transmission (Default value on Node: TRUE)
        """
        self.send_cmd(cmds.NodeIBSSConfigure(beacon_ts_update, beacon_transmit))


    def disassociate(self, device_list=None):
        """Remove associations of devices within the device_list from the association table

        .. note:: This command is not supported by IBSS nodes.  Please use diassociate_all().
            This function will raise a NotImplementedError.
        
        Attributes:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices or single 
                802.11 device for which to disassociate
        """
        msg = "ERROR:  disassociate(device_list) is not supported for IBSS nodes.  Please use disassociate_all()."
        raise NotImplementedError(msg)


    def set_channel(self, channel):
        """Sets the channel of the node.

        .. note:: This command is not supported for IBSS nodes.  Please update the 
            BSS info and have the IBSS node join the new BSS.  This function will
            print a warning and return the current channel.
        
        Args:
            channel (int, dict in util.wlan_channel array): Channel to set on the node
                (either the channel number as an it or an entry in the wlan_channel array)
        
        Returns:
            channel (dict):  Channel dictionary (see util.wlan_channel) that was set on the node.        
        """
        msg  = "WARNING:  set_channel(channel) is not supported for IBSS nodes.\n"
        msg += "    Please update the BSS info and have the IBSS node join the new BSS."
        print(msg)
        return self.get_channel()


    def stats_get_txrx(self, device_list=None, return_zeroed_stats_if_none=True):
        """Get the statistics from the node.

        .. note:: This function has the same implementation as WlanExpNode but 
            different default values.
        
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
        return super(WlanExpNodeIBSS, self).stats_get_txrx(device_list, return_zeroed_stats_if_none)


    def scan_start(self, time_per_channel=0.1, channel_list=None, ssid=None, bssid=None):
        """Scan the channel list once for BSS networks.
        
        Args:
            time_per_channel (float, int, optional):  Time (in float sec or int microseconds) to spend on each channel
            channel_list (list of int, list of dict in util.wlan_channel array, optional): Channel 
                to scan.  Defaults to all channels in util.py wlan_channel array.  Can provide either an 
                entry or list of entries from wlan_channel array or a channel number or list of channel numbers        
            ssid (str, optional):                     SSID to scan for as part of probe request.
                A value of None means that the node will scan for all SSIDs
            bssid (int, optional):                    BSSID to scan for as part of probe request.
                A value of None means that the node will scan for all BSSIDs (address: 0xFFFFFFFFFFFF)

        Returns:
            bss_infos (list of dict):  BSS_INFO dictionaries are defined in wlan_exp.log.entry_types
        """
        import time
        import wlan_exp.util as util

        wait_time = 0
        idle_time_per_loop = 1.0

        if channel_list is None:
            wait_time = time_per_channel * (len(util.wlan_channel) + 1)
        else:
            wait_time = time_per_channel * (len(channel_list) + 1)
        
        self.set_scan_parameters(time_per_channel, idle_time_per_loop, channel_list)
        self.scan_enable(ssid, bssid)
        time.sleep(wait_time)
        self.scan_disable()

        return self.get_bss_info('ASSOCIATED_BSS')        
    
       
    def set_scan_parameters(self, time_per_channel=0.1, idle_time_per_loop=1.0, channel_list=None):
        """Set the scan parameters.
        
        Args:
            time_per_channel (float, int, optional):    Time (in float sec or int microseconds) to spend on each channel
                (defaults to 0.1 sec)
            idle_time_per_loop (float, int, optional):  Time (in float sec or int microseconds) to wait between each scan loop 
                (defaults to 1 sec)
            channel_list (list of int, list of dict in util.wlan_channel array, optional): Channel 
                to scan.  Defaults to all channels in util.py wlan_channel array.  Can provide either an 
                entry or list of entries from wlan_channel array or a channel number or list of channel numbers        
        
        .. note::  If the channel list is not specified, then it will not be updated on the node (ie it will use 
            the current channel list)
        """
        self.send_cmd(cmds.NodeProcScanParam(cmds.CMD_PARAM_WRITE, time_per_channel, idle_time_per_loop, channel_list))
    
    
    def scan_enable(self, ssid=None, bssid=None):
        """Enables the scan utilizing the current scan parameters.
        
        Args:
            ssid (str, optional):                     SSID to scan for as part of probe request.
                A value of None means that the node will scan for all SSIDs
            bssid (int, optional):                    BSSID to scan for as part of probe request.
                A value of None means that the node will scan for all BSSIDs (address: 0xFFFFFFFFFFFF)
        """
        self.send_cmd(cmds.NodeProcScan(enable=True, ssid=ssid, bssid=bssid))
    
    
    def scan_disable(self):
        """Disables the current scan."""
        self.send_cmd(cmds.NodeProcScan(enable=False, ssid=None, bssid=None))


    def join(self, bss_info):
        """Join the provided BSS.
        
        Args:
            bss_info (dict):  BSS_INFO dictionary (defined in wlan_exp.log.entry_types)
                describing the BSS to join
            
        Returns:
            status (bool):  Always returns True.  Command just replaces "my_bss_info" so you always "join" the BSS.
        """
        return self.send_cmd(cmds.NodeProcJoin(bss_info))
    
            
    def scan_and_join(self, ssid, timeout=5.0):
        """Scan for the given network and try to join.
        
        Args:
            ssid (str, optional):            SSID to scan for as part of probe request.
            timeout (float, int, optional):  Time (in float sec or int microseconds) to scan for the network 
                before timing out (defaults to 5.0 sec)

        Returns:
            status (bool):
                * True      -- Inidcates you successfully joined the BSS
                * False     -- Indicates that you were not able to find the BSS
        """
        return self.send_cmd(cmd=cmds.NodeProcScanAndJoin(ssid=ssid, bssid=None, timeout=timeout), timeout=timeout)



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeIBSS object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN EXP IBSS Node:\n"
            msg += "    WLAN MAC addr :  {0}\n".format(mac_addr_to_str(self.wlan_mac_address))
            msg += "    Node ID       :  {0}\n".format(self.node_id)
            msg += "    Serial #      :  {0}\n".format(self.sn_str)
            msg += "    HW version    :  WARP v{0}\n".format(self.hw_ver)
        else:
            msg += "Node not initialized."

        if self.transport is not None:
            msg += "WLAN EXP "
            msg += str(self.transport)

        return msg


    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeIBSS, self).__repr__()
        msg = "WLAN EXP IBSS " + msg
        return msg

# End Class WlanExpNodeAp
