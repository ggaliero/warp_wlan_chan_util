% Mango 802.11 Reference Design
% WLAN PHY Tx Init script
% Copyright 2016 Mango Communications
% Distributed under the Mango Research License:
% http://mangocomm.com/802.11/license

%clear
addpath('./util');
addpath('./mcode_blocks');
addpath('./blackboxes');

%Define sane values for maximum parameter values; these maximums define
% the bit widths of various signals throughout the design. Increasing
% these maximum values will increae resource usage in hardware.
MAX_NUM_BYTES = 4096;
MAX_NUM_SC = 64;
MAX_CP_LEN = 32;
MAX_NUM_SAMPS = 50e3;
MAX_NUM_SYMS = 600;
MAX_DATA_BITS_PER_SYM_PERIOD = 2*260; %64-QAM 5/6 rate N_SS=2
MAX_NCBPS = 312; %Max coded bits per OFDM sym sets size of de-interleave RAM

%%
%Define a few interesting MPDU payloads. These byte sequences start with
% the MAC header, followed by the MAC payload, followed by the FCS. The FCS
% is calculated and inserted by the PHY automatically, so 4 zeros are defined
% as placeholders in each byte sequence below.

%Null data frame:
% Frame Control field: 0x4811
% Duration: 0x2c00 (44 usec)
% Receiver address:    40-d8-55-04-21-4a
% Transmitter address: 40-d8-55-04-21-5a
% Destination address: 40-d8-55-04-21-4a
% Fragment/Seq Num field: 0xf092
% FCS placeholder: 0x00000000
MPDU_Null_Data = sscanf('48 11 2c 00 40 d8 55 04 21 4a 40 d8 55 04 21 5a 40 d8 55 04 21 4a f0 92 00 00 00 00', '%02x');

%Data frames:
% Frame Control field: 0x0801
% Duration: 0x2c00 (44 usec)
% Receiver address:    40-d8-55-04-21-4a
% Transmitter address: 40-d8-55-04-21-5a
% Destination address: 40-d8-55-04-21-6a
% Fragment/Seq Num field: 0xb090
% LLC header: aa-aa-03-00-00-00-08-00
% Arbitrary payload: 00-01-02...0f
% FCS placeholder: 0x00000000

%Short pkt - 16 payload bytes
MPDU_Data_short = sscanf(['08 01 2c 00 40 d8 55 04 21 4a 40 d8 55 04 21 5a 40 d8 55 04 21 6a b0 90 aa aa 03 00 00 00 08 00 ' sprintf('%02x ', [0:15]) ' 00 00 00 00'], '%02x');

%Mid-size pkt - 150 payload bytes
MPDU_Data_mid = sscanf(['08 01 2c 00 40 d8 55 04 21 4a 40 d8 55 04 21 5a 40 d8 55 04 21 6a b0 90 aa aa 03 00 00 00 08 00 ' sprintf('%02x ', mod([1:513], 256)) ' 00 00 00 00'], '%02x');

%Long pkt - 1420 payload bytes
MPDU_Data_long = sscanf(['08 01 2c 00 40 d8 55 04 21 4a 40 d8 55 04 21 5a 40 d8 55 04 21 6a b0 90 aa aa 03 00 00 00 08 00 ' sprintf('%02x ', mod([1:1434], 256)) ' 00 00 00 00'], '%02x');

%ACK frame:
% Frame Control field: 0xd400
% Duration: 0x0000 (0 usec)
% Receiver address: 40-d8-55-04-21-4a
% FCS placeholder: 0x00000000
ControlFrame_ACK = sscanf('d4 00 00 00 40 d8 55 04 21 4a 00 00 00 00', '%02x');

