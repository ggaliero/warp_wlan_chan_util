import sys

# Throw exception if using Python older than 2.7.5
#  Older Python versions do not support struct.unpack() features requires by wlan_exp
v = sys.version_info
if (v[0] == 2) and ( (v[1] < 7) or ( (v[1] == 7) and (v[2] < 5))):
    raise Exception('ERROR: wlan_exp requires Python version 2.7.5 or later. Current python version %d.%d.%d not supported' % (v[0], v[1], v[2]))

