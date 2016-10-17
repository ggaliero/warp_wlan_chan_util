.. _wlan_exp_node_sta:

.. include:: globals.rst


Station (STA) Node
------------------
Subclass of WlanExpNode that interfaces to an 802.11 Reference Design node running the
station (STA) application in CPU High. A STA node supports all the :doc:`common node<node>`
methods plus the STA-specific methods described below.

STA Node Methods
................

.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.configure_bss
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.is_associated
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.disassociate
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.set_aid
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.join_network
.. automethod:: wlan_exp.node_sta.WlanExpNodeSta.is_joining

