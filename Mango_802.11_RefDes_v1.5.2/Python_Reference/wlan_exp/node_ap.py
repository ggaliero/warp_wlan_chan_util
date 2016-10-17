# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Access Point Node
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

"""

import wlan_exp.node as node
import wlan_exp.cmds as cmds


__all__ = ['WlanExpNodeAp']


class WlanExpNodeAp(node.WlanExpNode):
    """wlan_exp Node class for the 802.11 Reference Design AP MAC project

    Args:
        network_config (transport.NetworkConfiguration) : Network configuration of the node
    """

    #-------------------------------------------------------------------------
    # Node Commands
    #-------------------------------------------------------------------------
    def configure_bss(self, bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):
        """Configure the BSS information of the node

        Each node is either a member of no BSS (colloquially "unassociated")
        or a member of one BSS.  A node requires a minimum valid set of BSS 
        information to be a member of a BSS. The minimum valid set of BSS 
        information for an AP is:

            #. BSSID: 48-bit MAC address
            #. Channel: Logical channel for Tx/Rx by BSS members
            #. SSID: Variable length string (ie the name of the network)
            #. Beacon Interval:  Interval (in TUs) for beacons

        If a node is not a member of a BSS (i.e. ``n.get_bss_info()`` returns
        ``None``), then the node requires all parameters of a minimum valid 
        set of BSS information be specified (i.e. Channel, SSID, and
        Beacon Interval).  For an AP, if BSSID is not specified, then it is 
        assumed to be the wlan_mac_address of the node.  
        
        See https://warpproject.org/trac/wiki/802.11/wlan_exp/bss
        for more documentation on BSS information / configuration.

        Args:
            bssid (int, str):  48-bit ID of the BSS either None or
                the wlan_mac_address of the node.  If not specified, it is 
                by default the wlan_mac_address of the node.
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Integer number of beacon Time Units in [10, 65534]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
                A value of None will disable beacons;  A value of False will not
                update the current beacon interval.
            ht_capable (bool):  Is the PHY mode HTMF (True) or NONHT (False)?

        """
        if bssid is not None:
            if bssid is not False:
                # User supplied a not-None BSSID argument
                error = False

                if type(bssid) in [int, long]:
                    if (bssid != self.wlan_mac_address):
                        error = True
                elif type(bssid) is str:
                    import wlan_exp.util as util
                    try:
                        if (util.str_to_mac_addr(bssid) != self.wlan_mac_address):
                            error = True
                    except:
                        error = True

                if (error):
                    raise AttributeError("BSSID must be either None or the wlan_mac_address of the node.")
            else:
                # User did not provide a BSSID argument - use the AP's MAC address
                bssid = self.wlan_mac_address

        resp_args = self.send_cmd(cmds.NodeConfigBSS(bssid=bssid, ssid=ssid, channel=channel,
                                                     beacon_interval=beacon_interval, ht_capable=ht_capable))
        
        # Process response arguments
        if (resp_args is not False):
            status  = resp_args[0]
            msg     = "ERROR:  Invalid response from node:\n"
            ret_val = True
            
            # Check status
            if (status & cmds.ERROR_CONFIG_BSS_BSSID_INVALID):
                if type(self.bssid) in [int, long]:
                    import wlan_exp.util as util
                    self.bssid = util.mac_addr_to_str(self.bssid)
                msg    += "    BSSID {0} was invalid.\n".format(self.bssid)
                ret_val = False
            
            if (status & cmds.ERROR_CONFIG_BSS_BSSID_INSUFFICIENT_ARGUMENTS):
                msg    += "    Insufficient arguments to create BSS.  Must provide:\n"
                if (bssid is False): 
                    msg    += "        BSSID\n"
                if (ssid is None):
                    msg    += "        SSID\n"
                if (channel is None):
                    msg    += "        CHANNEL\n"
                if (beacon_interval is False):
                    msg    += "        BEACON_INTERVAL\n"
                ret_val = False
            
            if (status & cmds.ERROR_CONFIG_BSS_CHANNEL_INVALID):
                msg    += "    Channel {0} was invalid.\n".format(self.channel)
                ret_val = False
            
            if (status & cmds.ERROR_CONFIG_BSS_BEACON_INTERVAL_INVALID):
                msg    += "    Beacon interval {0} was invalid.\n".format(self.beacon_interval)
                ret_val = False
            
            if (status & cmds.ERROR_CONFIG_BSS_HT_CAPABLE_INVALID):
                msg    += "    HT capable {0} was invalid.\n".format(self.ht_capable)
                ret_val = False
            
            if not ret_val:
                print(msg)


    def enable_beacon_mac_time_update(self, enable):
        """Enable / Disable MAC time update from beacons

        Raises NotImplementedError().  Current AP implementation does not 
        support updating MAC time from beacon receptions

        Args:
            enable (bool):  True - enable MAC time updates from beacons
                            False - disable MAC time updates from beacons

        """
        raise NotImplementedError("Current AP implementation does not support updating MAC time from beacon receptions")


    def is_associated(self, device_list):
        """Are the devices in the device_list in the AP association table?

        Args:
            device_list (WlanDevice):  List of WLAN device (or sub-class of
                WLAN device)

        Returns:
            associated (list of bool):  List of booleans describing whether each given device is associated with the AP

        If the device_list is a single device, then only a boolean is returned.  
        If the device_list is a list of devices, then a list of booleans will 
        be returned in the same order as the devices in the list.
        """
        ret_val  = []
        ret_list = True
        is_member = False

        if device_list is not None:
            # Convert device_list to a list if it is not already one; set flag to not return a list
            if type(device_list) is not list:
                device_list = [device_list]
                ret_list    = False

            for device in device_list:
                # Check the station info to see if device is there
                
                bss_member_list = self.get_bss_members()
                
                for bss_member in bss_member_list:
                    if bss_member['mac_addr'] == device.wlan_mac_address:
                        is_member = True

                if is_member is True:
                    ret_val.append(True)
                else:
                    ret_val.append(False)
        else:
            ret_val = False

        # Need to return a single value and not a list
        if not ret_list:
            ret_val = ret_val[0]

        return ret_val



    #-------------------------------------------------------------------------
    # Internal Node methods
    #-------------------------------------------------------------------------
    def _check_allowed_rate(self, mcs, phy_mode, verbose=False):
        """Check that rate parameters are allowed

        Args:
            mcs (int):           Modulation and coding scheme (MCS) index
            phy_mode (str, int): PHY mode (from util.phy_modes)

        Returns:
            valid (bool):  Are all parameters valid?
        """

        # TODO: implement AP-specific rate checking here
        #  Allow all supported rates for now

        return self._check_supported_rate(mcs, phy_mode, verbose)



    #-------------------------------------------------------------------------
    # AP specific commands
    #-------------------------------------------------------------------------
    def disassociate(self, device_list):
        """De-authenticate specific devices and remove the devices from the AP's
        association tables. This method triggers transmission of a de-authenticaion
        packet to the targeted STA nodes. The STA nodes are then removed from the AP
        association table.

        Args:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11
                devices or single 802.11 device for which to disassociate
        """
        try:
            for device in device_list:
                self.send_cmd(cmds.NodeDisassociate(device))
        except TypeError:
            self.send_cmd(cmds.NodeDisassociate(device_list))


    def disassociate_all(self):
        """De-authenticates all devices and removes all devices from the AP's
        association tables. This method triggers transmission of a de-authenticaion
        packet to every associated STA node. The STA nodes are then removed from the AP
        association table.

        """
        self.send_cmd(cmds.NodeDisassociate())


    def enable_DTIM_multicast_buffering(self, enable):
        """Enable / Disable DTIM buffering of multicast data

        The Delivery Traffic Indication Map (DTIM) keeps track of STA sleep
        states and will buffer traffic for the node based on those sleep
        states.  When an AP is configured with enable_DTIM_multicast_buffering(False),
        it will include the multicast queue in the normal polling of queues,
        independent of any STA sleep states.

        Args:
            enable (bool):  True - enable DTIM multicast buffering
                            False - disable DTIM multicast buffering
                            (Default value on Node: True)
        """
        self.send_cmd(cmds.NodeAPConfigure(dtim_multicast_buffering=enable))


    def set_authentication_address_filter(self, allow):
        """Command to set the authentication address filter on the node.

        This command will reset the current address filter and then set the
        address filter to the values in the allow list.  The filter only affects
        over-the-air associations.  Assocaitions created by wlan_exp will
        bypass any filters configured by this method.

        Clients will be allowed to associate if they pass any of the filters
        that are set.

        Args:
            allow (list of tuple):  List of (address, mask) tuples that will be
                used to filter addresses on the node.  A tuple can be substituted
                with a predefined string:  "NONE", "ALL", or "MANGO-W3"

        For the mask, bits that are 0 are treated as "any" and bits that are 1 
        are treated as "must equal".  For the address, locations of one bits 
        in the mask must match the incoming addresses to pass the filter.

        Examples:

        * Only allow client with MAC address ``01:23:45:67:89:AB``:

            >>> n_ap.set_authentication_address_filter(allow=(0x0123456789AB, 0xFFFFFFFFFFFF))

        * Only allow clients with MAC addresses starting with ``01:23:45``:

            >>> n_ap.set_authentication_address_filter(allow=(0x012345000000, 0xFFFFFF000000))

        * Allow clients with MAC addresses starting with ``01:23:45`` or ``40:``

            >>> n_ap.set_authentication_address_filter(allow=[(0x012345000000, 0xFFFFFF000000), (0x400000000000, 0xFF0000000000)])
        * Use one of the pre-defined address filter configurations:

            >>> n_ap.set_authentication_address_filter(allow='NONE')     # Same as allow=(0x000000000000, 0xFFFFFFFFFFFF)
            >>> n_ap.set_authentication_address_filter(allow='ALL')      # Same as allow=(0x000000000000, 0x000000000000)
            >>> n_ap.set_authentication_address_filter(allow='MANGO-W3') # Same as allow=(0x40d855042000, 0xFFFFFFFFF000)

        """
        filters = []

        if (type(allow) is not list):
            allow = [allow]

        for value in allow:
            # Process pre-defined strings
            if type(value) is str:
                if   (value == 'NONE'):
                    filters.append((0x000000000000, 0xFFFFFFFFFFFF))
                elif (value == 'ALL'):
                    filters.append((0x000000000000, 0x000000000000))
                elif (value == 'MANGO-W3'):
                    filters.append((0x40d855042000, 0xFFFFFFFFF000))
                else:
                    msg  = "\n    String '{0}' not recognized.".format(value)
                    msg += "\n    Please use 'NONE', 'ALL', 'MANGO-W3' or a (address, mask) tuple"
                    raise AttributeError(msg)

            elif type(value) is tuple:
                import wlan_exp.util as util

                # Process address
                if type(value[0]) in [int, long]:
                    address = value[0]
                elif type(value[0]) is str:
                    try:
                        address = util.str_to_mac_addr(value[0])
                    except:
                        raise AttributeError("Address {0} is not valid".format(value[0]))
                else:
                    raise AttributeError("Address type {0} is not valid".format(type(value[0])))

                # Process mask
                if type(value[1]) in [int, long]:
                    mask = value[1]
                elif type(value[1]) is str:
                    try:
                        mask = util.str_to_mac_addr(value[1])
                    except:
                        raise AttributeError("Mask {0} is not valid".format(value[1]))
                else:
                    raise AttributeError("Mask type {0} is not valid".format(type(value[1])))

                filters.append((address, mask))

            else:
                msg  = "\n    Value {0} with type {1} not recognized.".format(value, type(value))
                msg += "\n    Please use 'NONE', 'ALL', 'MANGO-W3' or a (address, mask) tuple"
                raise AttributeError(msg)

        self.send_cmd(cmds.NodeAPSetAuthAddrFilter(filters))


    def add_association(self, device_list, disable_timeout=None):
        """Adds each device in ``device_list`` to the list of associated stations at the AP. If a device
        is also an 802.11 Reference Design STA, the STA is also configured with the BSS of the AP. In this
        case the AP and STA attain the same association state as if they had associated via the standard
        wireless handshake. This method bypasses any any authentication address filtering at the AP.

        Args:
            device_list (list of WlanExpNode / WlanDevice):  List of 802.11 devices
                or single 802.11 device to add to the AP's association table
            disable_timeout (bool, optional):  Disables the AP's normal inactivity timeout for the new associations.
            The AP periodically checks for associated stations with no recent Tx/Rx activity and removes inactive
            nodes from its list of associated stations. Set this parameter to True to force to AP to keep the new
            associations created by this method, even if the stations are inactive.
        """
        ret_val = []
        ret_list = True

        # Convert entries to a list if it is not already one
        if type(device_list) is not list:
            device_list = [device_list]
            ret_list    = False

        # Get the AP's current BSS configuration
        bss_info = self.get_bss_info()

        if bss_info is None:
            msg  = "\n    Cannot add association:  AP BSS configuration is currently null."
            msg += "\n    Configure the AP's BSS using configure_bss() before calling add_association()."
            raise Exception(msg)

        bssid           = bss_info['bssid']
        channel         = bss_info['channel']
        ssid            = bss_info['ssid']
        beacon_interval = bss_info['beacon_interval']
        
        if (bss_info['capabilities'] & bss_info.get_consts().capabilities.HT_CAPABLE):
            ht_capable  = True
        else:
            ht_capable  = False
        
        
        if (beacon_interval == 0):
            beacon_interval = None

        for device in device_list:
            ret_val.append(self._add_association(device=device, bssid=bssid, 
                                                 channel=channel, ssid=ssid, 
                                                 beacon_interval=beacon_interval,
                                                 ht_capable=ht_capable,
                                                 disable_timeout=disable_timeout))

        # Need to return a single value and not a list
        if not ret_list:
            ret_val = ret_val[0]

        return ret_val



    #-------------------------------------------------------------------------
    # Internal AP methods
    #-------------------------------------------------------------------------
    def _add_association(self, device, bssid, channel, ssid, beacon_interval, ht_capable, disable_timeout):
        """Internal command to add an association."""
        ret_val = False

        import wlan_exp.node_ibss as node_ibss

        if isinstance(device, node_ibss.WlanExpNodeIBSS):
            print("WARNING:  Could not add association for IBSS node '{0}'".format(device.description))
            return ret_val

        aid = self.send_cmd(cmds.NodeAPAddAssociation(device, disable_timeout))

        if (aid != cmds.CMD_PARAM_ERROR):
            import wlan_exp.node_sta as node_sta

            if isinstance(device, node_sta.WlanExpNodeSta):
                device.configure_bss(bssid=bssid, ssid=ssid, channel=channel, 
                                     beacon_interval=beacon_interval, ht_capable=ht_capable)
                device.set_aid(aid=aid)
                ret_val = True
            else:
                msg  = "\nWARNING:  Device {0} is not a wlan_exp node \n".format(device.description)
                msg += "    instance.  The device has been added to the AP's list of associated stations.\n"
                msg += "    However, wlan_exp cannot update association state of the device.\n"
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
            msg += "AP Node:\n"
            msg += "    WLAN MAC addr :  {0}\n".format(mac_addr_to_str(self.wlan_mac_address))
            msg += "    Node ID       :  {0}\n".format(self.node_id)
            msg += "    Serial #      :  {0}\n".format(self.sn_str)
            msg += "    HW version    :  WARP v{0}\n".format(self.hw_ver)
            try:
                import wlan_exp.defaults as defaults
                cpu_low_type = defaults.WLAN_EXP_LOW_TYPES[(self.node_type & defaults.WLAN_EXP_LOW_MASK)]
                msg += "    CPU Low Type  :  {0}\n".format(cpu_low_type)
            except:
                pass            
        else:
            msg += "Node not initialized."

        if self.transport is not None:
            msg += "wlan_exp "
            msg += str(self.transport)

        return msg


    def __repr__(self):
        """Return node name and description"""
        msg = super(WlanExpNodeAp, self).__repr__()
        msg = "AP   " + msg
        return msg

# End class