%100-Byte random payload with valid FCS, used with IEEE waveform generator
%wgen_pyld = sscanf(['26 bd 8e b7 f4 13 e7 5c 31 c6 a4 5b ac 95 e9 5c 7c dc 42 10 6d 73 0f 0c 82 8e d9 b2 c8 48 f3 e2 75 fe 92 20 f4 74 f1 fa e0 ae cb 06 55 70 ed 63 1a 9b e7 6e 2b 61 7a df f4 fc b6 20 ba d1 09 8f 31 ea 8b de 1d 77 ce 78 c3 0b dd 25 d2 55 63 23 7c 31 cd 35 f5 75 1e 08 b2 2e 5b a4 67 3f 95 26 a3 54 37 cd'], '%02x');

ltg_pyld = sscanf('08 02 26 00 40 d8 55 04 24 f4 40 d8 55 04 24 b6 40 d8 55 04 24 b6 30 f1 aa aa 03 00 00 00 90 90 13 3f bc 02 00 00 00 00 79 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 96 6d 42 55', '%02x'); 

%Setup params for the sim
% In hardware these params are set per packet by the MAC core/code
% Bypass if running multiple sims via the gen_sim_waveforms script
if(~exist('gen_waveform_mode', 'var'))
    tx_sim = struct();
    tx_sim.MAC_payload = 1:100;
    tx_sim.payload_len = length(tx_sim.MAC_payload);
    tx_sim.PHY_mode = 2; %1=11a, 2=11n
    tx_sim.mcs = 0;
    tx_sim.samp_rate = 40; %Must be in [10 20 40]
    tx_sim.num_pkts = 1;
end

%MCS6, length=57, HTSIG should be:
% 06 39 00 07 E0 00 (per IEEE waveform gen)
% instead, current sim is:
% 06 39 00 07 48 00 - clearly CRC calc is wrong for some MCS/Length values

%Construct the initial value for the simulated packet buffer
% This code is only for sim - CPU Low handles the packet buffer
%  init in the actual design

%Tx packet buffer bytes (zero indexed):
%11a Tx:
% [ 0: 2] SIGNAL field
% [ 3: 4] SERVICE field (always [0 0])
% [ 5:15] Reserved (ignored by Tx PHY logic)
% [16: N] MAC payload
%
%11n Tx:
% [ 0: 2] L-SIG field
% [ 3: 8] HT-SIG
% [ 9:10] SERVICE field (always [0 0])
% [11:15] Reserved (ignored by Tx PHY logic)
% [16: N] MAC payload

PPDU_bytes = zeros(1, MAX_NUM_BYTES);

[SIGNAL, HTSIG] = calc_phy_preamble(tx_sim.PHY_mode, tx_sim.mcs, tx_sim.payload_len);

if(tx_sim.PHY_mode == 1) 
    PPDU_bytes(1:3) = SIGNAL;

    %Insert SERVICE field (always 0)
    PPDU_bytes(4:5) = [0 0];
    
    %Reserved bytes
    PPDU_bytes(6:16) = zeros(1,11);
else
    [SIGNAL, HTSIG] = calc_phy_preamble(2, tx_sim.mcs, tx_sim.payload_len);
    
    %11n mode - L-SIG contains rate=6Mbps, length corresponding to TX_TIME
    PPDU_bytes(1:3) = SIGNAL;

    %HT-SIG field
    PPDU_bytes(4:9) = HTSIG;
    
    %Insert SERVICE field (always 0)
    PPDU_bytes(10:11) = [0 0];
    
    %Reserved bytes
    PPDU_bytes(12:16) = zeros(1, 5);
end

%Insert MAC payload - payload starts at byte index 16
PPDU_bytes(16 + (1:numel(tx_sim.MAC_payload))) = tx_sim.MAC_payload;

