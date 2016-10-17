# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - WARP Node
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides class definition for WARP Node.

Functions (see below for more information):
    WarpNode()        -- Base class for WARP nodes
    WarpNodeFactory() -- Base class for creating a WarpNode

Integer constants:
    NODE_TYPE, NODE_ID, NODE_HW_GEN, NODE_SERIAL_NUM, 
      NODE_FPGA_DNA -- Node hardware parameter constants 

If additional hardware parameters are needed for sub-classes of WarpNode, 
please make sure that the values of these hardware parameters are not reused.

"""

from . import cmds
from . import exception as ex


__all__ = ['WarpNode', 'WarpNodeFactory']


# Node Parameter Identifiers
#     - The C counterparts are found in *_node.h
NODE_TYPE               = 0
NODE_ID                 = 1
NODE_HW_GEN             = 2
NODE_SERIAL_NUM         = 3
NODE_FPGA_DNA           = 4



class WarpNode(object):
    """Base Class for WARP node.
    
    The WARP node represents one node in a network.  This class is the 
    primary interface for interacting with nodes by providing methods for 
    sending commands and checking status of nodes.
    
    By default, the base WARP node provides many useful node attributes
    as well as a transport component.
    
    Attributes:
        node_type            -- Unique type of the WARP node
        node_id              -- Unique identification for this node
        name                 -- User specified name for this node (supplied by user scripts)
        description          -- String description of this node (auto-generated)
        serial_number        -- Node's serial number, read from EEPROM on hardware
        fpga_dna             -- Node's FPGA'a unique identification (on select hardware)
        hw_ver               -- WARP hardware version of this node

        transport            -- Node's transport object
        transport_broadcast  -- Node's broadcast transport object
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

    transport                = None
    transport_broadcast      = None
    transport_tracker        = None
    
    def __init__(self, network_config=None):
        if network_config is not None:
            self.network_config = network_config
        else:
            from . import config

            self.network_config = config.NetworkConfiguration()

        self.transport_tracker = 0


    def __del__(self):
        """Clear the transport object to close any open socket connections
        in the event the node is deleted"""
        if self.transport:
            self.transport.transport_close()
            self.transport = None

        if self.transport_broadcast:
            self.transport_broadcast.transport_close()
            self.transport_broadcast = None


    def set_init_configuration(self, serial_number, node_id, node_name, 
                               ip_address, unicast_port, broadcast_port):
        """Set the initial configuration of the node."""
        from . import util

        host_id      = self.network_config.get_param('host_id')
        tx_buf_size  = self.network_config.get_param('tx_buffer_size')
        rx_buf_size  = self.network_config.get_param('rx_buffer_size')
        tport_type   = self.network_config.get_param('transport_type')

        (sn, sn_str) = util.get_serial_number(serial_number, output=False)
        
        if (tport_type == 'python'):
            from . import transport_eth_ip_udp_py as unicast_tp
            from . import transport_eth_ip_udp_py_broadcast as broadcast_tp

            if self.transport is None:
                self.transport = unicast_tp.TransportEthIpUdpPy()

            if self.transport_broadcast is None:
                self.transport_broadcast = broadcast_tp.TransportEthIpUdpPyBroadcast(self.network_config)
        else:
            print("Transport not defined\n")
        
        # Set Node information        
        self.node_id       = node_id
        self.name          = node_name
        self.serial_number = sn
        self.sn_str        = sn_str

        # Set Node Unicast Transport information
        self.transport.transport_open(tx_buf_size, rx_buf_size)
        self.transport.set_ip_address(ip_address)
        self.transport.set_unicast_port(unicast_port)
        self.transport.set_broadcast_port(broadcast_port)
        self.transport.set_src_id(host_id)
        self.transport.set_dest_id(node_id)

        # Set Node Broadcast Transport information
        self.transport_broadcast.transport_open(tx_buf_size, rx_buf_size)
        self.transport_broadcast.set_ip_address(ip_address)
        self.transport_broadcast.set_unicast_port(unicast_port)
        self.transport_broadcast.set_broadcast_port(broadcast_port)
        self.transport_broadcast.set_src_id(host_id)
        self.transport_broadcast.set_dest_id(0xFFFF)
        

    def configure_node(self, jumbo_frame_support=False):
        """Get remaining information from the node and set remaining parameters."""
        
        self.transport.ping(self)
        self.transport.test_payload_size(self, jumbo_frame_support)        

        resp = self.get_info()
        try:
            self.process_parameters(resp)
        except ex.ParameterError as err:
            print(err)
            raise ex.NodeError(self, "Configuration Error")

        # Set description
        self.description = self.__repr__()


    def set_name(self, name):
        """Set the name of the node.
        
        The name provided will affect the Python environment only 
        (ie it will update strings in child classes but will not be 
        transmitted to the node.)
            
        Args:
            name (str):  User provided name of the node.        
        """
        self.name        = name
        self.description = self.__repr__()



    # -------------------------------------------------------------------------
    # Commands for the Node
    # -------------------------------------------------------------------------
    def identify(self):
        """Identify the node
        
        The node will physically identify itself by:
        
          * Blinking the Hex Display (for approx 10 seconds)
          * Output Node ID and IP adress to UART output
        """
        self.send_cmd(cmds.NodeIdentify(self.serial_number))

    def ping(self):
        """'Ping' the node 
        
        Send an empty packet to the node via the transport to test connectivity
        between the host and the node.  This is the simplest command that can
        be processed by the node and is similar to the "ping" command used
        check network connectivity.
        """
        self.transport.ping(self, output=True)

    def get_type(self):
        """Get the type of the node."""
        if self.node_type is None:
            return self.send_cmd(cmds.NodeGetType())
        else:
            return self.node_type

    def get_info(self):
        """Get the Hardware Information from the node."""
        return self.send_cmd(cmds.NodeGetHwInfo())

    def get_temp(self):
        """Get the temperature of the node."""
        (curr_temp, _, _) = self.send_cmd(cmds.NodeGetTemperature()) # Min / Max temp not used
        return curr_temp

    def setup_network_inf(self):
        """Setup the transport network information for the node."""
        self.send_cmd_broadcast(cmds.NodeSetupNetwork(self))
        
    def reset_network_inf(self):
        """Reset the transport network information for the node."""
        self.send_cmd_broadcast(cmds.NodeResetNetwork(self.serial_number))

    # -------------------------------------------------------------------------
    # Parameter Framework
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
        if   (group == cmds.GROUP_NODE):
            self.process_parameter(identifier, length, values)
        elif (group == cmds.GROUP_TRANSPORT):
            self.transport.process_parameter(identifier, length, values)
        else:
            raise ex.ParameterError("Group", "Unknown Group: {}".format(group))


    def process_parameter(self, identifier, length, values):
        """Extract values from the parameters"""
        if   (identifier == NODE_TYPE):
            if (length == 1):
                self._set_node_type(values[0])
            else:
                raise ex.ParameterError("NODE_TYPE", "Incorrect length")

        elif (identifier == NODE_ID):
            if (length == 1):
                self.node_id = values[0]
            else:
                raise ex.ParameterError("NODE_ID", "Incorrect length")

        elif (identifier == NODE_HW_GEN):
            if (length == 1):
                self.hw_ver = (values[0] & 0xFF)
            else:
                raise ex.ParameterError("NODE_HW_GEN", "Incorrect length")

        elif (identifier == NODE_SERIAL_NUM):
            if (length == 1):
                from . import util

                (sn, sn_str) = util.get_serial_number(values[0], output=False)
                self.serial_number = sn
                self.sn_str        = sn_str
            else:
                raise ex.ParameterError("NODE_SERIAL_NUM", "Incorrect length")

        elif (identifier == NODE_FPGA_DNA):
            if (length == 2):
                self.fpga_dna = (2**32 * values[1]) + values[0]
            else:
                raise ex.ParameterError("NODE_FPGA_DNA", "Incorrect length")

        else:
            raise ex.ParameterError(identifier, "Unknown node parameter")


    # -------------------------------------------------------------------------
    # Transmit / Receive methods for the Node
    # -------------------------------------------------------------------------
    def send_cmd(self, cmd, max_attempts=2, max_req_size=None, timeout=None):
        """Send the provided command.
        
        Args:
            cmd          -- Class of command to send
            max_attempts -- Maximum number of attempts to send a given command
            max_req_size -- Maximum request size (applys only to Buffer Commands)
            timeout      -- Maximum time to wait for a response from the node
        """
        from . import transport

        resp_type = cmd.get_resp_type()
        
        if  (resp_type == transport.TRANSPORT_NO_RESP):
            payload = cmd.serialize()
            self.transport.send(payload, robust=False)

        elif (resp_type == transport.TRANSPORT_RESP):
            resp = self._receive_resp(cmd, max_attempts, timeout)
            return cmd.process_resp(resp)

        elif (resp_type == transport.TRANSPORT_BUFFER):
            resp = self._receive_buffer(cmd, max_attempts, max_req_size, timeout)
            return cmd.process_resp(resp)

        else:
            raise ex.TransportError(self.transport, 
                                    "Unknown response type for command")


    def _receive_resp(self, cmd, max_attempts, timeout):
        """Internal method to receive a response for a given command payload"""
        from . import message

        reply = b''
        done = False
        resp = message.Resp()

        payload = cmd.serialize()
        self.transport.send(payload)

        while not done:
            try:
                reply = self.transport.receive(timeout)
                self._receive_success()
            except ex.TransportError:
                self._receive_failure()

                if self._receive_failure_exceeded(max_attempts):
                    raise ex.TransportError(self.transport, 
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
        from . import message

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

        # To not hurt the performance of the transport, do not request more 
        # data than can fit in the RX buffer
        if (fragment_size > self.transport.rx_buffer_size):
            fragment_size = self.transport.rx_buffer_size
        
        # Allocate a complete response buffer        
        resp = message.Buffer(buffer_id, flags, start_byte, total_size)

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
                    # fragment.  Only return the truncated buffer.
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
                except ex.TransportError:
                    self._receive_failure()
                    if print_warnings:
                        print("WARNING:  Transport timeout.  Requesting missing data.")
                    
                    # If there is a timeout, then request missing part of the buffer
                    if self._receive_failure_exceeded(max_attempts):
                        if print_warnings:
                            print("ERROR:  Max re-transmissions without reply from node.")
                        raise ex.TransportError(self.transport, 
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
                        
                        # Use the standard send to get a Buffer with missing data. 
                        # This avoids any race conditions when requesting 
                        # multiple missing locations.  Make sure that max_attempts
                        # are set to 1 for the re-request to not get in to an 
                        # infinite loop
                        try:
                            location_resp = self.send_cmd(cmd, max_attempts=max_attempts)
                            self._receive_success()
                        except ex.TransportError:
                            # Timed out on a re-request.  There is an error so
                            # just clean up the response and get out of the loop.
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
        
    
    def send_cmd_broadcast(self, cmd, pkt_type=None):
        """Send the provided command over the broadcast transport.

        Currently, broadcast commands cannot have a response.
        
        Args:
            cmd -- Class of command to send
        """
        self.transport_broadcast.send(payload=cmd.serialize(), pkt_type=pkt_type)


    def receive_resp(self, timeout=None):
        """Return a list of responses that are sitting in the host's 
        receive queue.  It will empty the queue and return them all the 
        calling method."""
        from . import message

        output = []
        
        resp = self.transport.receive(timeout)
        
        if resp:
            # Create a list of response object if the list of bytes is a 
            # concatenation of many responses
            done = False
            
            while not done:
                msg = message.Resp()
                msg.deserialize(resp)
                resp_len = msg.sizeof()

                if resp_len < len(resp):
                    resp = resp[(resp_len):]
                else:
                    done = True
                    
                output.append(msg)
        
        return output



    # -------------------------------------------------------------------------
    # Transport Tracker
    # -------------------------------------------------------------------------
    def _receive_success(self):
        """Internal method - Successfully received a packet."""
        self.transport_tracker = 0

    
    def _receive_failure(self):
        """Internal method - Had a receive failure."""
        self.transport_tracker += 1


    def _receive_failure_exceeded(self, max_attempts):
        """Internal method - More recieve failures than max_attempts."""
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


    def __str__(self):
        """Pretty print WarpNode object"""
        msg = ""

        if self.serial_number is not None:
            msg += "Node:\n"
            msg += "    Node ID       :  {0}\n".format(self.node_id)
            msg += "    Serial #      :  {0}\n".format(self.sn_str)
            msg += "    HW version    :  WARP v{0}\n".format(self.hw_ver)
        else:
            msg += "Node not initialized."

        if self.transport is not None:
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



class WarpNodeFactory(WarpNode):
    """Sub-class of WARP node used to help with node configuration and setup.
    
    This class will maintian the dictionary of Node Types.  The dictionary
    contains the 32-bit Node Type as a key and the corresponding class name 
    as a value.
    
    To add new Node Types, you can sub-class WarpNodeFactory and add your own 
    Node Types.
    
    Attributes:
        type_dict -- Dictionary of Node Types to class names
    """
    type_dict           = None


    def __init__(self, network_config=None):
        from . import defaults

        super(WarpNodeFactory, self).__init__(network_config)
 
        self.type_dict = {}

        # Add default classes to the factory
        self.node_add_class(defaults.DEFAULT_NODE_TYPE, 
                            defaults.DEFAULT_NODE_CLASS,
                            defaults.DEFAULT_NODE_DESCRIPTION)
        
    
    def setup(self, node_dict):
        self.set_init_configuration(serial_number=node_dict['serial_number'],
                                    node_id=node_dict['node_id'], 
                                    node_name=node_dict['node_name'], 
                                    ip_address=node_dict['ip_address'], 
                                    unicast_port=node_dict['unicast_port'], 
                                    broadcast_port=node_dict['broadcast_port'])


    def create_node(self, network_config=None, network_reset=True):
        """Based on the Node Type, dynamically create and return the correct node."""
        
        node = None

        # Initialize the node network interface
        if network_reset:
            # Send broadcast command to reset the node network interface
            self.reset_network_inf()
    
            # Send broadcast command to initialize the node network interface
            self.setup_network_inf()

        try:
            # Send unicast command to get the node type
            node_type = self.get_type()
            
            # Get the node class from the Factory dictionary
            node_class = self.node_get_class(node_type)
        
            if node_class is not None:
                node = self.node_eval_class(node_class, network_config)
                node.set_init_configuration(serial_number=self.serial_number,
                                            node_id=self.node_id,
                                            node_name=self.name,
                                            ip_address=self.transport.ip_address,
                                            unicast_port=self.transport.unicast_port,
                                            broadcast_port=self.transport.broadcast_port)

                msg  = "Initializing {0}".format(node.sn_str)
                if node.name is not None:
                    msg += " as {0}".format(node.name)
                print(msg)
            else:
                self.print_node_types()
                msg  = "ERROR:  Node {0}\n".format(self.sn_str)
                msg += "    Unknown node type: 0x{0:8x}\n".format(node_type)
                print(msg)

        except ex.TransportError as err:
            msg  = "ERROR:  Node {0}\n".format(self.sn_str)
            msg += "    Node is not responding.  Please ensure that the \n"
            msg += "    node is powered on and is properly configured.\n"
            print(msg)
            print(err)

        return node
        

    def node_eval_class(self, node_class, network_config):
        """Evaluate the node_class string to create a node.  
        
        This method should be overridden in each sub-class with the same
        overall structure but a different import.  Please call the super
        class function instead of returning an error so that the calls 
        will propagate to catch all node types.
        
        The network_config is used as part of the node_class string to 
        initialize the node.
        """
        # IDEs might think that the following imports are not necessary.
        # However, they are required for the eval() that is performed for 
        # the node class.  This will be similar for all sub-classes of
        # WarpNodeFactory.
        from . import defaults
        from . import node
        
        tmp_node = None

        try:
            tmp_node = eval(node_class, globals(), locals())
        except:
            # Catch all errors that might occur when trying to create the node.
            # Since this is the parent class of all nodes, raise a ConfigError 
            # if the node cannot be created here. 
            pass
        
        if tmp_node is None:
            self.print_node_types()
            msg = "Cannot create node of class: {0}".format(node_class)
            raise ex.ConfigError(msg)
        else:
            return tmp_node


    def node_add_class(self, node_type, class_name, description):
        if (node_type in self.type_dict):
            print("WARNING: Changing definition of {0}".format(node_type))
            
        self.type_dict[node_type] = {}
        self.type_dict[node_type]['class']        = class_name
        self.type_dict[node_type]['description']  = description
 

    def node_get_class(self, node_type):
        """Get the class string of the node from the type."""
        if (node_type in self.type_dict.keys()):
            return self.type_dict[node_type]['class']
        else:
            return None


    def node_get_description(self, node_type):
        """Get the description of the node from the type."""
        if (node_type in self.type_dict.keys()):
            return self.type_dict[node_type]['description']
        else:
            return None


    def get_type_dict(self):
        """Get the type dictonary."""
        return self.type_dict


    def print_node_types(self):
        msg = "Node Types: \n"
        for node_type in self.type_dict.keys():
            msg += "    0x{0:08x} = ".format(node_type)
            msg += "'{0}'\n".format(self.type_dict[node_type]) 
        print(msg)


# End Class

















