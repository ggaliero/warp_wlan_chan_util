# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Transport
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

This module provides the base class for WARPNet transports.

Integer constants:
    TRANSPORT_TYPE, TRANSPORT_HW_ADDR, TRANSPORT_IP_ADDR, 
      TRANSPORT_UNICAST_PORT, TRANSPORT_BCAST_PORT, TRANSPORT_GRP_ID
      -- Transport hardware parameter constants 

    TRANSPORT_NO_RESP, TRANSPORT_WN_RESP, TRANSPORT_WN_BUFFER
      -- Transport response types

If additional hardware parameters are needed for sub-classes of WnNode, please
make sure that the values of these hardware parameters are not reused.
"""


__all__ = ['Transport']


# WARPNet Command Group Names
# WARPNet Transport Parameter Identifiers
#   NOTE:  The C counterparts are found in *_transport.h
TRANSPORT_TYPE          = 0
TRANSPORT_HW_ADDR       = 1
TRANSPORT_IP_ADDR       = 2
TRANSPORT_UNICAST_PORT  = 3
TRANSPORT_BCAST_PORT    = 4
TRANSPORT_GRP_ID        = 5

# WARPNet Transport response types
TRANSPORT_NO_RESP       = 0
TRANSPORT_WN_RESP       = 1
TRANSPORT_WN_BUFFER     = 2

# WARPNet Transport Types
TRANSPORT_TYPE_UDP      = 0


class Transport(object):
    """Base class for WARPNet transports.

    Attributes:
        hdr -- Transport header object
    """
    hdr = None

# End Class