%Reshape byte vector to u32 vector, necessary to initialize the 32-bit BRAM in the simulation
PPDU_bytes4 = reshape(PPDU_bytes, 4, numel(PPDU_bytes)/4);
PPDU_words = sum(PPDU_bytes4 .* repmat(2.^[0:8:24]', 1, size(PPDU_bytes4,2)));


%Define the complex-valued sequence for the preamble ROMs
PLCP_Preamble = PLCP_Preamble_gen;
Preamble_IQ = PLCP_Preamble.Preamble_t;

%% Register Init

%Sane initial values for PHY config registers. These values will be overwritten
% at run-time by the software in CPU Low
PHY_CONFIG_NUM_SC = 64;
PHY_CONFIG_CP_LEN = 16;
PHY_CONFIG_FFT_SCALING = bin2dec('101010');
PHY_TX_ACTIVE_EXTENSION = 120;
PHY_TX_RF_EN_EXTENSION = 50;
PHY_TX_RXSIG_INVALID_EXTENSION = 120;

REG_Tx_Timing = ...
    2^0  * (PHY_TX_ACTIVE_EXTENSION) + ... %b[7:0]
    2^8  * (PHY_TX_RF_EN_EXTENSION) + ... %b[15:8]
    2^16 * (PHY_TX_RXSIG_INVALID_EXTENSION) + ... %b[23:16]
    0;

REG_TX_FFT_Config = ...
    2^0  * (PHY_CONFIG_NUM_SC) +...  %b[7:0]
    2^8  * (PHY_CONFIG_CP_LEN) +...  %b[15:8]
    2^24 * (PHY_CONFIG_FFT_SCALING) + ... b[29:24]
    0;

REG_TX_Config = ...
    2^0  * 1 + ... %Force RxEN to radio_controller
    2^1  * 0 + ... %Reset scrambler per pkt
    2^2  * 1 + ... %Enable Tx on RF A
    2^3  * 0 + ... %Enable Tx on RF B
    2^4  * 0 + ... %Enable Tx on RF C
    2^5  * 0 + ... %Enable Tx on RF D
    2^6  * 1 + ... %Use ant mask from MAC hw port
    2^7  * 0 + ... %Use un-delayed TX_ACTIVE signal on debug port
    2^12 * 1 + ... %PHY mode (1=11a, 2=11n), only used for software-initiated Tx
    0;

REG_TX_PKT_BUF_SEL = ...
    2^0  * 0  + ... %b[3:0] pkt buf index
    2^4  * 32 + ... %b[9:4] timestamp insert start byte
    2^10 * 31 + ... %b[15:10] timestamp insert end byte
    2^16 * 0  + ... %b[23:16] pkt buf address offset
    0;

REG_TX_Output_Scaling = (2.0 * 2^12) + (2^16 * 2.0 * 2^12); %UFix16_12 values


%% Cyclic Redundancy Check parameters
CRCPolynomial32 = hex2dec('04c11db7'); %CRC-32, for payload FCS
CRC_Table32 = CRC_table_gen(CRCPolynomial32, 32);

CRCPolynomial8 = hex2dec('07'); %CRC-8, for HT-SIG
CRC_Table8 = CRC_table_gen(CRCPolynomial8, 8);

%%
%Load the MCS info table
[mcs_rom_11ag, mcs_rom_11n] = tx_mcs_info_rom_init();



%% Constellation scaling

%Common scaling for preamble and all constellations that keeps all points within
% numeric range of Fix16_15 values at input to IFFT
ALL_MOD_SCALING = ( 1.0 - (2^-15) )/(7/sqrt(42));
Preamble_IQ_scaled = ALL_MOD_SCALING * Preamble_IQ;

Mod_Constellation_BPSK(1) = ALL_MOD_SCALING * -1;
Mod_Constellation_BPSK(2) = ALL_MOD_SCALING *  1;

Mod_Constellation_QPSK(1) = ALL_MOD_SCALING * -1/sqrt(2);
Mod_Constellation_QPSK(2) = ALL_MOD_SCALING *  1/sqrt(2);

Mod_Constellation_16QAM(1) = ALL_MOD_SCALING * -3/sqrt(10);
Mod_Constellation_16QAM(2) = ALL_MOD_SCALING * -1/sqrt(10);
Mod_Constellation_16QAM(3) = ALL_MOD_SCALING *  3/sqrt(10);
Mod_Constellation_16QAM(4) = ALL_MOD_SCALING *  1/sqrt(10);

Mod_Constellation_64QAM(1) = ALL_MOD_SCALING * -7/sqrt(42);
Mod_Constellation_64QAM(2) = ALL_MOD_SCALING * -5/sqrt(42);
Mod_Constellation_64QAM(3) = ALL_MOD_SCALING * -1/sqrt(42);
Mod_Constellation_64QAM(4) = ALL_MOD_SCALING * -3/sqrt(42);
Mod_Constellation_64QAM(5) = ALL_MOD_SCALING *  7/sqrt(42);
Mod_Constellation_64QAM(6) = ALL_MOD_SCALING *  5/sqrt(42);
Mod_Constellation_64QAM(7) = ALL_MOD_SCALING *  1/sqrt(42);
Mod_Constellation_64QAM(8) = ALL_MOD_SCALING *  3/sqrt(42);


%% HT preamble

%Define the frequency domain 20MHz HT-STF (IEEE 802.11-2012 20.3.9.4.5)
% Array indexes [0 ... 63] correspond to subcarriers [0 1 ... 31 -32 -31 ... -1]
ht_stf_20_fd = zeros(1,64);
ht_stf_20_fd(1:27) = [0 0 0 0 -1-1i 0 0 0 -1-1i 0 0 0 1+1i 0 0 0 1+1i 0 0 0 1+1i 0 0 0 1+1i 0 0];
ht_stf_20_fd(39:64) = [0 0 1+1i 0 0 0 -1-1i 0 0 0 1+1i 0 0 0 -1-1i 0 0 0 -1-1i 0 0 0 1+1i 0 0 0];

%Define the frequency domain 20MHz HT-LTF (IEEE 802.11-2012 20.3.9.4.6)
% Array indexes [0 ... 63] correspond to subcarriers [0 1 ... 31 -32 -31 ... -1]
ht_ltf_20_fd = [0 1 -1 -1 1 1 -1 1 -1 1 -1 -1 -1 -1 -1 1 1 -1 -1 1 -1 1 -1 1 1 1 1 -1 -1 0 0 0 0 0 0 0 1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1 1 1 -1 -1 1 1 -1 1 -1 1 1 1 1];

%Define init vector for HT preamble ROM
% 20MHz HT-STF and HT-LTF are concatenated
% FIXME: HT-STF scaling isn't quite right - it should be a bit bigger
% Need to figure out best way to build that, given IFFT Fix16_15 input type
ht_preamble_rom = [ht_stf_20_fd ALL_MOD_SCALING .* ht_ltf_20_fd];

%% Interleaver and subcarrier maps

% Use util script to generate interleaver address mapping ROM contents
wlan_tx_interleave_rom = wlan_tx_interleave_rom_gen();

%Initialize a vector defining the subcarrier map
% This vector is used by the interleaver control logic to select which
% subcarriers carry data symbols. A value of MAX_NUM_SC tells the hardware to
% not use the subcarrier for data.
sc_ind_data_11a = [2:7 9:21 23:27 39:43 45:57 59:64];
sc_data_sym_map_11a = MAX_NUM_SC*ones(1,MAX_NUM_SC);
sc_data_sym_map_11a(sc_ind_data_11a) = 0:length(sc_ind_data_11a)-1;
sc_data_sym_map_11a_bool = double(sc_data_sym_map_11a ~= MAX_NUM_SC);

sc_ind_data_11n = [2:7 9:21 23:29 37:43 45:57 59:64];
sc_data_sym_map_11n = MAX_NUM_SC*ones(1,MAX_NUM_SC);
sc_data_sym_map_11n(sc_ind_data_11n) = 0:length(sc_ind_data_11n)-1;
sc_data_sym_map_11n_bool = double(sc_data_sym_map_11n ~= MAX_NUM_SC);

