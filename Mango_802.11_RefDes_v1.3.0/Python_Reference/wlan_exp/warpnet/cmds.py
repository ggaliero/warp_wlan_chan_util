# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Commands
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2015, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------
MODIFICATION HISTORY:

Ver   Who  Date     Changes
----- ---- -------- -----------------------------------------------------
1.00a ejw  1/23/14  Initial release

------------------------------------------------------------------------------

This module provides class definitions for all WARPNet commands.  

Functions (see below for more information):
    GetWarpNetNodeType()
    Identify()
    GetHwInfo() 
    SetupNetwork()
    ResetNetwork()
    Ping()    
    TestPayloadSize()
    AddNodeGrpId()
    ClearNodeGrpId()

Integer constants:
    GRPID_WARPNET, GRPID_NODE, GRPID_TRANS - WARPNet Command Groups
    GRP_WARPNET, GRP_NODE, GRP_TRANSPORT - WARPNet Command Group Names

Many other constants may be defined; please do not use these values when 
defining other sub-classes of WARPNet Cmd and BufferCmd.

"""


from . import message as wn_message


__all__ = ['NodeGetWarpNetType', 'NodeIdentify', 'NodeGetHwInfo', 
           'NodeSetupNetwork', 'NodeResetNetwork', 'NodeGetTemperature', 
           'Ping', 'TestPayloadSize', 'AddNodeGrpId', 'ClearNodeGrpId']


# Reserved WARPNet Type
WARPNET_RSVD_TYPE                                = 0xFFFFFFFF


# WARPNet Command Groups
GRPID_WARPNET                                    = 0xFF
GRPID_NODE                                       = 0x00
GRPID_TRANS                                      = 0x10


# WARPNet Command Group names
GRP_WARPNET                                      = 'warpnet'
GRP_NODE                                         = 'node'
GRP_TRANSPORT                                    = 'transport'


# WARPNet Command IDs
#   NOTE:  The C counterparts are found in *_node.h
CMDID_WARPNET_TYPE                               = 0xFFFFFF


# WARPNet Node Command IDs
#   NOTE:  The C counterparts are found in *_node.h
CMDID_INFO                                       = 0x000001
CMDID_IDENTIFY                                   = 0x000002

CMD_PARAM_IDENTIFY_ALL_NODES                     = 0xFFFFFFFF
CMD_PARAM_IDENTIFY_NUM_BLINKS                    = 25
CMD_PARAM_IDENTIFY_BLINK_TIME                    = 0.4

CMDID_NODE_NETWORK_SETUP                         = 0x000003
CMDID_NODE_NETWORK_RESET                         = 0x000004

CMD_PARAM_NETWORK_RESET_ALL_NODES                = 0xFFFFFFFF

CMDID_NODE_TEMPERATURE                           = 0x000005


# WARPNet Transport Command IDs
#   NOTE:  The C counterparts are found in *_transport.h
CMDID_PING                                       = 0x000001
CMDID_IP                                         = 0x000002
CMDID_PORT                                       = 0x000003
CMDID_PAYLOAD_SIZE_TEST                          = 0x000004
CMDID_NODE_GRPID_ADD                             = 0x000005
CMDID_NODE_GRPID_CLEAR                           = 0x000006


# Local Constants
_CMD_GRPID_WARPNET                               = (GRPID_WARPNET << 24)
_CMD_GRPID_NODE                                  = (GRPID_NODE << 24)
_CMD_GRPID_TRANS                                 = (GRPID_TRANS << 24)


# -----------------------------------------------------------------------------
# Class Definitions for WARPNet Commands
# -----------------------------------------------------------------------------

class NodeGetWarpNetType(wn_message.Cmd):
    """Command to get the WARPNet Node Type of the node"""
    def __init__(self):
        super(NodeGetWarpNetType, self).__init__()
        self.command = _CMD_GRPID_WARPNET + CMDID_WARPNET_TYPE
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return WARPNET_RSVD_TYPE

# End Class


# -----------------------------------------------------------------------------
# Node Commands
# -----------------------------------------------------------------------------

class NodeIdentify(wn_message.Cmd):
    """Command to blink the WARPNet Node LEDs."""
    def __init__(self, serial_number):
        super(NodeIdentify, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_IDENTIFY

        if (serial_number == CMD_PARAM_IDENTIFY_ALL_NODES):
            (sn, sn_str) = (CMD_PARAM_IDENTIFY_ALL_NODES, "All nodes")
        else:
            from . import util as wn_util

            (sn, sn_str) = wn_util.wn_get_serial_number(serial_number, output=False)

        time_factor    = 6             # Time on node is microseconds
        time_to_send   = int(round(CMD_PARAM_IDENTIFY_BLINK_TIME, time_factor) * (10**time_factor))

        self.id = sn_str
        self.add_args(sn)
        self.add_args(CMD_PARAM_IDENTIFY_NUM_BLINKS)
        self.add_args(time_to_send)
             
    def process_resp(self, resp):
        import time
        print("Blinking LEDs of node: {0}".format(self.id))
        time.sleep(CMD_PARAM_IDENTIFY_NUM_BLINKS * CMD_PARAM_IDENTIFY_BLINK_TIME)

# End Class


class NodeGetHwInfo(wn_message.Cmd):
    """Command to get the hardware parameters from the node."""
    def __init__(self):
        super(NodeGetHwInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_INFO
    
    def process_resp(self, resp):
        return resp.get_args()

# End Class


class NodeSetupNetwork(wn_message.Cmd):
    """Command to perform initial network setup of a node."""
    def __init__(self, node):
        super(NodeSetupNetwork, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_NETWORK_SETUP
        self.add_args(node.serial_number)
        self.add_args(node.node_id)
        self.add_args(node.transport.ip_to_int(node.transport.ip_address))
        self.add_args(node.transport.unicast_port)
        self.add_args(node.transport.bcast_port)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeResetNetwork(wn_message.Cmd):
    """Command to reset the network configuration of a node."""
    def __init__(self, serial_number, output=False):
        super(NodeResetNetwork, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_NETWORK_RESET
        self.output = output
        
        if (serial_number == CMD_PARAM_NETWORK_RESET_ALL_NODES):
            (sn, sn_str) = (CMD_PARAM_NETWORK_RESET_ALL_NODES, "All nodes")
        else:
            from . import util as wn_util

            (sn, sn_str) = wn_util.wn_get_serial_number(serial_number, output=False)
        
        self.id = sn_str
        self.add_args(sn)
    
    def process_resp(self, resp):
        if (self.output):
            print("Reset network config of node: {0}".format(self.id))

# End Class


class NodeGetTemperature(wn_message.Cmd):
    """Command to get the temperature of a node.
    
    NOTE:  The response must be converted to Celsius with the given formula:
        ((double(temp)/65536.0)/0.00198421639) - 273.15
        - http://www.xilinx.com/support/documentation/user_guides/ug370.pdf
        - 16 bit value where 10 MSBs are an ADC value
    """
    def __init__(self):
        super(NodeGetTemperature, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TEMPERATURE
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=3):
            args = resp.get_args()
            curr_temp = ((float(args[0])/65536.0)/0.00198421639) - 273.15
            min_temp  = ((float(args[1])/65536.0)/0.00198421639) - 273.15
            max_temp  = ((float(args[2])/65536.0)/0.00198421639) - 273.15
            return (curr_temp, min_temp, max_temp)
        else:
            return (0, 0, 0)

# End Class


# -----------------------------------------------------------------------------
# Transport Commands
# -----------------------------------------------------------------------------
class Ping(wn_message.Cmd):
    """Command to ping the node."""
    def __init__(self):
        super(Ping, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMDID_PING
    
    def process_resp(self, resp):
        pass

# End Class


class TestPayloadSize(wn_message.Cmd):
    """Command to perform a payload size test on a node."""
    def __init__(self, size):
        super(TestPayloadSize, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMDID_PAYLOAD_SIZE_TEST
        
        for i in range(size):
            self.add_args(i)
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return 0
        
# End Class


class AddNodeGrpId(wn_message.Cmd):
    """Command to add a Node Group ID to the node so that it can process
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(AddNodeGrpId, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMDID_NODE_GRPID_ADD
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


class ClearNodeGrpId(wn_message.Cmd):
    """Command to clear a Node Group ID to the node so that it can ignore
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(ClearNodeGrpId, self).__init__()
        self.command = _CMD_GRPID_TRANS + CMDID_NODE_GRPID_CLEAR
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


