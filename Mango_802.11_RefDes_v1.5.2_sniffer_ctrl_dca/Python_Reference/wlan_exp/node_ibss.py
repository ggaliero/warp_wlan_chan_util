# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Ad hoc (IBSS) Node
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


__all__ = ['WlanExpNodeIBSS']


class WlanExpNodeIBSS(node.WlanExpNode):
    """wlan_exp Node class for the 802.11 Reference Design IBSS MAC project
    
    Args:
        network_config (transport.NetworkConfiguration) : Network configuration of the node
    """

    #-------------------------------------------------------------------------
    # Node Commands
    #-------------------------------------------------------------------------

    def get_txrx_counts(self, device_list=None, return_zeroed_counts_if_none=True):
        """Get the counts from the node.

        .. note:: This function has the same implementation as WlanExpNode but 
            different default values.
        
        Args:
            device_list (list of WlanExpNode, WlanExpNode, WlanDevice, optional): List of devices
                for which to get counts.  See note below for more information.
            return_zeroed_counts_if_none(bool, optional):  If no counts exist on the node for
                the specified device(s), return a zeroed counts dictionary with proper timestamps
                instead of None.

        Returns:
            counts_dictionary (list of dictionaries, dictionary): Counts for the device(s) specified.


        The dictionaries returned by this method have the following fields:
        
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | Field                       | Description                                                                                         |
            +=============================+=====================================================================================================+
            | retrieval_timestamp         |  Value of System Time in microseconds when structure retrieved from the node                        |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mac_addr                    |  MAC address of remote node whose statics are recorded here                                         |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | associated                  |  Boolean indicating whether remote node is currently associated with this node                      |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | data_num_rx_bytes           |  Total number of bytes received in DATA packets from remote node                                    |
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
            | mgmt_num_rx_bytes           |  Total number of bytes received in management packets from remote node                              |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_bytes_success   |  Total number of bytes successfully transmitted in management packets to remote node                |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_bytes_total     |  Total number of bytes transmitted (successfully or not) in management packets to remote node       |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_rx_packets         |  Total number of management packets received from remote node                                       |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_packets_success |  Total number of management packets successfully transmitted to remote node                         |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_packets_total   |  Total number of management packets transmitted (successfully or not) to remote node                |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | mgmt_num_tx_attempts        |  Total number of low-level attempts of management packets to remote node (includes re-transmissions)|
            +-----------------------------+-----------------------------------------------------------------------------------------------------+
            | latest_txrx_timestamp       |  System Time value of last transmission / reception                                                 |
            +-----------------------------+-----------------------------------------------------------------------------------------------------+


        If the device_list is a single device, then a single dictionary or 
        None is returned.  If the device_list is a list of devices, then a
        list of dictionaries will be returned in the same order as the devices 
        in the list.  If any of the staistics are not there, None will be 
        inserted in the list.  If the device_list is not specified, then all 
        the counts on the node will be returned.
        """
        return super(WlanExpNodeIBSS, self).get_txrx_counts(device_list, return_zeroed_counts_if_none)


    def configure_bss(self, bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):
        """Configure the BSS information of the node
        
        Each node is either a member of no BSS (colloquially "unassociated")
        or a member of one BSS.  A node requires a minimum valid set of BSS 
        information to be a member of a BSS. The minimum valid set of BSS 
        information for an IBSS node is:
        
            #. BSSID: 48-bit MAC address
            #. Channel: Logical channel for Tx/Rx by BSS members
            #. SSID: Variable length string (ie the name of the network)
            #. Beacon Interval:  Interval (in TUs) for beacons

        If a node is not a member of a BSS (i.e. ``n.get_bss_info()`` returns
        ``None``), then the node requires all parameters of a minimum valid 
        set of BSS information be specified (i.e. BSSID, Channel, SSID, and
        Beacon Interval).  
        
        For an IBSS node, the BSSID must be locally administered.  To create a 
        locally administered BSSID, a utility method is provided in util.py:
        
            ``bssid = util.create_locally_administered_bssid(node.wlan_mac_address)``
        
        See https://warpproject.org/trac/wiki/802.11/wlan_exp/bss
        for more documentation on BSS information / configuration.
        
        
        Args:
            bssid (int, str):  48-bit ID of the BSS either as a integer or 
                colon delimited string of the form ``'01:23:45:67:89:ab'``. The 
                ``bssid`` must be a valid locally administered BSSID. Use 
                wlan_exp.util.create_locally_administered_bssid() to generate a valid
                locally administered BSSID based on the node's MAC address.
            ssid (str):  SSID string (Must be 32 characters or less)
            channel (int): Channel number on which the BSS operates
            beacon_interval (int): Integer number of beacon Time Units in [10, 65534]
                (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
                A value of None will disable beacons;  A value of False will not 
                update the current beacon interval.
            ht_capable (bool):  Is the PHY mode HTMF (True) or NONHT (False)?        
        """
        import wlan_exp.util as util
        
        if bssid is not False:
            if bssid is not None:
                if not util.is_locally_administered_bssid(bssid):
                    msg  = "IBSS BSSIDs must be 'locally administered'.  Use \n"
                    msg += "    util.create_locally_administered_bssid() to \n"
                    msg += "    create a 'locally adminstered' BSSID."
                    raise AttributeError(msg)
        
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
        # TODO: implement IBSS-specific rate checking here
        #  Allow all supported rates for now

        return self._check_supported_rate(mcs, phy_mode, verbose)



    #-------------------------------------------------------------------------
    # IBSS specific Commands 
    #-------------------------------------------------------------------------



    #-------------------------------------------------------------------------
    # Internal IBSS methods
    #-------------------------------------------------------------------------



    #-------------------------------------------------------------------------
    # Misc methods for the Node
    #-------------------------------------------------------------------------
    def __str__(self):
        """Pretty print WlanExpNodeIBSS object"""
        msg = ""

        if self.serial_number is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "IBSS Node:\n"
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
        msg = super(WlanExpNodeIBSS, self).__repr__()
        msg = "IBSS " + msg
        return msg

# End class 
