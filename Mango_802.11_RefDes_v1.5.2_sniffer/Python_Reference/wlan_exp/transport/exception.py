# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Exceptions
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides class definitions for exceptions.

Functions (see below for more information):
    ConfigError()
    ParameterError()
    NodeError()
    TransportError()

"""

__all__ = ['ConfigError', 'ParameterError', 'NodeError', 'TransportError']


# -----------------------------------------------------------------------------
# Exception base class
# -----------------------------------------------------------------------------

class Error(Exception):
    """Base class for exceptions."""
    pass

# End Class


# -----------------------------------------------------------------------------
# Types of exceptions
# -----------------------------------------------------------------------------

class ConfigError(Error):
    """Exception for configuration file errors.
    
    Attributes:
        message -- explanation message of the error
    """    
    def __init__(self, message):
        self.message = message

    def __str__(self):
        msg  = "Config Error:"
        msg += "    {0} \n".format(self.message)
        return msg
        
# End Class


class ParameterError(Error):
    """Exception for parameter errors.
    
    Attributes:
        name    -- name of the parameter
        message -- explanation message of the error
    """
    def __init__(self, parameter, message):
        self.parameter = parameter
        self.message = message

    def __str__(self):
        msg = ""
        if self.parameter is not None:
            msg += "Parameter {0} \n".format(self.parameter)
        msg += "    Error: {0} \n".format(self.message)
        return msg

# End Class


class NodeError(Error):
    """Exception for nodes.
    
    Attributes:
        node    -- node in which the error occurred.
        message -- explanation message of the error
    """
    def __init__(self, node, message):
        self.node = node
        self.message = message

    def __str__(self):
        msg = ""
        if self.node is not None:        
            msg += "{0} \n    ".format(self.node.description)
        msg += "Error: {0} \n".format(self.message)
        return msg

# End Class


class TransportError(Error):
    """Exception for transports.
    
    Attributes:
        transport -- transport in which the error occurred.
        message   -- explanation message of the error
    """
    
    def __init__(self, transport, message):
        self.transport = transport
        self.message = message

    def __str__(self):
        msg = ""
        if self.transport is not None:
            msg += "{0} \n".format(self.transport)
        msg += "    Error: {0} \n".format(self.message)
        return msg

# End Class

