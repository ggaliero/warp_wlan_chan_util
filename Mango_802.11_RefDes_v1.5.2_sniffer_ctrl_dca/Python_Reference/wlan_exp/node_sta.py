# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Client (STA) Node
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


__all__ = ['WlanExpNodeSta']


class WlanExpNodeSta(node.WlanExpNode):
    """wlan_exp Node class for the 802.11 Reference Design STA MAC project
    
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
        information for an STA is:

            #. BSSID: 48-bit MAC address
            #. Channel: Logical channel for Tx/Rx by BSS members
            #. SSID: Variable length string (ie the name of the network)

        If a node is not a member of a BSS (i.e. ``n.get_bss_info()`` returns
        ``None``), then the node requires all parameters of a minimum valid 
        set of BSS information be specified (i.e. BSSID, Channel, and SSID).  
        
        See https://warpproject.org/trac/wiki/802.11/wlan_exp/bss
        for more documentation on BSS information / configuration.

        
        Args:
            bssid (int, str):  48-bit ID of the BSS either as a integer or 
                colon delimited string of the form ``'01:23:45:67:89:ab'``
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Integer number of beacon Time Units in [10, 65534]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
                A value of None will disable beacons;  A value of False will not 
                update the current beacon interval.
            ht_capable (bool):  Is the PHY mode HTMF (True) or NONHT (False)?
        """
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


    def disassociate(self):
        """Causes the STA node to transmit a de-authenticate packet and to reset
        its BSS status to null. After this method the STA will be disassociated from
        its former BSS.

        """
        self.send_cmd(cmds.NodeDisassociate())


    def is_associated(self, ap):
        """Is the AP in the STA's association table?

        Args:
            ap (WlanDevice):  WLAN Device

        Returns:
            associated (bool):  Boolean describing whether the STA is associated with the AP

        """
        ret_val  = False

        if ap is not None:
            my_station_info = self.get_station_info(ap)

            if my_station_info is not None:
                ret_val = True

        return ret_val



    #-------------------------------------------------------------------------
    # STA specific Commands 
    #-------------------------------------------------------------------------
    def _check_allowed_rate(self, mcs, phy_mode, verbose=False):
        """Check that rate parameters are allowed

        Args:
            mcs (int):           Modulation and coding scheme (MCS) index
            phy_mode (str, int): PHY mode (from util.phy_modes)

        Returns:
            valid (bool):  Are all parameters valid?
        """
        # TODO: implement STA-specific rate checking here
        #  Allow all supported rates for now

        return self._check_supported_rate(mcs, phy_mode, verbose)



    def set_aid(self, aid):
        """Set the Association ID (AID) of the STA.

        Normally the AID is assigned by the AP during the probe/authentication/association
        handshake. However if the BSS configuration of the STA is set via the
        configure_bss() method there is no AP-assigned AID. Thankfully this is only a
        cosmetic problem. The reference code does not use the AID for any MAC processing.
        By default the AID is only used to set the hex display at the STA node. This
        method will have the same affect.

        Args:
            aid (int): Association ID (must be in [1, 255])
        """
        self.send_cmd(cmds.NodeSTASetAID(aid))
    

    def join_network(self, ssid, bssid=None, channel=None, timeout=5.0):
        """Join the specified network (BSS).

        By specifying the SSID, the STA will try to join the given network.  If
        the bssid and channel of the network are also known, they can be 
        provided.  Otherwise, the STA will scan for the given SSID to find the 
        corresponding bssid and channel.  If the SSID is None, then any 
        on-going join process will be halted.  
        
        If the node is currently associated with the given BSS, then the node
        state is not changed and this method will return "success" immediately.
        
        By default, the join process has a timeout that will automatically
        halt the join process once the timeout is exceeded.  Depending on the 
        scan parameters, this timeout value may need to be adjusted to allow
        the STA to scan all channels before the timeout period is exceeded.  
        If the timeout is None, then the method will return immediately and
        the methods is_joining() and get_bss_info() can be used to determine 
        if the STA has joined the BSS.
        
        If this method starts an active scan, the scan will use the parameters
        previously configured with node.set_scan_parameters().

        Args:
            ssid (str):  SSID string (Must be 32 characters or less); a value
                of None will stop any current join process.
            bssid (int, str):  48-bit ID of the BSS either as a integer or 
                colon delimited string of the form:  XX:XX:XX:XX:XX:XX
            channel (int): Channel number on which the BSS operates
            timeout (float):  Time to complete the join process; a value of 
                None will cause the method to return immediately and allow 
                the join process to continue forever until the node has joined
                the network or the join has failed.
                
        """
        status = True
        
        # Check if node is currently associated with ssid / bssid
        #     - _check_associated_node() returns True if ssid is None
        if self._check_associated_node(ssid, bssid):
            # Still need to send the Join command to stop join process
            if ssid is None:
                self.send_cmd(cmds.NodeSTAJoin(ssid=None))
        else:
            # Perform the join process

            # Send the join command
            status = self.send_cmd(cmds.NodeSTAJoin(ssid=ssid, bssid=bssid, channel=channel))
            
            if timeout is not None:
                import time
                
                # Get the start time so that the timeout can be enforced
                start_time = time.time()

                # Check when join process completes that STA has joined BSS                
                while ((time.time() - start_time) < timeout):
                    if self.is_joining():
                        # Sleep for 0.1 seconds to not flood the node
                        time.sleep(0.1)
                    else:
                        status = self._check_associated_node(ssid, bssid)
                        break
        
        return status        


    def is_joining(self):
        """Queries if the STA is currently attempting to join a network. The join
        state machine runs until the target network is successfully joined or
        the joing process is terminated by the user. This method tests whether
        the join process is still running. To check success or failure of the
        join process, use node.get_bss_info().
        
        Returns:
            status (bool):

                * True      -- Inidcates node is currently in the join process
                * False     -- Indicates node is not currently in the join process
        """
        return self.send_cmd(cmds.NodeSTAJoinStatus())



    #-------------------------------------------------------------------------
    # Internal STA methods
    #-------------------------------------------------------------------------
    def _check_associated_node(self, ssid, bssid):
        """Check if node is currently associated to the given BSS
        
        This method only checks that the bss_info of the node matches the
        provided ssid / bssid and is not a substitute for is_associated().
        
        This method will only check the bssid after it matches the SSID.  If
        either argument is None, then it will not be checked.  If SSID is 
        None, then the method will return True.
        
        Args:
            ssid  (str):  SSID of the BSS
            bssid (str):  BSSID of the BSS

        Returns:
            associated (bool): Do the SSID / BSSID matche the BSS of the node?
        """
        if ssid is not None:
            bss_info = self.get_bss_info()
        
            if bss_info is None:
                return False
            
            if (bss_info['ssid'] != ssid):
                return False
        
            if bssid is not None:
                if (bss_info['bssid'] != bssid):
                    return False
        
        return True



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeSta object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "STA Node:\n"
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
        msg = super(WlanExpNodeSta, self).__repr__()
        msg = "STA  " + msg
        return msg

# End class 
