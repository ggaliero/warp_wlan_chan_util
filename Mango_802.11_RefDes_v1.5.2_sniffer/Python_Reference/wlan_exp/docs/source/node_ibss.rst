.. _wlan_exp_node_ibss:

.. include:: globals.rst

Ad hoc (IBSS) Node
------------------
Subclass of WlanExpNode that interfaces to an 802.11 Reference Design node running the
Access Point (AP) application in CPU High. An AP node supports all the :doc:`common node<node>`
methods plus the AP-specific methods described below.

IBSS Node Methods
.................

.. automethod:: wlan_exp.node_ibss.WlanExpNodeIBSS.configure_bss
.. automethod:: wlan_exp.node_ibss.WlanExpNodeIBSS.get_txrx_counts
