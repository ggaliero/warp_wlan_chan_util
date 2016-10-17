# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework 
    - Transport / Network / Basic Node Commands
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides class definitions for transport, network, and basic node 
commands.  

Functions (see below for more information):
    GetType()
    Identify()
    GetHwInfo() 
    SetupNetwork()
    ResetNetwork()
    Ping()    
    TestPayloadSize()
    AddNodeGrpId()
    ClearNodeGrpId()

Integer constants:
    GRPID_NODE, GRPID_TRANS - Command Groups
    GRP_NODE, GRP_TRANSPORT - Command Group Names

Many other constants may be defined; please do not use these values when 
defining other sub-classes of CmdResp and BufferCmd.

"""


from . import message


__all__ = ['NodeGetType', 'NodeIdentify', 'NodeGetHwInfo', 
           'NodeSetupNetwork', 'NodeResetNetwork', 'NodeGetTemperature', 
           'TransportPing', 'TransportTestPayloadSize', 
           'TransportAddNodeGroupId', 'TransportClearNodeGroupId']


# Command Groups
GROUP_NODE                                       = 0x00
GROUP_TRANSPORT                                  = 0x10
GROUP_USER                                       = 0x20


# Command Group names
GROUP_NAME_NODE                                  = 'node'
GROUP_NAME_TRANSPORT                             = 'transport'
GROUP_NAME_USER                                  = 'user'


# Node Command IDs
#     - The C counterparts are found in *_node.h
CMDID_NODE_TYPE                                  = 0x000000

CMD_PARAM_NODE_TYPE_RSVD                         = 0xFFFFFFFF

CMDID_NODE_INFO                                  = 0x000001
CMDID_NODE_IDENTIFY                              = 0x000002

CMD_PARAM_NODE_IDENTIFY_ALL_NODES                = 0xFFFFFFFF
CMD_PARAM_NODE_IDENTIFY_NUM_BLINKS               = 25
CMD_PARAM_NODE_IDENTIFY_BLINK_TIME               = 0.4

CMDID_NODE_NETWORK_SETUP                         = 0x000003
CMDID_NODE_NETWORK_RESET                         = 0x000004

CMD_PARAM_NODE_NETWORK_RESET_ALL_NODES           = 0xFFFFFFFF

CMDID_NODE_TEMPERATURE                           = 0x000005


# Transport Command IDs
#     - The C counterparts are found in *_transport.h
CMDID_TRANSPORT_PING                             = 0x000001
CMDID_TRANSPORT_PAYLOAD_SIZE_TEST                = 0x000002
CMDID_TRANSPORT_NODE_GROUP_ID_ADD                = 0x000100
CMDID_TRANSPORT_NODE_GROUP_ID_CLEAR              = 0x000101


# Local Constants
_CMD_GROUP_NODE                                  = (GROUP_NODE << 24)
_CMD_GROUP_TRANSPORT                             = (GROUP_TRANSPORT << 24)
_CMD_GROUP_USER                                  = (GROUP_USER << 24)



# -----------------------------------------------------------------------------
# Node Commands
# -----------------------------------------------------------------------------

class NodeGetType(message.Cmd):
    """Command to get the Type of the node"""
    def __init__(self):
        super(NodeGetType, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TYPE
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return CMD_PARAM_NODE_TYPE_RSVD

# End Class


class NodeIdentify(message.Cmd):
    """Command to blink the Node LEDs."""
    def __init__(self, serial_number):
        super(NodeIdentify, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_IDENTIFY

        if (serial_number == CMD_PARAM_NODE_IDENTIFY_ALL_NODES):
            (sn, sn_str) = (CMD_PARAM_NODE_IDENTIFY_ALL_NODES, "All nodes")
        else:
            from . import util

            (sn, sn_str) = util.get_serial_number(serial_number, output=False)

        time_factor    = 6             # Time on node is microseconds
        time_to_send   = int(round(CMD_PARAM_NODE_IDENTIFY_BLINK_TIME, time_factor) * (10**time_factor))

        self.id = sn_str
        self.add_args(sn)
        self.add_args(CMD_PARAM_NODE_IDENTIFY_NUM_BLINKS)
        self.add_args(time_to_send)
             
    def process_resp(self, resp):
        import time
        print("Blinking LEDs of node: {0}".format(self.id))
        time.sleep(CMD_PARAM_NODE_IDENTIFY_NUM_BLINKS * CMD_PARAM_NODE_IDENTIFY_BLINK_TIME)

# End Class


class NodeGetHwInfo(message.Cmd):
    """Command to get the hardware parameters from the node."""
    def __init__(self):
        super(NodeGetHwInfo, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_INFO
    
    def process_resp(self, resp):
        return resp.get_args()

# End Class


class NodeSetupNetwork(message.Cmd):
    """Command to perform initial network setup of a node."""
    def __init__(self, node):
        super(NodeSetupNetwork, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_NETWORK_SETUP
        self.add_args(node.serial_number)
        self.add_args(node.node_id)
        self.add_args(node.transport.ip_to_int(node.transport.ip_address))
        self.add_args(node.transport.unicast_port)
        self.add_args(node.transport.broadcast_port)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeResetNetwork(message.Cmd):
    """Command to reset the network configuration of a node."""
    def __init__(self, serial_number, output=False):
        super(NodeResetNetwork, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_NETWORK_RESET
        self.output = output
        
        if (serial_number == CMD_PARAM_NODE_NETWORK_RESET_ALL_NODES):
            (sn, sn_str) = (CMD_PARAM_NODE_NETWORK_RESET_ALL_NODES, "All nodes")
        else:
            from . import util

            (sn, sn_str) = util.get_serial_number(serial_number, output=False)
        
        self.id = sn_str
        self.add_args(sn)
    
    def process_resp(self, resp):
        if (self.output):
            print("Reset network config of node: {0}".format(self.id))

# End Class


class NodeGetTemperature(message.Cmd):
    """Command to get the temperature of a node.
    
    The response must be converted to Celsius with the given formula:
        ((double(temp)/65536.0)/0.00198421639) - 273.15
        - http://www.xilinx.com/support/documentation/user_guides/ug370.pdf
        - 16 bit value where 10 MSBs are an ADC value
    """
    def __init__(self):
        super(NodeGetTemperature, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TEMPERATURE
    
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
class TransportPing(message.Cmd):
    """Command to ping the node."""
    def __init__(self):
        super(TransportPing, self).__init__()
        self.command = _CMD_GROUP_TRANSPORT + CMDID_TRANSPORT_PING
    
    def process_resp(self, resp):
        pass

# End Class


class TransportTestPayloadSize(message.Cmd):
    """Command to perform a payload size test on a node."""
    def __init__(self, size):
        super(TransportTestPayloadSize, self).__init__()
        self.command = _CMD_GROUP_TRANSPORT + CMDID_TRANSPORT_PAYLOAD_SIZE_TEST
        
        for i in range(size):
            self.add_args(i)
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return 0
        
# End Class


class TransportAddNodeGroupId(message.Cmd):
    """Command to add a Node Group ID to the node so that it can process
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(TransportAddNodeGroupId, self).__init__()
        self.command = _CMD_GROUP_TRANSPORT + CMDID_TRANSPORT_NODE_GROUP_ID_ADD
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


class TransportClearNodeGroupId(message.Cmd):
    """Command to clear a Node Group ID to the node so that it can ignore
    broadcast commands that are received from that node group."""
    def __init__(self, group):
        super(TransportClearNodeGroupId, self).__init__()
        self.command = _CMD_GROUP_TRANSPORT + CMDID_TRANSPORT_NODE_GROUP_ID_CLEAR
        self.add_args(group)
    
    def process_resp(self, resp):
        pass

# End Class


