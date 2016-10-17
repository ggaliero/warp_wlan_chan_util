# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Commands
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides class definitions for all wlan_exp commands.

"""

import wlan_exp.transport.cmds as cmds
import wlan_exp.transport.message as message
import wlan_exp.transport.transport_eth_ip_udp as transport

__all__ = [# Log command classes
           'LogGetEvents', 'LogConfigure', 'LogGetStatus', 'LogGetCapacity', 
           'LogAddExpInfoEntry',
           # Counts command classes
           'CountsConfigure', 'CountsGetTxRx',
           # LTG classes
           'LTGConfigure', 'LTGStart', 'LTGStop', 'LTGRemove', 'LTGStatus',
           # Node command classes
           'NodeResetState', 'NodeConfigure', 'NodeProcWLANMACAddr', 
           'NodeProcTime', 'NodeSetLowToHighFilter', 'NodeProcRandomSeed', 
           'NodeLowParam', 'NodeProcTxPower', 'NodeProcTxRate', 
           'NodeProcTxAntMode', 'NodeProcRxAntMode',
           # Scan command classes
           'NodeProcScanParam', 'NodeProcScan', 
           # Association command classes
           'NodeConfigBSS', 'NodeDisassociate', 'NodeGetStationInfo', 
           'NodeGetBSSInfo',
           # Queue command classes
           'QueueTxDataPurgeAll',
           # AP command classes
           'NodeAPConfigure', 'NodeAPAddAssociation', 'NodeAPSetAuthAddrFilter',
           # STA command classes
           'NodeSTASetAID', 'NodeSTAJoin', 'NodeSTAJoinStatus',
           # IBSS command classes
           # None
           # User command classes
           'UserSendCmd'
          ]


# Command IDs
#  Each command is identified by a unique 24-bit integer ID
#  The IDs defined here must match the corresponding definitions in
#   wlan_exp_node.h
CMDID_NODE_RESET_STATE                           = 0x001000
CMDID_NODE_CONFIGURE                             = 0x001001
CMDID_NODE_CONFIG_BSS                            = 0x001002
CMDID_NODE_TIME                                  = 0x001010
CMDID_NODE_CHANNEL                               = 0x001011
CMDID_NODE_TX_POWER                              = 0x001012
CMDID_NODE_TX_RATE                               = 0x001013
CMDID_NODE_TX_ANT_MODE                           = 0x001014
CMDID_NODE_RX_ANT_MODE                           = 0x001015
CMDID_NODE_LOW_TO_HIGH_FILTER                    = 0x001016
CMDID_NODE_RANDOM_SEED                           = 0x001017
CMDID_NODE_WLAN_MAC_ADDR                         = 0x001018
CMDID_NODE_LOW_PARAM                             = 0x001020

CMD_PARAM_WRITE                                  = 0x00000000
CMD_PARAM_READ                                   = 0x00000001
CMD_PARAM_WRITE_DEFAULT                          = 0x00000002
CMD_PARAM_READ_DEFAULT                           = 0x00000004
CMD_PARAM_RSVD                                   = 0xFFFFFFFF

CMD_PARAM_SUCCESS                                = 0x00000000
CMD_PARAM_WARNING                                = 0xF0000000
CMD_PARAM_ERROR                                  = 0xFF000000

CMD_PARAM_UNICAST                                = 0x00000000
CMD_PARAM_MULTICAST_DATA                         = 0x00000001
CMD_PARAM_MULTICAST_MGMT                         = 0x00000002

CMD_PARAM_NODE_CONFIG_ALL                        = 0xFFFFFFFF

CMD_PARAM_NODE_RESET_FLAG_LOG                    = 0x00000001
CMD_PARAM_NODE_RESET_FLAG_TXRX_COUNTS            = 0x00000002
CMD_PARAM_NODE_RESET_FLAG_LTG                    = 0x00000004
CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE          = 0x00000008
CMD_PARAM_NODE_RESET_FLAG_BSS                    = 0x00000010
CMD_PARAM_NODE_RESET_FLAG_NETWORK_LIST           = 0x00000020

CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE           = 0x00000001
CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE    = 0x00000002
CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL   = 0x80000000

ERROR_CONFIG_BSS_BSSID_INVALID                   = 0x000001
ERROR_CONFIG_BSS_BSSID_INSUFFICIENT_ARGUMENTS    = 0x000002
ERROR_CONFIG_BSS_CHANNEL_INVALID                 = 0x000004
ERROR_CONFIG_BSS_BEACON_INTERVAL_INVALID         = 0x000008
ERROR_CONFIG_BSS_HT_CAPABLE_INVALID              = 0x000010

CMD_PARAM_TIME_ADD_TO_LOG                        = 0x00000002
CMD_PARAM_RSVD_TIME                              = 0xFFFFFFFF

TIME_TYPE_FLOAT                                  = 0x00000000
TIME_TYPE_INT                                    = 0x00000001

CMD_PARAM_RSVD_MAC_ADDR                          = 0x00000000

CMD_PARAM_NODE_TX_POWER_LOW                      = 0x00000010
CMD_PARAM_NODE_TX_POWER_ALL                      = 0x00000020

CMD_PARAM_NODE_TX_ANT_ALL                        = 0x00000010

CMD_PARAM_RX_FILTER_FCS_GOOD                     = 0x1000
CMD_PARAM_RX_FILTER_FCS_ALL                      = 0x2000
CMD_PARAM_RX_FILTER_FCS_NOCHANGE                 = 0xF000

CMD_PARAM_RX_FILTER_HDR_ADDR_MATCH_MPDU          = 0x0001
CMD_PARAM_RX_FILTER_HDR_ALL_MPDU                 = 0x0002
CMD_PARAM_RX_FILTER_HDR_ALL                      = 0x0003
CMD_PARAM_RX_FILTER_HDR_NOCHANGE                 = 0x0FFF

CMD_PARAM_RANDOM_SEED_VALID                      = 0x00000001
CMD_PARAM_RANDOM_SEED_RSVD                       = 0xFFFFFFFF

# Low Param IDs -- in sync with wlan_mac_low.h
CMD_PARAM_LOW_PARAM_BB_GAIN                      = 0x00000001
CMD_PARAM_LOW_PARAM_LINEARITY_PA                 = 0x00000002
CMD_PARAM_LOW_PARAM_LINEARITY_VGA                = 0x00000003
CMD_PARAM_LOW_PARAM_LINEARITY_UPCONV             = 0x00000004
CMD_PARAM_LOW_PARAM_AD_SCALING                   = 0x00000005
CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER            = 0x00000006
CMD_PARAM_LOW_PARAM_PHY_SAMPLE_RATE              = 0x00000008

CMD_PARAM_LOW_PARAM_DCF_RTS_THRESH               = 0x10000001
CMD_PARAM_LOW_PARAM_DCF_DOT11SHORTRETRY          = 0x10000002
CMD_PARAM_LOW_PARAM_DCF_DOT11LONGRETRY           = 0x10000003
CMD_PARAM_LOW_PARAM_DCF_PHYSICAL_CS_THRESH       = 0x10000004
CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MIN               = 0x10000005
CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MAX               = 0x10000006

CMD_PARAM_NODE_MIN_MIN_PKT_DET_POWER_DBM         = -90
CMD_PARAM_NODE_MAX_MIN_PKT_DET_POWER_DBM         = -30

# LTG commands and defined values
CMDID_LTG_CONFIG                                 = 0x002000
CMDID_LTG_START                                  = 0x002001
CMDID_LTG_STOP                                   = 0x002002
CMDID_LTG_REMOVE                                 = 0x002003
CMDID_LTG_STATUS                                 = 0x002004

CMD_PARAM_LTG_ERROR                              = 0x000001

CMD_PARAM_LTG_ALL_LTGS                           = 0xFFFFFFFF

CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART              = 0x00000001

CMD_PARAM_LTG_RUNNING                            = 0x00000001
CMD_PARAM_LTG_STOPPED                            = 0x00000000


# Log commands and defined values
CMDID_LOG_CONFIG                                 = 0x003000
CMDID_LOG_GET_STATUS                             = 0x003001
CMDID_LOG_GET_CAPACITY                           = 0x003002
CMDID_LOG_GET_ENTRIES                            = 0x003003
CMDID_LOG_ADD_EXP_INFO_ENTRY                     = 0x003004

CMDID_LOG_ENABLE_ENTRY                           = 0x003006

CMD_PARAM_LOG_GET_ALL_ENTRIES                    = 0xFFFFFFFF

CMD_PARAM_LOG_CONFIG_FLAG_LOGGING                = 0x00000001
CMD_PARAM_LOG_CONFIG_FLAG_WRAP                   = 0x00000002
CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS           = 0x00000004
CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU              = 0x00000008
CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL              = 0x00000010


# Counts commands and defined values
CMDID_COUNTS_GET_TXRX                            = 0x004001

CMD_PARAM_COUNTS_CONFIG_FLAG_PROMISC             = 0x00000001

CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE           = 0x80000000


# Queue commands and defined values
CMDID_QUEUE_TX_DATA_PURGE_ALL                    = 0x005000


# Scan commands and defined values
CMDID_NODE_SCAN_PARAM                            = 0x006000
CMDID_NODE_SCAN                                  = 0x006001

CMD_PARAM_NODE_SCAN_ENABLE                       = 0x00000001
CMD_PARAM_NODE_SCAN_DISABLE                      = 0x00000000


# Association commands and defined values
CMDID_GET_BSS_MEMBERS                            = 0x007001
CMDID_GET_BSS_INFO                               = 0x007002
CMDID_GET_STATION_INFO_LIST                      = 0x007003

CMDID_NODE_DISASSOCIATE                          = 0x007010
CMDID_NODE_ADD_ASSOCIATION                       = 0x007011

CMD_PARAM_ADD_ASSOCIATION_DISABLE_INACTIVITY_TIMEOUT = 0x00000001
CMD_PARAM_ADD_ASSOCIATION_HT_CAPABLE_STA         = 0x00000004

CMD_PARAM_GET_ALL_ASSOCIATED                     = 0x0000000000000000

CMD_PARAM_MAX_SSID_LEN                           = 32
CMD_PARAM_RSVD_CHANNEL                           = 0


# AP commands and defined values
CMDID_NODE_AP_CONFIG                             = 0x100000
CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER     = 0x100001

CMD_PARAM_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER   = 0x00000001


# STA commands and defined values
CMDID_NODE_STA_JOIN                              = 0x100000
CMDID_NODE_STA_JOIN_STATUS                       = 0x100001
CMDID_NODE_STA_SET_AID                           = 0x100002


# IBSS commands and defined values


# Developer commands and defined values
CMDID_DEV_MEM_HIGH                               = 0xFFF000
CMDID_DEV_MEM_LOW                                = 0xFFF001
CMDID_DEV_EEPROM                                 = 0xFFF002


# Local Constants
_CMD_GROUP_NODE                                  = (cmds.GROUP_NODE << 24)
_CMD_GROUP_USER                                  = (cmds.GROUP_USER << 24)


#-----------------------------------------------------------------------------
# Class Definitions for wlan_exp Commands
#-----------------------------------------------------------------------------

#--------------------------------------------
# Log Commands
#--------------------------------------------
class LogGetEvents(message.BufferCmd):
    """Command to retreive log data from a wlan_exp node"""
    def __init__(self, size, start_byte=0):
        command = _CMD_GROUP_NODE + CMDID_LOG_GET_ENTRIES

        if (size == CMD_PARAM_LOG_GET_ALL_ENTRIES):
            size = message.CMD_BUFFER_GET_SIZE_FROM_DATA

        super(LogGetEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class LogConfigure(message.Cmd):
    """Command to configure the Event log.

    Attributes (default state on the node is in CAPS):
        log_enable           -- Enable the event log (TRUE/False)
        log_wrap_enable      -- Enable event log wrapping (True/FALSE)
        log_full_payloads    -- Record full Tx/Rx payloads in event log (True/FALSE)
        log_txrx_mpdu        -- Enable Tx/Rx log entries for MPDU frames (TRUE/False)
        log_txrx_ctrl        -- Enable Tx/Rx log entries for CTRL frames (TRUE/False)
    """
    def __init__(self, log_enable=None, log_wrap_enable=None,
                       log_full_payloads=None, log_txrx_mpdu=None, 
                       log_txrx_ctrl=None):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_CONFIG

        flags = 0
        mask  = 0

        if log_enable is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOGGING
            if log_enable:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOGGING

        if log_wrap_enable is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_WRAP
            if log_wrap_enable:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_WRAP

        if log_full_payloads is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS
            if log_full_payloads:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS

        if log_txrx_mpdu is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU
            if log_txrx_mpdu:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU

        if log_txrx_ctrl is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL
            if log_txrx_ctrl:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL

        self.add_args(flags)
        self.add_args(mask)

    def process_resp(self, resp):
        pass

# End Class


class LogGetStatus(message.Cmd):
    """Command to get the state information about the log."""
    def __init__(self):
        super(LogGetStatus, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_GET_STATUS

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=4):
            args = resp.get_args()
            return (args[0], args[1], args[2], args[3])
        else:
            return (0,0,0,0)

# End Class


class LogGetCapacity(message.Cmd):
    """Command to get the log capacity and current use."""
    def __init__(self):
        super(LogGetCapacity, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_GET_CAPACITY

    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=2):
            args = resp.get_args()
            return (args[0], args[1])
        else:
            return (0,0)

# End Class


class LogAddExpInfoEntry(message.Cmd):
    """Command to write a EXP_INFO Log Entry to the node."""
    def __init__(self, info_type, message):
        super(LogAddExpInfoEntry, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LOG_ADD_EXP_INFO_ENTRY

        self.add_args(info_type)

        if message is None:
            self.add_args(0)
        else:
            import struct
            data_len = len(message)
            data_buf = bytearray(message, 'UTF-8')

            # Add initial agruments
            self.add_args(data_len)

            # Zero pad so that the data buffer is 32-bit aligned
            if ((len(data_buf) % 4) != 0):
                data_buf += bytearray(4 - (len(data_buf) % 4))

            idx = 0
            while (idx < len(data_buf)):
                arg = struct.unpack_from('!I', data_buf[idx:idx+4])
                self.add_args(arg[0])
                idx += 4

    def process_resp(self, resp):
        pass

# End Class


#--------------------------------------------
# Counts Commands
#--------------------------------------------
class CountsGetTxRx(message.BufferCmd):
    """Command to get the counts from the node for a given node."""
    def __init__(self, node=None, return_zeroed_counts_if_none=False):
        flags       = 0

        # Compute the argument for the BufferCmd flags
        if return_zeroed_counts_if_none:
            flags += CMD_PARAM_COUNTS_RETURN_ZEROED_IF_NONE

        # Call parent initialziation
        super(CountsGetTxRx, self).__init__(flags=flags)
        self.command = _CMD_GROUP_NODE + CMDID_COUNTS_GET_TXRX
        mac_address  = None

        if node is not None:
            mac_address = node.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a Buffer of all Tx / Rx counts.  Need to convert to a list
        #   of TxRxCounts()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "TxRxCounts()")

        return ret_val

# End Class



#--------------------------------------------
# Local Traffic Generation (LTG) Commands
#--------------------------------------------
class LTGCommon(message.Cmd):
    """Common code for LTG Commands."""
    name = None

    def __init__(self, ltg_id=None):
        super(LTGCommon, self).__init__()

        if ltg_id is not None:
            if type(ltg_id) is not int:
                raise TypeError("LTG ID must be an integer.")
            self.add_args(ltg_id)
        else:
            self.add_args(CMD_PARAM_LTG_ALL_LTGS)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "Could not {0} the LTG with that LTG ID.".format(self.name)
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=1,
                              status_errors=status_errors,
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[0]
        else:
            return CMD_PARAM_ERROR

# End Class


class LTGConfigure(message.Cmd):
    """Command to configure an LTG with the given traffic flow to the
    specified node.
    """
    name = 'configure'

    def __init__(self, traffic_flow, auto_start=False):
        super(LTGConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LTG_CONFIG

        flags = 0

        if auto_start:
            flags += CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART

        self.add_args(flags)

        for arg in traffic_flow.serialize():
            self.add_args(arg)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "\n\nERROR MESSAGE: Could not create the LTG.  Check that the node \n"
        error_msg    += "has enough heap available to perform this operation. Commonly, \n"
        error_msg    += "this can occur if LTGs are not removed (ie 'ltg_remove(ltg_id)') \n"
        error_msg    += "when they are finished being used.\n"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2,
                              status_errors=status_errors,
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[1]
        else:
            return CMD_PARAM_ERROR

# End Class


class LTGStart(LTGCommon):
    """Command to start an LTG. The LTG must have been previously configured. If no
    ltg_id is provided the node will start all LTG instances that are currently configured.
    """
    name = 'start'

    def __init__(self, ltg_id=None):
        super(LTGStart, self).__init__(ltg_id)
        self.command = _CMD_GROUP_NODE + CMDID_LTG_START

# End Class


class LTGStop(LTGCommon):
    """Command to stop an LTG. The LTG must have been previously configured. If no
    ltg_id is provided the node will stop all LTG instances that are currently configured.
    """
    name = 'stop'

    def __init__(self, ltg_id=None):
        super(LTGStop, self).__init__(ltg_id)
        self.command = _CMD_GROUP_NODE + CMDID_LTG_STOP

# End Class


class LTGRemove(LTGCommon):
    """Command to remove an LTG instance. If no ltg_id is provided the node will remove all
    LTG instances that are currently configured.
    """
    name = 'remove'

    def __init__(self, ltg_id=None):
        super(LTGRemove, self).__init__(ltg_id)
        self.command = _CMD_GROUP_NODE + CMDID_LTG_REMOVE

# End Class


class LTGStatus(message.Cmd):
    """Command to get the status of the LTG."""
    name        = 'status'

    def __init__(self, ltg_id):
        super(LTGStatus, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_LTG_STATUS

        if type(ltg_id) is not int:
            raise TypeError("LTG ID must be an integer.")
        self.add_args(ltg_id)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "\n\nERROR MESSAGE:  Could not find status for given LTG ID.\n"
        error_msg    += "Please check that it has not been removed."
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=6,
                              status_errors=status_errors,
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()

            start_timestamp = (2**32 * args[3]) + args[2]
            stop_timestamp  = (2**32 * args[5]) + args[4]

            return (True, (args[1] == CMD_PARAM_LTG_RUNNING), start_timestamp, stop_timestamp)
        else:
            return (False, False, 0, 0)

# End Class



#--------------------------------------------
# Configure Node Attribute Commands
#--------------------------------------------
class NodeResetState(message.Cmd):
    """Command to reset the state of a portion of the node defined by the flags.

    Attributes:
        flags -- [0] NODE_RESET_LOG
                 [1] NODE_RESET_TXRX_STATS
                 [2] NODE_RESET_LTG
                 [3] NODE_RESET_TX_DATA_QUEUE
                 [4] NODE_RESET_FLAG_BSS
                 [5] NODE_RESET_FLAG_NETWORK_LIST
    """
    def __init__(self, flags):
        super(NodeResetState, self).__init__()
        self.command = _CMD_GROUP_NODE +  CMDID_NODE_RESET_STATE
        self.add_args(flags)

    def process_resp(self, resp):
        pass

# End Class


class NodeConfigure(message.Cmd):
    """Command to configure flag parameters on the node

    Attributes:
        dsss_enable (bool) -- Whether DSSS packets are received.
        beacon_mac_time_update (bool) -- Whether MAC time is updated from beacons
        print_level (int) -- Controls verbosity of wlan_exp prints to the node's UART
    """
    def __init__(self, dsss_enable=None, beacon_mac_time_update=None, print_level=None):
        super(NodeConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_CONFIGURE

        flags = 0
        mask  = 0
        level = 0

        if dsss_enable is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE
            if dsss_enable:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE

        if beacon_mac_time_update is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE
            if beacon_mac_time_update:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TIME_UPDATE

        if print_level is not None:
            if (type(print_level) is str):
                import wlan_exp.util as util
                level = CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL + util.phy_modes[print_level]
            else:
                level = CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL + print_level            

        self.add_args(flags)
        self.add_args(mask)
        self.add_args(level)

    def process_resp(self, resp):
        pass

# End Class


class NodeProcWLANMACAddr(message.Cmd):
    """Command to get / set the WLAN MAC Address on the node.

    Attributes:
        cmd           -- Sub-command to send over the command.  Valid values are:
                           CMD_PARAM_READ
                           CMD_PARAM_WRITE
        wlan_mac_addr -- 48-bit MAC address to write (optional)
    """
    def __init__(self, cmd, wlan_mac_address=None):
        super(NodeProcWLANMACAddr, self).__init__()
        self.command  = _CMD_GROUP_NODE + CMDID_NODE_WLAN_MAC_ADDR

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)
            _add_mac_address_to_cmd(self, wlan_mac_address)

        elif (cmd == CMD_PARAM_READ):
            self.add_args(cmd)

        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the WLAN MAC address on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=3, status_errors=status_errors, name='from the WLAN Mac Address command'):
            args = resp.get_args()
            addr = (2**32 * args[1]) + args[2]
        else:
            addr = None

        return addr

# End Class


class NodeProcTime(message.Cmd):
    """Command to get / set the time on the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       TIME_ADD_TO_LOG
        node_time -- Time to set the on the node
        time_id   -- ID to use identify the time command in the log.
    """
    def __init__(self, cmd, node_time, time_id=None):
        super(NodeProcTime, self).__init__()
        self.command  = _CMD_GROUP_NODE + CMDID_NODE_TIME

        # Read the time as int microseconds
        if (cmd == CMD_PARAM_READ):
            self.add_args(CMD_PARAM_READ)
            self.add_args(CMD_PARAM_RSVD_TIME)             # Reads do not need a time_id
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)

        # Write the time / Add time to log
        else:
            import time

            # Set the command
            if (cmd == CMD_PARAM_WRITE):
                self.add_args(CMD_PARAM_WRITE)
            else:
                self.add_args(CMD_PARAM_TIME_ADD_TO_LOG)
                node_time = None

            # By default set the time_id to a random number between [0, 2^32)
            if time_id is None:
                import random
                time_id = 2**32 * random.random()

            self.add_args(int(time_id))

            # Add the time
            _add_time_to_cmd64(self, node_time)
            
            # Get the current time on the host
            _add_time_to_cmd64(self, time.time())


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the time on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=5, status_errors=status_errors, name='from the Time command'):
            args = resp.get_args()
            mac_time = (2**32 * args[2]) + args[1]
            sys_time = (2**32 * args[4]) + args[3]
        else:
            mac_time = 0
            sys_time = 0

        return (mac_time, sys_time)

# End Class


class NodeSetLowToHighFilter(message.Cmd):
    """Command to set the low to high filter on the node.

    Attributes:
        mac_header -- MAC header filter.  Values can be:
                        'MPDU_TO_ME' -- Pass any unicast-to-me or multicast data or
                                        management packet
                        'ALL_MPDU'   -- Pass any data or management packet (no address filter)
                        'ALL'        -- Pass any packet (no type or address filters)
        FCS        -- FCS status filter.  Values can be:
                        'GOOD'       -- Pass only packets with good checksum result
                        'ALL'        -- Pass packets with any checksum result
    """
    def __init__(self, cmd, mac_header=None, fcs=None):
        super(NodeSetLowToHighFilter, self).__init__()
        self.command  = _CMD_GROUP_NODE + CMDID_NODE_LOW_TO_HIGH_FILTER

        self.add_args(cmd)

        rx_filter = 0

        if mac_header is None:
            rx_filter += CMD_PARAM_RX_FILTER_HDR_NOCHANGE
        else:
            mac_header = str(mac_header)
            mac_header.upper()

            if   (mac_header == 'MPDU_TO_ME'):
                rx_filter += CMD_PARAM_RX_FILTER_HDR_ADDR_MATCH_MPDU
            elif (mac_header == 'ALL_MPDU'):
                rx_filter += CMD_PARAM_RX_FILTER_HDR_ALL_MPDU
            elif (mac_header == 'ALL'):
                rx_filter += CMD_PARAM_RX_FILTER_HDR_ALL
            else:
                msg  = "WARNING:  Not a valid mac_header value.\n"
                msg += "    Provided:  {0}\n".format(mac_header)
                msg += "    Requires:  ['MPDU_TO_ME', 'ALL_MPDU', 'ALL']"
                print(msg)
                rx_filter += CMD_PARAM_RX_FILTER_HDR_NOCHANGE

        if fcs is None:
            rx_filter += CMD_PARAM_RX_FILTER_FCS_NOCHANGE
        else:
            fcs = str(fcs)
            fcs.upper()

            if   (fcs == 'GOOD'):
                rx_filter += CMD_PARAM_RX_FILTER_FCS_GOOD
            elif (fcs == 'ALL'):
                rx_filter += CMD_PARAM_RX_FILTER_FCS_ALL
            else:
                msg  = "WARNING: Not a valid fcs value.\n"
                msg += "    Provided:  {0}\n".format(fcs)
                msg += "    Requires:  ['GOOD', 'ALL']"
                print(msg)
                rx_filter += CMD_PARAM_RX_FILTER_FCS_NOCHANGE

        self.add_args(rx_filter)


    def process_resp(self, resp):
        pass

# End Class

class NodeProcChannel(message.Cmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        channel   -- 802.11 Channel for the node.  Should be a valid channel defined
                       in wlan_exp.util wlan_channel table.
    """
    channel  = None

    def __init__(self, cmd, channel=None):
        super(NodeProcChannel, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_CHANNEL

        self.add_args(cmd)
        
        if channel is not None:
            self.channel = _get_channel_number(channel)

        if self.channel is not None:
            self.add_args(self.channel)
        else:
            self.add_args(CMD_PARAM_RSVD_CHANNEL)

    
    def process_resp(self, resp):
        import wlan_exp.util as util
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the channel on the node"
        status_errors = { error_code : error_msg }
        
        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from the Channel command'):
            args = resp.get_args()
            if self.channel is not None:
                if (args[1] != self.channel):
                    msg  = "WARNING: Channel mismatch.\n"
                    msg += "    Tried to set channel to {0}\n".format(util.channel_to_str(self.channel))
                    msg += "    Actually set channel to {0}\n".format(util.channel_to_str(args[1]))
                    print(msg)
            return util.get_channel_info(args[1])
        else:
            return None

# End Class

class NodeProcRandomSeed(message.Cmd):
    """Command to set the random seed of the node.

    If a seed is not provided, then the seed is not updated.
    
    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ   (Not supported)
                       CMD_PARAM_WRITE
        high_seed -- Random number generator seed for CPU high
        low_seed  -- Random number generator seed for CPU low

    import random
    high_seed = random.randint(0, 0xFFFFFFFF)    
    """
    def __init__(self, cmd, high_seed=None, low_seed=None):
        super(NodeProcRandomSeed, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_RANDOM_SEED

        if (cmd == CMD_PARAM_READ):
            raise AttributeError("Read not supported for NodeProcRandomSeed.")

        self.add_args(CMD_PARAM_WRITE)

        if high_seed is not None:
            self.add_args(CMD_PARAM_RANDOM_SEED_VALID)
            self.add_args(high_seed)
        else:
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)

        if low_seed is not None:
            self.add_args(CMD_PARAM_RANDOM_SEED_VALID)
            self.add_args(high_seed)
        else:
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)
            self.add_args(CMD_PARAM_RANDOM_SEED_RSVD)

    def process_resp(self, resp):
        pass

