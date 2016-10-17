.. _wlan_exp_device:

.. include:: globals.rst

WLAN Device
-----------

The WLAN device represents an 802.11 device in a WLAN network.  This can 
be used to define non-WARP nodes so that the experiment framework can 
interact with them.

Class
`````
.. autoclass:: wlan_exp.device.WlanDevice
   :members:


Example
```````
To create a WLAN device we first must specify the MAC address and name of the device::

    # WLAN devices
    # Contains a list of tuples: (MAC Address, 'String description of device')
    #  MAC addresses must be expressed as uint64 values
    #  For example, use 0x0123456789AB for MAC address 01:23:45:67:89:AB
    WLAN_DEVICE_LIST  = [(0x000000000000, 'My Device')]

Once we have the device parameters, we can then create WLAN devices for each device::

    # Setup WLAN devices
    devices = []
    
    for device in WLAN_DEVICE_LIST:
        devices.append(wlan_device.WlanDevice(mac_address=device[0], name=device[1]))

Finally, we can interact with the devices within the WLAN Experiments Framework::

    # Wait for devices to associate with the AP
    print("Waiting for devices to join:")

    total_devices = len(devices)
    tmp_devices   = list(devices)
    num_joined    = 0

    while (total_devices != num_joined):
    
        for device in tmp_devices:
            if n_ap.is_associated(device):
                print("    {0} joined".format(device.name))
                num_joined += 1
                tmp_devices.remove(device)
