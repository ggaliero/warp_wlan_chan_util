# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Version
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
1.00a ejw  03/18/14 Initial release

------------------------------------------------------------------------------

This module provides WARPNet version information and commands.

Functions (see below for more information):
    wn_ver()       -- Returns WARPNet version
    wn_ver_check() -- Checks the provided version against the WARPNet version
    wn_ver_str()   -- Returns string of WARPNet version

Integer constants:
    WN_MAJOR, WN_MINOR, WN_REVISION, WN_XTRA, WN_RELEASE
        -- WARPNet verision constants

"""

import os
import inspect

from . import exception as wn_ex


__all__ = ['wn_ver', 'wn_ver_check', 'wn_ver_str']


# WARPNet Version defines
WN_MAJOR                = 2
WN_MINOR                = 1
WN_REVISION             = 0
WN_XTRA                 = str('')
WN_RELEASE              = True


# Version string
version  = "WARPNet v"
version += "{0:d}.".format(WN_MAJOR)
version += "{0:d}.".format(WN_MINOR)
version += "{0:d} ".format(WN_REVISION)
version += "{0:s} ".format(WN_XTRA)


# Version number
version_number  = "{0:d}.".format(WN_MAJOR)
version_number += "{0:d}.".format(WN_MINOR)
version_number += "{0:d} ".format(WN_REVISION)


# Status defines for wn_ver_check
WN_VERSION_SAME   = 0
WN_VERSION_NEWER  = 1
WN_VERSION_OLDER  = -1


# -----------------------------------------------------------------------------
# WARPNet Version Utilities
# -----------------------------------------------------------------------------
def wn_ver():
    """Returns the version of WARPNet for this package.
    
    Attributes:
        do_print   -- Print output about the WARPNet version
    """
    # Print the release message if this is not an official release    
    if not WN_RELEASE: 
        print("-" * 60)
        print("You are running a version of WARPNet that may not be ")
        print("compatible with released WARPNet bitstreams. Please use ")
        print("at your own risk.")
        print("-" * 60)
        
    return (WN_MAJOR, WN_MINOR, WN_REVISION)
    
# End of wn_ver()


def wn_ver_check(ver_str=None, major=None, minor=None, revision=None,
                 caller_desc=None):
    """Checks the version of WARPNet for this package.
    
    This function will print a warning message if the version specified 
    is older than the current version and will raise an WnVersionError 
    if the version specified is newer than the current version.
    
    Attributes:
        ver_str  -- Version string returned by wn_ver_str()
        major    -- Major release number for WARPNet
        minor    -- Minor release number for WARPnet
        revision -- Revision release number for WARPNet
        
    The ver_str attribute takes precedence over the major, minor, revsion
    attributes.
    """
    status    = WN_VERSION_SAME
    print_msg = False
    raise_ex  = False
    
    if ver_str is not None:
        try:
            temp = ver_str.split(" ")
            (major, minor, revision) = temp[0].split(".")
        except AttributeError:
            msg  = "ERROR: input parameter ver_str not valid"
            raise AttributeError(msg)

    # If ver_str was specified, then major, minor, revision should be defined
    #   and contain strings.  Need to convert to integers.        
    try:
        major    = int(major)
        minor    = int(minor)
        revision = int(revision)
    except ValueError:
        msg  = "ERROR: input parameters major, minor, revision not valid"
        raise AttributeError(msg)
    
    # Check the provided version vs the current version
    if (caller_desc is None):
        msg  = "WARPNet Version Mismatch: \n"
        msg += "    Caller is using wlan_exp package version: {0}\n".format(wn_ver_str(major, minor, revision))
    else:
        msg  = "WARPNet Version Mismatch: \n"
        msg += "    " + str(caller_desc)
        
    msg += "    Current warpnet package version: {0}".format(wn_ver_str())

    if (major == WN_MAJOR):
        if (minor == WN_MINOR):
            if (revision != WN_REVISION):
                # Since MAJOR & MINOR versions match, only print a warning
                if (revision < WN_REVISION):
                    # Do nothing; Python must be the same or newer than C code
                    pass
                else:
                    msg      += "WARNING: " + msg + " (older)\n" 
                    status    = WN_VERSION_OLDER
                    print_msg = True
        else:
            if (minor < WN_MINOR):
                msg      += "WARNING: " + msg + " (newer)\n"
                status    = WN_VERSION_NEWER
                print_msg = True
            else:
                msg      += "ERROR: " + msg + " (older)\n" 
                status    = WN_VERSION_OLDER
                raise_ex  = True
    else:
        if (major < WN_MAJOR):
            msg      += "WARNING: " + msg + " (newer)\n"
            status    = WN_VERSION_NEWER
            print_msg = True
        else:
            msg      += "ERROR: " + msg + " (older)\n"
            status    = WN_VERSION_OLDER
            raise_ex  = True

    msg += "        ({0})\n".format(__file__)

    if print_msg:
        print(msg)

    if raise_ex:
        raise wn_ex.VersionError(msg)

    return status
    
# End of wn_ver_check()


def print_wn_ver():
    """Print the WARPNet Version."""
    print("WARPNet v" + wn_ver_str() + "\n\n")
    print("Framework Location:")
    print(os.path.dirname(
              os.path.abspath(inspect.getfile(inspect.currentframe()))))

# End of print_wn_ver()


def wn_ver_str(major=WN_MAJOR, minor=WN_MINOR, 
               revision=WN_REVISION, xtra=WN_XTRA):
    """Return a string of the WARPNet version.
    
    NOTE:  This will raise a VersionError if the arguments are not
    integers.    
    """
    try:
        msg  = "{0:d}.".format(major)
        msg += "{0:d}.".format(minor)
        msg += "{0:d} ".format(revision)
        msg += "{0:s}".format(xtra)
    except ValueError:
        # Set output string to default values so program can continue
        error  = "WARNING:  Unknown Argument - All arguments should be integers\n"
        error += "    Setting WARPNet version string to default."
        print(error)
        
        msg  = "{0:d}.".format(WN_MAJOR)
        msg += "{0:d}.".format(WN_MINOR)
        msg += "{0:d} ".format(WN_REVISION)
        msg += "{0:s}".format(WN_XTRA)
    
    return msg
    
# End of wn_ver_str()

