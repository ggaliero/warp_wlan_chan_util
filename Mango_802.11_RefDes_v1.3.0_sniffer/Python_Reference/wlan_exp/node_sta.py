# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node - Station (STA)
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


__all__ = ['WlanExpNodeSta']


class WlanExpNodeSta(node.WlanExpNode):
    """802.11 Station (STA) functionality for a WLAN Experiment node.
    
    Args:
        network_config (warpnet.NetworkConfiguration): Network configuration of the node
        mac_type (int):                                CPU Low MAC type
    """
    
    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the Node
    #-------------------------------------------------------------------------
    def sta_configure(self, beacon_ts_update=None):
        """Configure the STA behavior.

        By default all attributes are set to None.  Only attributes that 
        are given values will be updated on the node.  If an attribute is
        not specified, then that attribute will retain the same value on
        the node.

        Args:
            beacon_ts_update (bool): Enable timestamp updates from beacons (Default value on Node: TRUE)
        """
        self.send_cmd(cmds.NodeSTAConfigure(beacon_ts_update))


    def disassociate(self, device_list):
        """Remove associations of devices within the device_list from the association table

        .. note:: This command is not supported by IBSS nodes.  Please use diassociate_all().
            This function will raise a NotImplementedError.
        
        Attributes:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices or single 
                802.11 device for which to disassociate
        """
        print("Error:  disassociate(device_list) is not supported for STA nodes.  Please use disassociate_all().")
        raise NotImplementedError


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


    def join(self, bss_info, timeout=5.0):
        """Join the provided BSS.
        
        Args:
            bss_info (dict):                 BSS_INFO dictionary (defined in wlan_exp.log.entry_types)
                describing the BSS to join
            timeout (float, int, optional):  Time (in float sec or int microseconds) to try to join the network 
                before timing out (defaults to 5.0 sec)
            
        Returns:
            status (bool):  
                * True      -- Inidcates you successfully joined the BSS
                * False     -- Indicates that you were not able to join the BSS
        """
        return self.send_cmd(cmds.NodeProcJoin(bss_info, timeout), timeout=timeout)
    
            
    def scan_and_join(self, ssid, timeout=5.0):
        """Scan for the given network and try to join.
        
        Args:
            ssid (str, optional):            SSID to scan for as part of probe request.
            timeout (float, int, optional):  Time (in float sec or int microseconds) to scan for the network 
                before timing out; Time to try to join the network before timing out (defaults to 5.0 sec)

        Returns:
            status (bool):
                * True      -- Inidcates you successfully joined the BSS
                * False     -- Indicates that you were not able to find or join the BSS

        .. note:: The timeout for scan and join is double for a STA.  Basically, the
            timeout is used once for the scan process and then once for the join process.
        """
        return self.send_cmd(cmd=cmds.NodeProcScanAndJoin(ssid=ssid, bssid=None, timeout=timeout), timeout=(2*timeout))



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeSta object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN EXP STA Node:\n"
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
        msg = super(WlanExpNodeSta, self).__repr__()
        msg = "WLAN EXP STA " + msg
        return msg

# End Class WlanExpNodeSta
