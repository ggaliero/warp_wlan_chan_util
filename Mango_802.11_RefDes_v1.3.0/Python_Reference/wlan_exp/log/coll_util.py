# -*- coding: utf-8 -*-
"""
------------------------------------------------------------------------------
WLAN Experiment Collision Utility
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
1.00a crh  1/23/14  Initial release

------------------------------------------------------------------------------

This module provides helper functions for finding packet collisions within 
log files.

"""

import numpy as np
import sys

def _collision_idx_finder_binarysearch(src_ts, src_dur, int_ts, int_dur):
     
    num_src = src_ts.shape[0]
    num_int = int_ts.shape[0]
    
    maxlen = num_src+num_int
    src_coll_idx = np.zeros([maxlen],dtype=np.int)
    int_coll_idx = np.zeros([maxlen],dtype=np.int)       
    
    coll_index = 0;
    
    for src_idx in range(num_src):        
        
        curr_src_ts = src_ts[src_idx]
        curr_src_dur = src_dur[src_idx] 
        
        curr_src_tx_end = curr_src_ts + curr_src_dur
        
        num_iter = 0
        
        int_idx = num_int/2
        int_idx_high = num_int-1
        int_idx_low = 0

        # Start in middle and branch out   
        while 1:
            num_iter = num_iter+1
            if (int_idx == int_idx_low or int_idx == int_idx_high):
                # We're done. No overlap
                break            
            
            curr_int_ts = int_ts[int_idx]
            curr_int_dur = int_dur[int_idx]
            
            if ( curr_int_ts < (curr_src_tx_end) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
                            
                # We found an overlap
                src_coll_idx[coll_index] = src_idx
                int_coll_idx[coll_index] = int_idx 
                coll_index = coll_index+1         
                
                # Keep iterating forward on int_idx until there isn't an overlap                                                        
                while int_idx < (num_int-1):
                    int_idx = int_idx+1
                    curr_int_ts = int_ts[int_idx]
                    curr_int_dur = int_dur[int_idx]                    
                    if ( curr_int_ts < (curr_src_tx_end) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
                        # Found another overlap
                        src_coll_idx[coll_index] = src_idx
                        int_coll_idx[coll_index] = int_idx 
                        coll_index = coll_index+1                             
                    else:
                        # No more collisions, move on
                        break                        
                break
            elif ( curr_int_ts < curr_src_ts ):
                # Overlap may be later -- move index forward
                int_idx_low = int_idx                
            else:
                # Overlap may be earlier -- move index back
                int_idx_high = int_idx

            int_idx = (int_idx_high - int_idx_low)/2 + int_idx_low 
                                                            
    src_coll_idx = src_coll_idx[0:coll_index]                                                        
    int_coll_idx = int_coll_idx[0:coll_index]                                                           
        
    return (src_coll_idx,int_coll_idx)

# End def



def _collision_idx_finder_linearsearch(src_ts, src_dur, int_ts, int_dur):
     
    num_src = src_ts.shape[0]
    num_int = int_ts.shape[0]
    
    maxlen = num_src+num_int
    src_coll_idx = np.zeros([maxlen],dtype=np.int)
    int_coll_idx = np.zeros([maxlen],dtype=np.int)
    
    int_idx_start = 0
    coll_index = 0
        
    for src_idx in range(num_src):        
        
        curr_src_ts = src_ts[src_idx]
        curr_src_dur = src_dur[src_idx] 
        
        curr_src_tx_end = curr_src_ts + curr_src_dur
        
        num_iter = 0
        
        for int_idx in range(int_idx_start,num_int):
            num_iter = num_iter+1
            curr_int_ts = int_ts[int_idx]
            curr_int_dur = int_dur[int_idx]

            if ( curr_int_ts < (curr_src_tx_end) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):                
                # Found an overlap
                src_coll_idx[coll_index] = src_idx
                int_coll_idx[coll_index] = int_idx 
                int_idx_start = int_idx;
                coll_index = coll_index + 1
                # Keep iterating forward on int_idx until there isn't an overlap                                                        
                while int_idx < (num_int-1):
                    int_idx = int_idx+1
                    curr_int_ts = int_ts[int_idx]
                    curr_int_dur = int_dur[int_idx]                    
                    if ( curr_int_ts < (curr_src_tx_end) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
                        # Found another overlap
                        src_coll_idx[coll_index] = src_idx
                        int_coll_idx[coll_index] = int_idx 
                        coll_index = coll_index+1                             
                    else:
                        # No more collisions, move on
                        break                           
                break
            elif ( curr_int_ts > (curr_src_tx_end) ):
                int_idx_start = int_idx;
                break                                                    
    
    src_coll_idx = src_coll_idx[0:coll_index]                                                        
    int_coll_idx = int_coll_idx[0:coll_index]
    
    return (src_coll_idx,int_coll_idx)

# End def



def _collision_idx_finder(src_ts, src_dur, int_ts, int_dur):
    if sys.version_info[0] == 2:
        # The binary search method runs considerably faster under Python 2.7 than Python 3.3
        return _collision_idx_finder_binarysearch(src_ts, src_dur, int_ts, int_dur)
    else:
        # The linear search method runs considerably faster under Python 3.3 than Python 2.7
        return _collision_idx_finder_linearsearch(src_ts, src_dur, int_ts, int_dur)

# End def
