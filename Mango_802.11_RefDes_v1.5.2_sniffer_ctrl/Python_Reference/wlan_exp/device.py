# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Device Classes
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
"""
import sys

__all__ = ['WlanDevice']


# Fix to support Python 2.x and 3.x
if sys.version[0]=="3": long=None


class WlanDevice(object):
    """Class for WLAN Device. This is the parent class for all wlan_exp node class definitions.
    This class also provides a node type for devices which exist in a wireless network but are not
    controlled by wlan_exp (i.e. a Wi-Fi client connected to an 802.11 Reference Design AP).
    
    Args:
        mac_address (int, str): Medium Access Control (MAC) address of the WLAN device (48-bits)
             The mac_address should be of the format:  0x0123456789AB or '01:23:45:67:89:AB'
        name (string):          User generated description of the WLAN device
        ht_capable (bool):      Indicates if device has PHY capable of HT (802.11n) rates 

    **Class Members:**
    
    Attributes:
        device_type (int):     Unique type of the WLAN Device
        name (string):         User generated description of the WLAN device
        wlan_mac_address(int): MAC Address of WLAN Device
        ht_capable (bool):     Indicates if device has PHY capable of HT (802.11n) rates 
    """
    device_type           = None
    name                  = None
    description           = None
    
    wlan_mac_address      = None
    
    ht_capable            = None

    def __init__(self, mac_address, name=None, ht_capable=False):
        self.name = name
        self.ht_capable = ht_capable
                
        if mac_address is not None:        
            if type(mac_address) in [int, long]:
                self.wlan_mac_address = mac_address
            elif type(mac_address) is str:
                try:
                    import wlan_exp.util as util                    
                    self.wlan_mac_address = util.str_to_mac_addr(mac_address)
                    
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


