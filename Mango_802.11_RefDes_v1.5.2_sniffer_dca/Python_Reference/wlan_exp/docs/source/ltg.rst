
.. include:: globals.rst

LTG Flow Configurations
.......................

An LTG Flow Configuration is defined by two things:
 * Packet **payload** specification - responsible for setting the Tx parameters (rate, power) and creating the MAC header and payload of LTG packets.
 * Packet generation **schedule** - responsible for scheduling the creation and enqueue of LTG packets.

The 802.11 Reference Design code defines two schedules and three payload specifications. These schedules and payloads are used to define three
LTG Flow Configurations in the reference code.

For more information on the LTG framework in C please refer to the user guide `LTG docs <http://warpproject.org/trac/wiki/802.11/MAC/Upper/MACHighFramework/LTG>`_.

Flow Config
```````````
.. autoclass:: wlan_exp.ltg.FlowConfig

.. autoclass:: wlan_exp.ltg.FlowConfigCBR

.. autoclass:: wlan_exp.ltg.FlowConfigAllAssocCBR

.. autoclass:: wlan_exp.ltg.FlowConfigRandomRandom


Schedule
````````
.. autoclass:: wlan_exp.ltg.Schedule

.. autoclass:: wlan_exp.ltg.SchedulePeriodic

.. autoclass:: wlan_exp.ltg.ScheduleUniformRandom


Payload
```````
.. autoclass:: wlan_exp.ltg.Payload

.. autoclass:: wlan_exp.ltg.PayloadFixed

.. autoclass:: wlan_exp.ltg.PayloadUniformRandom

.. autoclass:: wlan_exp.ltg.PayloadAllAssocFixed
