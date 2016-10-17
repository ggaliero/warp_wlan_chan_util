.. _wlan_exp_node_ap:

.. include:: globals.rst

Access Point (AP) Node
----------------------
Subclass of WlanExpNode that interfaces to an 802.11 Reference Design node running the
Access Point (AP) application in CPU High. An AP node supports all the :doc:`common node<node>`
methods plus the AP-specific methods described below.

AP Node Methods
...............

.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.configure_bss
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.add_association
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.is_associated
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.disassociate
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.disassociate_all
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.set_authentication_address_filter
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.enable_DTIM_multicast_buffering
.. automethod:: wlan_exp.node_ap.WlanExpNodeAp.enable_beacon_mac_time_update