# End Class


class NodeLowParam(message.Cmd):
    """Command to set parameter in CPU Low

    Attributes:
        cmd          -- Sub-command to send over the command.  Valid values are:
                          CMD_PARAM_READ   (Not supported)
                          CMD_PARAM_WRITE
        param_id     -- ID of parameter to modify
        param_values -- Scalar or list of u32 values to write

    """
    def __init__(self, cmd, param_id, param_values=None):
        super(NodeLowParam, self).__init__()

        self.command    = _CMD_GROUP_NODE + CMDID_NODE_LOW_PARAM

        if (cmd == CMD_PARAM_READ):
            raise AttributeError("Read not supported for NodeLowParam.")

        # Caluculate the size of the entire message to CPU Low [PARAM_ID, ARGS[]]
        size = 1
        if param_values is not None:
            try:
                size += len(param_values)
            except TypeError:
                pass

        self.add_args(cmd)
        self.add_args(size)
        self.add_args(param_id)

        if param_values is not None:
            try:
                for v in param_values:
                    self.add_args(v)
            except TypeError:
                self.add_args(param_values)

    def process_resp(self, resp):
        """ Message format:
                respArgs32[0]   Status
        """
        error_code_base         = CMD_PARAM_ERROR
        error_code_cs_thresh    = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_DCF_PHYSICAL_CS_THRESH
        error_code_cw_min       = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MIN
        error_code_cw_max       = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_DCF_CW_EXP_MAX

        status_errors = {error_code_base      : "Error setting low parameter",
                         error_code_cs_thresh : "Could not set carrier sense threshold",
                         error_code_cw_min    : "Could not set minimum contention window",
                         error_code_cw_max    : "Could not set maximum contention window"}

        resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Low Param command')

        return None

