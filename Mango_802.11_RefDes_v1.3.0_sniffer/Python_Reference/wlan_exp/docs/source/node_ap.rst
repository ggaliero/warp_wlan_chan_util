.. _wlan_exp_node_ap:

.. include:: globals.rst

WLAN Exp Node - Access Point (AP)
---------------------------------
Sub-class of WlanExpNode that adds 802.11 Access Point (AP) functionality.  

For more information please refer to the `AP documentation <http://warpproject.org/trac/wiki/802.11/MAC/Upper/AP>`_.


Class
.....
.. autoclass:: wlan_exp.node_ap.WlanExpNodeAp


AP Commands
...........
These commands are specific to Access Points.

.. autoclass:: wlan_exp.node_ap.WlanExpNodeAp
   :members: ap_configure, set_dtim_period, get_dtim_period, set_authentication_address_filter, set_ssid, set_beacon_interval, get_beacon_interval, add_association








