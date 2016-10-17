# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Commands
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

This module provides class definitions for all WLAN Exp commands.  

"""

import wlan_exp.warpnet.cmds as wn_cmds
import wlan_exp.warpnet.message as wn_message
import wlan_exp.warpnet.transport_eth_udp as wn_transport



__all__ = [# Log command classes
           'LogGetEvents', 'LogConfigure', 'LogGetStatus', 'LogGetCapacity', 'LogAddExpInfoEntry', 
           'LogAddStatsTxRx',
           # Stats command classes
           'StatsConfigure', 'StatsGetTxRx', 
           # LTG classes
           'LTGConfigure', 'LTGStart', 'LTGStop', 'LTGRemove', 'LTGStatus',
           # Node command classes
           'NodeResetState', 'NodeConfigure', 'NodeProcWLANMACAddr', 'NodeProcTime', 
           'NodeSetLowToHighFilter', 'NodeProcChannel', 'NodeProcRandomSeed', 'NodeLowParam', 
           'NodeProcTxPower', 'NodeProcTxRate', 'NodeProcTxAntMode', 'NodeProcRxAntMode', 
           # Association command classes
           'NodeGetSSID', 'NodeDisassociate', 'NodeGetStationInfo', 'NodeGetBSSInfo',
           # Queue command classes
           'QueueTxDataPurgeAll',
           # AP command classes
           'NodeAPConfigure', 'NodeAPProcDTIMPeriod', 'NodeAPAddAssociation', 'NodeAPSetSSID', 
           'NodeAPSetAuthAddrFilter',
           # STA command classes
           'NodeSTAConfigure',
           # IBSS command classes
           'NodeIBSSConfigure', 
           # Common command classes for STA / IBSS
           'NodeProcScanParam', 'NodeProcScan', 'NodeProcJoin', 'NodeProcScanAndJoin' 
          ]


# WLAN Exp Command IDs (Extension of WARPNet Command IDs)
#   NOTE:  The C counterparts are found in wlan_exp_node.h
#   NOTE:  All Command IDs (CMDID_*) must be unique 24-bit numbers

# Node commands and defined values
CMDID_NODE_RESET_STATE                           = 0x001000
CMDID_NODE_CONFIGURE                             = 0x001001
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
CMD_PARAM_ERROR                                  = 0xFF000000

CMD_PARAM_UNICAST                                = 0x00000000
CMD_PARAM_MULTICAST_DATA                         = 0x00000001
CMD_PARAM_MULTICAST_MGMT                         = 0x00000002

CMD_PARAM_NODE_CONFIG_ALL                        = 0xFFFFFFFF 

CMD_PARAM_NODE_RESET_FLAG_LOG                    = 0x00000001
CMD_PARAM_NODE_RESET_FLAG_TXRX_STATS             = 0x00000002
CMD_PARAM_NODE_RESET_FLAG_LTG                    = 0x00000004
CMD_PARAM_NODE_RESET_FLAG_TX_DATA_QUEUE          = 0x00000008
CMD_PARAM_NODE_RESET_FLAG_ASSOCIATIONS           = 0x00000010
CMD_PARAM_NODE_RESET_FLAG_BSS_INFO               = 0x00000020

CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE           = 0x00000001
CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL   = 0x80000000

CMD_PARAM_TIME_ADD_TO_LOG                        = 0x00000002
CMD_PARAM_RSVD_TIME                              = 0xFFFFFFFF

TIME_TYPE_FLOAT                                  = 0x00000000
TIME_TYPE_INT                                    = 0x00000001

CMD_PARAM_RSVD_CHANNEL                           = 0x00000000

CMD_PARAM_RSVD_MAC_ADDR                          = 0xFFFFFFFF

CMD_PARAM_NODE_TX_POWER_LOW                      = 0x00000010
CMD_PARAM_NODE_TX_POWER_ALL                      = 0x00000020
CMD_PARAM_NODE_TX_POWER_MAX_DBM                  = 21
CMD_PARAM_NODE_TX_POWER_MIN_DBM                  = -9

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
CMD_PARAM_LOW_PARAM_PHYSICAL_CS_THRESH           = 0x00000001
CMD_PARAM_LOW_PARAM_CW_EXP_MIN                   = 0x00000002
CMD_PARAM_LOW_PARAM_CW_EXP_MAX                   = 0x00000003
CMD_PARAM_LOW_PARAM_TS_OFFSET                    = 0x00000004
CMD_PARAM_LOW_PARAM_BB_GAIN                      = 0x00000005
CMD_PARAM_LOW_PARAM_LINEARITY_PA                 = 0x00000006
CMD_PARAM_LOW_PARAM_LINEARITY_VGA                = 0x00000007
CMD_PARAM_LOW_PARAM_LINEARITY_UPCONV             = 0x00000008
CMD_PARAM_LOW_PARAM_AD_SCALING                   = 0x00000009
CMD_PARAM_LOW_PARAM_PKT_DET_MIN_POWER            = 0x0000000A
CMD_PARAM_LOW_PARAM_DCF_RTS_THRESH               = 0x10000001
CMD_PARAM_LOW_PARAM_DCF_DOT11SHORTRETRY          = 0x10000002
CMD_PARAM_LOW_PARAM_DCF_DOT11LONGRETRY           = 0x10000003

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
CMDID_LOG_ADD_STATS_TXRX                         = 0x003005
CMDID_LOG_ENABLE_ENTRY                           = 0x003006
CMDID_LOG_STREAM_ENTRIES                         = 0x003007


CMD_PARAM_LOG_GET_ALL_ENTRIES                    = 0xFFFFFFFF

CMD_PARAM_LOG_CONFIG_FLAG_LOGGING                = 0x00000001
CMD_PARAM_LOG_CONFIG_FLAG_WRAP                   = 0x00000002
CMD_PARAM_LOG_CONFIG_FLAG_LOG_PAYLOADS           = 0x00000004
CMD_PARAM_LOG_CONFIG_FLAG_LOG_WN_CMDS            = 0x00000008
CMD_PARAM_LOG_CONFIG_FLAG_TXRX_MPDU              = 0x00000010
CMD_PARAM_LOG_CONFIG_FLAG_TXRX_CTRL              = 0x00000020


# Statistics commands and defined values
CMDID_STATS_CONFIG_TXRX                          = 0x004000
CMDID_STATS_GET_TXRX                             = 0x004001

CMD_PARAM_STATS_CONFIG_FLAG_PROMISC              = 0x00000001

CMD_PARAM_STATS_RETURN_ZEROED_IF_NONE            = 0x80000000


# Queue commands and defined values
CMDID_QUEUE_TX_DATA_PURGE_ALL                    = 0x005000


# Association commands and defined values
CMDID_NODE_GET_SSID                              = 0x006000
CMDID_GET_STATION_INFO                           = 0x006001
CMDID_GET_BSS_INFO                               = 0x006002

CMDID_NODE_DISASSOCIATE                          = 0x006010
CMDID_NODE_ADD_ASSOCIATION                       = 0x006011

CMD_PARAM_ADD_ASSOCIATION_ALLOW_TIMEOUT          = 0x00000001

CMD_PARAM_GET_ALL_ASSOCIATED                     = 0xFFFFFFFFFFFFFFFF


# Common commands for STA / IBSS
CMDID_NODE_SCAN_PARAM                            = 0x007000
CMDID_NODE_SCAN                                  = 0x007001
CMDID_NODE_JOIN                                  = 0x007002
CMDID_NODE_SCAN_AND_JOIN                         = 0x007003

CMD_PARAM_NODE_SCAN_ENABLE                       = 0x00000001
CMD_PARAM_NODE_SCAN_DISABLE                      = 0x00000000

CMD_PARAM_NODE_JOIN_FAILED                       = 0xFFFFFFFF


# AP commands and defined values
CMDID_NODE_AP_CONFIG                             = 0x100000
CMDID_NODE_AP_DTIM_PERIOD                        = 0x100001
CMDID_NODE_AP_SET_SSID                           = 0x100002
CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER     = 0x100003
CMDID_NODE_AP_BEACON_INTERVAL                    = 0x100004

CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING       = 0x00000001

CMD_PARAM_MAX_SSID_LEN                           = 32


# STA commands and defined values
CMDID_NODE_STA_CONFIG                            = 0x200000


# STA commands and defined values
CMDID_NODE_IBSS_CONFIG                           = 0x300000


# Common command parameters between STA / IBSS
CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE      = 0x00000001
CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT       = 0x00000002


# Developer commands and defined values
CMDID_DEV_MEM_HIGH                               = 0xFFF000
CMDID_DEV_MEM_LOW                                = 0xFFF001



# Local Constants
_CMD_GRPID_NODE              = (wn_cmds.GRPID_NODE << 24)


#-----------------------------------------------------------------------------
# Class Definitions for WLAN Exp Commands
#-----------------------------------------------------------------------------

#--------------------------------------------
# Log Commands
#--------------------------------------------
class LogGetEvents(wn_message.BufferCmd):
    """Command to get the WLAN Exp log events of the node"""
    def __init__(self, size, start_byte=0):
        command = _CMD_GRPID_NODE + CMDID_LOG_GET_ENTRIES
        
        if (size == CMD_PARAM_LOG_GET_ALL_ENTRIES):
            size = wn_message.CMD_BUFFER_GET_SIZE_FROM_DATA
        
        super(LogGetEvents, self).__init__(
                command=command, buffer_id=0, flags=0, start_byte=start_byte, size=size)

    def process_resp(self, resp):
        return resp

# End Class


class LogConfigure(wn_message.Cmd):
    """Command to configure the Event log.
    
    Attributes (default state on the node is in CAPS):
        log_enable           -- Enable the event log (TRUE/False)
        log_wrap_enable      -- Enable event log wrapping (True/FALSE)
        log_full_payloads    -- Record full Tx/Rx payloads in event log (True/FALSE)
        log_warpnet_commands -- Record WARPNet commands in event log (True/FALSE)   
        log_txrx_mpdu        -- Enable Tx/Rx log entries for MPDU frames (TRUE/False)   
        log_txrx_ctrl        -- Enable Tx/Rx log entries for CTRL frames (TRUE/False)   
    """
    def __init__(self, log_enable=None, log_wrap_enable=None, 
                       log_full_payloads=None, log_warpnet_commands=None,
                       log_txrx_mpdu=None, log_txrx_ctrl=None):
        super(LogConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_CONFIG

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

        if log_warpnet_commands is not None:
            mask += CMD_PARAM_LOG_CONFIG_FLAG_LOG_WN_CMDS
            if log_warpnet_commands:
                flags += CMD_PARAM_LOG_CONFIG_FLAG_LOG_WN_CMDS
                
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


class LogGetStatus(wn_message.Cmd):
    """Command to get the state information about the log."""
    def __init__(self):
        super(LogGetStatus, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_GET_STATUS
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=4):
            args = resp.get_args()
            return (args[0], args[1], args[2], args[3])
        else:
            return (0,0,0,0)

# End Class


class LogGetCapacity(wn_message.Cmd):
    """Command to get the log capacity and current use."""
    def __init__(self):
        super(LogGetCapacity, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_GET_CAPACITY
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=2):
            args = resp.get_args()
            return (args[0], args[1])
        else:
            return (0,0)

# End Class


class LogStreamEntries(wn_message.Cmd):
    """Command to configure the node log streaming."""
    def __init__(self, enable, host_id, ip_address, port):
        super(LogStreamEntries, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_STREAM_ENTRIES
        
        if (type(ip_address) is str):
            addr = wn_transport.ip_to_int(ip_address)
        elif (type(ip_address) is int):
            addr = ip_address
        else:
            raise TypeError("IP Address must be either a str or int")

        arg = (2**16 * int(host_id)) + (int(port) & 0xFFFF)

        self.add_args(enable)
        self.add_args(addr)
        self.add_args(arg)
    
    def process_resp(self, resp):
        pass

# End Class


class LogAddExpInfoEntry(wn_message.Cmd):
    """Command to write a EXP_INFO Log Entry to the node."""
    def __init__(self, info_type, message):
        super(LogAddExpInfoEntry, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_ADD_EXP_INFO_ENTRY

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


class LogAddStatsTxRx(wn_message.Cmd):
    """Command to add the current statistics to the Event log"""
    def __init__(self):
        super(LogAddStatsTxRx, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LOG_ADD_STATS_TXRX
    
    def process_resp(self, resp):
        if resp.resp_is_valid(num_args=1):
            args = resp.get_args()
            return args[0]
        else:
            return 0

# End Class



#--------------------------------------------
# Stats Commands
#--------------------------------------------
class StatsConfigure(wn_message.Cmd):
    """Command to configure the Statistics collection.
    
    Attributes (default state on the node is in CAPS):
        promisc_stats        -- Enable promiscuous statistics collection (TRUE/False)
    """
    def __init__(self, promisc_stats=None):
        super(StatsConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_STATS_CONFIG_TXRX

        flags = 0
        mask  = 0

        if promisc_stats is not None:
            mask += CMD_PARAM_STATS_CONFIG_FLAG_PROMISC
            if promisc_stats:
                flags += CMD_PARAM_STATS_CONFIG_FLAG_PROMISC
                
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class StatsGetTxRx(wn_message.BufferCmd):
    """Command to get the statistics from the node for a given node."""
    def __init__(self, node=None, return_zeroed_stats_if_none=False):
        flags = 0

        # Compute the argument for the BufferCmd flags
        if return_zeroed_stats_if_none:
            flags += CMD_PARAM_STATS_RETURN_ZEROED_IF_NONE

        # Call parent initialziation
        super(StatsGetTxRx, self).__init__(flags=flags)
        self.command = _CMD_GRPID_NODE + CMDID_STATS_GET_TXRX

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = CMD_PARAM_GET_ALL_ASSOCIATED            

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a WARPNet Buffer of all stats entries.  Need to convert to 
        #   a list of statistics dictionaries.
        import wlan_exp.log.entry_types as entry_types
        
        index   = 0
        data    = resp.get_bytes()
        ret_val = entry_types.entry_txrx_stats.deserialize(data[index:])

        if (False):
            msg = "Statistics Data buffer:"
            for i, byte in enumerate(data[index:]):
                if ((i % 16) == 0): msg += "\n    "
                try:
                    msg += "0x{0:02X} ".format(ord(byte))
                except TypeError:
                    msg += "0x{0:02X} ".format(byte)
            print(msg)

        return ret_val

# End Class



#--------------------------------------------
# Local Traffic Generation (LTG) Commands
#--------------------------------------------
class LTGCommon(wn_message.Cmd):
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
        error_msg     = "LTG {0} command was not successful.".format(self.name)
        status_errors = { error_code : error_msg }
        
        if resp.resp_is_valid(num_args=1, 
                              status_errors=status_errors, 
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[0]
        else:
            return CMD_PARAM_LTG_ERROR
        
# End Class


class LTGConfigure(wn_message.Cmd):
    """Command to configure an LTG with the given traffic flow to the 
    specified node.
    """
    name = 'configure'

    def __init__(self, traffic_flow, auto_start=False):
        super(LTGConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LTG_CONFIG

        flags = 0
        
        if auto_start:
            flags += CMD_PARAM_LTG_CONFIG_FLAG_AUTOSTART
        
        self.add_args(flags)
        
        for arg in traffic_flow.serialize():
            self.add_args(arg)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "LTG {0} command was not successful.".format(self.name)
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, 
                              status_errors=status_errors, 
                              name='for the LTG {0} command'.format(self.name)):
            args = resp.get_args()
            return args[1]
        else:
            return CMD_PARAM_LTG_ERROR
    
# End Class


class LTGStart(LTGCommon):
    """Command to start a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will start all 
    configured LTGs on the node.
    """
    name = 'start'

    def __init__(self, ltg_id=None):
        super(LTGStart, self).__init__(ltg_id)
        self.command = _CMD_GRPID_NODE + CMDID_LTG_START

# End Class


class LTGStop(LTGCommon):
    """Command to stop a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will stop all 
    configured LTGs on the node.
    """
    name = 'stop'

    def __init__(self, ltg_id=None):
        super(LTGStop, self).__init__(ltg_id)
        self.command = _CMD_GRPID_NODE + CMDID_LTG_STOP
    
# End Class


class LTGRemove(LTGCommon):
    """Command to remove a configured LTG to the given node.
    
    NOTE:  By providing no node argument, this command will remove all 
    configured LTGs on the node.
    """
    name = 'remove'

    def __init__(self, ltg_id=None):
        super(LTGRemove, self).__init__(ltg_id)
        self.command = _CMD_GRPID_NODE + CMDID_LTG_REMOVE
    
# End Class


class LTGStatus(wn_message.Cmd):
    """Command to get the status of the LTG."""
    time_factor = 6
    name        = 'status'

    def __init__(self, ltg_id):
        super(LTGStatus, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_LTG_STATUS

        if type(ltg_id) is not int:
            raise TypeError("LTG ID must be an integer.")
        self.add_args(ltg_id)

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR + CMD_PARAM_LTG_ERROR
        error_msg     = "LTG {0} command was not successful.".format(self.name)
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
class NodeResetState(wn_message.Cmd):
    """Command to reset the state of a portion of the node defined by the flags.
    
    Attributes:
        flags -- [0] NODE_RESET_LOG
                 [1] NODE_RESET_TXRX_STATS
                 [2] NODE_RESET_LTG
                 [3] NODE_RESET_TX_DATA_QUEUE
                 [4] NODE_RESET_ASSOCIATIONS
                 [5] NODE_RESET_BSS_INFO
    """
    def __init__(self, flags):
        super(NodeResetState, self).__init__()
        self.command = _CMD_GRPID_NODE +  CMDID_NODE_RESET_STATE        
        self.add_args(flags)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeConfigure(wn_message.Cmd):
    """Command to configure flag parameters on the node
    
    Attributes:
        dsss_enable -- Whether DSSS packets are received. 
    """
    def __init__(self, dsss_enable=None, print_level=None):
        super(NodeConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_CONFIGURE
        
        flags = 0
        mask  = 0
        level = 0

        if dsss_enable is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE
            if dsss_enable:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_DSSS_ENABLE

        if print_level is not None:
            level = CMD_PARAM_NODE_CONFIG_SET_WLAN_EXP_PRINT_LEVEL + print_level
        
        self.add_args(flags)
        self.add_args(mask)
        self.add_args(level)

    def process_resp(self, resp):
        pass

# End Class


class NodeProcWLANMACAddr(wn_message.Cmd):
    """Command to get / set the WLAN MAC Address on the node.
    
    Attributes:
        cmd           -- Sub-command to send over the WARPNet command.  Valid values are:
                           CMD_PARAM_READ
                           CMD_PARAM_WRITE
        wlan_mac_addr -- 48-bit MAC address to write (optional)
    """
    def __init__(self, cmd, wlan_mac_address=None):
        super(NodeProcWLANMACAddr, self).__init__()
        self.command  = _CMD_GRPID_NODE + CMDID_NODE_WLAN_MAC_ADDR

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
        error_msg     = "Could not get the WLAN MAC address on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=3, status_errors=status_errors, name='from the WLAN Mac Address command'):
            args = resp.get_args()
            addr = (2**32 * args[1]) + args[2]
        else:
            addr = None

        return addr

# End Class


class NodeProcTime(wn_message.Cmd):
    """Command to get / set the time on the node.
    
    NOTE:  Python time functions operate on floating point numbers in 
        seconds, while the WnNode operates on microseconds.  In order
        to be more flexible, this class can be initialized with either
        type of input.  However, it will only return an integer number
        of microseconds.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       TIME_ADD_TO_LOG
        node_time -- Time as either an integer number of microseconds or 
                       a floating point number in seconds.
        time_id   -- ID to use identify the time command in the log.
    """
    time_factor = 6
    time_type   = None
    
    def __init__(self, cmd, node_time, time_id=None):
        super(NodeProcTime, self).__init__()
        self.command  = _CMD_GRPID_NODE + CMDID_NODE_TIME

        # Read the time as a float
        if (cmd == CMD_PARAM_READ):
            self.time_type = TIME_TYPE_FLOAT
            self.add_args(CMD_PARAM_READ)
            self.add_args(CMD_PARAM_RSVD_TIME)             # Reads do not need a time_id
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)
            self.add_args(CMD_PARAM_RSVD_TIME)

        # Write the time / Add time to log
        else:
            import time

            # By default set the time_id to a random number between [0, 2^32)
            if time_id is None:
                import random
                time_id = 2**32 * random.random()

            if (cmd == CMD_PARAM_WRITE):
                self.add_args(CMD_PARAM_WRITE)
            else:
                self.add_args(CMD_PARAM_TIME_ADD_TO_LOG)
                node_time = None

            # Get the current time on the host
            now = int(round(time.time(), self.time_factor) * (10**self.time_factor))
            
            self.add_args(int(time_id))
            self.time_type = _add_time_to_cmd64(self, node_time, self.time_factor)
            self.add_args((now & 0xFFFFFFFF))
            self.add_args(((now >> 32) & 0xFFFFFFFF))


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the time on the node"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=3, status_errors=status_errors, name='from the Time command'):
            args = resp.get_args()
            time = (2**32 * args[2]) + args[1]
        else:
            time = 0

        ret_val = 0
        
        if   (self.time_type == TIME_TYPE_FLOAT):
            ret_val = float(time / (10**self.time_factor))
        elif (self.time_type == TIME_TYPE_INT):
            ret_val = time
            
        return ret_val

# End Class


class NodeSetLowToHighFilter(wn_message.Cmd):
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
        self.command  = _CMD_GRPID_NODE + CMDID_NODE_LOW_TO_HIGH_FILTER

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


class NodeProcChannel(wn_message.Cmd):
    """Command to get / set the channel of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        channel   -- 802.11 Channel for the node.  Should be a valid channel defined
                       in wlan_exp.util wlan_channel table.
    """
    channel  = None

    def __init__(self, cmd, channel=None):
        super(NodeProcChannel, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_CHANNEL

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
            return util.find_channel_by_channel_number(args[1])
        else:
            return None

# End Class


class NodeProcRandomSeed(wn_message.Cmd):
    """Command to set the random seed of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ   (Not supported)
                       CMD_PARAM_WRITE
        high_seed -- Random number generator seed for CPU high
        low_seed  -- Random number generator seed for CPU low
    
    import random
    high_seed = random.randint(0, 0xFFFFFFFF)
    NOTE:  If a seed is not provided, then the seed is not updated.
    """
    def __init__(self, cmd, high_seed=None, low_seed=None):
        super(NodeProcRandomSeed, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_RANDOM_SEED

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


class NodeLowParam(wn_message.Cmd):
    """Command to set parameter in CPU Low
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_WRITE
                       CMD_PARAM_READ

        param     -- ID of parameter to modify

        values    -- When cmd==CMD_PARAM_WRITE, scalar or list of u32 values to write
                     When cmd==CMD_PARAM_READ, None

    """
    read_write = None
    
    def __init__(self, cmd, param, values=None):
        super(NodeLowParam, self).__init__()        
        
        self.command    = _CMD_GRPID_NODE + CMDID_NODE_LOW_PARAM
        self.read_write = cmd

        # Caluculate the size of the entire message to CPU Low [PARAM_ID, ARGS[]]
        size = 1
        if values is not None:
            size += len(values)

        self.add_args(cmd)
        self.add_args(size)
        self.add_args(param)

        if values is not None:
            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)
            
    def process_resp(self, resp):
        """ Message format:
                respArgs32[0]   Status
                respArgs32[1]   Size in words of PARAM ARGS
                respArgs32[2]   PARAM_ID
                respArgs32[3:N] PARAM ARGS
        """
        error_code_cs_thresh    = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_PHYSICAL_CS_THRESH
        error_code_cw_min       = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_CW_EXP_MIN
        error_code_cw_max       = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_CW_EXP_MAX
        error_code_ts_offset    = CMD_PARAM_ERROR + CMD_PARAM_LOW_PARAM_TS_OFFSET

        status_errors = { error_code_cs_thresh : "Could not get / set carrier sense threshold",
                          error_code_cw_min    : "Could not get / set minimum contention window",
                          error_code_cw_max    : "Could not get / set maximum contention window", 
                          error_code_ts_offset : "Could not get / set the timestamp offset"}

        if (self.read_write == CMD_PARAM_READ):
            args = resp.get_args()
            if resp.resp_is_valid(num_args=(args[1] + 3), status_errors=status_errors, name='from Low Param command'):
                return args[1:]
            else:
                return None
        else:
            return None

# End Class
        

class NodeProcTxPower(wn_message.Cmd):
    """Command to get / set the transmit power of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
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
        power     -- Transmit power for the WARP node (in dBm).
        device    -- 802.11 device for which the rate is being set.  
    """
    rate     = None
    dev_name = None
    
    def __init__(self, cmd, tx_type, power=None, device=None):
        super(NodeProcTxPower, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TX_POWER
        mac_address  = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(self.check_power(power))
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
        """Check the power value is valid."""
        if power is None:
            raise ValueError("Must supply value to set Tx power.")
        
        if (power > CMD_PARAM_NODE_TX_POWER_MAX_DBM):
            msg  = "WARNING:  Requested power too high.\n"
            msg += "    Adjusting transmit power from {0} to {1}".format(power, CMD_PARAM_NODE_TX_POWER_MAX_DBM)
            print(msg)
            power = CMD_PARAM_NODE_TX_POWER_MAX_DBM

        if (power < CMD_PARAM_NODE_TX_POWER_MIN_DBM):
            msg  = "WARNING:  Requested power too low. \n"
            msg += "    Adjusting transmit power from {0} to {1}".format(power, CMD_PARAM_NODE_TX_POWER_MIN_DBM)
            print(msg)
            power = CMD_PARAM_NODE_TX_POWER_MIN_DBM

        # Shift the value so that there are only positive integers over the wire
        return (power - CMD_PARAM_NODE_TX_POWER_MIN_DBM)
            
            
    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set the transmit power on the node"
        status_errors = { error_code : error_msg }
        
        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx power command'):
            args = resp.get_args()
            
            # Shift the value since only positive integers are transmitted over the wire
            return (args[1] + CMD_PARAM_NODE_TX_POWER_MIN_DBM)
        else:
            return None

# End Class


class NodeProcTxRate(wn_message.Cmd):
    """Command to get / set the transmit rate of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT 
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
        rate      -- 802.11 transmit rate for the node.  Should be an entry
                     from the rates table in wlan_exp.util.  Checking is
                     done on the node and the current rate will always be 
                     returned by the node.
        device    -- 802.11 device for which the rate is being set.  
    """
    def __init__(self, cmd, tx_type, rate=None, device=None):
        super(NodeProcTxRate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TX_RATE
        mac_address  = None

        self.add_args(cmd)

        self.add_args(self.check_type(tx_type))
        
        if rate is not None:
            self.add_args(self.check_rate(rate))
        else:
            self.add_args(0)

        if device is not None:
            mac_address   = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)
            
    
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
            
    
    def check_rate(self, rate):
        """Check the rate to see if it is valid."""
        try:
            return rate['index']
        except (KeyError, TypeError):
            msg  = "The TX rate must be an entry from the rates table in wlan_exp.util"
            raise ValueError(msg)
    
    
    def process_resp(self, resp):
        import wlan_exp.util as util

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set transmit rate on the node"
        status_errors = { error_code : error_msg }
        
        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx rate command'):
            args = resp.get_args()
            return util.find_tx_rate_by_index(args[1])
        else:
            return None

# End Class


class NodeProcTxAntMode(wn_message.Cmd):
    """Command to get / set the transmit antenna mode of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
                       CMD_PARAM_WRITE_DEFAULT
                       CMD_PARAM_READ_DEFAULT 
        tx_type   -- Which Tx parameters to set.  Valid values are:
                       CMD_PARAM_UNICAST
                       CMD_PARAM_MULTICAST_DATA
                       CMD_PARAM_MULTICAST_MGMT
        ant_mode  -- Transmit antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  
        device    -- 802.11 device for which the rate is being set.  
    """
    def __init__(self, cmd, tx_type, ant_mode=None, device=None):
        super(NodeProcTxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_TX_ANT_MODE
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
            
    
    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        try:
            return ant_mode['index']
        except KeyError:
            msg  = "The antenna mode must be an entry from the wlan_tx_ant_mode list in wlan_exp.util\n"
            raise ValueError(msg)
    
    
    def process_resp(self, resp):
        import wlan_exp.util as util

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set transmit antenna mode of the node"
        status_errors = { error_code : error_msg }
        
        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Tx antenna mode command'):
            args = resp.get_args()
            return util.find_tx_ant_mode_by_index(args[1])
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeProcRxAntMode(wn_message.Cmd):
    """Command to get / set the receive antenna mode of the node.
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        ant_mode  -- Receive antenna mode for the node.  Checking is
                     done both in the command and on the node.  The current
                     antenna mode will be returned by the node.  
    """
    def __init__(self, cmd, ant_mode=None):
        super(NodeProcRxAntMode, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_RX_ANT_MODE
        
        self.add_args(cmd)
        if ant_mode is not None:
            self.add_args(self.check_ant_mode(ant_mode))
        else:
            self.add_args(CMD_PARAM_NODE_CONFIG_ALL)


    def check_ant_mode(self, ant_mode):
        """Check the antenna mode to see if it is valid."""
        try:
            return ant_mode['index']
        except KeyError:
            msg  = "The antenna mode must be an entry from the wlan_rx_ant_mode\n"
            msg += "    list in wlan_exp.util\n"
            raise ValueError(msg)
    
    def process_resp(self, resp):
        import wlan_exp.util as util

        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could get / set receive antenna mode of the node"
        status_errors = { error_code : error_msg }
        
        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Rx antenna mode command'):
            args = resp.get_args()
            return util.find_rx_ant_mode_by_index(args[1])
        else:
            return CMD_PARAM_ERROR

# End Class



#--------------------------------------------
# Association Commands
#--------------------------------------------
class NodeGetSSID(wn_message.Cmd):
    """Command to get the SSID of the node."""
    ssid = None

    def __init__(self):
        super(NodeGetSSID, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_GET_SSID


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get SSID from Node."
        status_errors = { error_code : error_msg }

        return _get_ssid_from_resp(resp, status_errors)

# End Class


class NodeDisassociate(wn_message.Cmd):
    """Command to remove associations from the association table."""
    description = None

    def __init__(self, device=None):
        super(NodeDisassociate, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_DISASSOCIATE

        if device is not None:
            self.description = device.description
            mac_address      = device.wlan_mac_address
        else:
            self.description = "All nodes"
            mac_address      = 0xFFFFFFFFFFFFFFFF            

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not disassociate from {0}.".format(self.description)
        status_errors = { error_code : error_msg }

        resp.resp_is_valid(num_args=1, status_errors=status_errors, 
                           name='from Disassociate command')

# End Class


class NodeGetStationInfo(wn_message.BufferCmd):
    """Command to get the station info for a given node."""
    def __init__(self, node=None):
        super(NodeGetStationInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_GET_STATION_INFO

        if node is not None:
            mac_address = node.wlan_mac_address
        else:
            mac_address = CMD_PARAM_GET_ALL_ASSOCIATED            

        _add_mac_address_to_cmd(self, mac_address)


    def process_resp(self, resp):
        # Contains a WWARPNet Buffer of all station info entries.  Need to 
        #   convert to a list of station info dictionaries.
        import wlan_exp.log.entry_types as entry_types

        index   = 0
        data    = resp.get_bytes()
        ret_val = entry_types.entry_station_info.deserialize(data[index:])

        if (False):
            msg = "Station Info Data buffer:"
            for i, byte in enumerate(data[index:]):
                if ((i % 16) == 0): msg += "\n    "
                try:
                    msg += "0x{0:02X} ".format(ord(byte))
                except TypeError:
                    msg += "0x{0:02X} ".format(byte)
            print(msg)

        # Clean up the station info entries
        for val in ret_val:
            if (val['host_name'][0] == '\x00'):
                val['host_name'] = '\x00'
            else:
                import ctypes
                val['host_name'] = ctypes.c_char_p(val['host_name']).value

        return ret_val

# End Class


class NodeGetBSSInfo(wn_message.BufferCmd):
    """Command to get the BSS info for a given BSS ID."""
    def __init__(self, bssid=None):
        super(NodeGetBSSInfo, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_GET_BSS_INFO

        if bssid is None:
            bssid = 0x0000000000000000
        elif type(bssid) is str:
            bssid = CMD_PARAM_GET_ALL_ASSOCIATED      # If BSSID is a string, then get all bss info

        _add_mac_address_to_cmd(self, bssid)


    def process_resp(self, resp):
        # Contains a WWARPNet Buffer of all station info entries.  Need to 
        #   convert to a list of station info dictionaries.
        import wlan_exp.log.entry_types as entry_types

        index   = 0
        data    = resp.get_bytes()
        ret_val = entry_types.entry_bss_info.deserialize(data[index:])

        if (False):
            msg = "BSS Info Data buffer:"
            for i, byte in enumerate(data[index:]):
                if ((i % 16) == 0): msg += "\n    "
                try:
                    msg += "0x{0:02X} ".format(ord(byte))
                except TypeError:
                    msg += "0x{0:02X} ".format(byte)
            print(msg)

        # Clean up the bss info entries
        #   - Remove extra characters in the SSID
        #   - Convert the BSS ID to an integer so it can be treated like a MAC address
        for val in ret_val:
            import sys
            import ctypes
            val['ssid']      = ctypes.c_char_p(val['ssid']).value

            # Fix to support Python 2.x and 3.x
            if sys.version[0]=="2":
                val['bssid_int'] = sum([ord(b) << (8 * i) for i, b in enumerate(val['bssid'][::-1])])
            elif sys.version[0]=="3":
                val['bssid_int'] = sum([b << (8 * i) for i, b in enumerate(val['bssid'][::-1])])
            else:
                print("WARNING:  Unsupported python version.")
                val['bssid_int'] = 0

        return ret_val

# End Class



#--------------------------------------------
# Queue Commands
#--------------------------------------------
class QueueTxDataPurgeAll(wn_message.Cmd):
    """Command to purge all data transmit queues on the node."""
    def __init__(self):
        super(QueueTxDataPurgeAll, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_QUEUE_TX_DATA_PURGE_ALL
        
    def process_resp(self, resp):
        pass

# End Class



#--------------------------------------------
# AP Specific Commands
#--------------------------------------------
class NodeAPConfigure(wn_message.Cmd):
    """Command to configure the AP.
    
    Attributes (default state on the node is in CAPS):
        power_savings   -- Enable power saving mode (TRUE/False)
    """
    def __init__(self, support_power_savings=None):
        super(NodeAPConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_CONFIG

        flags = 0
        mask  = 0

        if support_power_savings is not None:
            mask += CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING
            if support_power_savings:
                flags += CMD_PARAM_NODE_AP_CONFIG_FLAG_POWER_SAVING
                
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeAPProcDTIMPeriod(wn_message.Cmd):
    """Command to get / set the number of beacon intervals between DTIM beacons
    
    Attributes:
        cmd         -- Sub-command to send over the WARPNet command.  Valid values are:
                         CMD_PARAM_READ
                         CMD_PARAM_WRITE
        num_beacons -- Number of beacon intervals between DTIM beacons (0 - 255)
    """
    def __init__(self, cmd, num_beacons=None):
        super(NodeAPProcDTIMPeriod, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_DTIM_PERIOD

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)

            if num_beacons is None:
                msg = "Number of beacon intervals [0,255] must be provided for WRITE"
                raise ValueError(msg)
            
            if (num_beacons <   0): num_beacons = 0
            if (num_beacons > 255): num_beacons = 255
            
            self.add_args(num_beacons)

        elif (cmd == CMD_PARAM_READ):
            self.add_args(cmd)

        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)
            

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not get / set number of beacon intervals between DTIM beacons"
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from DTIM Period command'):
            args = resp.get_args()
            return (args[1] & 0xFF)
        else:
            return CMD_PARAM_ERROR

# End Class


class NodeAPAddAssociation(wn_message.Cmd):
    """Command to add the association to the association table on the AP.
    
    Attributes:
        device        -- Device to add to the association table
        allow_timeout -- Allow the association to timeout if inactive
    
    NOTE:  This adds an association with the default tx/rx params.  If
        allow_timeout is not specified, the default on the node is to 
        not allow timeout of the association.
    """
    description = None

    def __init__(self, device, allow_timeout=None):
        super(NodeAPAddAssociation, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_ADD_ASSOCIATION

        flags = 0
        mask  = 0

        if allow_timeout is not None:
            mask += CMD_PARAM_ADD_ASSOCIATION_ALLOW_TIMEOUT
            if allow_timeout:
                flags += CMD_PARAM_ADD_ASSOCIATION_ALLOW_TIMEOUT
        
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


class NodeAPSetSSID(wn_message.Cmd):
    """Command to set the SSID of the AP."""
    ssid = None

    def __init__(self, ssid):
        super(NodeAPSetSSID, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_SET_SSID
        
        self.ssid = ssid            
        self.add_args(CMD_PARAM_WRITE)

        _add_ssid_to_cmd(self, ssid)
            

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set SSID on Node."
        status_errors = { error_code : error_msg }
        
        ssid = _get_ssid_from_resp(resp, status_errors)
 
        if self.ssid is not None:
            if (self.ssid != ssid):
                msg  = "WARNING:  SSID requested:  {0} \n".format(self.ssid)
                msg += "    Does not equal SSID returned:  {0}".format(ssid)
                print(msg)
        
        return ssid

# End Class


class NodeAPSetAuthAddrFilter(wn_message.Cmd):
    """Command to set the authentication address filter on the node.
    
    Attributes:
        allow  -- List of (address, mask) tuples that will be used to filter addresses
                  on the node.

    NOTE:  For the mask, bits that are 0 are treated as "any" and bits that are 1 are 
    treated as "must equal".  For the address, locations of one bits in the mask 
    must match the incoming addresses to pass the filter.
    """
    def __init__(self, allow):
        super(NodeAPSetAuthAddrFilter, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_SET_AUTHENTICATION_ADDR_FILTER

        length = len(allow)

        if (length > 50):
            msg  = "Currently, the WLAN Exp framework does not support more than "
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


class NodeAPProcBeaconInterval(wn_message.Cmd):
    """Command to get / set the time interval between beacons
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
                       CMD_PARAM_READ
                       CMD_PARAM_WRITE
        interval -- Number of Time Units (TU) between beacons [1, 65535]
    """
    def __init__(self, cmd, interval=None):
        super(NodeAPProcBeaconInterval, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_AP_BEACON_INTERVAL

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)

            if interval is None:
                msg = "Interval between beacons [1,65535] must be provided for WRITE"
                raise ValueError(msg)
            
            if (interval <     1): interval = 1
            if (interval > 65535): interval = 65535
            
            self.add_args(interval)

        elif (cmd == CMD_PARAM_READ):
            self.add_args(cmd)

        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)
            

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not read/write beacon interval."
        status_errors = { error_code : error_msg }

        if resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Beacon interval command'):
            args = resp.get_args()
            return (args[1] & 0xFFFF)
        else:
            return CMD_PARAM_ERROR

# End Class



#--------------------------------------------
# STA Specific Commands
#--------------------------------------------
class NodeSTAConfigure(wn_message.Cmd):
    """Command to configure the STA.
    
    Attributes (default state on the node is in CAPS):
        beacon_ts_update    -- Enable timestamp updates from beacons (TRUE/False)
    """
    def __init__(self, beacon_ts_update=None):
        super(NodeSTAConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_STA_CONFIG

        flags = 0
        mask  = 0

        if beacon_ts_update is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE
            if beacon_ts_update:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE
                
        self.add_args(flags)
        self.add_args(mask)
    
    def process_resp(self, resp):
        pass

# End Class


class NodeSTAAddAssociation(wn_message.Cmd):
    """Command to add the association to the association table on the STA.
    
    Attributes:
        device        -- Device to add to the association table
        aid           -- Association ID returned by the AP from the associate command
        channel       -- Channel of the association
        ssid          -- SSID of the association
    
    NOTE:  This adds an association with the default tx/rx params
    """
    description = None

    def __init__(self, device, aid, channel, ssid):
        super(NodeSTAAddAssociation, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_ADD_ASSOCIATION

        flags = 0
        mask  = 0

        # Currently, there are no association flags, but adding this into the 
        # message structure for future use.

        self.add_args(flags)
        self.add_args(mask)

        self.description = device.description
        mac_address      = device.wlan_mac_address

        _add_mac_address_to_cmd(self, mac_address)

        self.add_args(aid)
        
        tmp_chan = _get_channel_number(channel)

        if tmp_chan is not None:
            self.add_args(tmp_chan)
        else:
            self.add_args(CMD_PARAM_RSVD_CHANNEL)
        
        _add_ssid_to_cmd(self, ssid)        


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could add STA association to {0}.".format(self.description)
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, 
                               name='from STA associate command')):
            return True
        else:
            return False

# End Class



#--------------------------------------------
# IBSS Specific Commands
#--------------------------------------------
class NodeIBSSConfigure(wn_message.Cmd):
    """Command to configure the IBSS.
    
    Attributes (default state on the node is in CAPS):
        beacon_ts_update    -- Enable timestamp updates from beacons (TRUE/False)
        beacon_transmit     -- Enable beacon transmission (TRUE/False)
    
    NOTE:
        Allowed values of the (beacon_ts_update, beacon_transmit) are: 
            (True, True), (True, False), (False, False)
        (ie you must update your timestamps if you want to send beacons)
    """
    def __init__(self, beacon_ts_update=None, beacon_transmit=None):
        super(NodeIBSSConfigure, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_IBSS_CONFIG

        flags = 0
        mask  = 0

        if beacon_ts_update is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE
            if beacon_ts_update:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE

        if beacon_transmit is not None:
            mask += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT
            if beacon_transmit:
                flags += CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT

        legal_values = [0, CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE, 
                        (CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TS_UPDATE + CMD_PARAM_NODE_CONFIG_FLAG_BEACON_TRANSMIT)]

        if ((mask & flags) not in legal_values):
            msg  = "Allowed values of the (beacon_ts_update, beacon_transmit) are:\n"
            msg += "    (True, True), (True, False), (False, False)\n"
            msg += "Provided: ({0}, {1})".format(beacon_ts_update, beacon_transmit)
            raise ValueError(msg)

        self.add_args(flags)
        self.add_args(mask)

    
    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not configure IBSS Node."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from IBSS Configure command')):
            return True
        else:
            return False

# End Class



#--------------------------------------------
# STA / IBSS Common Commands
#--------------------------------------------
class NodeProcScanParam(wn_message.Cmd):
    """Command to configure the scan parameters
    
    Attributes:
        cmd                -- Command for Process Scan Param:
                                CMD_PARAM_WRITE
        time_per_channel   -- Time (in float sec) to spend on each channel (optional)
        idle_time_per_loop -- Time (in float sec) to wait between each scan loop (optional)
        channel_list       -- Channels to scan (optional)
                                Defaults to all channels in util.py wlan_channel array
                                Can provide either an entry or list of entries from 
                                  wlan_channel array or a channel number or list of 
                                  channel numbers
    """
    time_factor = 6
    time_type   = None

    def __init__(self, cmd, time_per_channel=None, idle_time_per_loop=None, channel_list=None):
        super(NodeProcScanParam, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_SCAN_PARAM

        if (cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)

            # Add the time_per_channel to the command
            _add_time_to_cmd32(self, time_per_channel, self.time_factor)

            # Add the idle_time_per_loop to the command
            _add_time_to_cmd32(self, idle_time_per_loop, self.time_factor)

            # Format the channel list
            if channel_list is not None:

                if type(channel_list) is list:
                    self.add_args(len(channel_list))
                    
                    for channel in channel_list:
                        self.add_channel(channel)
                else:
                    self.add_args(1)
                    self.add_channel(channel_list)       
            else:
                self.add_args(CMD_PARAM_RSVD)

        else:
            msg = "Unsupported command: {0}".format(cmd)
            raise ValueError(msg)


    def add_channel(self, channel):
        """Internal method to add a channel"""
        chan_to_add = CMD_PARAM_RSVD_CHANNEL
        
        try:
            chan_to_add = channel['index']
        except (KeyError, TypeError):
            import wlan_exp.util as util
            
            tmp_chan = util.find_channel_by_channel_number(channel)
            
            if tmp_chan is not None:
                chan_to_add = tmp_chan['index']
            else:
                msg  = "Unknown channel:  {0}".format(channel)
                raise ValueError(msg)                    
        
        self.add_args(chan_to_add)
            
    
    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not set scan parameters."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=1, status_errors=status_errors, name='from Scan parameter command')):
            return True
        else:
            return False

# End Class


class NodeProcScan(wn_message.Cmd):
    """Command to enable / disable active scan
    
    Attributes:
        enable -- Whether we are enabling (True) or disabling (False) active scan
        ssid   -- SSID to scan for as part of probe request (optional)
                      A value of None means that the node will scan for all SSIDs
        bssid  -- BSSID to scan for as part of probe request (optional)
                      A value of None means that the node will scan for all BSSIDs
    """
    def __init__(self, enable, ssid=None, bssid=None):
        super(NodeProcScan, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_SCAN

        if enable:
            self.add_args(CMD_PARAM_NODE_SCAN_ENABLE)
        else:
            self.add_args(CMD_PARAM_NODE_SCAN_DISABLE)

        _add_mac_address_to_cmd(self, bssid)

        if ssid is not None:
            _add_ssid_to_cmd(self, ssid)
        else:
            _add_ssid_to_cmd(self, "")       # Default SSID to scan for "all SSIDs"
        

    def process_resp(self, resp):
        pass

# End Class


class NodeProcJoin(wn_message.Cmd):
    """Command to join a given BSS
    
    Attributes:
        bss_info -- Dictionary discribing the BSS to join
        timeout  -- Maximum amount of time (in float seconds) to try to join the 
                    given network (optional)
    """
    time_factor = 6
    
    def __init__(self, bss_info, timeout=None):
        super(NodeProcJoin, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_JOIN

        _add_time_to_cmd32(self, timeout, self.time_factor)

        if type(bss_info) is dict:
            # Convert BSS info dictionary to bytes for transfer
            import struct
            import wlan_exp.log.entry_types as entry_types
    
            data_to_send = entry_types.entry_bss_info.serialize(bss_info)
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
        else:
            msg = "BSS info parameter must be a bss_info dictionary."
            raise TypeError(msg)
        

    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not join the network."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Join command')):
            args = resp.get_args()
            if (args[1] != CMD_PARAM_NODE_JOIN_FAILED):
                return True
            else:
                return False
        else:
            return False

# End Class


class NodeProcScanAndJoin(wn_message.Cmd):
    """Command to scan for the given network and join it if present
    
    Attributes:
        ssid   -- SSID to scan for as part of probe request
        bssid  -- BSSID to scan for as part of probe request (optional)
                      A value of None means that the node will scan for all BSSIDs
        timeout  -- Maximum amount of time (in float seconds) to try to scan for the 
                    given network and depending on the type of node the maximum 
                    amount of time to try to join the given network (optional)
    """
    time_factor = 0                              # Need to only transfer seconds
    
    def __init__(self, ssid, bssid=None, timeout=5.0):
        super(NodeProcScanAndJoin, self).__init__()
        self.command = _CMD_GRPID_NODE + CMDID_NODE_SCAN_AND_JOIN

        _add_time_to_cmd32(self, timeout, self.time_factor)

        _add_mac_address_to_cmd(self, bssid)

        if ssid is not None:
            _add_ssid_to_cmd(self, ssid)
        else:
            raise ValueError("Must provide a valid SSID for Scan and Join.")


    def process_resp(self, resp):
        error_code    = CMD_PARAM_ERROR
        error_msg     = "Could not join the network."
        status_errors = { error_code : error_msg }

        if (resp.resp_is_valid(num_args=2, status_errors=status_errors, name='from Join command')):
            args = resp.get_args()
            if (args[1] != CMD_PARAM_NODE_JOIN_FAILED):
                return True
            else:
                return False
        else:
            return False

# End Class



#--------------------------------------------
# Memory Access Commands - For developer use only
#--------------------------------------------
class NodeMemAccess(wn_message.Cmd):
    """Command to read/write memory in CPU High / CPU Low
    
    Attributes:
        cmd       -- Sub-command to send over the WARPNet command.  Valid values are:
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
        if(high):
            self.command = _CMD_GRPID_NODE + CMDID_DEV_MEM_HIGH
        else:
            self.command = _CMD_GRPID_NODE + CMDID_DEV_MEM_LOW

        if(cmd == CMD_PARAM_READ):
            self.add_args(cmd)
            self.add_args(address)
            self.add_args(length)

            self._read_len = length

        elif(cmd == CMD_PARAM_WRITE):
            self.add_args(cmd)
            self.add_args(address)
            self.add_args(length)

            try:
                for v in values:
                    self.add_args(v)
            except TypeError:
                self.add_args(values)

        else:
            raise Exception('ERROR: NodeMemAccess constructor arguments invalid');
    
    def process_resp(self, resp):
        if (self._read_len is not None): # Was a read command
            error_code    = CMD_PARAM_ERROR
            error_msg     = "Memory access failed."
            status_errors = { error_code : error_msg }

            if resp.resp_is_valid(num_args=(2 + self._read_len), status_errors=status_errors, 
                                  name='from CPU Mem command'):
                args = resp.get_args()

                if(len(args) == 3):
                    return args[2]
                elif(len(args) > 3):
                    return args[2:]
                else:
                    raise Exception('ERROR: invalid response to read_mem - N_ARGS = {0}'.format(len(args)))
            else:
                return CMD_PARAM_ERROR
        else: # Was a write command
            pass

