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

__all__ = ['WlanDevice']


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None


class WlanDevice(object):
    """Class for WLAN Device.
    
    Args:
        mac_address (int, str): Medium Access Control (MAC) address of the WLAN device (48-bits)
        name (string):          User generated description of the WLAN device

    **Class Members:**
    
    Attributes:
        device_type (int):     Unique type of the WLAN Device
        name (string):         User generated description of the WLAN device
        wlan_mac_address(int): MAC Address of WLAN Device

    """
    device_type           = None
    name                  = None
    description           = None
    
    wlan_mac_address      = None
    
    def __init__(self, mac_address, name=None):
        self.name = name
                
        if mac_address is not None:        
            if type(mac_address) in [int, long]:
                self.wlan_mac_address = mac_address
            elif type(mac_address) is str:
                try:
                    mac_addr_int = mac_address
                    mac_addr_int = ''.join('{0:02X}:'.format(ord(x)) for x in mac_addr_int)[:-1]
                    mac_addr_int = '0x' + mac_addr_int.replace(':', '')                
                    mac_addr_int = int(mac_addr_int, 0)
                    
                    self.wlan_mac_address = mac_addr_int
                except TypeError:
                    raise TypeError("MAC address is not valid")
            else:
                raise TypeError("MAC address is not valid")
        else:
            raise TypeError("MAC address is not valid")

        self.description  = self.__repr__()


    #-------------------------------------------------------------------------
    # WLAN Commands for the Device
    #-------------------------------------------------------------------------


    # -------------------------------------------------------------------------
    # Misc methods for the Device
    # -------------------------------------------------------------------------
    def __repr__(self):
        """Return device description"""
        msg = ""
        
        if self.wlan_mac_address is not None:
            from wlan_exp.util import mac_addr_to_str
            msg += "WLAN Device '{0}'".format(mac_addr_to_str(self.wlan_mac_address))
            
            if self.name is not None:
                msg += " ({0})".format(self.name)
        else:
            msg += "Node not initialized."
        
        return msg

# End Class WlanDevice


