# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Local Traffic Generation (LTG)
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

This module provides definitions for Local Traffic Generation (LTG) on
WLAN Exp nodes.

It is assumed that users will extend these classes to create their own
LTG flow configurations.  When an LTG component is serialized, it follows
the following structure:

+------------+------------+---------------------------------+
| Word       | Bits       | Function                        |
+============+============+=================================+
| [0]        |  [31:16]   | Type                            |
+------------+------------+---------------------------------+
| [0]        |  [15:0]    | Length (number of 32 bit words) |
+------------+------------+---------------------------------+
| [1:length]              | Parameters                      |
+------------+------------+---------------------------------+

When a LTG flow configuration is serialized, the schedule is serialized
first and then the payload.
"""


__all__ = ['Schedule', 'SchedulePeriodic', 'ScheduleUniformRandom',
           'Payload', 'PayloadFixed', 'PayloadUniformRandom',
           'FlowConfig', 'FlowConfigCBR', 'FlowConfigRandomRandom']



# LTG Schedules
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_SCHED_TYPE_PERIODIC                = 1
LTG_SCHED_TYPE_UNIFORM_RAND            = 2


# LTG Payloads
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_PYLD_TYPE_FIXED                    = 1
LTG_PYLD_TYPE_UNIFORM_RAND             = 2
LTG_PYLD_TYPE_ALL_ASSOC_FIXED          = 3

# LTG Payload Min/Max
#
#   Currently, the minimum payload size is determined by the size of the LTG
# header in the payload (as of 0.96, the LTG header contains the LLC header,
# a 64-bit unique sequence number and a 32-bit LTG ID).  Currently, the maximum
# payload size is 1500 bytes.  This is a relatively arbitrary amount chosen
# because 1) in 0.96 the 802.11 phy cannot transmit more than 1600 bytes total
# per packet; 2) 1500 bytes is about the size of a standard Ethernet MTU.  If
# sizes outside the range are requested, the functions will print a warning and
# adjust the value to the appropriate boundary.
#
LTG_PYLD_MIN                           = 20
LTG_PYLD_MAX                           = 1500


# LTG Constants
#   NOTE:  The C counterparts are found in *_ltg.h
LTG_REMOVE_ALL                         = 0xFFFFFFFF
LTG_START_ALL                          = 0xFFFFFFFF
LTG_STOP_ALL                           = 0xFFFFFFFF


#-----------------------------------------------------------------------------
# LTG Schedules
#-----------------------------------------------------------------------------
class Schedule(object):
    """Base class for LTG Schedules."""
    ltg_type = None

    def get_type(self):
        """Return the type of the LTG Schedule.

        Returns:
            type (int):  Type of Schedule
        """
        return self.ltg_type

    def get_params(self):
        """Returns a list of parameters of the LTG Schedule.

        Returns:
            params (list of int):  Parameters of the Schedule
        """
        raise NotImplementedError

    def serialize(self):
        """Returns a list of 32 bit intergers that can be added as arguments to a WnCmd.

        Returns:
            words (list of int):  List of 32-bit words representing the Schedule
        """
        args = []
        params = self.get_params()
        args.append(int((self.ltg_type << 16) + len(params)))
        for param in params:
            args.append(int(param))
        return args

    def enforce_min_resolution(self, resolution):
        """Enforce the minimum resolution on the Schedule.

        Args:
            resolution (int):  Resolution of the schedule
        """
        raise NotImplementedError

# End Class WlanExpLTGSchedule


class SchedulePeriodic(Schedule):
    """LTG Schedule that will generate a payload every 'interval' microseconds.

    Args:
        interval (float):              Interval between packets (in float seconds)
        duration (float, optional):    Duration of the traffic flow (in float seconds)
    """
    time_factor = 6                    # Time on node is in microseconds
    interval    = None
    duration    = None

    def __init__(self, interval, duration=None):
        self.ltg_type = LTG_SCHED_TYPE_PERIODIC
        self.interval = int(round(float(interval), self.time_factor) * (10**self.time_factor))
        if duration is None:
            self.duration = 0
        else:
            self.duration = int(round(float(duration), self.time_factor) * (10**self.time_factor))

    def get_params(self):
        """Returns a list of parameters of the LTG Schedule.

        Returns:
            params (list of int):  Parameters of the Schedule
        """
        duration0 = ((self.duration >> 32) & 0xFFFFFFFF)
        duration1 = (self.duration & 0xFFFFFFFF)
        return [self.interval, duration0, duration1]

    def enforce_min_resolution(self, resolution):
        """Enforce the minimum resolution on the Schedule.

        Args:
            resolution (int):  Resolution of the schedule
        """
        temp_interval = (self.interval // resolution) * resolution
        if (temp_interval != self.interval):
            msg  = "WARNING:  Cannot schedule LTG with interval: {0} us\n".format(self.interval)
            msg += "    Minimum LTG resolution is {0} us.\n".format(resolution)
            msg += "    Adjusting interval to {0} us".format(temp_interval)
            print(msg)
            self.interval = temp_interval

# End Class WlanExpLTGSchedPeriodic


class ScheduleUniformRandom(Schedule):
    """LTG Schedule that will generate a payload a uniformly random time
    between min_interval and max_interval microseconds.

    Args:
        min_interval (float):          Minimum interval between packets (in float seconds)
        max_interval (float):          Maximum interval between packets (in float seconds)
        duration (float, optional):    Duration of the traffic flow (in float seconds)
    """
    time_factor  = 6                   # Time on node is in microseconds
    min_interval = None
    max_interval = None
    duration     = None

    def __init__(self, min_interval, max_interval, duration=None):
        self.ltg_type     = LTG_PYLD_TYPE_UNIFORM_RAND
        self.min_interval = int(round(float(min_interval), self.time_factor) * (10**self.time_factor))
        self.max_interval = int(round(float(max_interval), self.time_factor) * (10**self.time_factor))
        if duration is None:
            self.duration = 0
        else:
            self.duration = int(round(float(duration), self.time_factor) * (10**self.time_factor))

    def get_params(self):
        """Returns a list of parameters of the LTG Schedule.

        Returns:
            params (list of int):  Parameters of the Schedule
        """
        duration0 = ((self.duration >> 32) & 0xFFFFFFFF)
        duration1 = (self.duration & 0xFFFFFFFF)
        return [self.min_interval, self.max_interval, duration0, duration1]

    def enforce_min_resolution(self, resolution):
        """Enforce the minimum resolution on the Schedule.

        Args:
            resolution (int):  Resolution of the schedule
        """
        temp_interval = (self.min_interval // resolution) * resolution
        if (temp_interval != self.min_interval):
            msg  = "WARNING:  Cannot schedule LTG with min interval: {0} us\n".format(self.min_interval)
            msg += "    Minimum LTG resolution is {0} us.\n".format(resolution)
            msg += "    Adjusting interval to {0} us".format(temp_interval)
            print(msg)
            self.min_interval = temp_interval

        temp_interval = (self.max_interval // resolution) * resolution
        if (temp_interval != self.max_interval):
            msg  = "WARNING:  Cannot schedule LTG with interval: {0} us\n".format(self.max_interval)
            msg += "    Minimum LTG resolution is {0} us.\n".format(resolution)
            msg += "    Adjusting interval to {0} us".format(temp_interval)
            print(msg)
            self.max_interval = temp_interval

# End Class WlanExpLTGSchedUniformRand


#-----------------------------------------------------------------------------
# LTG Payloads
#-----------------------------------------------------------------------------
class Payload(object):
    """Base class for LTG Payloads."""
    ltg_type = None

    def get_type(self):
        """Return the type of the LTG Payload.

        Returns:
            type (int):  Type of Payload
        """
        return self.ltg_type

    def get_params(self):
        """Returns a list of parameters of the LTG Payload.

        Returns:
            params (list of int):  Parameters of the Payload
        """
        raise NotImplementedError

    def serialize(self):
        """Returns a list of 32 bit intergers that can be added as arguments to a WnCmd.

        Returns:
            words (list of int):  List of 32-bit words representing the Schedule
        """
        args = []
        params = self.get_params()
        args.append(int((self.ltg_type << 16) + len(params)))
        for param in params:
            args.append(int(param))
        return args

    def validate_length(self, length):
        """Returns a valid LTG Payload length and prints warnings if length was invalid.

        Args:
            length (int):  Length of the payload (in bytes)

        Returns:
            length (int):  Adjusted length of the payload (in bytes)

        """
        msg  = "WARNING:  Adjusting LTG Payload length from {0} ".format(length)

        if (length < LTG_PYLD_MIN):
            msg += "to {0} (min).".format(LTG_PYLD_MIN)
            print(msg)
            length = LTG_PYLD_MIN

        if (length > LTG_PYLD_MAX):
            msg += "to {0} (max).".format(LTG_PYLD_MAX)
            print(msg)
            length = LTG_PYLD_MAX

        return length

# End Class Payload


class PayloadFixed(Payload):
    """LTG payload that will generate a fixed payload of the given length.

    Args:
        dest_addr (int):   Destination MAC address
        length (int):      Minimum length of the LTG payload (in bytes)

    """
    dest_addr = None
    length    = None

    def __init__(self, dest_addr, length):
        self.ltg_type  = LTG_PYLD_TYPE_FIXED
        self.dest_addr = dest_addr
        self.length    = self.validate_length(length)

    def get_params(self):
        """Returns a list of parameters of the LTG Payload.

        Returns:
            params (list of int):  Parameters of the Payload
        """
        addr0 = ((self.dest_addr >> 32) & 0xFFFF)
        addr1 = (self.dest_addr & 0xFFFFFFFF)
        return [addr0, addr1, self.length]

# End Class PayloadFixed


class PayloadUniformRandom(Payload):
    """LTG payload that will generate a payload with uniformly random size
    between min_length and max_length bytes.

    Args:
        dest_addr (int):       Destination MAC address
        min_length (int):      Minimum length of the LTG payload (in bytes)
        max_length (int):      Maximum length of the LTG payload (in bytes)
    """
    dest_addr  = None
    min_length = None
    max_length = None

    def __init__(self, dest_addr, min_length, max_length):
        self.ltg_type   = LTG_PYLD_TYPE_UNIFORM_RAND
        self.dest_addr  = dest_addr
        self.min_length = self.validate_length(min_length)
        self.max_length = self.validate_length(max_length)

    def get_params(self):
        """Returns a list of parameters of the LTG Payload.

        Returns:
            params (list of int):  Parameters of the Payload
        """
        addr0 = ((self.dest_addr >> 32) & 0xFFFF)
        addr1 = (self.dest_addr & 0xFFFFFFFF)
        return [addr0, addr1, self.min_length, self.max_length]

# End Class PayloadUniformRand


class PayloadAllAssocFixed(Payload):
    """LTG payload that will generate a fixed payload of the given length
    to all associated deivces.

    Args:
        length (int):      Length of the LTG payload (in bytes)
    """
    length    = None

    def __init__(self, length):
        self.ltg_type  = LTG_PYLD_TYPE_ALL_ASSOC_FIXED
        self.length    = self.validate_length(length)

    def get_params(self):
        """Returns a list of parameters of the LTG Payload.

        Returns:
            params (list of int):  Parameters of the Payload
        """
        return [self.length]

# End Class PayloadAllAssocFixed


#-----------------------------------------------------------------------------
# LTG Flow Configurations
#-----------------------------------------------------------------------------
class FlowConfig(object):
    """Base class for LTG Flow Configurations."""
    ltg_schedule = None
    ltg_payload  = None

    def get_schedule(self):  return self.ltg_schedule
    def get_payload(self):   return self.ltg_payload

    def serialize(self):
        """Returns a list of 32 bit intergers that can be added as arguments to a WnCmd.

        Returns:
            words (list of int):  List of 32-bit words representing the FlowConfig
        """
        args = []

        for arg in self.ltg_schedule.serialize():
            args.append(arg)

        for arg in self.ltg_payload.serialize():
            args.append(arg)

        return args

    def enforce_min_resolution(self, resolution):
        """Enforce the minimum resolution on the Schedule.

        Args:
            resolution (int):  Resolution of the schedule
        """
        self.ltg_schedule.enforce_min_resolution(resolution)

# End Class FlowConfig


class FlowConfigCBR(FlowConfig):
    """Class to implement a Constant Bit Rate LTG flow configuration to a given device.

    Args:
        dest_addr (int):      Destination MAC address
        payload_length (int): Length of the LTG payload (in bytes)
        interval (float):     Interval between packets (in float seconds)
        duration (float):     Duration of the traffic flow (in float seconds)
    """
    def __init__(self, dest_addr, payload_length, interval, duration=None):
        self.ltg_schedule = SchedulePeriodic(interval, duration)
        self.ltg_payload  = PayloadFixed(dest_addr, payload_length)

# End Class FlowConfigCBR


class FlowConfigAllAssocCBR(FlowConfig):
    """Class to implement a Constant Bit Rate LTG flow configuration to all associated
    devices.

    Args:
        payload_length (int): Length of the LTG payload (in bytes)
        interval (float):     Interval between packets (in float seconds)
        duration (float):     Duration of the traffic flow (in float seconds)
    """
    def __init__(self, payload_length, interval, duration=None):
        self.ltg_schedule = SchedulePeriodic(interval, duration)
        self.ltg_payload  = PayloadAllAssocFixed(payload_length)

# End Class FlowConfigCBR


class FlowConfigRandomRandom(FlowConfig):
    """Class to implement an LTG flow configuration with random period and random
    sized payload to a given device.

    Args:
        dest_addr (int):               Destination MAC address
        min_payload_length (int):      Minimum length of the LTG payload (in bytes)
        max_payload_length (int):      Maximum length of the LTG payload (in bytes)
        min_interval (float):          Minimum interval between packets (in float seconds)
        max_interval (float):          Maximum interval between packets (in float seconds)
        duration (float, optional):    Duration of the traffic flow (in float seconds)
    """
    def __init__(self, dest_addr, min_payload_length, max_payload_length, min_interval, max_interval, duration=None):
        self.ltg_schedule = ScheduleUniformRandom(min_interval, max_interval, duration)
        self.ltg_payload  = PayloadUniformRandom(dest_addr, min_payload_length, max_payload_length)

# End Class FlowConfigRandom