# End Class


class NodeProcTxPower(message.Cmd):
    """Command to get / set the transmit power of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
                       CMD_PARAM_NODE_TX_POWER_LOW
                       CMD_PARAM_NODE_TX_POWER_ALL
        power     -- Tuple:
                       Transmit power for the WARP node (in dBm)
                       Maximum transmit power for the WARP node (in dBm)
                       Minimum transmit power for the WARP node (in dBm)
        device    -- 802.11 device for which the rate is being set.
    """
    max_tx_power = None
    min_tx_power = None

    def __init__(self, cmd, tx_type, power, device=None):
        super(NodeProcTxPower, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TX_POWER
        mac_address  = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        # Get max/min power 
        self.max_tx_power = power[1]
        self.min_tx_power = power[2]        
        self.add_args(self.check_power(power))

        if device is not None:
            mac_address = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def check_type(self, tx_type):
        """Check the tx type value is valid."""
        return_type = None
        valid_types = [('CMD_PARAM_UNICAST',           CMD_PARAM_UNICAST),
                       ('CMD_PARAM_MULTICAST_DATA',    CMD_PARAM_MULTICAST_DATA),
                       ('CMD_PARAM_MULTICAST_MGMT',    CMD_PARAM_MULTICAST_MGMT),
                       ('CMD_PARAM_NODE_TX_POWER_LOW', CMD_PARAM_NODE_TX_POWER_LOW),
                       ('CMD_PARAM_NODE_TX_POWER_ALL', CMD_PARAM_NODE_TX_POWER_ALL)]

        for tmp_type in valid_types:
            if (tx_type == tmp_type[1]):
                return_type = tx_type
                break

        if return_type is not None:
            return return_type
        else:
            msg  = "The type must be one of: ["
            for tmp_type in valid_types:
                msg += "{0} ".format(tmp_type[0])
            msg += "]"
            raise ValueError(msg)


    def check_power(self, power):
        """Return a valid power (in dBm) from the power tuple."""
        if power is None:
            raise ValueError("Must supply value to set Tx power.")
            
        ret_val = power[0]
            
        if (power[0] > power[1]):
            msg  = "WARNING:  Requested power too high.\n"
            msg += "    Adjusting transmit power from {0} to {1}".format(power[0], power[1])
            print(msg)
            ret_val = power[1]

        if (power[0] < power[2]):
            msg  = "WARNING:  Requested power too low. \n"
            msg += "    Adjusting transmit power from {0} to {1}".format(power[0], power[2])
            print(msg)
            ret_val = power[2]

        # Shift the value so that there are only positive integers over the wire
        return (ret_val - power[2])


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the transmit power on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx power command'):
            args = resp.get_args()

            # Shift the value since only positive integers are transmitted over the wire
            return (args[1] + self.min_tx_power)
        else:
            return None

# End Class


class NodeProcTxRate(message.Cmd):
    """Command to get / set the transmit rate of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
        rate      -- Tuple of values that make up Tx rate
                       [0] - mcs       -- Modulation and coding scheme (MCS) index
                       [1] - phy_mode  -- PHY mode (from util.phy_modes)
        device    -- 802.11 device for which the rate is being set.
    """
    cmd            = None
    tx_type        = None
    mac_address    = None
    
    def __init__(self, cmd, tx_type, rate=None, device=None):
        super(NodeProcTxRate, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TX_RATE
        self.cmd     = cmd
        self.tx_type = tx_type
        
        mac_address  = None
        mcs          = None
        phy_mode     = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        if (rate is not None):
            mcs      = rate[0]
            phy_mode = rate[1]

        if (mcs is not None):
            self.add_args(mcs)
        else:
            self.add_args(0)

        if (phy_mode is not None):
            if (type(phy_mode) is str):
                import wlan_exp.util as util
                self.add_args(util.phy_modes[phy_mode])
            else:
                self.add_args(phy_mode)
        else:
            self.add_args(0)

        if device is not None:
            mac_address   = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)
        
        self.mac_address = mac_address


    def check_type(self, tx_type):
        """Check the tx type value is valid."""
        return_type = None
        valid_types = [('CMD_PARAM_UNICAST',           CMD_PARAM_UNICAST),
                       ('CMD_PARAM_MULTICAST_DATA',    CMD_PARAM_MULTICAST_DATA),
                       ('CMD_PARAM_MULTICAST_MGMT',    CMD_PARAM_MULTICAST_MGMT)]

        for tmp_type in valid_types:
            if (tx_type == tmp_type[1]):
                return_type = tx_type
                break

        if return_type is not None:
            return return_type
        else:
            msg  = "The type must be one of: ["
            for tmp_type in valid_types:
                msg += "{0} ".format(tmp_type[0])
            msg += "]"
            raise ValueError(msg)

    def process_resp(self, resp):
        if (resp.resp_is_valid(num_args=3, name='from Tx rate command')):
            args    = resp.get_args()
            status  = args[0]
            ret_val = True
            
            # Check status
            if (status == CMD_PARAM_ERROR):                
                msg     = "ERROR:  Invalid response from node:\n"
                               
                if   self.tx_type == CMD_PARAM_UNICAST:
                    tx_type = "TX unicast"
                elif self.tx_type == CMD_PARAM_MULTICAST_DATA:
                    tx_type = "TX multicast data"
                elif self.tx_type == CMD_PARAM_MULTICAST_MGMT:
                    tx_type = "TX multicast management"
                
                if   self.cmd == CMD_PARAM_READ:
                    cmd = "get {0} rate".format(tx_type)
                elif self.cmd == CMD_PARAM_WRITE:
                    cmd = "set {0} rate".format(tx_type)
                elif self.cmd == CMD_PARAM_READ_DEFAULT:
                    cmd = "get {0} for new station infos".format(tx_type)
                elif self.cmd == CMD_PARAM_WRITE_DEFAULT:
                    cmd = "set {0} for new station infos".format(tx_type)
                    
                msg    += "    Could not {0} ".format(cmd)
 
                if (self.mac_address is not None):
                    import wlan_exp.util as util
                    msg += "for node {0}\n".format(util.mac_addr_to_str(self.mac_address))
                
                ret_val = False
            
            if (status == CMD_PARAM_WARNING):
                if ((args[1] == 0) or (args[1] == 7)):
                    mcs = args[1]
                else:
                    mcs = args[1] + 1
                
                msg     = "\nWARNING:  At least one associated station is not HT capable. \n"
                msg    += "    Rates for non-HT stations were configured with mcs={0} and\n".format(mcs)
                msg    += "    phy_mode='NONHT' (ie ({0}, 1)). Use node.get_station_info() \n".format(mcs)
                msg    += "    to confirm HT capabilities of associated stations.\n"
                print(msg)
            
            if not ret_val:
                print(msg)
                return None
            else:
                return (args[1], args[2])
        else:
            return None

# End Class


class NodeProcTxAntMode(message.Cmd):
    """Command to get / set the transmit antenna mode of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
                       CMD_PARAM_NODE_TX_ANT_ALL
        ant_mode  -- Transmit antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.
        device    -- 802.11 device for which the rate is being set.
    """
    def __init__(self, cmd, tx_type, ant_mode=None, device=None):
        super(NodeProcTxAntMode, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_TX_ANT_MODE
        mac_address  = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(0)

        if device is not None:
            mac_address = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def check_type(self, tx_type):
        """Check the tx type value is valid."""
        return_type = None
        valid_types = [('CMD_PARAM_UNICAST',           CMD_PARAM_UNICAST),
                       ('CMD_PARAM_MULTICAST_DATA',    CMD_PARAM_MULTICAST_DATA),
                       ('CMD_PARAM_MULTICAST_MGMT',    CMD_PARAM_MULTICAST_MGMT),
                       ('CMD_PARAM_NODE_TX_ANT_ALL',   CMD_PARAM_NODE_TX_ANT_ALL)]

        for tmp_type in valid_types:
            if (tx_type == tmp_type[1]):
                return_type = tx_type
                break

        if return_type is not None:
            return return_type
        else:
            msg  = "The type must be one of: ["
            for tmp_type in valid_types:
                msg += "{0} ".format(tmp_type[0])
            msg += "]"
            raise ValueError(msg)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        import wlan_exp.util as util
        try:
            return util.wlan_tx_ant_modes[ant_mode]
        except KeyError:
            msg  = "The antenna mode must be one of:  "
            for k in sorted(util.wlan_tx_ant_modes.keys()):
                msg += "'{0}' ".format(k)
            raise ValueError(msg)


    def process_resp(self, resp):
        import wlan_exp.util as util

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set transmit antenna mode of the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx antenna mode command'):
            args = resp.get_args()
            
            ant_mode = [k for k, v in util.wlan_tx_ant_modes.iteritems() if v == args[1]]

            if ant_mode:
                return ant_mode[0]
            else:
                msg  = "Invalid antenna mode returned by node: {0}\n".format(args[1])
                raise ValueError(msg)
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeProcRxAntMode(message.Cmd):
    """Command to get / set the receive antenna mode of the node.

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        ant_mode  -- Receive antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.
    """
    def __init__(self, cmd, ant_mode=None):
        super(NodeProcRxAntMode, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_RX_ANT_MODE

        self.add_args(cmd)
        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(CMD_PARAM_NODE_CONFIG_ALL)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        import wlan_exp.util as util
        try:
            return util.wlan_rx_ant_modes[ant_mode]
        except KeyError:
            msg  = "The antenna mode must be one of:  "
            for k in sorted(util.wlan_rx_ant_modes.keys()):
                msg += "'{0}' ".format(k)
            raise ValueError(msg)

    def process_resp(self, resp):
        import wlan_exp.util as util

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could get / set receive antenna mode of the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Rx antenna mode command'):
            args = resp.get_args()
            
            ant_mode = [k for k, v in util.wlan_rx_ant_modes.iteritems() if v == args[1]]

            if ant_mode:
                return ant_mode[0]
            else:
                msg  = "Invalid antenna mode returned by node: {0}\n".format(args[1])
                raise ValueError(msg)            
        else:
            return CMD_PARAM_ERROR

# End Class



#--------------------------------------------
# Scan Commands
#--------------------------------------------
class NodeProcScanParam(message.Cmd):
    """Command to configure the scan parameters

    Attributes:
        cmd                      -- Command for Process Scan Param:
                                        CMD_PARAM_WRITE
        time_per_channel         -- Time (in float sec) to spend on each channel (optional)
        num_probe_tx_per_channel -- Time (in float sec) to wait between each probe transmission (optional)
        channel_list             -- List of channels to scan (optional)
        ssid                     -- SSID (optional)
    """
    time_type   = None

    def __init__(self, cmd, time_per_channel=None, num_probe_tx_per_channel=None, channel_list=None, ssid=None):
        super(NodeProcScanParam, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_SCAN_PARAM

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)

            # Add the time_per_channel to the command
            _add_time_to_cmd32(self, time_per_channel)

            # Add num_probe_tx_per_channel to the command
            if num_probe_tx_per_channel is not None:
                self.add_args(num_probe_tx_per_channel)
            else:
                self.add_args(CMD_PARAM_RSVD)

            # Format the channel list
            if channel_list is not None:
                self.add_args(len(channel_list))

                for channel in channel_list:
                    self.add_channel(channel)
            else:
                self.add_args(CMD_PARAM_RSVD)

            # Add SSID
            #     - SSID should be added last to the command so that the corresponding
            #       C code does not have to compute the index of the next argument 
            #       after the SSID
            if ssid is not None:
                _add_ssid_to_cmd(self, ssid)
            else:
                self.add_args(CMD_PARAM_RSVD)                
                
        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)


    def add_channel(self, channel):
        """Internal method to add a channel"""
        import wlan_exp.util as util
        
        if channel not in util.wlan_channels:
            msg  = "Unknown channel:  {0}".format(channel)
            raise ValueError(msg)

        self.add_args(channel)
        

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set scan parameters."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Scan parameter command')):
            return True
        else:
            return False

# End Class


class NodeProcScan(message.Cmd):
    """Command to enable / disable active scan

    Attributes:
        enable -- Enabling (True) or disabling (False) active scan
    """
    def __init__(self, enable=None):
        super(NodeProcScan, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_SCAN

        if enable is None:        
            self.add_args(CMD_PARAM_RSVD)
        else:
            if enable:
                self.add_args(CMD_PARAM_NODE_SCAN_ENABLE)
            else:
                self.add_args(CMD_PARAM_NODE_SCAN_DISABLE)

    def process_resp(self, resp):
        ret_val = False
        
        if (resp.resp_is_valid(num_args=2, name='from Scan command')):
            args   = resp.get_args()
            status = args[0]
            
            # Check status
            if (status & CMD_PARAM_ERROR):
                msg  = "ERROR:  Could not start scan:\n"
                msg += "    Node BSS must be None.  Use conifgure_bss(None) before\n"
                msg += "    starting scan.\n"
                print(msg)
                ret_val = False
            else:
                if (args[1] == 1):
                    ret_val = True
                else:
                    ret_val = False
        
        return ret_val

# End Class



#--------------------------------------------
# Association Commands
#--------------------------------------------
class NodeConfigBSS(message.Cmd):
    """Command to configure the BSS on the node.

    Command assumes all parameters are appropriately validated and processed.

    Attributes:
        bssid (int):  48-bit ID of the BSS either as a integer; A value of 
            None will remove current BSS on the node (similar to node.reset(bss=True));
            A value of False will not update the current bssid.
        ssid (str):  SSID string (Must be 32 characters or less); A value of 
            None will not update the current SSID.
        channel (int): Channel on which the BSS operates; A value of None will
            not update the current channel.
        beacon_interval (int): Integer number of beacon Time Units in [1, 65534]
            (http://en.wikipedia.org/wiki/TU_(Time_Unit); a TU is 1024 microseconds);
            A value of None will disable beacons;  A value of False will not 
            update the current beacon interval.
        ht_capable (bool):  TBD.  A value of None will not update the current
            value of HT capable.
    """
    bssid           = None
    ssid            = None
    channel         = None
    beacon_interval = None
    ht_capable      = None
    
    
    def __init__(self,  bssid=False, ssid=None, channel=None, beacon_interval=False, ht_capable=None):
        super(NodeConfigBSS, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_CONFIG_BSS
        
        import struct
        import wlan_exp.info as info

        # Create BSS Config         
        bss_config = info.BSSConfig(bssid=bssid, ssid=ssid, channel=channel, 
                                    beacon_interval=beacon_interval, 
                                    ht_capable=ht_capable)

        # Set local variables for error messages
        self.bssid           = bssid
        self.ssid            = ssid
        self.channel         = channel
        self.beacon_interval = beacon_interval
        self.ht_capable      = ht_capable

        # Convert BSSConfig() to bytes for transfer
        data_to_send = bss_config.serialize()
        data_len     = len(data_to_send)

        self.add_args(data_len)

        data_buf = bytearray(data_to_send)

        # Zero pad so that the data buffer is 32-bit aligned
        if ((len(data_buf) % 4) != 0):
            data_buf += bytearray(4 - (len(data_buf) % 4))

        idx = 0
        while (idx < len(data_buf)):
            arg = struct.unpack_from('!I', data_buf[idx:idx+4])
            self.add_args(arg[0])
            idx += 4
            

    def process_resp(self, resp):
        # Check number of arguments
        if (resp.resp_is_valid(num_args=1, name='from config BSS command')):
            # All error checking is done by the individual implementations
            return resp.get_args()
        else:
            return False

# End Class


class NodeDisassociate(message.Cmd):
    """Command to remove associations from the association table."""
    description = None

    def __init__(self, device=None):
        super(NodeDisassociate, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_DISASSOCIATE

        if device is not None:
            self.description = device.description
            mac_address      = device.wlan_mac_address
        else:
            self.description = "All nodes"
            mac_address      = CMD_PARAM_GET_ALL_ASSOCIATED

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not disassociate from {0}.".format(self.description)
        status_errors = { error_code : error_msg }

        resp.resp_is_valid(num_args=1, status_errors=status_errors,
                           name='from Disassociate command')

# End Class


class NodeGetBSSMembers(message.BufferCmd):
    """Command to get BSS Members."""
    def __init__(self, node=None):
        super(NodeGetBSSMembers, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_GET_BSS_MEMBERS

        mac_address = CMD_PARAM_GET_ALL_ASSOCIATED

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a Buffer of all station infos.  Need to convert to a list
        #   of StationInfo()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "StationInfo()")

        return ret_val

# End Class

class NodeGetStationInfoList(message.BufferCmd):
    """Command to get list of all Station Infos."""
    def __init__(self, node=None):
        super(NodeGetStationInfoList, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_GET_STATION_INFO_LIST

        mac_address = CMD_PARAM_GET_ALL_ASSOCIATED

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a Buffer of all station infos.  Need to convert to a list
        #   of StationInfo()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "StationInfo()")

        return ret_val

# End Class


class NodeGetBSSInfo(message.BufferCmd):
    """Command to get the BSS info for a given BSS ID."""
    def __init__(self, bssid=None):
        super(NodeGetBSSInfo, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_GET_BSS_INFO

        if bssid is None:
            self.add_args(CMD_PARAM_RSVD)
            self.add_args(CMD_PARAM_RSVD)
        else:
            if type(bssid) is str:
                bssid = CMD_PARAM_GET_ALL_ASSOCIATED      # If BSSID is a string, then get all bss info
    
            _add_mac_address_to_cmd(self, bssid)


    def process_resp(self, resp):
        # Contains a Buffer of all BSS infos.  Need to convert to a list of BSSInfo()
        import wlan_exp.info as info

        index   = 0
        data    = resp.get_bytes()
        ret_val = info.deserialize_info_buffer(data[index:], "BSSInfo()")

        return ret_val

# End Class



#--------------------------------------------
# Queue Commands
#--------------------------------------------
class QueueTxDataPurgeAll(message.Cmd):
    """Command to purge all data transmit queues on the node."""
    def __init__(self):
        super(QueueTxDataPurgeAll, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_QUEUE_TX_DATA_PURGE_ALL

    def process_resp(self, resp):
        pass

# End Class



#--------------------------------------------
# AP Specific Commands
#--------------------------------------------
class NodeAPConfigure(message.Cmd):
    """Command to configure the AP.

    Attributes (default state on the node is in CAPS):
        dtim_multicast_buffering   -- Enable DTIM multicast buffering (TRUE/False)
    """
    def __init__(self, dtim_multicast_buffering=None):
        super(NodeAPConfigure, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_AP_CONFIG

        flags = 0
        mask  = 0

        if dtim_multicast_buffering is not None:
            mask += CMD_PARAM_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER
            if dtim_multicast_buffering:
                flags += CMD_PARAM_AP_CONFIG_FLAG_DTIM_MULTICAST_BUFFER

        self.add_args(flags)
        self.add_args(mask)

    def process_resp(self, resp):
        pass

# End Class


class NodeAPAddAssociation(message.Cmd):
    """Command to add the association to the association table on the AP.

    Attributes:
        device        -- Device to add to the association table
        disable_timeout -- Disable the AP's inactivity check for this STA
    """
    description = None

    def __init__(self, device, disable_timeout=None):
        super(NodeAPAddAssociation, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_ADD_ASSOCIATION

        flags = 0
        mask  = 0

        if disable_timeout is not None:
            mask += CMD_PARAM_ADD_ASSOCIATION_DISABLE_INACTIVITY_TIMEOUT
            if disable_timeout:
                flags += CMD_PARAM_ADD_ASSOCIATION_DISABLE_INACTIVITY_TIMEOUT

        # Set the station_info ht_capable flag based on the device's ht_capable attribute
        mask += CMD_PARAM_ADD_ASSOCIATION_HT_CAPABLE_STA
        if device.ht_capable:
          flags += CMD_PARAM_ADD_ASSOCIATION_HT_CAPABLE_STA

        self.add_args(flags)
        self.add_args(mask)

        self.description = device.description
        mac_address      = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not add AP association to {0}.".format(self.description)
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors,
                               name='from AP associate command')):
            args = resp.get_args()
            return args[1]
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeAPSetAuthAddrFilter(message.Cmd):
    """Command to set the authentication address filter on the node.

    Attributes:
        allow  -- List of (address, mask) tuples that will be used to filter addresses
                  on the node.

    For the mask, bits that are 0 are treated as "any" and bits that are 1 are
    treated as "must equal".  For the address, locations of one bits in the mask
    must match the incoming addresses to pass the filter.
    """
    def __init__(self, allow):
        super(NodeAPSetAuthAddrFilter, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER

        length = len(allow)

        if (length > 50):
            msg  = "Currently, the wlan_exp framework does not support more than "
            msg += "50 address ranges in the association address filter."
            raise AttributeError(msg)

        self.add_args(CMD_PARAM_WRITE)
        self.add_args(length)

        for addr_range in allow:
            self.add_args(((addr_range[0] >> 32) & 0xFFFF))
            self.add_args((addr_range[0] & 0xFFFFFFFF))
            self.add_args(((addr_range[1] >> 32) & 0xFFFF))
            self.add_args((addr_range[1] & 0xFFFFFFFF))


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set authentication address filter on the node."
        status_errors = { error_code : error_msg }

        resp.resp_is_valid(num_args=1, status_errors=status_errors,
                           name='from AP set association address filter command')

# End Class



#--------------------------------------------
# STA Specific Commands
#--------------------------------------------
class NodeSTASetAID(message.Cmd):
    """Command to get the AID of the node"""
    def __init__(self, aid):
        super(NodeSTASetAID, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_STA_SET_AID
        
        if ((aid < 1) or (aid > 255)):
            raise AttributeError("AID value must be in [1, 255].")
        
        self.add_args(aid)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set the AID of node."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from set AID command')):
            return True
        else:
            return False

# End Class


class NodeSTAJoin(message.Cmd):
    """Command to join a given BSS. The resulting behavior of the STA depends on which arguments
    are proived.

    If only an SSID is provided the STA will start its scanning state machine to search for a BSS
    with a matching SSID and will attempt to join the first matching network.

    If BSSID, SSID and channel are all provided the STA will immediately attempt joining the specified
    BSS without performing a scan.

    Attributes:
        ssid     -- SSID of BSS to join.  If value is None, then this will 
                    stop the join process
        bssid    -- (optional) BSSID of the BSS to join
        channel  -- (optional) Channel of BSS to join
    """
    def __init__(self, ssid, bssid=None, channel=None):
        super(NodeSTAJoin, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_STA_JOIN

        # Add BSSID
        #     - Command assumes that this is a valid bssid
        if bssid is None:
            bssid = None
        else:
            # Check that a channel is specified
            if channel is None:
                raise AttributeError("Join must specify both BSSID and channel if either BSSID or channel is specified.")
            
        _add_mac_address_to_cmd(self, bssid)

        # Add Channel
        #     - Command assumes that this is a valid channel
        if channel is None:
            channel = 0
        else:
            # Check that a bssid is specified
            if bssid is None:
                raise AttributeError("Join must specify both BSSID and channel if either BSSID or channel is specified.")

        self.add_args(channel)

        # Add SSID
        #     - SSID should be added last to the command so that the corresponding
        #       C code does not have to compute the index of the next argument 
        #       after the SSID
        if ssid is None:
            self.add_args(CMD_PARAM_RSVD)
        else:
            _add_ssid_to_cmd(self, ssid)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not join the network."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Join command')):
            return True
        else:
            return False

# End Class


class NodeSTAJoinStatus(message.Cmd):
    """Command to get the join status of the node"""
    def __init__(self):
        super(NodeSTAJoinStatus, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_NODE_STA_JOIN_STATUS


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get join status of node."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Join Status command')):
            args = resp.get_args()
            if (args[1] == 1):
                return True
            else:
                return False
        else:
            return False

# End Class



#--------------------------------------------
# IBSS Specific Commands
#--------------------------------------------



#--------------------------------------------
# Memory Access Commands - For developer use only
#--------------------------------------------
class NodeMemAccess(message.Cmd):
    """Command to read/write memory in CPU High / CPU Low

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        high      -- True for CPU_High access, False for CPU_Low

        address   -- u32 memory address to read/write

        values    -- When cmd==CMD_PARAM_WRITE, scalar or list of u32 values to write
                     When cmd==CMD_PARAM_READ, None

        length    -- When cmd==CMD_PARAM_WRITE, None
                     When cmd==CMD_PARAM_READ, number of u32 values to read starting at address
    """
    _read_len = None

    def __init__(self, cmd, high, address, values=None, length=None):
        super(NodeMemAccess, self).__init__()
        if (high):
            self.command = _CMD_GROUP_NODE + CMDID_DEV_MEM_HIGH
        else:
            self.command = _CMD_GROUP_NODE + CMDID_DEV_MEM_LOW

        self.add_args(cmd)
        self.add_args(address)
        self.add_args(length)

        if (cmd == CMD_PARAM_READ):
            self._read_len = length

        elif (cmd == CMD_PARAM_WRITE):
            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)

        else:
            raise Exception('ERROR: NodeMemAccess constructor arguments invalid');


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Memory access failed."
        status_errors = { error_code : error_msg }
        
        if (self._read_len is not None): 
            # Read command
            if (self.command == _CMD_GROUP_NODE + CMDID_DEV_MEM_HIGH):
                msg = "from CPU High Mem read command"
            else:
                msg = "from CPU Low Mem read command"
            
            if resp.resp_is_valid(num_args=(2 + self._read_len), status_errors=status_errors, name=msg):
                args = resp.get_args()

                if (len(args) == 3):
                    return args[2]
                elif (len(args) > 3):
                    return args[2:]
                else:
                    raise Exception('ERROR: Invalid memory read response.  Num Args = {0}'.format(len(args)))
            else:
                return CMD_PARAM_ERROR
        else: 
            # Write command
            if (self.command == _CMD_GROUP_NODE + CMDID_DEV_MEM_HIGH):
                msg = "from CPU High Mem write command"
            else:
                msg = "from CPU Low Mem write command"
            
            if resp.resp_is_valid(num_args=1, status_errors=status_errors, name=msg):
                pass
            else:
                return CMD_PARAM_ERROR

# End ClassNodeAPProcBeaconInterval



#--------------------------------------------
# EEPROM Access Commands - For developer use only
#--------------------------------------------
class NodeEEPROMAccess(message.Cmd):
    """Command to read/write EEPROM

    Attributes:
        cmd       -- Sub-command to send over the command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        address   -- u16 memory address to read/write

        values    -- When cmd==CMD_PARAM_WRITE, scalar or list of u8 values to write
                     When cmd==CMD_PARAM_READ, None

        length    -- When cmd==CMD_PARAM_WRITE, number of u8 values to write
                     When cmd==CMD_PARAM_READ, number of u8 values to read
                     
        on_board  -- True for "On Board" EEPROM access, False for "FMC" EEPROM access        
    """
    _read_len = None

    def __init__(self, cmd, address, values=None, length=None, on_board=True):
        super(NodeEEPROMAccess, self).__init__()
        self.command = _CMD_GROUP_NODE + CMDID_DEV_EEPROM
        
        self.add_args(cmd)
        self.add_args(on_board)
        self.add_args(address)
        self.add_args(length)

        if (cmd == CMD_PARAM_READ):
            self._read_len = length

        elif (cmd == CMD_PARAM_WRITE):
            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)

        else:
            raise Exception('ERROR: NodeEEPROMAccess constructor arguments invalid');


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Memory access failed."
        status_errors = { error_code : error_msg }
            
        if (self._read_len is not None):
            # Read command
            msg = "from EEPROM write command"
        
            if resp.resp_is_valid(num_args=(2 + self._read_len), status_errors=status_errors, name=msg):
                args = resp.get_args()

                if (len(args) == 3):
                    return args[2]
                elif (len(args) > 3):
                    return args[2:]
                else:
                    raise Exception('ERROR: Invalid EEPROM read response:  Num Args = {0}'.format(len(args)))
            else:
                return CMD_PARAM_ERROR
        else: 
            # Write command
            msg = "from EEPROM write command"
        
            if resp.resp_is_valid(num_args=1, status_errors=status_errors, name=msg):
                pass
            else:
                return CMD_PARAM_ERROR

# End Class



#--------------------------------------------
# User Commands
#--------------------------------------------
class UserSendCmd(message.Cmd):
    """Command to send User Command to the node

    Attributes:
        cmdid     -- User-defined Command ID
        args      -- Scalar or list of u32 arguments to write
    """
    def __init__(self, cmd_id, args=None):
        super(UserSendCmd, self).__init__()

        self.command    = _CMD_GROUP_USER + cmd_id

        if args is not None:
            try:
                for a in args:
                    self.add_args(a)
            except TypeError:
                self.add_args(args)


    def process_resp(self, resp):
        """ Message format:
                respArgs32[0]   Status
        """
        args = resp.get_args()

        try:
            return args[0:]
        except:
            return None

# End Class



#--------------------------------------------
# Misc Helper methods
#--------------------------------------------
def _add_mac_address_to_cmd(cmd, mac_address):
    if mac_address is not None:
        if type(mac_address) is str:
            import wlan_exp.util as util
            mac_address = util.str_to_mac_addr(mac_address)
            
        cmd.add_args(((mac_address >> 32) & 0xFFFF))
        cmd.add_args((mac_address & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_MAC_ADDR)
        cmd.add_args(CMD_PARAM_RSVD_MAC_ADDR)

# End def


def _add_ssid_to_cmd(cmd, ssid):
    """Internal method to add an ssid to the given command"""
    import struct

    ssid_len = len(ssid)

    if (ssid_len > CMD_PARAM_MAX_SSID_LEN):
        ssid_len = CMD_PARAM_MAX_SSID_LEN
        ssid     = ssid[:CMD_PARAM_MAX_SSID_LEN]

        msg  = "WARNING:  Maximum SSID length is {0} ".format(CMD_PARAM_MAX_SSID_LEN)
        msg += "truncating to {0}.".format(ssid)
        print(msg)

    cmd.add_args(ssid_len)

    # Null-teriminate the string for C
    ssid    += "\0"
    ssid_buf = bytearray(ssid, 'UTF-8')

    # Zero pad so that the ssid buffer is 32-bit aligned
    if ((len(ssid_buf) % 4) != 0):
        ssid_buf += bytearray(4 - (len(ssid_buf) % 4))

    idx = 0
    while (idx < len(ssid_buf)):
        arg = struct.unpack_from('!I', ssid_buf[idx:idx+4])
        cmd.add_args(arg[0])
        idx += 4

# End def


def _add_time_to_cmd64(cmd, time):
    """Internal method to add a 64-bit time value to the given command."""
    if time is not None:
        # Convert to int microseconds
        if   (type(time) is float):
            time_factor    = 6         # Python time functions uses float seconds
            time_to_send   = int(round(time, time_factor) * (10**time_factor))
        elif (type(time) in [int, long]):
            time_to_send   = time
        else:
            raise TypeError("Time must be an integer number of microseconds.")

        cmd.add_args((time_to_send & 0xFFFFFFFF))
        cmd.add_args(((time_to_send >> 32) & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_TIME)
        cmd.add_args(CMD_PARAM_RSVD_TIME)

# End def


def _add_time_to_cmd32(cmd, time):
    """Internal method to add a 32-bit time value to the given command."""
    if time is not None:
        # Convert to int microseconds
        if   (type(time) is float):
            time_factor    = 6         # Python time functions uses float seconds
            time_to_send   = int(round(time, time_factor) * (10**time_factor))
        elif (type(time) is int):
            time_to_send   = time
        else:
            raise TypeError("Time must be an integer number of microseconds.")

        if (time_to_send > 0xFFFFFFFF):
            time_to_send = 0xFFFFFFFF
            print("WARNING:  Time value (in microseconds) exceeds 32-bits. Saturating to 0xFFFFFFFF.\n")

        cmd.add_args((time_to_send & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_TIME)

# End def


def _get_ssid_from_resp(resp, status_errors):
    """Internal method to process an SSID from a response."""
    args       = resp.get_args()
    arg_length = len(args)

    resp.resp_is_valid(num_args=arg_length, status_errors=status_errors,
                              name='while processing SSID')

    # Actually check the number of arguments
    if(arg_length >= 2):
        length = args[1]
    else:
        raise Exception('ERROR: invalid response to process SSID - N_ARGS = {0}'.format(len(args)))

    # Get the SSID from the response
    if (length > 0):
        import struct
        ssid_buffer = struct.pack('!%dI' % (arg_length - 2), *args[2:] )
        ssid_tuple  = struct.unpack_from('!%ds' % length, ssid_buffer)

        # Python 3 vs 2 issue
        try:
            ssid    = str(ssid_tuple[0], encoding='UTF-8')
        except TypeError:
            ssid    = str(ssid_tuple[0])
    else:
        ssid        = ""

    return ssid

# End def

def _get_channel_number(channel):
    """Internal method to get the channel number."""
    try:
        my_channel = channel['index']
    except (KeyError, TypeError):
        import wlan_exp.util as util
        
        tmp_chan = util.get_channel_info(channel)
        
        if tmp_chan is not None:
            my_channel = tmp_chan['channel']
        else:
            msg  = "Unknown channel:  {0}".format(channel)
            raise ValueError(msg)

    return my_channel

# End def
