# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
Mango 802.11 Reference Design Experiments Framework - Version Utils
------------------------------------------------------------------------------
Authors:   Chris Hunter (chunter [at] mangocomm.com)
           Patrick Murphy (murphpo [at] mangocomm.com)
           Erik Welsh (welsh [at] mangocomm.com)
License:   Copyright 2014-2016, Mango Communications. All rights reserved.
           Distributed under the WARP license (http://warpproject.org/license)
------------------------------------------------------------------------------

This module provides wlan_exp version information and commands.

Functions (see below for more information):
    wlan_exp_ver()       -- Returns wlan_exp version
    wlan_exp_ver_check() -- Checks the provided version against the wlan_exp version
    wlan_exp_ver_str()   -- Returns string of wlan_exp version

Integer constants:
    WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION, WLAN_EXP_XTRA, 
        WLAN_EXP_RELEASE -- wlan_exp verision constants

"""

import os
import inspect


__all__ = ['wlan_exp_ver', 'wlan_exp_ver_check', 'wlan_exp_ver_str']


# Version defines
WLAN_EXP_MAJOR          = 1
WLAN_EXP_MINOR          = 5
WLAN_EXP_REVISION       = 2
WLAN_EXP_XTRA           = str('')
WLAN_EXP_RELEASE        = True


# Version string
version  = "wlan_exp v"
version += "{0:d}.".format(WLAN_EXP_MAJOR)
version += "{0:d}.".format(WLAN_EXP_MINOR)
version += "{0:d} ".format(WLAN_EXP_REVISION)
version += "{0:s} ".format(WLAN_EXP_XTRA)


# Version number
version_number  = "{0:d}.".format(WLAN_EXP_MAJOR)
version_number += "{0:d}.".format(WLAN_EXP_MINOR)
version_number += "{0:d} ".format(WLAN_EXP_REVISION)


# Status defines for wlan_ver_check
WLAN_EXP_VERSION_SAME   = 0
WLAN_EXP_VERSION_NEWER  = 1
WLAN_EXP_VERSION_OLDER  = -1


#-----------------------------------------------------------------------------
# Version Exception
#-----------------------------------------------------------------------------

class VersionError(Exception):
    """Exception for version errors.
    
    Attributes:
        message -- explanation message of the error
    """
    def __init__(self, message):
        self.message = message

    def __str__(self):
        msg  = "Version Error:"
        msg += "    {0} \n".format(self.message)
        return msg
        
# End Class


#-----------------------------------------------------------------------------
# Version Utilities
#-----------------------------------------------------------------------------
def wlan_exp_ver():
    """Returns the version of WlanExp for this package."""    
    # Print the release message if this is not an official release    
    if not WLAN_EXP_RELEASE: 
        print("-" * 60)
        print("You are running a version of wlan_exp that may not be ")
        print("compatible with released wlan_exp bitstreams. Please use ")
        print("at your own risk.")
        print("-" * 60)
    
    return (WLAN_EXP_MAJOR, WLAN_EXP_MINOR, WLAN_EXP_REVISION)
    
# End of wlan_exp_ver()


def wlan_exp_ver_check(ver_str=None, major=None, minor=None, revision=None,
                       caller_desc=None):
    """Checks the version of wlan_exp for this package.
    
    This function will print a warning message if the version specified 
    is older than the current version and will raise a VersionError 
    if the version specified is newer than the current version.
    
    Args:
        ver_str  -- Version string returned by wlan_exp_ver_str()
        major    -- Major release number for wlan_exp
        minor    -- Minor release number for wlan_exp
        revision -- Revision release number for wlan_exp
        
    The ver_str attribute takes precedence over the major, minor, revsion
    attributes.
    """
    status    = WLAN_EXP_VERSION_SAME
    print_msg = False
    raise_ex  = False
    
    if not ver_str is None:
        try:
            temp = ver_str.split(" ")
            (major, minor, revision) = temp[0].split(".")
        except AttributeError:
            msg  = "ERROR: input parameter ver_str not valid"
            raise AttributeError(msg)

    # If ver_str was not specified, then major, minor, revision should be defined
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
        msg  = "wlan_exp Version Mismatch: \n"
        msg += "    Caller is using wlan_exp package version: {0}\n".format(wlan_exp_ver_str(major, minor, revision))
    else:
        msg  = "wlan_exp Version Mismatch: \n"
        msg += "    " + str(caller_desc)
        
    msg += "    Current wlan_exp package version: {0}".format(wlan_exp_ver_str())

    # Given there might be changes that break things, always raise an 
    # exception on version mismatch
    if (major == WLAN_EXP_MAJOR):
        if (minor == WLAN_EXP_MINOR):
            if (revision != WLAN_EXP_REVISION):
                if (revision < WLAN_EXP_REVISION):
                    msg      += " (newer)\n"
                    status    = WLAN_EXP_VERSION_NEWER
                    raise_ex  = True
                else:
                    msg      += " (older)\n"
                    status    = WLAN_EXP_VERSION_OLDER
                    raise_ex  = True
        else:
            if (minor < WLAN_EXP_MINOR):
                msg      += " (newer)\n"
                status    = WLAN_EXP_VERSION_NEWER
                raise_ex  = True
            else:
                msg      += " (older)\n"
                status    = WLAN_EXP_VERSION_OLDER
                raise_ex  = True
    else:
        if (major < WLAN_EXP_MAJOR):
            msg      += " (newer)\n"
            status    = WLAN_EXP_VERSION_NEWER
            raise_ex  = True
        else:
            msg      += " (older)\n"
            status    = WLAN_EXP_VERSION_OLDER
            raise_ex  = True

    msg += "        ({0})\n".format(__file__)

    if print_msg:
        print(msg)

    if raise_ex:
        raise VersionError(msg)
    
    return status
    
# End def


def print_wlan_exp_ver():
    """Print the wlan_exp Version."""
    print("wlan_exp v" + wlan_exp_ver_str() + "\n")
    print("Framework Location:")
    print(os.path.dirname(
              os.path.abspath(inspect.getfile(inspect.currentframe()))))

# End def


def wlan_exp_ver_str(major=WLAN_EXP_MAJOR, minor=WLAN_EXP_MINOR, 
                     revision=WLAN_EXP_REVISION, xtra=WLAN_EXP_XTRA):
    """Return a string of the wlan_exp version.
    
    This will raise a VersionError if the arguments are not integers.    
    """
    try:
        msg  = "{0:d}.".format(major)
        msg += "{0:d}.".format(minor)
        msg += "{0:d} ".format(revision)
        msg += "{0:s}".format(xtra)
    except ValueError:
        # Set output string to default values so program can continue
        error  = "WARNING:  Unknown Argument - All arguments should be integers\n"
        error += "    Setting wlan_exp version string to default."
        print(error)
        
        msg  = "{0:d}.".format(WLAN_EXP_MAJOR)
        msg += "{0:d}.".format(WLAN_EXP_MINOR)
        msg += "{0:d} ".format(WLAN_EXP_REVISION)
        msg += "{0:s}".format(WLAN_EXP_XTRA)
        
    return msg
    
# End def


def wlan_exp_ver_code_to_str(ver_code):
    """Convert four byte version code with format [x major minor rev] to a string."""
    ver = int(ver_code)
    return wlan_exp_ver_str(((ver >> 24) & 0xFF), ((ver >> 16) & 0xFF), ((ver >> 0) & 0xFF))

# End def



