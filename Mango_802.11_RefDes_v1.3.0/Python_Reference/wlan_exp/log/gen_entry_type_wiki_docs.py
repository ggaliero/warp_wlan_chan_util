"""
Create wiki documentation from entry types
"""

if __name__ == '__main__':
    import wlan_exp.version as version
    from wlan_exp.log.entry_types import log_entry_types as log_entry_types
    
    
    print("== {0} Log Entry Types ==".format(version.version))
    
    for tid in log_entry_types.keys():
        if(type(tid) is int and tid != 0):
            print(log_entry_types[tid].generate_entry_doc(fmt='wiki'))

