function [signal_bytes, ht_sig_bytes] = calc_phy_preamble(phy_mode, mcs, len)
% phy_mode: 1 = 11a, 2 = 11n
% mcs: integer MCS index
% length: integer payload length, in bytes

if phy_mode == 1
    %802.11a SIGNAL field - 24 bits (IEEE 802.11-2012 18.3.4)
    % [ 0: 3]: RATE (8 4-bit magic numbers, encoded in switch/case below)
    % [    4]: Reserved (1'b0)
    % [ 5:16]: LENGTH
    % [   17]: Parity (XOR of bits 0-16)
    % [18:23]: TAIL (6'b0)

    switch mcs
        case 0 %BPSK 1/2
            SIG_RATE = uint8(11); %1101 -> 1011
        case 1 %BPSK 3/4
            SIG_RATE = uint8(15); %1111 -> 1111
        case 2 %QPSK 1/2
            SIG_RATE = uint8(10); %0101 -> 1010
        case 3 %QPSK 3/4
            SIG_RATE = uint8(14); %0111 -> 1110
        case 4 %16QAM 1/2
            SIG_RATE = uint8(9); %1001 -> 1001
        case 5 %16QAM 3/4
            SIG_RATE = uint8(13); %1011 -> 1101
        case 6' %64QAM 2/3
            SIG_RATE = uint8(8); %0001 -> 1000
        case 7 %64QAM 3/4
            SIG_RATE = uint8(12); %0011 -> 1100
        otherwise
            error('Invalid mod_order or code_rate');
    end

    signal_bytes = calc_signal_field(len, SIG_RATE);
    ht_sig_bytes = [];

    return;

elseif phy_mode == 2
    %802.11n L-SIG field - 24 bits (IEEE 802.11-2012 20.3.9.3.5)
    % [ 0: 3]: RATE (4-bit magic number)
    % [    4]: Reserved (1'b0)
    % [ 5:16]: LENGTH
    % [   17]: Parity (XOR of bits 0-16)
    % [18:23]: TAIL (6'b0)

    %L-SIG is same format as 11a SIGNAL, but with RATE always 6Mb and LENGTH
    % set such that LENGTH/6Mb matches durration of HT transmission
    % Using equation from IEEE 802.11-2012 9.23.4
    % L-SIG.LENGTH = (3*ceil( (TXTIME - 6 - 20) / 4) - 3)
    %  where TXTIME is actual duration of the HT transmission
    % Calculating TXTIME is complicated in general; for now, since our PHY only
    % supports the 1SS, long-GI rates, we'll just use a 8-entry LUT
    mcs_datarates = [6.5 13 19.5 26 39 52 38.5 65]; %Table 20-30
    ht_txtime_usec = 16 + 5*4 + ((2+len)*8 + 6)/mcs_datarates(mcs+1) + 6; %approx, good enough for sim
    LSIG_LENGTH = 3*ceil((ht_txtime_usec - 6 - 20)/4) - 3;
    LSIG_RATE = uint8(11); %6Mb
    
    signal_bytes = calc_signal_field(LSIG_LENGTH, LSIG_RATE);
    
    %802.11n HT-SIG field - 48 bits (IEEE 802.11-2012 20.3.9.4.3)
    %HTSIG1
    % [ 0: 6]: MCS
    % [    7]: CBW 20/40
    % [ 8:23]: HT LENGTH
    %HTSIG2
    % [    0]: Smoothing
    % [    1]: Not Sounding
    % [    2]: Reserved (1'b1)
    % [    3]: Aggregation
    % [ 4: 5]: STBC
    % [    6]: FEC CODING
    % [    7]: SHORT GI
    % [ 8: 9]: Num ext spatial streams
    % [10:17]: Checksum (CRC8 of HTSIG1[0:23] and HTSIG2[0:9])
    % [18:23]: Tail (6'b0)
    HTSIG1_b0 = uint8(bitand(uint8(mcs), hex2dec('3f'))); %CBW is 0 for 20MHz
    HTSIG1_b1 = uint8(bitand(uint16(len), hex2dec('ff')));
    HTSIG1_b2 = uint8(bitshift( bitand(uint16(len), hex2dec('ff00')), -8));

    HTSIG2_b0 = uint8(bin2dec('00000111'));  %Yes smoothing
    HTSIG2_b1 = uint8(0); %0 ESS, CRC filled in by hardware
    HTSIG2_b2 = uint8(0); %CRC filled in by hardware, tail always 0

    ht_sig_bytes = [HTSIG1_b0 HTSIG1_b1 HTSIG1_b2 HTSIG2_b0 HTSIG2_b1 HTSIG2_b2];
end
end

function signal_bytes = calc_signal_field(length, rate)
%Select 12 LSB of length argument
    length_u = bitand(uint16(length), hex2dec('fff'));

    %Isolate the 3 parts of LENGTH that span byte boundaries
    length_2to0 = uint8(bitand(length_u, 7));
    length_10to3 = uint8(bitand(bitshift(length_u, -3), 255));
    length_msb = uint8(bitshift(length_u, -11));

    %Calculate the parity bit
    parity = mod(sum(sum(dec2bin([uint32(length_u) uint32(rate)]) == '1')), 2);

    %Calculate final 3-byte output
    b0 = bitor(rate, bitshift(length_2to0, 5));
    b1 = length_10to3;
    b2 = bitor(length_msb, bitshift(parity, 1));

    %Return as array - sim uses this to replace bytes in init value of simulated packet buffer
    signal_bytes = [b0 b1 b2];
end
