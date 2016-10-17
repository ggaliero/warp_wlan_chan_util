#cython: boundscheck=False
#cython: wraparound=False

# This is a faster implementation of coll_util that uses cython and OpenMP.
#   On a 2013 Macbook Pro Retina, the speed of finding overlapping transmissions
#   within two tx vectors of ~135,000 transmissions is the following:
#       Pure Python (coll_util.py):                 2.018 seconds
#       Cython, no OpenMP (coll_util_fast.pyx):     0.128 seconds
#       Cython, with OpenMP (coll_util_fast.pyx):   0.056 seconds
#
# The downside is that this file must be explicitly compiled, which is a process
# that is potentially tricky depending on your platform. Unless you are dealing
# with extremely large datasets, it probably isn't worth it.
#
# Should you choose to build this file, there are two approaches:
# 1) Implicit building with pyximport
#       Pros: If it works on your platform, then it's basically just like an import
#       Cons: * I'm sure it's possible, but I haven't been able to get OpenMP to compile
#               in with this method.
#             * I'm sure it's possible, but I haven't been able to get this to work
#               under Windows.
#   In wlan_exp.log.util.py, find the "find_overlapping_tx_low" method.
#   Replace "import wlan_exp.log.coll_util as collision_utility" with the following:
#         import pyximport
#         pyximport.install(setup_args={'include_dirs':[np.get_include()]})
#         import wlan_exp.log.coll_util_fast as collision_utility 
#
# 2) Explicit building
#       Pros: * I've gotten this to work under Windows with the MinGW compiler,
#               but even then I still wasn't able to compile in OpenMP.
#             * OpenMP works under OSX with this method (provided you download
#               gcc from homebrew... the provided Clang doesn't play nice with
#               OpenMP)
#
#   The setup.py file in this directly is like a makefile. It's currently set up
#   my platform, so you should look online how to construct it for yours. Note:
#   the "gcc" under Mac OSX Mavericks is not really gcc, but rather clang. I had
#   some trouble getting OpenMP compiled in with this, so I used homebrew to install
#   gcc-4.8. I think explicitly set the CC environment to gcc-4.8 in setup.py.
#
#   To build the file, navigate to this folder in a terminal and enter:
#   python setup.py build_ext --inplace
#
#   This will create a coll_util_fast.so, which is the compiled object. Now
#   you can simply "import wlan_exp.log.coll_util_fast as collision_utility" from
#   wlan_exp.log.util.py and it will import the already-compiled version.

from cython.parallel cimport prange
cimport cython

import numpy as np
cimport numpy as np

DTYPE = np.uint64
ctypedef np.uint64_t DTYPE_t

def _collision_idx_finder_l(np.ndarray[DTYPE_t, ndim=1] src_ts, np.ndarray[DTYPE_t, ndim=1] src_dur, np.ndarray[DTYPE_t, ndim=1] int_ts, np.ndarray[DTYPE_t, ndim=1] int_dur):
    
    import wlan_exp.log.util as log_util
    import wlan_exp.util as wlan_exp_util    
    
    assert src_ts.dtype == DTYPE and src_dur.dtype == DTYPE and int_ts.dtype == DTYPE and int_dur.dtype == DTYPE
  
    cdef int num_src = src_ts.shape[0]
    cdef int num_int = src_ts.shape[0]
    
    cdef np.ndarray[DTYPE_t, ndim=1] src_coll_idx = np.zeros([num_src], dtype=DTYPE)
    cdef np.ndarray[DTYPE_t, ndim=1] int_coll_idx = np.zeros([num_int], dtype=DTYPE)

    cdef DTYPE_t curr_src_ts
    cdef DTYPE_t curr_src_dur
    cdef DTYPE_t curr_int_ts
    cdef DTYPE_t curr_int_dur
    
    int_ts_end = int_ts + int_dur
    
    cdef int src_idx, int_idx, int_idx_start
    
    int_idx_start = 0
    
    cdef int int_idx_high, int_idx_low

    for src_idx in prange(num_src, nogil=True):
        
        curr_src_ts = src_ts[src_idx]
        curr_src_dur = src_dur[src_idx]    

        int_idx = num_int/2
        int_idx_high = num_int-1
        int_idx_low = 0

        #start in middle and branch out   
        while 1:
            if (int_idx == int_idx_low or int_idx == int_idx_high):
                #We're done. No overlap
                break            
            
            curr_int_ts = int_ts[int_idx]
            curr_int_dur = int_dur[int_idx]
            
            if ( curr_int_ts < (curr_src_ts + curr_src_dur) ) and ( curr_src_ts < (curr_int_ts + curr_int_dur) ):
                #We found an overlap
                src_coll_idx[src_idx] = src_idx
                int_coll_idx[src_idx] = int_idx 
                break
            elif ( curr_int_ts < curr_src_ts ):
                #Overlap may be later -- move index forward
                int_idx_low = int_idx                
            else:
                #Overlap may be earlier -- move index back
                int_idx_high = int_idx

            int_idx = (int_idx_high - int_idx_low)/2 + int_idx_low
        
                                                                
        
    return (src_coll_idx,int_coll_idx)

