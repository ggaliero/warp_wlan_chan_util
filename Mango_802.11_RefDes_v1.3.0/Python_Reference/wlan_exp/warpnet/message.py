# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WARPNet Messages
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

This module provides class definitions for all WARPNet over-the-wire 
messages.

Functions (see below for more information):
    TransportHeader() -- WARPNet Transport Header
    Cmd() -- Base class for WARPNet Commands the require WnResp responses
    BufferCmd() -- Base class for WARPNet Commands that require WnBuffer responses
    Resp() -- WARPNet responses (single packet)
    Buffer() -- WARPNet responses (multiple packets)

Integer constants:
    PKTTYPE_TRIGGER, PKTTYPE_HTON_MSG, PKTTYPE_NTOH_MSG, PKTTYPE_NTOH_MSG_ASYNC
      - Transport Header Packet Types

"""

import struct

from . import transport as wn_transport

__all__ = ['TransportHeader', 'Cmd', 'BufferCmd', 'Resp', 'Buffer']

# Transport Header defines
PKTTYPE_TRIGGER                        = 0
PKTTYPE_HTON_MSG                       = 1
PKTTYPE_NTOH_MSG                       = 2
PKTTYPE_NTOH_MSG_ASYNC                 = 3


# Buffer Command defines
CMD_BUFFER_GET_SIZE_FROM_DATA          = 0xFFFFFFFF



class Message(object):
    """Base class for WARPNet messages.
    
    Attributes:
        const   -- Dictionary of constants
    """
    const = None
    
    def __init__(self):
        self.const = dict()
    
    def add_const(self, name, value):
        """Add a constant to the Message."""
        if (name in self.const.keys()):
            print("WARNING:  Updating value of constant: {0}".format(name))
        self.const[name] = value

    def get_const(self, name):
        """Get a constant from the Message."""
        if (name in self.const.keys()):
            return self.const[name]
        else:
            msg  = "Constant {0} does not exist ".format(name)
            msg += "in {0}".format(self.__class__.__name__)
            raise AttributeError(msg)
    
    def serialize(self,): raise NotImplementedError
    def deserialize(self, data_buffer): raise NotImplementedError
    def sizeof(self,): raise NotImplementedError

# End Class


class TransportHeader(Message):
    """Class for WARPNet transport header.
    
    Attributes:
        dest_id  -- (uint16) Destination ID of the message
        src_id   -- (uint16) Source ID of the message
        reserved -- (uint8)  Reserved field in the header
        pkt_type -- (uint8)  Packet Type
        length   -- (uint16) Length of the payload in bytes
        seq_num  -- (uint16) Sequence number of the message
        flags    -- (uint16) Flags of the message
    """
    
    def __init__(self, dest_id=0, src_id=0, reserved=0, 
                 pkt_type=PKTTYPE_HTON_MSG, length=0, seq_num=0, flags=0):
        super(TransportHeader, self).__init__()
        self.dest_id = dest_id
        self.src_id = src_id
        self.reserved = reserved
        self.pkt_type = pkt_type
        self.length = length
        self.seq_num = seq_num
        self.flags = flags

    def serialize(self):
        """Return a bytes object of a packed transport header."""
        return struct.pack('!2H 2B 3H',
                           self.dest_id, self.src_id,
                           self.reserved, self.pkt_type, self.length, 
                           self.seq_num, self.flags)

    def deserialize(self, data_buffer):
        """Not used for Transport headers"""
        pass
    
    def sizeof(self):
        """Return the size of the transport header."""
        return struct.calcsize('!2H 2B 3H')
    
    def increment(self, step=1):
        """Increment the sequence number of the header by a given step."""
        self.seq_num = (self.seq_num + step) % 0xFFFF
        
    def set_type(self, pkt_type):
        """Sets the pkt_type field of the transport header.
        
        Attributes:
            pkt_type -- String of the packet type:
                "trigger" = PKTTYPE_TRIGGER
                "message" = PKTTYPE_HTON_MSG
        """
        pkt_type = pkt_type.lower()

        if   (pkt_type == 'trigger'):
            self.pkt_type = PKTTYPE_TRIGGER
        elif (pkt_type == 'message'):
            self.pkt_type = PKTTYPE_HTON_MSG
        else:
            print("Uknown packet type: {}".format(pkt_type))

    def set_length(self, value):    self.length = value        
    def set_src_id(self, value):    self.src_id = value
    def set_dest_id(self, value):   self.dest_id = value

    def get_length(self):            return self.length
    def get_src_id(self):            return self.src_id
    def get_dest_id(self):           return self.dest_id
        
    def response_required(self):
        """Sets bit 0 of the flags since a response is required."""
        self.flags = self.flags | 0x1
            
    def response_not_required(self):
        """Clears bit 0 of the flags since a response is not required."""
        self.flags = self.flags & 0xFFFE

    def reset(self):
        """Reset the sequence number of the transport header."""
        self.seq_num = 1
    
    def is_reply(self, input_data):
        """Checks input_data to see if it is a valid reply to the last
        outgoing packet.
        
        Checks:
            input_data.dest_id == self.src_id
            input_data.src_id  == self.dest_id
            input_data.seq_num == self.seq_num
            
        Raises a TypeError excpetion if input data is not the correct size.
        """
        if len(input_data) == self.sizeof():
            dataTuple = struct.unpack('!2H 2B 3H', input_data[0:12])
            
            if ((self.dest_id != dataTuple[1]) or
                    (self.src_id  != dataTuple[0]) or
                    (self.seq_num != dataTuple[5])):
                msg  = "WARNING:  transport header mismatch:"
                msg += "[{0:d} {1:d}]".format(self.dest_id, dataTuple[1])
                msg += "[{0:d} {1:d}]".format(self.src_id, dataTuple[0])
                msg += "[{0:d} {1:d}]".format(self.seq_num, dataTuple[5])
                print(msg)
                return False
            else:
                return True
        else:
            raise TypeError(str("WnTransportHeader:  length of header " +
                                "did not match size of transport header"))

# End Class


class CmdRespMessage(Message):
    """Base class for WARPNet command / response messages.
    
    Attributes:
        command -- (uint32) WARPNet command / response
        length -- (uint16) Length of the cmd / resp args in bytes
        num_args -- (uint16) Number of uint32 arguments
        args -- (list of uint32) Arguments of the command / reponse
    """
    command   = None
    length    = None
    num_args  = None
    args      = None
    raw_data  = None
    
    def __init__(self, command=0, length=0, num_args=0, args=None):
        super(CmdRespMessage, self).__init__()
        self.command = command
        self.length = length
        self.num_args = num_args
        self.args = args or []
        self.raw_data = bytearray(self.serialize())

    def serialize(self):
        """Return a bytes object of a packed command / response."""
        ret_val = b''
        
        if self.num_args == 0:
            ret_val = struct.pack('!I 2H', self.command, self.length, 
                                  self.num_args)
        else:
            ret_val = struct.pack('!I 2H %dI' % self.num_args, self.command,
                                  self.length, self.num_args, *self.args)

        self.raw_data = bytearray(ret_val)
        return ret_val
                               

    def deserialize(self, data_buffer):
        """Populate the fields of a WnCmdResp from a buffer."""
        try:
            dataTuple = struct.unpack('!I 2H', data_buffer[0:8])
            self.command = dataTuple[0]
            self.length = dataTuple[1]
            self.num_args = dataTuple[2]
            self.args = list(struct.unpack_from('!%dI' % self.num_args, 
                                                data_buffer, offset=8))
            self.raw_data = bytearray(data_buffer)
        except struct.error as err:
            # Reset Cmd/Resp.  We want predictable behavior on error
            self.reset()
            print("Error unpacking WARPNet cmd/resp: {0}".format(err))
    
    def get_bytes(self):
        """Returns the data buffer as bytes."""
        return self.raw_data
    
    def sizeof(self):
        """Return the size of the cmd/resp including all attributes."""
        if self.num_args == 0:
            return struct.calcsize('!I 2H')
        else:        
            return struct.calcsize('!I 2H %dI' % self.num_args)

    def reset(self):
        """Reset the WnCmdResp object to a default state (all zeros)"""
        self.command = 0
        self.length = 0
        self.num_args = 0
        self.args = []
        
# End Class


class Cmd(CmdRespMessage):
    """Base Class for WARPNet commands.

    Attributes:
        resp_type -- Response type of the command.  See WARPNet transport
            for defined response types.  By default, a WnCmd will require
            a WnResp as a response.

    See documentation of WnCmdResp for additional attributes
    """
    resp_type = None
    
    def __init__(self, command=0, length=0, num_args=0, 
                 args=None, resp_type=None):
        super(Cmd, self).__init__(command, length, num_args, args)
        self.resp_type = resp_type or wn_transport.TRANSPORT_WN_RESP
    
    def set_args(self, *args):
        """Set the command arguments."""
        self.args = args
        self.num_args = len(args)
        self.length = self.num_args * 4
    
    def add_args(self, *args):
        """Append arguments to current command argument list.
        
        NOTE: since the transport only operates on unsigned 32 bit integers, we 
        will convert all values to 32 bit unsigned integers.        
        """
        import ctypes

        for arg in args:
            if (arg < 0):
                arg = ctypes.c_uint32(arg).value
            self.args.append(arg)
 
        self.num_args += len(args)
        self.length += len(args) * 4

    def get_resp_type(self):
        return self.resp_type

    def process_resp(self, resp):
        """Process the response of the WARPNet command."""
        raise NotImplementedError

    def __str__(self):
        """Pretty print the WnCommand"""
        msg = ""
        if self.command is not None:
            msg += "WARPNet Command [{0:d}] ".format(self.command)
            msg += "({0:d} bytes): \n".format(self.length)
            
            if (self.num_args > 0):
                msg += "    Args [0:{0:d}]  : \n".format(self.num_args)
                for i in range(len(self.args)):
                    msg += "        0x{0:08x}\n".format(self.args[i])
        return msg

# End Class


class BufferCmd(CmdRespMessage):
    """Base Class for WARPNet Buffer commands.

    Arguments:
        buffer_id  -- (uint32) ID of buffer for this message.
        flags      -- (uint32) Flags associated with this message.
        start_byte -- (uint32) Starting address of the buffer for this message.
        size       -- (uint32) Size of the buffer in bytes
                          - Reserved value:  CMD_BUFFER_GET_SIZE_FROM_DATA

    Attributes:
        resp_type -- Response type of the command.  See WARPNet transport
            for defined response types.  By default, a BufferCmd will require
            a WnBuffer as a repsonse.

    Note:  The wire format of a Buffer command is:
        Word[0]     -- buffer id
        Word[1]     -- flags
        Word[2]     -- start_address of transfer
        Word[3]     -- size of transfer (in bytes)
        Word[4 - N] -- Additional arguments
    
    To add additional arguments to a BufferCmd, use the add_args() method.

    See documentation of WnCmdResp for additional attributes
    """
    resp_type  = None
    buffer_id  = None
    flags      = None
    start_byte = None
    size       = None
    
    def __init__(self, command=0, buffer_id=0, flags=0, start_byte=0, size=0):
        super(BufferCmd, self).__init__(command=command, length=16, num_args=4,
                                        args=[buffer_id, flags, start_byte, size])

        self.resp_type  = wn_transport.TRANSPORT_WN_BUFFER
        self.buffer_id  = buffer_id
        self.flags      = flags
        self.start_byte = start_byte
        if (size == CMD_BUFFER_GET_SIZE_FROM_DATA):
            self.size = 0
        else:
            self.size = size
    
    def get_resp_type(self):          return self.resp_type        
    def get_buffer_id(self):          return self.buffer_id
    def get_buffer_flags(self):       return self.flags
    def get_buffer_start_byte(self):  return self.start_byte
    def get_buffer_size(self):        return self.size    

    def update_start_byte(self, value):
        self.start_byte = value
        self.args[2] = value
    
    def update_size(self, value):
        self.size = value
        self.args[3] = value

    def add_args(self, *args):
        """Append arguments to current command argument list.
        
        NOTE: since the transport only operates on unsigned 32 bit integers, we 
        will convert all values to 32 bit unsigned integers.        
        """
        import ctypes

        for arg in args:
            if (arg < 0):
                arg = ctypes.c_uint32(arg).value
            self.args.append(arg)
 
        self.num_args += len(args)
        self.length += len(args) * 4

    def process_resp(self, resp):
        """Process the response of the WARPNet command."""
        raise NotImplementedError

    def __str__(self):
        """Pretty print the WnCommand"""
        msg = ""
        if self.command is not None:
            msg += "WARPNet Buffer Command [{0:d}] ".format(self.command)
            msg += "({0:d} bytes): \n".format(self.length)
            
            if (self.num_args > 0):
                msg += "    Args [0:{0:d}]  : \n".format(self.num_args)
                for i in range(len(self.args)):
                    msg += "        0x{0:08x}\n".format(self.args[i])
        return msg

# End Class


class Resp(CmdRespMessage):
    """Class for WARPNet responses.
    
    See documentation of WnCmdResp for attributes
    """
    def get_args(self):
        """Return the response arguments."""
        return self.args


    def resp_is_valid(self, num_args=None, status_errors=None, name=None):
        """Checks if the response is valid
        
        Args:
            num_args (int, optional):        Check that the number of arguments matches the
                provided value
            status_errors (dict, optional):  Dictionary of status errors for the form:
                { <status error value (int)> : <Error message (str)>}
            name (str, optional):            Name of the command
        
        Returns:
            is_valid (bool):   Is the response valid?
        """
        error = False
        msg   = "ERROR:  Invalid response from node:\n"
        
        if num_args is not None:
            if len(self.args) != num_args:
                msg  += "        Number of arguments in response ({0}) does not match\n".format(self.args)
                msg  += "        number of expected arguments ({0})\n".format(num_args)
                error = True

        if status_errors is not None:
            if self.args:
                if (self.args[0] in status_errors.keys()):
                    if name is not None:
                        msg  += "        Received status error {0}\n".format(name)
                    else:
                        msg  += "        Received status error\n"
                    msg  += "        {0}\n".format(status_errors[self.args[0]])
                    error = True
            else:
                if name is not None:
                    msg  += "        No status in response {0}\n".format(name)
                else:
                    msg  += "        No status in response\n"
                error = True

        if error:
            print(msg)
            print(self)
            return False
        else:
            return True
                

    def __str__(self):
        """Pretty print the WnResponse"""
        msg = ""
        if self.command is not None:
            msg += "WARPNet Response [{0:d}] ".format(self.command)
            msg += "({0:d} bytes): \n".format(self.length)
            
            if (self.num_args > 0):
                msg += "    Args [0:{0:d}]  : \n".format(self.num_args)
                for i in range(len(self.args)):
                    msg += "        0x{0:08x}\n".format(self.args[i])
        return msg

# End Class


class Buffer(Message):
    """Class for WARPNet buffer for transferring generic information.
    
    This object provides a container to transfer information that will be
    decoded by higher level functions.

    Attributes:
        complete   -- Flag to indicate if buffer contains all of the bytes
                        indicated by the size parameter
        start_byte -- Start byte of the buffer
        num_bytes  -- Number of bytes currently contained within the buffer

    Wire Data Format:
        command         -- (uint32) WARPNet command / response
        length          -- (uint16) Length of the cmd / resp args in bytes
        num_args        -- (uint16) Number of uint32 arguments
        buffer_id       -- (uint32) ID of buffer for this message.
        flags           -- (uint32) Flags associated with this message.
        bytes_remainig  -- (uint32) Number of bytes remain in the current request
        start_byte      -- (uint32) Address of start byte for the transfer
        size            -- (uint32) Size of the buffer in bytes
        buffer          -- (list of uint8) Content of the buffer 
    """
    complete    = None
    start_byte  = None
    num_bytes   = None
    tracker     = None

    buffer_id   = None
    flags       = None
    size        = None
    buffer      = None

    def __init__(self, buffer_id=0, flags=0, start_byte=0, size=0, data_buffer=None):
        self.buffer_id  = buffer_id
        self.flags      = flags
        self.start_byte = start_byte
        self.size       = size

        self.tracker    = [{0:start_byte, 1:start_byte, 2:0}]

        if data_buffer is not None:
            self._add_buffer_data(start_byte, data_buffer)
        else:
            # Create an empty buffer of the specified size
            self.complete  = False
            self.num_bytes = 0
            self.buffer    = bytearray(self.size)


    def serialize(self):
        """Return a bytes object of a packed buffer."""
        return self.serialize_cmd()


    def serialize_cmd(self, command=None, start_byte=None):
        """Return a bytes object of a packed buffer."""
        if command is None:      command = 0
        if start_byte is None:   start_byte = self.start_byte
        
        return struct.pack('!I 2H 5I %dB' % self.size, 
                           command, 20, 5,  # length = Num_args * 4 bytes / arg; Num_args = 5; 
                           self.buffer_id, self.flags, 0, start_byte,
                           self.size, *self.buffer)


    def deserialize(self, data_buffer):
        """Populate the fields of a WnBuffer with a message raw_data."""
        (args, data) = self._unpack_data(data_buffer) 
                
        self.buffer_id  = args[3]
        self.flags      = args[4]
        bytes_remaining = args[5]
        start_byte      = args[6]

        offset = (start_byte - self.start_byte)
        
        self._update_buffer_size(bytes_remaining)
        self._add_buffer_data(offset, data)
        self._set_buffer_complete()            


    def add_data_to_buffer(self, raw_data):
        """Add the raw data (with the format of a WnBuffer) to the current
        WnBuffer.
        
        Note:  This will check to make sure that data is for the given buffer
        as well as place it in the appropriate place indicated by the
        start_byte.        
        """
        (args, data) = self._unpack_data(raw_data) 

        buffer_id       = args[3]
        flags           = args[4]
        bytes_remaining = args[5]
        start_byte      = args[6]

        if (buffer_id == self.buffer_id):            
            offset = (start_byte - self.start_byte)
            
            self._update_buffer_size(bytes_remaining)
            self._add_buffer_data(offset, data)
            self._set_buffer_complete()            
 
            self.set_flags(flags)
            
        else:
            print("Data not intended for given WARPNet buffer.  Ignoring.")


    def append(self, wn_buffer):
        """Append the contents of the provided WnBuffer to the current WnBuffer."""
        curr_size = self.size
        new_size = curr_size + wn_buffer.get_buffer_size()
        
        self._update_buffer_size(new_size, force=1)
        self._add_buffer_data(curr_size, wn_buffer.get_bytes())
        self._set_buffer_complete()


    def merge(self, wn_buffer):
        """Merge the contents of the provided WnBuffer to the current WnBuffer."""
        start_byte = wn_buffer.get_start_byte()
        offset     = (start_byte - self.start_byte)
        size       = wn_buffer.get_buffer_size()
        end_byte   = offset + size
        
        if (end_byte > self.size):
            # Need to update the buffer to allocate more memory first
            self._update_buffer_size(end_byte, force=1)

        self._add_buffer_data(offset, wn_buffer.get_bytes())
        self._set_buffer_complete()
            


    def trim(self):
        """Trim the buffer to the largest contiguous number of bytes received."""
        locations = self.get_missing_byte_locations()
        
        if locations:
            # NOTE:  This assumes that the missing byte locations are in order
            #   with the first missing byte after the start byte in the first
            #   item in the list.
            missing_start   = locations[0][0]
            contiguous_size = missing_start - self.start_byte

            self.num_bytes = contiguous_size
            self._update_buffer_size(contiguous_size, force=1)
            self._set_buffer_complete()


    def sizeof(self):
        """Return the size of the buffer including all attributes."""
        return struct.calcsize('!5I %dB' % self.size)

    def get_buffer_id(self):           return self.buffer_id
    def get_flags(self):               return self.flags
    def get_start_byte(self):          return self.start_byte    
    def get_header_size(self):         return struct.calcsize('!5I')
    def get_buffer_size(self):         return self.size
    def get_occupancy(self):           return self.num_bytes

    def set_flags(self, flags):
        """Set the bits in the flags field based on the value provided."""
        self.flags = self.flags | flags

    def clear_flags(self,flags):
        """Clear the bits in the flags field based on the value provided."""
        self.flags = self.flags & ~flags

    def set_bytes(self, data_buffer):
        """Set the message bytes of the buffer."""
        self._update_buffer_size(len(data_buffer), force=1)
        self._add_buffer_data(0, data_buffer)
        self._set_buffer_complete()

    def get_bytes(self):
        """Return the message bytes of the buffer."""
        return self.buffer

    def get_missing_byte_locations(self):
        """Returns a list of tuples (start_index, end_index, size) that 
        contain the missing byte locations.
        """
        if not self.complete:
            return self._find_missing_bytes()
        else:
            return []

    def is_buffer_complete(self):
        """Return if the buffer is complete."""
        return self.complete

    def reset(self):
        """Reset the WnBuffer object to a default state (all zeros)"""
        self.buffer_id = 0
        self.flags     = 0
        self.size      = 0
        self.buffer    = bytearray(self.size)

    def __str__(self):
        """Pretty print the WnBuffer"""
        msg = ""
        if self.buffer is not None:
            msg += "WARPNet Buffer [{0:d}] ".format(self.buffer_id)
            msg += "({0:d} bytes): \n".format(self.size)
            msg += "    Flags    : 0x{0:08x} \n".format(self.flags)
            msg += "    Start    : {0:d}\n".format(self.start_byte)
            msg += "    Num bytes: {0:d}\n".format(self.num_bytes)
            msg += "    Complete : {0}\n".format(self.complete)

            if (False):
                msg += "    Data     : "            
                for i in range(len(self.buffer)):
                    if (i % 16) == 0:
                        msg += "\n        {0:02x} ".format(self.buffer[i])
                    else:
                        msg += "{0:02x} ".format(self.buffer[i])
        return msg


    # -------------------------------------------------------------------------
    # Internal helper methods
    # -------------------------------------------------------------------------
    def _unpack_data(self, raw_data):
        """Internal method to unpack a data buffer."""
        args = []
        data = []
        try:
            # Interpret the raw_data
            args = struct.unpack('!I 2H 5I', raw_data[0:28])
            size = args[7]
            data = struct.unpack_from('!%dB' % size, raw_data, offset=28)
        except struct.error as err:
            # Ignore the data.  We want predictable behavior on error
            print("Error unpacking WARPNet buffer: {0}\n".format(err),
                  "    Ignorning data.")
        
        return (args, data)


    def _update_buffer_size(self, size, force=0):
        """Internal method to update the size of the transfer."""
        if (self.size == 0):
            self.size   = size
            self.buffer = bytearray(self.size)
        elif (force == 1):
            # Update the size of the buffer
            old_size  = self.size
            self.size = size
            
            # Update the buffer allocation
            if (size > old_size):
                self.buffer.extend(bytearray(size - old_size))
            else:
                self.buffer = self.buffer[:size]


    def _add_buffer_data(self, buffer_offset, data_buffer):
        """Internal method to add data to the buffer
        
        Only self.size bytes were allocated for the buffer.  Therefore, we 
        will only take an offset from the start_byte (ie a relative address)
        for where to store the data in the buffer.
        
        NOTE:  If the provided buffer data is greater than specified buffer
            size, then the data will be truncated.
        """
        data_to_add_size = len(data_buffer)
        buffer_end_byte  = buffer_offset + data_to_add_size

        # If we are going to run off the end of the buffer, then truncate the add
        if (buffer_end_byte > self.size):
            buffer_end_byte  = self.size
            data_to_add_size = buffer_end_byte - buffer_offset

        # Update the tracker with the information
        #   NOTE:  Need to convert back to absolute addresses for tracker
        self._update_tracker((buffer_offset + self.start_byte), (buffer_end_byte + self.start_byte), data_to_add_size)
        
        # Add the data to the buffer
        if (data_to_add_size > 0):
            self.buffer[buffer_offset:buffer_end_byte] = data_buffer[:data_to_add_size]

        # Update the ocupancy of the buffer
        self.num_bytes = self._tracker_size()

        # Set the buffer complete flag            
        self._set_buffer_complete()


    def _set_buffer_complete(self):
        """Internal method to set the complete flag on the buffer."""
        if   (self.num_bytes == self.size):
            self.complete = True
        elif (self.num_bytes < self.size):
            self.complete = False
        else:
            print("WARNING: WnBuffer out of sync.  Should never reach here.")


    def _tracker_size(self):
        """Internal method to get the buffer size via the tracker."""
        size = 0
        for item in self.tracker:
            size += item[2]
        
        return size


    def _update_tracker(self, start_byte, end_byte, size):
        """Internal method to update the tracker."""
        done = False
        
        # Don't add duplicate entries
        for item in self.tracker:
            if (start_byte == item[0]) and (end_byte == item[1]) and (size == item[2]):
                return        
 
        # See if we can add this update to one of the current tracker entries       
        for item in self.tracker:
            if (start_byte == item[1]):
                item[1] += size
                item[2] += size
                done     = True
        
        if not done:
            self.tracker.append({0:start_byte, 1:end_byte, 2:size})
        
        # Try to compress the tracker
        self._compress_tracker()


    def _compress_tracker(self):
        """Internal method to compress the tracker."""
        # See if we can compress if there is more than one item
        if (len(self.tracker) > 1):
            tracker     = []
            tmp_tracker = sorted(self.tracker, key=lambda k: k[0])
            
            tmp_item = tmp_tracker[0]
            tracker.append(tmp_item)
            
            # For each remaining item if the start_byte equals the end_byte
            # of the start_item, then we can merge the items
            for item in tmp_tracker[1:]:
                if (item[0] == tmp_item[1]):
                    tmp_item[1]  = item[1]
                    tmp_item[2] += item[2]
                else:
                    tmp_item = item
                    tracker.append(tmp_item)
            
            self.tracker = tracker

    
    def _find_missing_bytes(self):
        """Internal method to find the missing bytes using the tracker."""
        ret_val       = []
        missing_bytes = self.size - self.num_bytes
        start         = self.start_byte
        end           = self.start_byte + self.size
        tmp_tracker   = sorted(self.tracker, key=lambda k: k[0]) 
        tracker_count = list(tmp_tracker)
        error         = False

        if (missing_bytes != 0):
            # Iterate through all the items in the list and remove
            # them as we build up the holes
            for item in tmp_tracker:

                # Process item but don't add a hole
                if   (start == item[0]):
                    start = item[1]
                    tracker_count.remove(item)
                    
                # There is a missing piece of the buffer to request
                elif ((start + missing_bytes) >= item[0]):
                    tmp_size       = item[0] - start
                    
                    if tmp_size < 0:
                        print("WARNING:  Issue with finding missing bytes.")
                        print("    Size between items is negative.")
                        print("    item            : ({0}, {1}, {2})".format(item[0], item[1], item[2]))
                        print("    start           : {0}".format(start))
                        error = True
                    else:
                        missing_bytes -= tmp_size
                        ret_val.append((start, item[0], tmp_size))
                        start          = item[1]
                        
                    tracker_count.remove(item)

                # There was an error in the tracker
                else:
                    print("WARNING:  Issue with tracking missing bytes.")
                    print("    Number of missing bytes does not cover hole between tracker items.")
                    print("    Missing Bytes   : {0}".format(missing_bytes))

                    tmp_size       = item[0] - start
                    missing_bytes  = 0
                    ret_val.append((start, item[0], tmp_size))
                    start          = item[1]
                    tracker_count.remove(item)
                    error = True
            
            if tracker_count:
                print("WARNING:  Issue with finding missing bytes.")
                print("    Not all tracker items processed.")
                error = True
    
            if error:
                print("    Tracker         : {0}".format(self.tracker))
                print("    Tmp Tracker     : {0}".format(tmp_tracker))
                print("    Remaining Items : {0}".format(tracker_count))
                print("    Locations       : {0}".format(ret_val))
        
            # Find any holes at the end of the buffer
            if (missing_bytes != 0):
                if (end != start):
                    tmp_size       = end - start
                    ret_val.append((start, end, tmp_size))
                    missing_bytes -= tmp_size

            # Print a warning because we missed something    
            if (missing_bytes != 0):
                print("WARNING: Could not find all missing bytes: {0}".format(missing_bytes))
        
        return ret_val


# End Class




