# -*- coding: utf-8 -*-
"""
.. ------------------------------------------------------------------------------
.. WLAN Experiment Programmable Attenuator Interface
.. ------------------------------------------------------------------------------
.. Authors:   Chris Hunter (chunter [at] mangocomm.com)
..            Patrick Murphy (murphpo [at] mangocomm.com)
..            Erik Welsh (welsh [at] mangocomm.com)
.. License:   Copyright 2014-2015, Mango Communications. All rights reserved.
..            Distributed under the WARP license (http://warpproject.org/license)
.. ------------------------------------------------------------------------------
.. MODIFICATION HISTORY:
..
.. Ver   Who  Date     Changes
.. ----- ---- -------- -----------------------------------------------------
.. 1.00a pom  8/23/14  Initial release
.. ------------------------------------------------------------------------------

"""

class ProgAttenController(object):
    atten_dev = None

    def __init__(self, serial_port=None, serial_num=None):
        import serial

        ser_port = self.find_atten_dev_port(serial_port, serial_num)
        
        if(ser_port is not None):
            self.atten_dev = serial.Serial(port=ser_port, baudrate=115200, timeout=0.25)

            # Disable console mode (prevents output echoing input)
            self.write_cmd('CONSOLE DISABLE')

            # Reset the attenuator state and clear input/output buffers
            self.write_cmd('*RST')
            self.write_cmd('*CLS')
            
            return
        else:
            raise Exception('ERROR: No programmable attenuator found - check your USB connections and driver!')

        
    def find_atten_dev_port(self, serial_port=None, serial_num=None):
        from serial.tools.list_ports import comports
        import re
        
        # comports returns iterable of 3-tuples:
        #   0: Port name ('COMX' on windows, ??? on OS X)
        #   1: Device description
        #   2: Device info
        
        # We're looking for: (where X in COMX is arbitrary integer)
        #   ('COMX',  'Weinschel 4205 USB COM Port (COMX)', 'USB VID:PID=25EA:106D SNR=000000'),
        
        for (port_name, dev_desc, dev_info) in comports():
            # Apply port name filter, if specified
            if(serial_port is not None and port_name != serial_port):
                # Keep looking                  
                continue
            
            desc_test = dev_desc.split('Weinschel 4205 USB COM Port')
            info_test = re.search('USB VID:PID=(....):(....) SNR=(.+)', dev_info)
            if(info_test):
                info_test = info_test.groups()
            else:
                info_test = []

            if(desc_test[0] == '' and len(info_test) == 3):
                # Found an attenuator
            
                # Check the serial number, if specified
                if(serial_num is not None and serial_num != info_test[2]):
                    # Keep looking                  
                    continue

                # Future extension: return serial number too
                #   ser_num = info_test[2] #keep as string; device supports alphanumeric ser nums
                print('Found attenuator on {0} ({1} {2})'.format(port_name, dev_desc, dev_info))
                return port_name
        
        # Did not find attenuator        
        return None


    def read(self):
        return self.atten_dev.read(200)


    def readline(self):
        return self.atten_dev.readline()


    def write(self, s):
        return self.atten_dev.write(s)
    
    
    def write_cmd(self, cmd):
        # Strip any stray line breaks, append one CR
        cmd = ('{0}\n').format(str(cmd).replace('\n', '').replace('\r', ''))
        
        # Write command to serial port
        stat = self.atten_dev.write(cmd)
        
        # Check that full command was written
        if(stat != len(cmd)):
            raise Exception('ERROR: Failed to write full command to serial port ({0} < {1})!'.format(stat, len(cmd)))


    def read_resp(self):
        # Attenuator responses are terminated by newlines
        #   It also replies with line breaks to (some?) commands, even when CONSOLE is off
        
        # Loop until:
        #   Actual response is received (more than just line breaks)
        #   serial.readline() times out (indicated by zero-length return)
        while True:
            r = self.readline()
            if(len(r) > 0):
                #Check if response is just line breaks
                r = r.replace('\n', '').replace('\r', '')
                if(len(r) == 0):
                    continue
                else:
                    #Got valid (more than line breaks) response
                    return r
            else:
                return None


    def set_atten(self, atten):
        # Round user supplied attenuation to nearest 0.5 dB in [0.0, 95.5]
        atten_actual = float(atten)
        
        # Constrain to supported attenuation range
        if(atten_actual > 95.5):
            atten_actual = 95.5
        elif(atten_actual < 0):
            atten_actual = 0.0

        # Round to nearest 0.1 dB
        atten_actual = round(float(atten_actual), 1)
        
        atten_str = '{0:2.1f}'.format(atten_actual)
    
        # Send command to attenuator
        self.write_cmd('ATTN ' + atten_str)
        
        # Read back the attenuation
        self.write_cmd('ATTN?')
        r = self.read_resp()
        
        if(str(r) != str(atten_str)):
            raise Exception('ERROR: failed to set attenuation ({0} != {1})'.format(atten_str, r))

    
    def close(self):
        self.atten_dev.close()

# End class