function [load_base_rate, rotate_bpsk, load_htstf, load_htltf, load_full_rate, pilot_shift, cyclic_prefix, cyclic_shift] = sym_config_decode(sym_cfg)

%Inputs:
% sym_cfg: OFDM sym config code, concatenated configuration fields

% Sym Config fields:
%     0: Load Base Rate - base-rate in 48 subcarriers (SIGNAL/HT-SIG fields)
%     1: Rotate BPSK - use QBPSK (+/- j) (HT-SIG fields)
%     2: Load HT-STF - no data bits
%     3: Load HT-LTF - no data bits
%     4: Load Full Rate - full-rate in 48 (a/g) or 52 (n/ac) subcarriers
%   6:5: Pilot shift (0,1,2,3)
%     7: Reserved
% 12: 8: Cyclic prefix length (samples)
% 15:13: Cyclic shift (samples)

%xl_slice(arg, from_bit, to_bit); bits indexed from LSB=0
% '==1' is quick conversion to boolean
load_base_rate = xl_slice(sym_cfg, 0, 0) == 1;
rotate_bpsk    = xl_slice(sym_cfg, 1, 1) == 1;
load_htstf     = xl_slice(sym_cfg, 2, 2) == 1;
load_htltf     = xl_slice(sym_cfg, 3, 3) == 1;
load_full_rate = xl_slice(sym_cfg, 4, 4) == 1;
pilot_shift    = xl_slice(sym_cfg, 6, 5);

cyclic_prefix  = xl_slice(sym_cfg, 12, 8);
cyclic_shift   = xl_slice(sym_cfg, 15, 13);

