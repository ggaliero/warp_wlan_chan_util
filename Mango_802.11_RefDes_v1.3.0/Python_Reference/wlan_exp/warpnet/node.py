# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Node
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

This module provides class definition for WARPNet Node.

Functions (see below for more information):
    WnNode() -- Base class for WARPNet node
    WnNodeFactory() -- Base class for creating WARPNet nodes

Integer constants:
    NODE_TYPE, NODE_ID, NODE_HW_GEN, NODE_DESIGN_VER, NODE_SERIAL_NUM, 
      NODE_FPGA_DNA -- Node hardware parameter constants 

If additional hardware parameters are needed for sub-classes of WnNode, please
make sure that the values of these hardware parameters are not reused.

"""

from . import version
from . import cmds as wn_cmds
from . import exception as wn_ex


__all__ = ['WnNode', 'WnNodeFactory']


# WARPNet Node Parameter Identifiers
#   NOTE:  The C counterparts are found in *_node.h
NODE_TYPE               = 0
NODE_ID                 = 1
NODE_HW_GEN             = 2
NODE_DESIGN_VER         = 3
NODE_FPGA_DNA           = 4
NODE_SERIAL_NUM         = 5



class WnNode(object):
    """Base Class for WARPNet node.
    
    The WARPNet node represents one node in a WARPNet network.  This class
    is the primary interface for interacting with nodes by providing methods
    for sending commands and checking status of nodes.
    
    By default, the base WARPNet node provides many useful node attributes
    as well as a transport component.
    
    Attributes:
        node_type     -- Unique type of the WARPNet node
        node_id       -- Unique identification for this node
        name          -- User specified name for this node (supplied by user scripts)
        description   -- String description of this node (auto-generated)
        serial_number -- Node's serial number, read from EEPROM on hardware
        fpga_dna      -- Node's FPGA'a unique identification (on select hardware)
        hw_ver        -- WARP hardware version of this node
        wn_ver_major  -- WARPNet version running on this node
        wn_ver_minor
        wn_ver_revision
        
        transport -- Node's transport object
        transport_bcast -- Node's broadcast transport object
    """
    network_config           = None

    node_type                = None
    node_id                  = None
    name                     = None
    description              = None
    serial_number            = None
    sn_str                   = None
    fpga_dna                 = None
    hw_ver                   = None
    wn_ver_major             = None
    wn_ver_minor             = None
    wn_ver_revision          = None

    transport                = None
    transport_bcast          = None
    transport_tracker        = None
    
    def __init__(self, network_config=None):
        (self.wn_ver_major, self.wn_ver_minor, self.wn_ver_revision) = version.wn_ver()
        
        if network_config is not None:
            self.network_config = network_config
        else:
            from . import config as wn_config

            self.network_config = wn_config.NetworkConfiguration()

        self.transport_tracker = 0


    def __del__(self):
        """Clears the transport object to close any open socket connections
        in the event the node is deleted"""
        if self.transport:
            self.transport.wn_close()
            self.transport = None

        if self.transport_bcast:
            self.transport_bcast.wn_close()
            self.transport_bcast = None


    def set_init_configuration(self, serial_number, node_id, node_name, 
                               ip_address, unicast_port, bcast_port):
        """Set the initial configuration of the node."""
        from . import util as wn_util

        host_id      = self.network_config.get_param('host_id')
        tx_buf_size  = self.network_config.get_param('tx_buffer_size')
        rx_buf_size  = self.network_config.get_param('rx_buffer_size')
        tport_type   = self.network_config.get_param('transport_type')

        (sn, sn_str) = wn_util.wn_get_serial_number(serial_number, output=False)
        
        if (tport_type == 'python'):
            from . import transport_eth_udp_py as unicast_tp
            from . import transport_eth_udp_py_bcast as bcast_tp

            if self.transport is None:
                self.transport = unicast_tp.TransportEthUdpPy()

            if self.transport_bcast is None:
                self.transport_bcast = bcast_tp.TransportEthUdpPyBcast(self.network_config)
        else:
            print("Transport not defined\n")
        
        # Set Node information        
        self.node_id       = node_id
        self.name          = node_name
        self.serial_number = sn
        self.sn_str        = sn_str

        # Set Node Unicast Transport information
        self.transport.wn_open(tx_buf_size, rx_buf_size)
        self.transport.set_ip_address(ip_address)
        self.transport.set_unicast_port(unicast_port)
        self.transport.set_bcast_port(bcast_port)
        self.transport.set_src_id(host_id)
        self.transport.set_dest_id(node_id)

        # Set Node Broadcast Transport information
        self.transport_bcast.wn_open(tx_buf_size, rx_buf_size)
        self.transport_bcast.set_ip_address(ip_address)
        self.transport_bcast.set_unicast_port(unicast_port)
        self.transport_bcast.set_bcast_port(bcast_port)
        self.transport_bcast.set_src_id(host_id)
        self.transport_bcast.set_dest_id(0xFFFF)
        

    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        
        self.transport.ping(self)
        self.transport.test_payload_size(self, jumbo_frame_support)        

        resp = self.node_get_info()
        try:
            self.process_parameters(resp)
        except wn_ex.ParameterError as err:
            print(err)
            raise wn_ex.NodeError(self, "Configuration Error")

        # Set description
        self.description = self.__repr__()


    def set_name(self, name):
        """Set the name of the node.
        
        Args:
            name (str):  User provided name of the node.
        
        .. note::  The name provided will affect the Python environment only (ie it will
            update strings in wlan_exp but will not be transmitted to the node.)
        """
        self.name        = name
        self.description = self.__repr__()



    # -------------------------------------------------------------------------
    # WARPNet Commands for the Node
    # -------------------------------------------------------------------------
    def node_identify(self):
        """Have the node physically identify itself."""
        self.send_cmd(wn_cmds.NodeIdentify(self.serial_number))

    def node_ping(self):
        """Ping the node."""
        self.transport.ping(self, output=True)

    def node_get_info(self):
        """Get the Hardware Information from the node."""
        return self.send_cmd(wn_cmds.NodeGetHwInfo())

    def node_get_temp(self):
        """Get the temperature of the node."""
        (curr_temp, _, _) = self.send_cmd(wn_cmds.NodeGetTemperature()) # Min / Max temp not used
        return curr_temp

    def node_setup_network_inf(self):
        """Setup the transport network information for the node."""
        self.send_cmd_bcast(wn_cmds.NodeSetupNetwork(self))
        
    def node_reset_network_inf(self):
        """Reset the transport network information for the node."""
        self.send_cmd_bcast(wn_cmds.NodeResetNetwork(self.serial_number))

    def node_get_warpnet_type(self):
        """Get the WARPNet node type of the node."""
        if self.node_type is None:
            return self.send_cmd(wn_cmds.NodeGetWarpNetType())
        else:
            return self.node_type

    # -------------------------------------------------------------------------
    # WARPNet Parameter Framework
    #   Allows for processing of hardware parameters
    # -------------------------------------------------------------------------
    def process_parameters(self, parameters):
        """Process all parameters.
        
        Each parameter is of the form::
                   | 31 ... 24 | 23 ... 16 | 15 ... 8 | 7 ... 0 |
            Word 0 | Reserved  | Group     | Length             |
            Word 1 | Parameter Identifier                       |
            Word 2 | Value 0 of Parameter                       |
            ...
            Word N | Value M of Parameter                       |
            
        where the number of parameters, M, is equal to the Length field
        """
        
        param_start = 0
        param_end   = len(parameters)
        
        while (param_start < param_end):
            param_group = (parameters[param_start] & 0x00FF0000) >> 16
            param_length = (parameters[param_start] & 0x0000FFFF)
            param_identifier = parameters[param_start+1]

            value_start = param_start + 2
            value_end = value_start + param_length
            
            param_values = parameters[value_start:value_end]
            
            # print(str("Param Start = " + str(param_start) + "\n" +
            #           "    param_group  = " + str(param_group) + "\n" + 
            #           "    param_length = " + str(param_length) + "\n" + 
            #           "    param_id     = " + str(param_identifier) + "\n"))
            
            self.process_parameter_group(param_group, param_identifier, 
                                         param_length, param_values)
            
            param_start = value_end


    def process_parameter_group(self, group, identifier, length, values):
        """Process the Parameter Group"""
        if   (group == wn_cmds.GRPID_NODE):
            self.process_parameter(identifier, length, values)
        elif (group == wn_cmds.GRPID_TRANS):
            self.transport.process_parameter(identifier, length, values)
        else:
            raise wn_ex.ParameterError("Group", "Unknown Group: {}".format(group))


    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if   (identifier == NODE_TYPE):
            if (length == 1):
                self._set_node_type(values[0])
            else:
                raise wn_ex.ParameterError("NODE_TYPE", "Incorrect length")

        elif (identifier == NODE_ID):
            if (length == 1):
                self.node_id = values[0]
            else:
                raise wn_ex.ParameterError("NODE_ID", "Incorrect length")

        elif (identifier == NODE_HW_GEN):
            if (length == 1):
                self.hw_ver = (values[0] & 0xFF)
            else:
                raise wn_ex.ParameterError("NODE_HW_GEN", "Incorrect length")

        elif (identifier == NODE_DESIGN_VER):
            if (length == 1):                
                self.wn_ver_major = (values[0] & 0xFF000000) >> 24
                self.wn_ver_minor = (values[0] & 0x00FF0000) >> 16
                self.wn_ver_revision = (values[0] & 0x0000FFFF)                
                
                # Check to see if there is a version mismatch
                self.check_wn_ver()
            else:
                raise wn_ex.ParameterError("NODE_DESIGN_VER", "Incorrect length")

        elif (identifier == NODE_SERIAL_NUM):
            if (length == 1):
                from . import util as wn_util

                (sn, sn_str) = wn_util.wn_get_serial_number(values[0], output=False)
                self.serial_number = sn
                self.sn_str        = sn_str
            else:
                raise wn_ex.ParameterError("NODE_SERIAL_NUM", "Incorrect length")

        elif (identifier == NODE_FPGA_DNA):
            if (length == 2):
                self.fpga_dna = (2**32 * values[1]) + values[0]
            else:
                raise wn_ex.ParameterError("NODE_FPGA_DNA", "Incorrect length")

        else:
            raise wn_ex.ParameterError(identifier, "Unknown node parameter")


    # -------------------------------------------------------------------------
    # Transmit / Receive methods for the Node
    # -------------------------------------------------------------------------
    def send_cmd(self, cmd, max_attempts=2, max_req_size=None, timeout=None):
        """Send the provided command.
        
        Attributes:
            cmd          -- WnCommand to send
            max_attempts -- Maximum number of attempts to send a given command
            max_req_size -- Maximum request size (applys only to Buffer Commands)
            timeout      -- Maximum time to wait for a response from the node
        """
        from . import transport as wn_transport

        resp_type = cmd.get_resp_type()
        
        if  (resp_type == wn_transport.TRANSPORT_NO_RESP):
            payload = cmd.serialize()
            self.transport.send(payload, robust=False)

        elif (resp_type == wn_transport.TRANSPORT_WN_RESP):
            resp = self._receive_resp(cmd, max_attempts, timeout)
            return cmd.process_resp(resp)

        elif (resp_type == wn_transport.TRANSPORT_WN_BUFFER):
            resp = self._receive_buffer(cmd, max_attempts, max_req_size, timeout)
            return cmd.process_resp(resp)

        else:
            raise wn_ex.TransportError(self.transport, 
                                       "Unknown response type for command")


    def _receive_resp(self, cmd, max_attempts, timeout):
        """Internal method to receive a response for a given command payload"""
        from . import message as wn_message

        reply = b''
        done = False
        resp = wn_message.Resp()

        payload = cmd.serialize()
        self.transport.send(payload)

        while not done:
            try:
                reply = self.transport.receive(timeout)
                self._receive_success()
            except wn_ex.TransportError:
                self._receive_failure()

                if self._receive_failure_exceeded(max_attempts):
                    raise wn_ex.TransportError(self.transport, 
                              "Max retransmissions without reply from node")

                self.transport.send(payload)
            else:
                resp.deserialize(reply)
                done = True
                
        return resp


    def _receive_buffer(self, cmd, max_attempts, max_req_size, timeout):
        """Internal method to receive a buffer for a given command payload.
        
        Depending on the size of the buffer, the framework will split a
        single large request into multiple smaller requests based on the 
        max_req_size.  This is to:
          1) Minimize the probability that the OS drops a packet
          2) Minimize the time that the Ethernet interface on the node is busy 
             and cannot service other requests

        To see performance data, set the 'display_perf' flag to True.
        """
        from . import message as wn_message

        display_perf    = False
        print_warnings  = True
        print_debug_msg = False
        
        reply           = b''

        buffer_id       = cmd.get_buffer_id()
        flags           = cmd.get_buffer_flags()
        start_byte      = cmd.get_buffer_start_byte()
        total_size      = cmd.get_buffer_size()

        tmp_resp        = None
        resp            = None

        if max_req_size is not None:
            fragment_size = max_req_size
        else:
            fragment_size = total_size
        
        # Allocate a complete response buffer        
        resp = wn_message.Buffer(buffer_id, flags, start_byte, total_size)

        if display_perf:
            import time
            print("Receive buffer")
            start_time = time.time()

        # If the transfer is more than the fragment size, then split the transaction
        if (total_size > fragment_size):
            size      = fragment_size
            start_idx = start_byte
            num_bytes = 0

            while (num_bytes < total_size):
                # Create fragmented command
                if (print_debug_msg):
                    print("\nFRAGMENT:  {0:10d}/{1:10d}\n".format(num_bytes, total_size))    
    
                # Handle the case of the last fragment
                if ((num_bytes + size) > total_size):
                    size = total_size - num_bytes

                # Update the command with the location and size of fragment
                cmd.update_start_byte(start_idx)
                cmd.update_size(size)
                
                # Send the updated command
                tmp_resp = self.send_cmd(cmd)
                tmp_size = tmp_resp.get_buffer_size()
                
                if (tmp_size == size):
                    # Add the response to the buffer and increment loop variables
                    resp.merge(tmp_resp)
                    num_bytes += size
                    start_idx += size
                else:
                    # Exit the loop because communication has totally failed for 
                    # the fragment and there is no point to request the next 
                    # fragment since we only return the truncated buffer.
                    if (print_warnings):
                        msg  = "WARNING:  Command did not return a complete fragment.\n"
                        msg += "  Requested : {0:10d}\n".format(size)
                        msg += "  Received  : {0:10d}\n".format(tmp_size)
                        msg += "Returning truncated buffer."
                        print(msg)

                    break
        else:
            # Normal buffer receive flow
            payload = cmd.serialize()
            self.transport.send(payload)
    
            while not resp.is_buffer_complete():
                try:
                    reply = self.transport.receive(timeout)
                    self._receive_success()
                except wn_ex.TransportError:
                    self._receive_failure()
                    if print_warnings:
                        print("WARNING:  Transport timeout.  Requesting missing data.")
                    
                    # If there is a timeout, then request missing part of the buffer
                    if self._receive_failure_exceeded(max_attempts):
                        if print_warnings:
                            print("ERROR:  Max re-transmissions without reply from node.")
                        raise wn_ex.TransportError(self.transport, 
                                  "Max retransmissions without reply from node")
    
                    # Get the missing locations
                    locations = resp.get_missing_byte_locations()

                    if print_debug_msg:
                        print(resp)
                        print(resp.tracker)
                        print("Missing Locations in Buffer:")
                        print(locations)

                    # Send commands to fill in the buffer
                    for location in locations:
                        if (print_debug_msg):
                            print("\nLOCATION: {0:10d}    {1:10d}\n".format(location[0], location[2]))

                        # Update the command with the new location
                        cmd.update_start_byte(location[0])
                        cmd.update_size(location[2])
                        
                        if (location[2] < 0):
                            print("ERROR:  Issue with finding missing bytes in response:")
                            print("Response Tracker:")
                            print(resp.tracker)
                            print("\nMissing Locations:")
                            print(locations)
                            raise Exception()
                        
                        # Use the standard send so that you get a WARPNet buffer 
                        #   with missing data.  This avoids any race conditions
                        #   when requesting multiple missing locations.  Make sure
                        #   that max_attempts are set to 1 for the re-request so
                        #   that we do not get in to an infinite loop
                        try:
                            location_resp = self.send_cmd(cmd, max_attempts=max_attempts)
                            self._receive_success()
                        except wn_ex.TransportError:
                            # If we have timed out on a re-request, then there 
                            # is something wrong and we should just clean up
                            # the response and get out of the loop.
                            if print_warnings:
                                print("WARNING:  Transport timeout.  Returning truncated buffer.")
                                print("  Timeout requesting missing location: {1} bytes @ {0}".format(location[0], location[2]))
                                
                            self._receive_failure()
                            resp.trim()
                            return resp
                        
                        if print_debug_msg:
                            print("Adding Response:")
                            print(location_resp)
                            print(resp)                            
                        
                        # Add the response to the buffer
                        resp.merge(location_resp)

                        if print_debug_msg:
                            print("Buffer after merge:")
                            print(resp)
                            print(resp.tracker)
                        
                else:
                    resp.add_data_to_buffer(reply)

        # Trim the final buffer in case there were missing fragments
        resp.trim()
        
        if display_perf:
            print("    Receive time: {0}".format(time.time() - start_time))
        
        return resp
        
    
    def send_cmd_bcast(self, cmd, pkt_type=None):
        """Send the provided command over the broadcast transport.

        NOTE:  Currently, broadcast commands cannot have a response.
        
        Attributes:
            cmd -- WnCommand to send
        """
        self.transport_bcast.send(payload=cmd.serialize(), pkt_type=pkt_type)


    def receive_resp(self, timeout=None):
        """Return a list of responses that are sitting in the host's 
        receive queue.  It will empty the queue and return them all the 
        calling method."""
        from . import message as wn_message

        output = []
        
        resp = self.transport.receive(timeout)
        
        if resp:
            # Create a list of response object if the list of bytes is a 
            # concatenation of many responses
            done = False
            
            while not done:
                wn_resp = wn_message.Resp()
                wn_resp.deserialize(resp)
                resp_len = wn_resp.sizeof()

                if resp_len < len(resp):
                    resp = resp[(resp_len):]
                else:
                    done = True
                    
                output.append(wn_resp)
        
        return output



    # -------------------------------------------------------------------------
    # Transport Tracker
    # -------------------------------------------------------------------------
    def _receive_success(self):
        """Internal method to indicate to the tracker that we successfully 
        received a packet.
        """
        self.transport_tracker = 0

    
    def _receive_failure(self):
        """Internal method to indicate to the tracker that we had a 
        receive failure.
        """
        self.transport_tracker += 1


    def _receive_failure_exceeded(self, max_attempts):
        """Internal method to indicate if we have had more recieve 
        failures than max_attempts.
        """
        if (self.transport_tracker < max_attempts):
            return False
        else:
            return True



    # -------------------------------------------------------------------------
    # Misc methods for the Node
    # -------------------------------------------------------------------------
    def _set_node_type(self, node_type):
        """Set the node_type """
        self.node_type = node_type


    def check_wn_ver(self):
        """Check the WARPNet version of the node against the current WARPNet version."""
        ver_str     = version.wn_ver_str(self.wn_ver_major, self.wn_ver_minor, 
                                         self.wn_ver_revision)

        caller_desc = "During initialization '{0}' returned version {1}".format(self.description, ver_str)

        status = version.wn_ver_check(major=self.wn_ver_major,
                                      minor=self.wn_ver_minor,
                                      revision=self.wn_ver_revision,
                                      caller_desc=caller_desc)

        if (status == version.WN_VERSION_NEWER):
            print("Please update the C code on the node to the proper WARPNet version.")
        
        if (status == version.WN_VERSION_OLDER):
            print("Please update the WARPNet installation to match the version on the node.")


    def __str__(self):
        """Pretty print WnNode object"""
        msg = ""

        if self.serial_number is not None:
            msg += "Node:\n"
            msg += "    Node ID       :  {0}\n".format(self.node_id)
            msg += "    Serial #      :  {0}\n".format(self.sn_str)
            msg += "    HW version    :  WARP v{0}\n".format(self.hw_ver)
        else:
            msg += "Node not initialized."

        if self.transport is not None:
            msg += "WARPNet "
            msg += str(self.transport)

        return msg


    def __repr__(self):
        """Return node name and description"""
        msg = ""
        if self.serial_number is not None:
            msg  = "{0}: ".format(self.sn_str)
            msg += "ID {0:5d}".format(self.node_id)
            
            if self.name is not None:
                msg += " ({0})".format(self.name)
        else:
            msg += "Node not initialized."
        
        return msg

# End Class



class WnNodeFactory(WnNode):
    """Sub-class of WARPNet node used to help with node configuration 
    and setup.
    
    This class will maintian the dictionary of WARPNet Node Types.  The 
    dictionary contains the 32-bit WARPNet Node Type as a key and the 
    corresponding class name as a value.
    
    To add new WARPNet Node Types, you can sub-class WnNodeConfig and 
    add your own WARPNet Node Types as part of your WnConfig file.
    
    Attributes:
        warpnet_dict -- Dictionary of WARPNet Node Types to class names
    """
    wn_dict             = None


    def __init__(self, network_config=None):
        from . import defaults as wn_defaults

        super(WnNodeFactory, self).__init__(network_config)
 
        self.wn_dict = {}

        # Add default classes to the factory
        self.node_add_class(wn_defaults.WN_NODE_TYPE, 
                            wn_defaults.WN_NODE_CLASS,
                            wn_defaults.WN_NODE_DESCRIPTION)
        
    
    def setup(self, node_dict):
        self.set_init_configuration(serial_number=node_dict['serial_number'],
                                    node_id=node_dict['node_id'], 
                                    node_name=node_dict['node_name'], 
                                    ip_address=node_dict['ip_address'], 
                                    unicast_port=node_dict['unicast_port'], 
                                    bcast_port=node_dict['bcast_port'])


    def create_node(self, network_config=None, network_reset=True):
        """Based on the WARPNet Node Type, dynamically create and return 
        the correct WARPNet node."""        
        node = None

        # Initialize the node network interface
        if network_reset:
            # Send broadcast command to reset WARPNet node network interface
            self.node_reset_network_inf()
    
            # Send broadcast command to initialize WARPNet node network interface
            self.node_setup_network_inf()

        try:
            # Send unicast command to get the WARPNet type
            wn_node_type = self.node_get_warpnet_type()
            
            # Get the node class from the Factory dictionary
            node_class = self.node_get_class(wn_node_type)
        
            if node_class is not None:
                node = self.node_eval_class(node_class, network_config)
                node.set_init_configuration(serial_number=self.serial_number,
                                            node_id=self.node_id,
                                            node_name=self.name,
                                            ip_address=self.transport.ip_address,
                                            unicast_port=self.transport.unicast_port,
                                            bcast_port=self.transport.bcast_port)

                msg  = "Initializing {0}".format(node.sn_str)
                if node.name is not None:
                    msg += " as {0}".format(node.name)
                print(msg)
            else:
                self.print_wn_node_types()
                msg  = "ERROR:  Node {0}\n".format(self.sn_str)
                msg += "    Unknown WARPNet type: 0x{0:8x}\n".format(wn_node_type)
                print(msg)

        except wn_ex.TransportError as err:
            msg  = "ERROR:  Node {0}\n".format(self.sn_str)
            msg += "    Node is not responding.  Please ensure that the \n"
            msg += "    node is powered on and is properly configured.\n"
            print(msg)
            print(err)

        return node
        

    def node_eval_class(self, node_class, network_config):
        """Evaluate the node_class string to create a node.  
        
        NOTE:  This should be overridden in each sub-class with the same
        overall structure but a different import.  Please call the super
        class function instead of returning an error so that the calls 
        will propagate to catch all node types.
        
        NOTE:  network_config is used as part of the node_class string
        to initialize the node.
        """
        from . import defaults
        from . import node
        
        tmp_node = None

        try:
            tmp_node = eval(node_class, globals(), locals())
        except:
            # We will catch all errors that might occur when trying to create
            # the node.  Since this is the parent class of all nodes, if we
            # cannot create the node here, we will raise a ConfigError
            pass
        
        if tmp_node is None:
            self.print_wn_node_types()
            msg = "Cannot create node of class: {0}".format(node_class)
            raise wn_ex.ConfigError(msg)
        else:
            return tmp_node


    def node_add_class(self, wn_node_type, class_name, description):
        if (wn_node_type in self.wn_dict):
            print("WARNING: Changing definition of {0}".format(wn_node_type))
            
        self.wn_dict[wn_node_type] = {}
        self.wn_dict[wn_node_type]['class']        = class_name
        self.wn_dict[wn_node_type]['description']  = description
 

    def node_get_class(self, wn_node_type):
        """Get the class string of the node from the WARPNet type."""
        node_type = wn_node_type
        
        if (node_type in self.wn_dict.keys()):
            return self.wn_dict[node_type]['class']
        else:
            return None


    def node_get_description(self, wn_node_type):
        """Get the description of the node from the WARPNet type."""
        node_type = wn_node_type
        
        if (node_type in self.wn_dict.keys()):
            return self.wn_dict[node_type]['description']
        else:
            return None


    def get_wn_type_dict(self):
        """Get the WARPNet Type dictonary."""
        return self.wn_dict


    def print_wn_node_types(self):
        msg = "WARPNet Node Types: \n"
        for wn_node_type in self.wn_dict.keys():
            msg += "    0x{0:08x} = ".format(wn_node_type)
            msg += "'{0}'\n".format(self.wn_dict[wn_node_type]) 
        print(msg)


# End Class

















