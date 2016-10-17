function deinterleave_mapping_ROM = wlan_rx_deinterleave_rom_gen()

% Define numbers of coded bits per OFDM symbol for each modulation rate
NCBPS_11a_BPSK  = 48;
NCBPS_11a_QPSK  = 96;
NCBPS_11a_16QAM = 192;
NCBPS_11a_64QAM = 288;

NCBPS_11n_BPSK  = 52;
NCBPS_11n_QPSK  = 104;
NCBPS_11n_16QAM = 208;
NCBPS_11n_64QAM = 312;

% Calculate interleaving vectors for each modulation scheme for each PHY mode
interleave_11a_BPSK  = calc_interleave_vec_11a(NCBPS_11a_BPSK,  1);
interleave_11a_QPSK  = calc_interleave_vec_11a(NCBPS_11a_QPSK,  2);
interleave_11a_16QAM = calc_interleave_vec_11a(NCBPS_11a_16QAM, 4);
interleave_11a_64QAM = calc_interleave_vec_11a(NCBPS_11a_64QAM, 6);
interleave_11n_BPSK  = calc_interleave_vec_11n(NCBPS_11n_BPSK,  1);
interleave_11n_QPSK  = calc_interleave_vec_11n(NCBPS_11n_QPSK,  2);
interleave_11n_16QAM = calc_interleave_vec_11n(NCBPS_11n_16QAM, 4);
interleave_11n_64QAM = calc_interleave_vec_11n(NCBPS_11n_64QAM, 6);

% The interleave_11x vectors define the 1:1 mapping of input bit index to
%  output bit index. The spec places the first bit (output index 0) in the
%  lowest (most negative) frequency subcarrier, subcarrier index -26 for 11a,
%  -28 for 11n. The Rx PHY FFT core outputs subcarriers in in natual order,
%  starting at subcarrier 0 (DC), then +1, +2, etc. Thus the deinterleaving
%  vectors must be rotated to align the bit indices with the IFFT
%  subcarrier indices. This is effectivly an fftshift() operation.
interleave_11a_BPSK  = mod(interleave_11a_BPSK  + (NCBPS_11a_BPSK/2),  NCBPS_11a_BPSK);
interleave_11a_QPSK  = mod(interleave_11a_QPSK  + (NCBPS_11a_QPSK/2),  NCBPS_11a_QPSK);
interleave_11a_16QAM = mod(interleave_11a_16QAM + (NCBPS_11a_16QAM/2), NCBPS_11a_16QAM);
interleave_11a_64QAM = mod(interleave_11a_64QAM + (NCBPS_11a_64QAM/2), NCBPS_11a_64QAM);
interleave_11n_BPSK  = mod(interleave_11n_BPSK  + (NCBPS_11n_BPSK/2),  NCBPS_11n_BPSK);
interleave_11n_QPSK  = mod(interleave_11n_QPSK  + (NCBPS_11n_QPSK/2),  NCBPS_11n_QPSK);
interleave_11n_16QAM = mod(interleave_11n_16QAM + (NCBPS_11n_16QAM/2), NCBPS_11n_16QAM);
interleave_11n_64QAM = mod(interleave_11n_64QAM + (NCBPS_11n_64QAM/2), NCBPS_11n_64QAM);

% The Rx PHY deinterleaver uses a single RAM to store all the bits for a
% single OFDM symbol. The same RAM is used for all rates. The RAM stores
% coded/punctured data bits for each subcarrier on 32-bit boundaries. By
% allocating 1 32-bit word per subcarrier the design can use an efficient BRAM
% architecture with 1 word write per subcarrier (concatenated LLR values) and parallel
% 2-soft-bit reads (soft values for 2 coded bits). Even with the aspect ratio change
% write and read operations require a single clock cycle.
%
% The vectors above define the interleaver mapping as bit indexes in:out.
% The operations below convert these to mapping 
%  (bit index in) : (interleaver RAM address out) by increasing the 
% output index values to place each subcarrier's bits on a 32-bit boundary.
% The BRAM output width is 4 bits, so the read addresses below access 8 entries
%  per 32-bit word.
%
% floor(x / bits_per_sym) calculates the subcarrier index of each output bit index
% mod(x, bits_per_sym) calculates the bit index within the subcarrier
ramaddr_11a_BPSK  = mod(interleave_11a_BPSK, 1)  + (8 * floor(interleave_11a_BPSK  / 1));
ramaddr_11a_QPSK  = mod(interleave_11a_QPSK, 2)  + (8 * floor(interleave_11a_QPSK  / 2));
ramaddr_11a_16QAM = mod(interleave_11a_16QAM, 4) + (8 * floor(interleave_11a_16QAM / 4));
ramaddr_11a_64QAM = mod(interleave_11a_64QAM, 6) + (8 * floor(interleave_11a_64QAM / 6));

ramaddr_11n_BPSK  = mod(interleave_11n_BPSK, 1)  + (8 * floor(interleave_11n_BPSK  / 1));
ramaddr_11n_QPSK  = mod(interleave_11n_QPSK, 2)  + (8 * floor(interleave_11n_QPSK  / 2));
ramaddr_11n_16QAM = mod(interleave_11n_16QAM, 4) + (8 * floor(interleave_11n_16QAM / 4));
ramaddr_11n_64QAM = mod(interleave_11n_64QAM, 6) + (8 * floor(interleave_11n_64QAM / 6));

%Concatenate all the mapping vectors into a single vector, to be used as
% the init value for the ROM which implements the mapping LUT. Each LUT entry 
% is used as a memory address. The Tx PHY logic constructs the RAM write address as:
%  addr[8:0] = {phy_mode_11n[0] mod_sel[1:0] bit_ind[5:0]}
% Each mod scheme occupies 512 entries (64 subcarriers * 8 RAM bits per subcarrier)
deinterleave_mapping_ROM = [];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11a_BPSK    zeros(1, 512-length(ramaddr_11a_BPSK))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11a_QPSK    zeros(1, 512-length(ramaddr_11a_QPSK))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11a_16QAM   zeros(1, 512-length(ramaddr_11a_16QAM))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11a_64QAM   zeros(1, 512-length(ramaddr_11a_64QAM))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11n_BPSK    zeros(1, 512-length(ramaddr_11n_BPSK))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11n_QPSK    zeros(1, 512-length(ramaddr_11n_QPSK))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11n_16QAM   zeros(1, 512-length(ramaddr_11n_16QAM))];
deinterleave_mapping_ROM = [deinterleave_mapping_ROM   ramaddr_11n_64QAM   zeros(1, 512-length(ramaddr_11n_64QAM))];

%Helper functions to calculate the interleaving vectors
% These are implementations of the interleaving functions in the 802.11 spec
function intlv_vec = calc_interleave_vec_11a(cbps, bpsc)
    N_CBPS = cbps;
    N_BPSC = bpsc;
    s = max(N_BPSC/2, 1);

    k = 0:N_CBPS-1;
    i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
    j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
    intlv_vec = j;
end

function intlv_vec = calc_interleave_vec_11n(cbps, bpsc)
    N_CBPS = cbps;
    N_BPSC = bpsc;
    s = max(N_BPSC/2, 1);

    k = 0:N_CBPS-1;
    i = (4 * N_BPSC) .* mod(k,13) + floor(k/13);
    j = s * floor(i/s) + mod( (i + N_CBPS - floor(13*i/N_CBPS)), s);
    intlv_vec = j;
end

end
