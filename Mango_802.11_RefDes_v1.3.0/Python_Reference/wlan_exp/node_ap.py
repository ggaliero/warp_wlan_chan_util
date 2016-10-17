# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Node - Access Point (AP)
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


__all__ = ['WlanExpNodeAp']


class WlanExpNodeAp(node.WlanExpNode):
    """802.11 Access Point (AP) functionality for a WLAN Experiment node.
    
    Args:
        network_config (warpnet.NetworkConfiguration): Network configuration of the node
        mac_type (int):                                CPU Low MAC type
    """

    #-------------------------------------------------------------------------
    # WLAN Exp Commands for the AP
    #-------------------------------------------------------------------------
    def ap_configure(self, support_power_savings=None):
        """Configure the AP behavior.

        By default all attributes are set to None.  Only attributes that 
        are given values will be updated on the node.  If an attribute is
        not specified, then that attribute will retain the same value on
        the node.

        Args:
            support_power_savings (bool): Enable power saving mode; TIM, queue pausing, etc (Default value on Node: TRUE)
        """
        self.send_cmd(cmds.NodeAPConfigure(support_power_savings))


    def set_dtim_period(self, num_beacons):
        """Command to set the number of beacon intervals between DTIM beacons.
        
        Args:
            num_beacons(int): Number of beacon intervals between DTIM beacons. 
                (Requires value between [0, 255])
        """
        return self.send_cmd(cmds.NodeAPProcDTIMPeriod(cmds.CMD_PARAM_WRITE, num_beacons))


    def get_dtim_period(self):
        """Command to get the number of beacon intervals between DTIM beacons."""
        return self.send_cmd(cmds.NodeAPProcDTIMPeriod(cmds.CMD_PARAM_READ))
        

    def set_authentication_address_filter(self, allow):
        """Command to set the authentication address filter on the node.
        
        Args:
            allow (list of tuple) List of (address, mask) tuples that will be used to filter 
                addresses on the node.
    
        .. note::  For the mask, bits that are 0 are treated as "any" and bits that are 1 are 
            treated as "must equal".  For the address, locations of one bits in the mask 
            must match the incoming addresses to pass the filter.
        """
        if (type(allow) is list):
            self.send_cmd(cmds.NodeAPSetAuthAddrFilter(allow))
        else:
            self.send_cmd(cmds.NodeAPSetAuthAddrFilter([allow]))


    def set_ssid(self, ssid):
        """Command to set the SSID of the AP.

        Args:
            ssid (str):  SSID to set on the AP (Must be 32 characters or less)
        
        Returns:
            ssid (str):  Current SSID on the AP.        
        """
        return self.send_cmd(cmds.NodeAPSetSSID(ssid))


    def set_beacon_interval(self, beacon_interval):
        """Command to set the beacon interval of the AP.

        Args:
            beacon_interval (int): Integer number of beacon Time Units in [1, 65535]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds)        

        Returns:
            beacon_interval (int): 
                Current number of Time Units between beacons
        """
        return self.send_cmd(cmds.NodeAPProcBeaconInterval(cmds.CMD_PARAM_WRITE, beacon_interval))


    def get_beacon_interval(self):
        """Command to get the beacon interval of the AP.

        Returns:
            beacon_interval (int): 
                Current number of Time Units between beacons
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds)
        """
        return self.send_cmd(cmds.NodeAPProcBeaconInterval(cmds.CMD_PARAM_READ))


    def add_association(self, device_list, allow_timeout=None):
        """Command to add an association to each device in the device list.
        
        Args:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices 
                or single 802.11 device to add to the association table
            allow_timeout (bool, optional):  Allow the association to timeout if inactive
        
        .. note::  This adds an association with the default tx/rx params.  If
            allow_timeout is not specified, the default on the node is to 
            not allow timeout of the association.

        .. note::  If the device is a WlanExpNodeSta, then this method will also
            add the association to that device.
        
        .. note::  The add_association method will bypass any association address filtering
            on the node.  One caveat is that if a device sends a de-authentication packet
            to the AP, the AP will honor it and completely remove the device from the 
            association table.  If the association address filtering is such that the
            device is not allowed to associate, then the device will not be allowed back
            on the AP even though at the start of the experiment the association was 
            explicitly added.
        """
        ret_val = []
        ret_list = True
        
        # Convert entries to a list if it is not already one
        if type(device_list) is not list:
            device_list = [device_list]
            ret_list    = False

        # Get the AP's current channel, SSID
        channel = self.get_channel()
        ssid    = self.get_ssid()

        for device in device_list:
            ret_val.append(self._add_association(device, channel, ssid, allow_timeout))

        # Need to return a single value and not a list
        if not ret_list:
            ret_val = ret_val[0]
        
        return ret_val
        

    def _add_association(self, device, channel, ssid, allow_timeout):
        """Internal command to add an association."""
        ret_val = False

        import wlan_exp.node_ibss as node_ibss
        
        if isinstance(device, node_ibss.WlanExpNodeIBSS):
            print("WARNING:  Could not add association for IBSS node '{0}'".format(device.description))
            return ret_val

        aid = self.send_cmd(cmds.NodeAPAddAssociation(device, allow_timeout))
        
        if (aid != cmds.CMD_PARAM_ERROR):
            import wlan_exp.node_sta as node_sta

            if isinstance(device, node_sta.WlanExpNodeSta):
                if device.send_cmd(cmds.NodeSTAAddAssociation(self, aid, channel, ssid)):
                    ret_val = True
            else:
                msg  = "\nWARNING:  Could not add association to non-STA node '{0}'\n".format(device.description) 
                msg += "    Please add the association to the AP manually on the device.\n"
                print(msg)
                ret_val = True
            
        return ret_val



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeAp object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN EXP AP Node:\n"
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
        msg = super(WlanExpNodeAp, self).__repr__()
        msg = "WLAN EXP AP " + msg
        return msg

# End Class WlanExpNodeAp
