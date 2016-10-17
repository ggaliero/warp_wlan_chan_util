# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Config
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

This module provides class definitions to manage the WARPNet configuration.

Functions (see below for more information):
    NetworkConfiguration() -- Specifies Network information for setup
    NodesConfiguration() -- Specifies Node information for setup

"""

import os

try:                 # Python 3
    import configparser
except ImportError:  # Python 2
    import ConfigParser as configparser


from . import defaults as wn_defaults
from . import util as wn_util
from . import exception as wn_ex


__all__ = ['NetworkConfiguration', 'NodesConfiguration']



class NetworkConfiguration(object):
    """Class for WARPNet Network configuration.
    
    This class contains a WARPNet network configuration using default values 
    in wn_defaults combined with parameters that are passed in.
    
    Config Structure:
        { 'network'              str,
          'host_id'              int,
          'unicast_port'         int,
          'bcast_port'           int,
          'tx_buf_size'          int,
          'rx_buf_size'          int,
          'transport_type'       str,
          'jumbo_frame_support'  bool }
    
    """
    config              = None
    
    def __init__(self, network=None, host_id=None, unicast_port=None,
                 bcast_port=None, tx_buffer_size=None, rx_buffer_size=None,
                 transport_type=None, jumbo_frame_support=None, quiet=False):
        """Initialize a NetworkConfiguration
        
        Attributes:
            network             -- Network interface
            host_id             -- Host ID
            unicast_port        -- Port for unicast traffic
            bcast_port          -- Port for broadcast traffic
            tx_buf_size         -- TX buffer size
            rx_buf_size         -- RX buffer size
            transport_type      -- Transport type
            jumbo_frame_support -- Support for Jumbo Ethernet frames
            
        The network configuration assumes a netmask of 255.255.255.0 for 
        all networks.
        
        """
        # Set initial values
        my_network             = wn_defaults.NETWORK
        my_host_id             = wn_defaults.HOST_ID
        my_unicast_port        = wn_defaults.UNICAST_PORT
        my_bcast_port          = wn_defaults.BCAST_PORT
        my_transport_type      = wn_defaults.TRANSPORT_TYPE
        my_jumbo_frame_support = wn_defaults.JUMBO_FRAME_SUPPORT

        (my_tx_buffer_size, 
         my_rx_buffer_size)    = wn_util._get_os_socket_buffer_size(
                                                 wn_defaults.TX_BUFFER_SIZE, 
                                                 wn_defaults.RX_BUFFER_SIZE)

        # Process input arguments        
        if host_id is not None:
            if (wn_util._check_host_id(host_id)):
                my_host_id = host_id            
                
        if unicast_port   is not None:  my_unicast_port   = unicast_port
        if bcast_port     is not None:  my_bcast_port     = bcast_port
        if tx_buffer_size is not None:  my_tx_buffer_size = tx_buffer_size
        if rx_buffer_size is not None:  my_rx_buffer_size = rx_buffer_size
        if transport_type is not None:  my_transport_type = transport_type

        if jumbo_frame_support is not None:
            if (type(jumbo_frame_support) is bool):
                my_jumbo_frame_support = jumbo_frame_support
            else:
                msg  = "Jumbo Frame Support must be a boolean.  Using {0}.".format(my_jumbo_frame_support) 
                print(msg)

        # Check host_interfaces last b/c you need the resolved bcast_port
        if network is not None:
            my_network = wn_util._check_network_interface(network, quiet)

        self.set_config(my_network, my_host_id, my_unicast_port, 
                        my_bcast_port, my_tx_buffer_size, my_rx_buffer_size, 
                        my_transport_type, my_jumbo_frame_support)


    def set_config(self, network, host_id, unicast_port, bcast_port,
                   tx_buffer_size, rx_buffer_size, transport_type, 
                   jumbo_frame_support):
        """Sets the config based on the parameters."""
        self.config = {}
        
        self.config['network']             = network
        self.config['bcast_address']       = wn_util._get_bcast_address(network)
        self.config['host_id']             = host_id
        self.config['unicast_port']        = unicast_port
        self.config['bcast_port']          = bcast_port
        self.config['tx_buffer_size']      = tx_buffer_size
        self.config['rx_buffer_size']      = rx_buffer_size
        self.config['transport_type']      = transport_type
        self.config['jumbo_frame_support'] = jumbo_frame_support


    def get_param(self, parameter):
        """Returns the value of the parameter within the section."""
        if (parameter in self.config.keys()):
            return self.config[parameter]
        else:
            print("Parameter {0} does not exist.".format(parameter))
        
        return None


    def set_param(self, parameter, value):
        """Sets the parameter to the given value."""
        if (parameter in self.config.keys()):
            self.config[parameter] = value
        else:
            print("Parameter {0} does not exist.".format(parameter))
 

    def __str__(self):
        msg = ""
        if self.config is not None:
            msg += "Network Configuration contains: \n"
            for parameter in self.config.keys():
                msg += "        {0:20s} = ".format(parameter)
                msg += "{0}\n".format(self.config[parameter])
        else:
            msg += "Network Configuration not intialized.\n"

        return msg            

# End Class



class NodesConfiguration(object):
    """Class for WARPNet Node Configuration.
    
    This class can load and store WARPNet Node configurations
    
    Attributes of a node:
        Node serial number
            node_id      -- Node ID
            node_name    -- Node Name
            ip_address   -- IP address of the Node
            unicast_port -- Unicast port of the Node
            bcast_port   -- Broadcast port of the Node
            use_node     -- Is this node part of the network
    
    Any parameter can be overridden by including it in the INI file.  
    
    When the configuration is read in, both a config and a shadow_config
    are created.  The config has values that will be written to the INI
    file, while the shadow_config has all values populated.  If values
    are not specified in the INI, they will get auto-populated defaults:
    
        node_id      - Monotonic counter starting at 0
        node_name    - None (unless specified by the user)
        ip_address   - wn_config.ini get_param('network', 'host_address') for 
                       the first three octets and "node_id + 1" for the last 
                       octet
        unicast_port - wn_config.ini get_param('network', 'unicast_port')
        bcast_port   - wn_config.ini get_param('network', 'bcast_port')
        use_node     - "True"

    NOTE:  In order to be as consistent as possible, all nodes in the 
    configuration file get a node id regardless of whether they are used.
    Also, we do not sort the INI in our testing, it seems like the nodes are
    initialized in the order they appear in the config but are not guarenteed.

    """
    network_config      = None
    config              = None
    config_file         = None
    node_id_counter     = None
    used_node_ids       = None
    used_node_ips       = None
    shadow_config       = None

    def __init__(self, ini_file=None, serial_numbers=None, network_config=None):
        """Initialize a WnNodesConfiguration
        
        Attributes:
            ini_file -- An INI file name that specified a nodes configuration
            serial_numbers -- A list of serial numbers of WARPv3 nodes
            network_config -- A NetworkConfiguration
        
        There are multiple ways to initialize a NodesConfiguration:
          1) List of serial_numbers
          2) INI file name
        
        For a list of serial numbers, the config will use all of the auto-
        population aspects to create a node for each serial number.  Serial
        numbers must be of the form:  "W3-a-XXXXX" where XXXXX is the 5 
        digit, zero-padded serial number printed on the WARPv3 board.
        
        For an INI file, you can create an INI file using the wn_nodes_setup()
        method in warpnet.wn_util.  In the INI file, you can specify as little
        as the serial numbers, up to a fully explicit configuration.
        
        If neither an ini_file nor serial_numbers are provided, the method
        will check for the NODES_CONFIG_INI_FILE specified in 
        warpnet.wn_defaults
        
        If both serial_numbers and an ini_file are provided, the ini_file 
        will be ignored.
        
        If not provided, the class will use the default WnHostConfiguration
        specified in wn_defaults.  Since there can be multiple host 
        interfaces as part of the WnHostConfiguration, the WnNodesConfiguration
        will only auto-populate IP address on the first host interface, ie
        host interface 0.  Therefore, if only serial_numbers are provided
        then all of those nodes will receive IP addresses on host interface 0.
        
        """
        self.node_id_counter = 0
        self.shadow_config   = {}
        self.used_node_ids   = []
        self.used_node_ips   = []

        # Initialize the Shadow Config from Host data
        if network_config is None:
            network_config = NetworkConfiguration()
        
        network_addr = network_config.get_param('network')
        u_port       = network_config.get_param('unicast_port')
        b_port       = network_config.get_param('bcast_port')

        # Compute the base IP address to auto-assign IP addresses 
        base_ip_address = wn_util._get_ip_address_subnet(network_addr) + ".{0}"

        # Initialize the shadow config
        self.init_shadow_config(base_ip_address, u_port, b_port)

        # Initialize the config based on rules documented above.
        #   NOTE:  This can raise exceptions if there are issues.
        if serial_numbers is None:            
            if ini_file is None:
                ini_file = wn_defaults.NODES_CONFIG_INI_FILE
                
            self.load_config(ini_file)

        else:
            self.set_default_config()
            
            for sn in serial_numbers:
                try:
                    self.add_node(sn)
                except TypeError as err:
                    print(err)



    # -------------------------------------------------------------------------
    # Methods for Config
    # -------------------------------------------------------------------------
    def set_default_config(self):
        """Set the default config."""
        self.config = configparser.ConfigParser()
        self.init_used_node_lists()


    def add_node(self, serial_number, ip_address=None, 
                 node_id=None, unicast_port=None, bcast_port=None, 
                 node_name=None, use_node=None):
        """Add a node to the NodesConfig structure.
        
        Only serial_number and ip_address are required in the ini file.  Other
        fields will not be populated in the ini file unless they require a 
        non-default value.  
        """
        (_, sn_str) = wn_util.wn_get_serial_number(serial_number)
        
        if (sn_str in self.config.sections()):
            print("Node {0} exists.  Please use set_param to modify the node.".format(sn_str))
        else:
            self.config.add_section(sn_str)

            # Populate optional parameters
            if ip_address   is not None: self.config.set(sn_str, 'ip_address', ip_address)
            if node_id      is not None: self.config.set(sn_str, 'node_id', node_id)
            if unicast_port is not None: self.config.set(sn_str, 'unicast_port', unicast_port)
            if bcast_port   is not None: self.config.set(sn_str, 'bcast_port', bcast_port)
            if node_name    is not None: self.config.set(sn_str, 'node_name', node_name)
            if use_node     is not None: self.config.set(sn_str, 'use_node', use_node)

        # Add node to shadow_config
        self.add_shadow_node(sn_str)


    def remove_node(self, serial_number):
        """Remove a node from the NodesConfig structure."""
        (_, sn_str) = wn_util.wn_get_serial_number(serial_number)

        if (not self.config.remove_section(sn_str)):
            print("Node {0} not in nodes configuration.".format(sn_str))
        else:
            self.remove_shadow_node(sn_str)
        

    def get_param(self, section, parameter):
        """Returns the value of the parameter within the config for the node."""        
        (_, sn_str) = wn_util.wn_get_serial_number(section)

        return self.get_param_helper(sn_str, parameter)


    def get_param_helper(self, section, parameter):
        """Returns the value of the parameter within the config section."""
        if (section in self.config.sections()):
            if (parameter in self.config.options(section)):
                return self._get_param_hack(section, parameter)
            else:
                return self._get_shadow_param(section, parameter)
        else:
            print("Node '{}' does not exist.".format(section))
        
        return ""


    def set_param(self, section, parameter, value):
        """Sets the parameter to the given value."""
        (_, sn_str) = wn_util.wn_get_serial_number(section)
        
        if (sn_str in self.config.sections()):
            if (parameter in self.config.options(sn_str)):
                self._set_param_hack(sn_str, parameter, value)
                self.update_shadow_config(sn_str, parameter, value)
            else:
                if (parameter in self.shadow_config['default'].keys()):
                    self._set_param_hack(sn_str, parameter, value)
                    self.update_shadow_config(sn_str, parameter, value)
                else:
                    print("Parameter {} does not exist in node '{}'.".format(parameter, sn_str))
        else:
            print("Section '{}' does not exist.".format(sn_str))


    def remove_param(self, section, parameter):
        """Removes the parameter from the config."""
        (_, sn_str) = wn_util.wn_get_serial_number(section)
        
        if (sn_str in self.config.sections()):
            if (parameter in self.config.options(sn_str)):
                self.config.remove_option(sn_str, parameter)
                
                # Re-populate the shadow_config
                self.remove_shadow_node(sn_str)
                self.add_shadow_node(sn_str)
            else:
                # Fail silently so there are no issues when a user tries to 
                #    remove a shadow_config parameter
                pass
        else:
            print("Section '{}' does not exist.".format(sn_str))


    def get_nodes_dict(self):
        """Returns a list of dictionaries that contain the parameters of each
        WnNode specified in the config."""
        output = []

        if not self.config.sections():        
            raise wn_ex.ConfigError("No Nodes in {0}".format(self.config_file))
        
        for node_config in self.config.sections():
            if (self.get_param_helper(node_config, 'use_node')):
                add_node = True

                try:
                    (sn, _) = wn_util.wn_get_serial_number(node_config)
                except TypeError as err:
                    print(err)
                    add_node = False

                if add_node:
                    node_dict = {
                        'serial_number': sn,
                        'node_id'      : self.get_param_helper(node_config, 'node_id'),
                        'node_name'    : self.get_param_helper(node_config, 'node_name'),
                        'ip_address'   : self.get_param_helper(node_config, 'ip_address'),
                        'unicast_port' : self.get_param_helper(node_config, 'unicast_port'),
                        'bcast_port'   : self.get_param_helper(node_config, 'bcast_port') }
                    output.append(node_dict)
        
        return output


    def load_config(self, config_file):
        """Loads the WARPNet config from the provided file."""
        self.config_file = os.path.normpath(config_file)
        
        # TODO: allow relative paths
        self.clear_shadow_config()
        self.config = configparser.ConfigParser()
        dataset = self.config.read(self.config_file)

        if len(dataset) != 1:
            msg = str("Error reading config file:\n" + self.config_file)
            raise wn_ex.ConfigError(msg)
        else:
            self.init_used_node_lists()
            self.load_shadow_config()


    def save_config(self, config_file=None, output=False):
        """Saves the WARPNet config to the provided file."""
        if config_file is not None:
            self.config_file = os.path.normpath(config_file)
        else:
            self.config_file = wn_defaults.NODES_CONFIG_INI_FILE
        
        if output:
            print("Saving config to: \n    {0}".format(self.config_file))

        try:
            with open(self.config_file, 'w') as configfile:
                self.config.write(configfile)
        except IOError as err:
            print("Error writing config file: {0}".format(err))


    # -------------------------------------------------------------------------
    # Methods for Shadow Config
    # -------------------------------------------------------------------------
    def init_shadow_config(self, ip_addr_base, unicast_port, bcast_port):
        """Initialize the 'default' section of the shadow_config."""
        self.shadow_config['default'] = {'node_id'     : 'auto',
                                         'node_name'   : None,
                                         'ip_address'  : ip_addr_base,
                                         'unicast_port': unicast_port,
                                         'bcast_port'  : bcast_port, 
                                         'use_node'    : True}

    def init_used_node_lists(self):
        """Initialize the lists used to keep track of fields that must 
        be unique.
        """
        self.used_node_ids = []
        self.used_node_ips = wn_util._get_all_host_ip_addrs()

        if self.config is not None:
            for section in self.config.sections():
                if ('node_id' in self.config.options(section)):
                    self.used_node_ids.append(self._get_param_hack(section, 'node_id'))
    
                if ('ip_address' in self.config.options(section)):
                    self.used_node_ips.append(self._get_param_hack(section, 'ip_address'))        


    def clear_shadow_config(self):
        """Clear everything in the shadow config except 'default' section."""
        for section in self.shadow_config.keys():
            if (section != 'default'):
                del self.shadow_config[section]
        
        self.init_used_node_lists()


    def load_shadow_config(self):
        """For each node in the config, populate the shadow_config."""        
        sections = self.config.sections()

        # Sort the config by serial number so there is consistent numbering
        # sections.sort()
        
        # Mirror any fields in the config and populate any missing fields 
        # with default values
        for section in sections:
            my_node_id      = self._get_node_id(section)
            my_node_name    = self._get_node_name(section, my_node_id)
            my_ip_address   = self._get_ip_address(section, my_node_id)
            my_unicast_port = self._get_unicast_port(section)
            my_bcast_port   = self._get_bcast_port(section)
            my_use_node     = self._get_use_node(section)
            
            # Set the node in the shadow_config
            self.set_shadow_node(section, my_ip_address, my_node_id, 
                                 my_unicast_port, my_bcast_port, 
                                 my_node_name, my_use_node)

        # TODO: Sanity check to make sure there a no duplicate Node IDs or IP Addresses


    def update_shadow_config(self, section, parameter, value):
        """Update the shadow_config with the given value."""
        self.shadow_config[section][parameter] = value


    def add_shadow_node(self, serial_number):
        """Add the given node to the shadow_config."""
        my_node_id      = self._get_node_id(serial_number)
        my_node_name    = self._get_node_name(serial_number, my_node_id)
        my_ip_address   = self._get_ip_address(serial_number, my_node_id)
        my_unicast_port = self._get_unicast_port(serial_number)
        my_bcast_port   = self._get_bcast_port(serial_number)
        my_use_node     = self._get_use_node(serial_number)

        # Set the node in the shadow_config
        self.set_shadow_node(serial_number, my_ip_address, my_node_id, 
                             my_unicast_port, my_bcast_port, 
                             my_node_name, my_use_node)

        

    def set_shadow_node(self, serial_number, ip_address, node_id, 
                        unicast_port, bcast_port, node_name, use_node):
        """Set the given node in the shadow_config."""
        self.shadow_config[serial_number] = {
            'node_id'      : node_id,
            'node_name'    : node_name,
            'ip_address'   : ip_address,
            'unicast_port' : unicast_port,
            'bcast_port'   : bcast_port,
            'use_node'     : use_node}

        self.used_node_ids.append(node_id)
        self.used_node_ips.append(ip_address)


    def remove_shadow_node(self, serial_number):
        """Remove the given node from the shadow_config."""
        self.used_node_ids.remove(self._get_shadow_param(serial_number, 'node_id'))
        self.used_node_ips.remove(self._get_shadow_param(serial_number, 'ip_address'))
        del self.shadow_config[serial_number]


    # -------------------------------------------------------------------------
    # Internal Methods
    # -------------------------------------------------------------------------
    def _get_next_node_id(self):
        next_node_id = self.node_id_counter
        
        while (next_node_id in self.used_node_ids):
            self.node_id_counter += 1
            next_node_id = self.node_id_counter

        return next_node_id
    
    
    def _get_next_node_ip(self, node_id):
        ip_addr_base = self.shadow_config['default']['ip_address']
 
        last_octet   = self._inc_node_ip(node_id)
        next_ip_addr = ip_addr_base.format(last_octet)

        while (next_ip_addr in self.used_node_ips):
            last_octet   = self._inc_node_ip(last_octet)
            next_ip_addr = ip_addr_base.format(last_octet)

        return next_ip_addr


    def _inc_node_ip(self, node_ip):
        my_node_ip = node_ip + 1

        if (my_node_ip > 254):
            my_node_ip = 1
        
        return my_node_ip


    def _get_node_id(self, section):
        if ('node_id' in self.config.options(section)):
            return self._get_param_hack(section, 'node_id')
        else:
            return self._get_next_node_id()
            

    def _get_node_name(self, section, node_id):
        if ('node_name' in self.config.options(section)):
            return self._get_param_hack(section, 'node_name')
        else:
            # We are not going to give the node a name unless explicitly told to do so
            #   return "Node {0}".format(node_id)
            return None


    def _get_ip_address(self, section, node_id):
        if ('ip_address' in self.config.options(section)):
            return self._get_param_hack(section, 'ip_address')
        else:
            return self._get_next_node_ip(node_id)


    def _get_unicast_port(self, section):
        if ('unicast_port' in self.config.options(section)):
            return self._get_param_hack(section, 'unicast_port')
        else:
            return self.shadow_config['default']['unicast_port']


    def _get_bcast_port(self, section):
        if ('bcast_port' in self.config.options(section)):
            return self._get_param_hack(section, 'bcast_port')
        else:
            return self.shadow_config['default']['bcast_port']


    def _get_use_node(self, section):
        if ('use_node' in self.config.options(section)):
            return self._get_param_hack(section, 'use_node')
        else:
            return True


    def _get_shadow_param(self, section, parameter):
        """Internal method to get shadow parameters.
        
        NOTE:  This is where to implement any per node defaults.
        """
        if (parameter in self.shadow_config[section].keys()):
            return self.shadow_config[section][parameter]
        else:
            print("Parameter {} does not exist in node '{}'.".format(parameter, section))
            return ""


    def _get_param_hack(self, section, parameter):
        """Internal method to work around differences in Python 2 vs 3"""
        if ((parameter == 'ip_address') or (parameter == 'node_name')):
            return self.config.get(section, parameter)
        else:
            return eval(self.config.get(section, parameter))

 
    def _set_param_hack(self, section, parameter, value):
        """Internal method to work around differences in Python 2 vs 3"""
        my_value = str(value)
        self.config.set(section, parameter, my_value)


    # -------------------------------------------------------------------------
    # Printing / Debug Methods
    # -------------------------------------------------------------------------
    def print_shadow_config(self):
        for section in self.shadow_config.keys():
            print("{0}".format(section))
            for parameter in self.shadow_config[section].keys():
                print("    {0} = {1}".format(parameter, self.shadow_config[section][parameter]))
            print("")


    def print_config(self):
        for section in self.config.sections():
            print("{0}".format(section))
            for parameter in self.config.options(section):
                print("    {0} = {1}".format(parameter, self.config.get(section, parameter)))
            print("")


    def print_nodes(self):
        return_val = {}
        print("Current Nodes:")
        if (len(self.config.sections()) == 0):
            print("    None")
        sections = self.config.sections()
        # sections.sort()
        for idx, val in enumerate(sections):
            node_id = self.get_param_helper(val, 'node_id')
            ip_addr = self.get_param_helper(val, 'ip_address')
            use_node = self.get_param_helper(val, 'use_node')
            msg = "    [{0}] {1} - Node {2:3d} at {3:10s}".format(idx, val, node_id, ip_addr)
            if (use_node):
                print(str(msg + "   active"))
            else:
                print(str(msg + " inactive"))                
            return_val[idx] = val
        return return_val


    def __str__(self):
        section_str = ""
        if self.config is not None:
            section_str += "contains parameters: \n"
            for section in self.config.sections():
                section_str = str(section_str + 
                                  "    Section '" + str(section) + "':\n" + 
                                  "        " + str(self.config.options(section)) + "\n")
        else:
            section_str += "not initialized. \n"
        
        if not self.config_file:
            return str("Default config " + section_str)
        else:
            return str(self.config_file + ": \n" + section_str)


# End Class
