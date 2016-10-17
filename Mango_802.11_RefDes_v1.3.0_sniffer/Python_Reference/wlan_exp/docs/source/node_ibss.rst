.. _wlan_exp_node_ibss:

.. include:: globals.rst

WLAN Exp Node - IBSS (Ad hoc)
-----------------------------
Sub-class of WlanExpNode that adds 802.11 IBSS (Ad hoc) functionality.  

For more information please refer to the `IBSS documentation <http://warpproject.org/trac/wiki/802.11/MAC/Upper/IBSS>`_.


Class
.....
.. autoclass:: wlan_exp.node_ibss.WlanExpNodeIBSS


IBSS Commands
.............
These commands are specific to IBSS nodes.

.. autoclass:: wlan_exp.node_ibss.WlanExpNodeIBSS
   :members: ibss_configure, disassociate, set_channel, stats_get_txrx, scan_start, set_scan_parameters, scan_enable, scan_disable, join, scan_and_join