# End Class



#--------------------------------------------
# Misc Helper methods
#--------------------------------------------
def _add_mac_address_to_cmd(cmd, mac_address):
    if mac_address is not None:
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


def _add_time_to_cmd64(cmd, time, time_factor):
    """Internal method to add a 64-bit time value to the given command.

    Returns:
        Type of the time argument    
    """
    ret_val = TIME_TYPE_FLOAT
    
    if time is not None:
        # Format the time appropriately
        if   (type(time) is float):
            time_to_send   = int(round(time, time_factor) * (10**time_factor))
            ret_val        = TIME_TYPE_FLOAT
        elif (type(time) is int):
            time_to_send   = time
            ret_val        = TIME_TYPE_INT
        else:
            raise TypeError("Time must be either a float or int")

        cmd.add_args((time_to_send & 0xFFFFFFFF))
        cmd.add_args(((time_to_send >> 32) & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_TIME)
        cmd.add_args(CMD_PARAM_RSVD_TIME)

    return ret_val

# End def


def _add_time_to_cmd32(cmd, time, time_factor):
    """Internal method to add a 32-bit time value to the given command.

    Returns:
        Type of the time argument    
    """
    ret_val = TIME_TYPE_FLOAT
    
    if time is not None:
        # Format the time appropriately
        if   (type(time) is float):
            time_to_send   = int(round(time, time_factor) * (10**time_factor))
            ret_val        = TIME_TYPE_FLOAT
        elif (type(time) is int):
            time_to_send   = time
            ret_val        = TIME_TYPE_INT
        else:
            raise TypeError("Time must be either a float or int")

        if (time_to_send > 0xFFFFFFFF):
            time_to_send = 0xFFFFFFFF
            print("WARNING:  Time value (in microseconds) exceeds 32-bits.  Truncating.\n")

        cmd.add_args((time_to_send & 0xFFFFFFFF))
    else:
        cmd.add_args(CMD_PARAM_RSVD_TIME)

    return ret_val

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
        
        tmp_chan = util.find_channel_by_channel_number(channel)
        
        if tmp_chan is not None:
            my_channel = tmp_chan['index']
        else:
            msg  = "Unknown channel:  {0}".format(channel)
            raise ValueError(msg)

    return my_channel

# End def

