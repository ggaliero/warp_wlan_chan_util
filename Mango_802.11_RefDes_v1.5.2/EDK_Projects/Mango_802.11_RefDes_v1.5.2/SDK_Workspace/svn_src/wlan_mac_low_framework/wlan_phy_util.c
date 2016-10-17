/** @file wlan_phy_util.c
 *  @brief Physical Layer Utility
 *
 *  This contains code for configuring low-level parameters in the PHY and hardware.
 *
 *  @copyright Copyright 2013-2016, Mango Communications. All rights reserved.
 *          Distributed under the Mango Communications Reference Design License
 *              See LICENSE.txt included in the design archive or
 *              at http://mangocomm.com/802.11/license
 *
 *  This file is part of the Mango 802.11 Reference Design (https://mangocomm.com/802.11)
 */
/***************************** Include Files *********************************/

// Xilinx SDK includes
#include "xparameters.h"
#include "stdio.h"
#include "stdarg.h"
#include "xio.h"
#include "string.h"

// WARP includes
#include "w3_userio.h"
#include "w3_ad_controller.h"
#include "w3_clock_controller.h"
#include "w3_iic_eeprom.h"
#include "radio_controller.h"

// WLAN includes
#include "wlan_mac_mailbox_util.h"
#include "wlan_phy_util.h"
#include "wlan_mac_low.h"

// LUT of number of ones in each byte (used to calculate PARITY in SIGNAL)
const u8 ones_in_chars[256] = {
         0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,   // 0x00 - 0x0F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x10 - 0x1F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x20 - 0x2F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x30 - 0x3F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x40 - 0x4F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x50 - 0x5F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x60 - 0x6F
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0x70 - 0x7F
         1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,   // 0x80 - 0x8F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0x90 - 0x9F
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0xA0 - 0xAF
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0xB0 - 0xBF
         2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,   // 0xC0 - 0xCF
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0xD0 - 0xDF
         3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,   // 0xE0 - 0xEF
         4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};  // 0xF0 - 0xFF

/*****************************************************************************/
/**
 * Initialize the WARP v3 node
 *
 * @param   None
 *
 * @return  int              - Status of the command:
 *                                 XST_SUCCESS - Command completed successfully
 *                                 XST_FAILURE - There was an error in the command
 *
 *****************************************************************************/
int w3_node_init() {

    int            ret_val             = XST_SUCCESS;
    int            status;
    u32            clkmod_status;

    // Enable excpetions
    microblaze_enable_exceptions();

    // Initialize w3_clock_controller hardware and AD9512 buffers
    //   NOTE:  The clock initialization will set the clock divider to 2 (for 40MHz clock) to RF A/B AD9963's
    status = clk_init(CLK_BASEADDR, 2);
    if(status != XST_SUCCESS) {
        xil_printf("ERROR: (w3_node_init) Clock initialization failed with error code: %d\n", status);
        ret_val = XST_FAILURE;
    }

    // Check for a clock module and configure clock inputs, outputs and dividers as needed
    clkmod_status = clk_config_read_clkmod_status(CLK_BASEADDR);

    switch(clkmod_status & CM_STATUS_SW) {
        case CM_STATUS_DET_NOCM:
        case CM_STATUS_DET_CMPLL_BYPASS:
            // No clock module - default config from HDL/driver is good as-is
            xil_printf("No clock module detected - selecting on-board clocks\n\n");
        break;

        case CM_STATUS_DET_CMMMCX_CFG_A:
            // CM-MMCX config A:
            //     Samp clk: on-board, RF clk: on-board
            //     Samp MMCX output: 80MHz, RF MMCX output: 80MHz
            xil_printf("CM-MMCX Config A Detected:\n");
            xil_printf("  RF: On-board\n  Samp: On-board\n  MMCX Outputs: Enabled\n\n");

            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
            clk_config_dividers(CLK_BASEADDR, 1, CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR);
        break;

        case CM_STATUS_DET_CMMMCX_CFG_B:
            // CM-MMCX config B:
            //     Samp clk: off-board, RF clk: off-board
            //     Samp MMCX output: 80MHz, RF MMCX output: 80MHz
            xil_printf("CM-MMCX Config B Detected:\n");
            xil_printf("  RF: Off-board\n  Samp: Off-board\n  MMCX Outputs: Enabled\n\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
            clk_config_dividers(CLK_BASEADDR, 1, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
        break;

        case CM_STATUS_DET_CMMMCX_CFG_C:
            // CM-MMCX config C:
            //     Samp clk: off-board, RF clk: off-board
            //     Samp MMCX output: Off, RF MMCX output: Off
            xil_printf("CM-MMCX Config C Detected:\n");
            xil_printf("  RF: Off-board\n  Samp: Off-board\n  MMCX Outputs: Disabled\n\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
            clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_OFF, (CLK_SAMP_OUTSEL_CLKMODHDR | CLK_RFREF_OUTSEL_CLKMODHDR));
        break;

        case CM_STATUS_DET_CMPLL_CFG_A:
            // CM-PLL config A:
            //     Samp clk: clock module PLL
            //     RF clk: on-board
            xil_printf("CM-PLL Config A Detected:\n");
            xil_printf("  RF: On-board\n  Samp: clock module PLL\n");

            // No changes from configuration applied by HDL and clk_init()
        break;

        case CM_STATUS_DET_CMPLL_CFG_B:
            // CM-PLL config B:
            //     Samp clk: clock module PLL
            //     RF clk: clock module PLL
            xil_printf("CM-PLL Config B Detected:\n");
            xil_printf("  RF: clock module PLL\n  Samp: clock module PLL\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
        break;

        case CM_STATUS_DET_CMPLL_CFG_C:
            // CM-PLL config C:
            //     Samp clk: clock module PLL
            //     RF clk: clock module PLL
            xil_printf("CM-PLL Config C Detected:\n");
            xil_printf("  RF: clock module PLL\n  Samp: clock module PLL\n");

            clk_config_input_rf_ref(CLK_BASEADDR, CLK_INSEL_CLKMOD);
        break;

        default:
            // Should be impossible
            xil_printf("ERROR: (w3_node_init) Invalid clock module switch settings! (0x%08x)\n", clkmod_status);
            ret_val = XST_FAILURE;
        break;
    }

#if WLAN_4RF_EN
    // Turn on clocks to FMC
    clk_config_outputs(CLK_BASEADDR, CLK_OUTPUT_ON, (CLK_SAMP_OUTSEL_FMC | CLK_RFREF_OUTSEL_FMC));

    // FMC samp clock divider = 2 (40MHz sampling reference, same as on-board AD9963 ref clk)
    clk_config_dividers(CLK_BASEADDR, 2, CLK_SAMP_OUTSEL_FMC);

    // FMC RF ref clock divider = 2 (40MHz RF reference, same as on-board MAX2829 ref clk)
    clk_config_dividers(CLK_BASEADDR, 2, CLK_RFREF_OUTSEL_FMC);
#endif

    // Initialize the AD9963 ADCs/DACs for on-board RF interfaces
    ad_init(AD_BASEADDR, AD_ALL_RF, 3);

    if(status != XST_SUCCESS) {
        xil_printf("ERROR: (w3_node_init) ADC/DAC initialization failed with error code: %d\n", status);
        ret_val = XST_FAILURE;
    }

    // Initialize the radio_controller core and MAX2829 transceivers for on-board RF interfaces
    status = radio_controller_init(RC_BASEADDR, RC_ALL_RF, 1, 1);

    if(status != XST_SUCCESS) {
        xil_printf("ERROR: (w3_node_init) Radio controller initialization failed with error code: %d\n", status);

        // Comment out the line below to allow the node to boot even if a radio PLL is unlocked
        ret_val = XST_FAILURE;
    }

    // Initialize the EEPROM read/write core
    iic_eeprom_init(EEPROM_BASEADDR, 0x64, XPAR_CPU_ID);

#ifdef WLAN_4RF_EN
    iic_eeprom_init(FMC_EEPROM_BASEADDR, 0x64);
#endif

    // Give the PHY control of the red user LEDs (PHY counts 1-hot on SIGNAL errors)
    //
    // NOTE: Uncommenting this line will make the RED LEDs controlled by hardware.
    //     This will move the LEDs on PHY bad signal events
    //
    // userio_set_ctrlSrc_hw(USERIO_BASEADDR, W3_USERIO_CTRLSRC_LEDS_RED);


    return ret_val;
}


/*****************************************************************************/
/**
 * Initialize the PHY
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_phy_init() {

    // Assert Tx and Rx resets
    REG_SET_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);

    /************ PHY Rx ************/

    // Set the max Tx/Rx packet sizes to 2KB (sane default for standard 802.11a/g links)
    wlan_phy_rx_set_max_pkt_len_kB(2);
    wlan_phy_rx_set_max_pktbuf_addr( PKT_BUF_SIZE - sizeof(rx_frame_info_t) - PHY_RX_PKT_BUF_PHY_HDR_SIZE );

    // Configure the DSSS Rx pipeline
    //  wlan_phy_DSSS_rx_config(code_corr, despread_dly, sfd_timeout)
    wlan_phy_DSSS_rx_config(0x30, 5, 140);

    // Configure the DSSS auto-correlation packet detector
    //  wlan_phy_pktDet_autoCorr_dsss_cfg(corr_thresh, energy_thresh, timeout_ones, timeout_count)
    //
    // To effectively disable DSSS detection with high thresholds, substitute with the following line:
    //     wlan_phy_rx_pktDet_autoCorr_dsss_cfg(0xFF, 0x3FF, 30, 40);
    wlan_phy_rx_pktDet_autoCorr_dsss_cfg(0x60, 400, 30, 40);

    // Configure DSSS Rx to wait for AGC lock, then hold AGC lock until Rx completes or times out
    REG_SET_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_DSSS_RX_AGC_HOLD | WLAN_RX_REG_CFG_DSSS_RX_REQ_AGC));

    // Enable LTS-based CFO correction
    REG_CLEAR_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_CFO_EST_BYPASS);

    // Enable byte order swap for payloads and chan ests
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_BUF_WEN_SWAP);
    REG_CLEAR_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_CHAN_EST_WEN_SWAP);

    // Enable writing OFDM chan ests to Rx pkt buffer
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_RECORD_CHAN_EST);

    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_BUSY_HOLD_PKT_DET);
//    REG_CLEAR_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_BUSY_HOLD_PKT_DET);

    // Block Rx inputs during Tx
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_USE_TX_SIG_BLOCK);

    // Enable HTMF waveform (11n waveform) detection in the PHY Rx
    //  Disabling HTMF detection reverts the PHY to <v1.3 behavior where
    //  every reception is handled as NONHT (11a)
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_ENABLE_HTMF_DET);

    // Keep CCA.BUSY asserted when DSSS Rx is active
    REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_DSSS_ASSERTS_CCA);

    // FFT config
    wlan_phy_rx_set_fft_window_offset(5);

    wlan_phy_rx_set_fft_scaling(5);

    // Set LTS correlation threshold and timeout
    //     1023 disables LTS threshold switch (one threshold worked across SNRs in our testing)
    //     Timeout value is doubled in hardware (350/2 becomes a timeout of 350 sample periods)
    wlan_phy_rx_lts_corr_config(1023 * PHY_RX_RSSI_SUM_LEN, 350/2);

    // LTS correlation thresholds (low NSR, high SNR)
    wlan_phy_rx_lts_corr_thresholds(12500, 12500);

    // Configure RSSI pkt det
    //     RSSI pkt det disabled by default (auto-corr detection worked across SNRs in our testing)
    //     The summing logic realizes a sum of the length specified + 1
     wlan_phy_rx_pktDet_RSSI_cfg( (PHY_RX_RSSI_SUM_LEN-1), ( PHY_RX_RSSI_SUM_LEN * 1023), 1);

    // Configure the default antenna selections as SISO Tx/Rx on RF A
    wlan_rx_config_ant_mode(RX_ANTMODE_SISO_ANTA);

    // Set physical carrier sensing threshold
    wlan_phy_rx_set_cca_thresh(PHY_RX_RSSI_SUM_LEN * wlan_mac_low_rx_power_to_rssi(-62));     // -62dBm from 802.11-2012

    // Set pre-quantizer scaling for decoder inputs
    //  These values were found empirically by vs PER by sweeping scaling and attenuation
    wlan_phy_rx_set_fec_scaling(15, 15, 18, 22);

    // Configure channel estimate capture (64 subcarriers, 4 bytes each)
    //     Chan ests start at sizeof(rx_frame_info) - sizeof(chan_est)
    wlan_phy_rx_pkt_buf_h_est_offset((PHY_RX_PKT_BUF_PHY_HDR_OFFSET - (64*4)));

    //Disable channel estimate smoothing
    wlan_phy_rx_chan_est_smoothing(0xFFF, 0x0);

    /************ PHY Tx ************/

    // De-assert all starts
    REG_CLEAR_BITS(WLAN_TX_REG_START, 0xFFFFFFFF);

    // Set digital scaling of preamble/payload signals before DACs (UFix12_0)
    wlan_phy_tx_set_scaling(0x2000, 0x2000); // Scaling of 2.0

    // Enable the Tx PHY 4-bit TxEn port that captures the MAC's selection of active antennas per Tx
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_USE_MAC_ANT_MASKS);

    // Configure the IFFT scaling and control logic
    //  Current PHY design requires 64 subcarriers, 16-sample cyclic prefix
    wlan_phy_tx_config_fft(0x2A, 64, 16);

    /*********** AGC ***************/

    wlan_agc_config(RX_ANTMODE_SISO_ANTA);

    /************ Wrap Up ************/

    // Set MSB of RSSI_THRESH register to use summed RSSI for debug output
    Xil_Out32(XPAR_WLAN_PHY_RX_MEMMAP_RSSI_THRESH, ((1<<31) | (PHY_RX_RSSI_SUM_LEN * 150)));

    // De-assert resets
    REG_CLEAR_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_RESET);

    // Let PHY Tx take control of radio TXEN/RXEN
    REG_CLEAR_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);
    REG_SET_BITS(WLAN_TX_REG_CFG, WLAN_TX_REG_CFG_SET_RC_RXEN);

    return;
}


/*****************************************************************************/
/**
 * Initialize the Radio Controller
 *
 * @param   None
 *
 * @return  None
 *
 * @note    This function supports both 2RF and 4RF configurations.  Also, there are
 *     sections (ie #if sections) that have been added to assist if clocking changes
 *     are needed.
 *
 *****************************************************************************/
void wlan_radio_init() {

#if 1

	// Setup clocking and filtering (20MSps, 2x interp/decimate in AD9963)
	clk_config_dividers(CLK_BASEADDR, 2, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));
	ad_config_filters(AD_BASEADDR, AD_ALL_RF, 2, 2);
	ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x32, 0x27);
	ad_spi_write(AD_BASEADDR, (AD_ALL_RF), 0x33, 0x08);

#else
    // Setup clocking and filtering:
    //     80MHz ref clk to AD9963
    //     20MSps Tx/Rx at FPGA
    //     80MHz DAC, 4x interp in AD9963
    //     40MHz ADC, 2x decimate in AD9963
    //
    clk_config_dividers(CLK_BASEADDR, 1, (CLK_SAMP_OUTSEL_AD_RFA | CLK_SAMP_OUTSEL_AD_RFB));

    ad_config_clocks(AD_BASEADDR, AD_ALL_RF, AD_DACCLKSRC_EXT, AD_ADCCLKSRC_EXT, AD_ADCCLKDIV_2, AD_DCS_OFF);

    ad_config_filters(AD_BASEADDR, AD_ALL_RF, 4, 2);
#endif

    // Setup all RF interfaces
    radio_controller_TxRxDisable(RC_BASEADDR, RC_ALL_RF);

    radio_controller_apply_TxDCO_calibration(AD_BASEADDR, EEPROM_BASEADDR, (RC_RFA | RC_RFB));
#ifdef WLAN_4RF_EN
    radio_controller_apply_TxDCO_calibration(AD_BASEADDR, FMC_EEPROM_BASEADDR, (RC_RFC | RC_RFD));
#endif

    radio_controller_setCenterFrequency(RC_BASEADDR, RC_ALL_RF, RC_24GHZ, 4);

    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RSSI_HIGH_BW_EN, 0);

    // Filter bandwidths
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXHPF_HIGH_CUTOFF_EN, 1);

#if 0
    // To set the gains manually for all radios:
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_REG);
    radio_controller_setRxHP(RC_BASEADDR, RC_ALL_RF, RC_RXHP_OFF);
    radio_controller_setRxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_SPI);

    // Set Rx gains
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_RF, 1);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_BB, 8);

#else
    // AGC
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_HW);
    radio_controller_setRxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_HW);
#endif

    // Set Tx gains
    //
    // NOTE:  To use software to control the Tx gains, use the following lines:
    //     radio_controller_setTxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_REG); //Used for software control of gains
    //     radio_controller_setTxGainTarget(RC_BASEADDR, RC_ALL_RF, 45);
    //
    radio_controller_setTxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_HW); //Used for hardware control of gains

    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXGAIN_BB, 1);

    // Set misc radio params
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_PADRIVER, 2);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_VGA, 0);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_TXLINEARITY_UPCONV, 0);

    // Set Tx state machine timing
    //
    // NOTE:  radio_controller_setTxDelays(dly_GainRamp, dly_PA, dly_TX, dly_PHY)
    //
    radio_controller_setTxDelays(RC_BASEADDR, 40, 20, 0, TX_RC_PHYSTART_DLY); //240 PA time after 180 PHY time is critical point

    // Configure the radio_controller Tx/Rx enable control sources
    //     The Tx PHY drives a 4-bit TxEn, one bit per RF interface
    //     The Tx PHY drives a 1-bit RxEn, common to all RF interfaces
    //     MAC software should select active Rx interface by changing RFA/RFB RxEn ctrl src between _HW and _REG
    radio_controller_setCtrlSource(RC_BASEADDR, RC_RFA, (RC_REG0_RXEN_CTRLSRC), RC_CTRLSRC_HW);
    radio_controller_setCtrlSource(RC_BASEADDR, RC_RFB, (RC_REG0_RXEN_CTRLSRC), RC_CTRLSRC_REG);

    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFA | RC_RFB), (RC_REG0_TXEN_CTRLSRC), RC_CTRLSRC_HW);

#ifdef WLAN_4RF_EN
    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFC | RC_RFD), (RC_REG0_TXEN_CTRLSRC), RC_CTRLSRC_HW);
    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFC | RC_RFD), (RC_REG0_RXEN_CTRLSRC), RC_CTRLSRC_REG);
#else
    // Disable any hardware control of RFC/RFD
    radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFC | RC_RFD), (RC_REG0_RXEN_CTRLSRC | RC_REG0_TXEN_CTRLSRC), RC_CTRLSRC_REG);
#endif

    return;
}


/*****************************************************************************/
/**
 * Configure the Automatic Gain Controller (AGC)
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void wlan_agc_config(u32 ant_mode) {
    // ant_mode argument allows per-antenna AGC settings, in case FMC module has different
    // response than on-board RF interfaces. Testing so far indicates the settings below
    // work fine for all RF interfaces

    // Post Rx_done reset delays for [rxhp, g_rf, g_bb]
    wlan_agc_set_reset_timing(4, 250, 250);

    // AGC config:
    //     RFG Thresh 3->2, 2->1, Avg_len_sel, V_DB_Adj, Init G_BB
    wlan_agc_set_config((256 - 56), (256 - 37), 0, 6, 24);

    // AGC RSSI->Rx power offsets
    wlan_agc_set_RSSI_pwr_calib(100, 85, 70);

    // AGC timing: start_dco, en_iir_filt
    wlan_agc_set_DCO_timing(100, (100 + 34));

    // AGC target output power (log scale)
    wlan_agc_set_target((64 - 16));

#if 0
    // To set the gains manually:

    //xil_printf("Switching to MGC\n");
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXHP_CTRLSRC, RC_CTRLSRC_REG);
    radio_controller_setRxHP(RC_BASEADDR, RC_ALL_RF, RC_RXHP_OFF);
    radio_controller_setRxGainSource(RC_BASEADDR, RC_ALL_RF, RC_GAINSRC_SPI);

    // Set Rx gains
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_RF, 2);
    radio_controller_setRadioParam(RC_BASEADDR, RC_ALL_RF, RC_PARAMID_RXGAIN_BB, 8);
#endif



    return;
}


/*****************************************************************************/
/**
 * Configure the Rx Antenna Mode
 *
 * @param   ant_mode         - Antenna mode to set
 *
 * @return  None
 *
 * @note    There is no corresponding wlan_tx_config_ant_mode() because the transmit
 *     antenna is set by the MAC software (ie mac_sw -> mac_hw -> phy_tx) for every packet
 *
 *****************************************************************************/
void wlan_rx_config_ant_mode(u32 ant_mode) {

    // Hold the Rx PHY in reset before changing any pkt det or radio enables
    REG_SET_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);

    // Disable all Rx modes first; selectively re-enabled in switch below
    REG_CLEAR_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D |
                                     WLAN_RX_REG_CFG_SWITCHING_DIV_EN |
                                     WLAN_RX_REG_CFG_PKT_DET_EN_EXT |
                                     WLAN_RX_REG_CFG_ANT_SEL_MASK));

    // Disable PHY control of all RF interfaces - selected interfaces to re-enabled below
    radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_REG);

    // Set the antenna mode
    //
    // For each antenna mode:
    //   - Enable packet detection
    //   - Select I/Q stream for Rx PHY
    //   - Give PHY control of Tx/Rx status
    //   - Configure AGC
    //
    switch (ant_mode) {
        case RX_ANTMODE_SISO_ANTA:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTA);
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFA, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_ANTA);
        break;

        case RX_ANTMODE_SISO_ANTB:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTB);
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFB, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_ANTB);
        break;

        case RX_ANTMODE_SISO_ANTC:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTC);
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFC, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_ANTC);
        break;

        case RX_ANTMODE_SISO_ANTD:
            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTD);
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFD, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_ANTD);
        break;

        case RX_ANTMODE_SISO_SELDIV_2ANT:
            REG_SET_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B | WLAN_RX_REG_CFG_SWITCHING_DIV_EN));
            // By enabling the antenna switching, the I/Q stream is automatically switched for Rx PHY
            radio_controller_setCtrlSource(RC_BASEADDR, (RC_RFA | RC_RFB), RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_SELDIV_2ANT);
        break;

        case RX_ANTMODE_SISO_SELDIV_4ANT:
            REG_SET_BITS(WLAN_RX_REG_CFG, (WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_B | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_C | WLAN_RX_REG_CFG_PKT_DET_EN_ANT_D | WLAN_RX_REG_CFG_SWITCHING_DIV_EN));
            // By enabling the antenna switching, the I/Q stream is automatically switched for Rx PHY
            radio_controller_setCtrlSource(RC_BASEADDR, RC_ALL_RF, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_SELDIV_4ANT);
        break;

        default:
            // Default to SISO on A if user provides invalid mode
            xil_printf("wlan_rx_config_ant_mode ERROR: Invalid Mode - Defaulting to SISO on A\n");

            REG_SET_BITS(WLAN_RX_REG_CFG, WLAN_RX_REG_CFG_PKT_DET_EN_ANT_A);
            wlan_phy_select_rx_antenna(RX_ANTMODE_SISO_ANTA);
            radio_controller_setCtrlSource(RC_BASEADDR, RC_RFA, RC_REG0_RXEN_CTRLSRC, RC_CTRLSRC_HW);
            wlan_agc_config(RX_ANTMODE_SISO_ANTA);
        break;
    }

    // Release the PHY Rx reset
    REG_CLEAR_BITS(WLAN_RX_REG_CTRL, WLAN_RX_REG_CTRL_RESET);

    return;
}

/*****************************************************************************/
/**
 * Calculates the PHY preamble (SIGNAL for 11a, L-SIG/HT-SIG for 11n) and writes
 * the preamble bytes to the specified packet buffer. The PHY preamble must be
 * written to the packet buffer for each transmission.
 *
 * @param   pkt_buf          - Packet buffer number
 * @param   phy_mode         - Sets waveform format; must be PHY_MODE_NON_HT (11a) or PHY_MODE_HTMF (11n)
 * @param   mcs              - MCS (modulation/coding scheme) index
 * @param   length           - Length of the packet being transmitted
 *
 * @return  None
 *
 *****************************************************************************/
void write_phy_preamble(u8 pkt_buf, u8 phy_mode, u8 mcs, u16 length) {

	u8* htsig_ptr;
	u16 lsig_length;

	// RATE field values for SIGNAL/L-SIG in PHY preamble (IEEE 802.11-2012 18.3.4.2)
	//  RATE field in SIGNAL/L-SIG is one of 8 4-bit values indicating modulation scheme and coding rate
	//  For 11a (NONHT) transmissions we map mcs index to SIGNAL.RATE directly
	//  For 11n (HTMF) transmissions the L-SIG.RATE field is always the lowest (BSPK 1/2)
	const u8 sig_rate_vals[8] = {0xB, 0xF, 0xA, 0xE, 0x9, 0xD, 0x8, 0xC};

	if((phy_mode & PHY_MODE_NONHT) == PHY_MODE_NONHT) {
		//11a mode - write SIGNAL (3 bytes)

		//Zero-out any stale header, also properly sets SERVICE and reserved bytes to 0
		bzero((u32*)(TX_PKT_BUF_TO_ADDR(pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET), PHY_TX_PKT_BUF_PHY_HDR_SIZE);

		//Set SIGNAL with actual rate/length
	    Xil_Out32((u32*)(TX_PKT_BUF_TO_ADDR(pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET), WLAN_TX_SIGNAL_CALC(sig_rate_vals[mcs], length));

	} else if((phy_mode & PHY_MODE_HTMF) == PHY_MODE_HTMF) {
		//11n mode - write L-SIG (3 bytes) and HT-SIG (6 bytes)

		//Zero-out any stale header, also properly sets SERVICE, reserved bytes, and auto-filled bytes of HT-SIG to 0
		bzero((u32*)(TX_PKT_BUF_TO_ADDR(pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET), PHY_TX_PKT_BUF_PHY_HDR_SIZE);

	    // L-SIG is same format as 11a SIGNAL, but with RATE always 6Mb and LENGTH
	    //  set such that LENGTH/6Mb matches duration of HT transmission
	    //  Using equation from IEEE 802.11-2012 9.23.4
	    //   L-SIG.LENGTH = (3*ceil( (TXTIME - 6 - 20) / 4) - 3)
	    //  where TXTIME is actual duration of the HT transmission
		//   The ceil((TXTIME - 6 - 20)/4) term represents the number of OFDM symbols after the L-SIG symbol
		//  (-6-20) are (T_EXT-T_NONHT_PREAMBLE); (-3) accounts for service/tail

		// Calc (3*(num_payload_syms+num_ht_preamble_syms) = (3*(num_payload_syms+4))
		lsig_length = 3*wlan_ofdm_calc_num_payload_syms(length, mcs, phy_mode) + 12;

		Xil_Out32((u32*)(TX_PKT_BUF_TO_ADDR(pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET), WLAN_TX_SIGNAL_CALC(sig_rate_vals[0], lsig_length));

		//Assign pointer to first byte of HTSIG (PHY header base + 3 for sizeof(L-SIG))
	    htsig_ptr = (u8*)(TX_PKT_BUF_TO_ADDR(pkt_buf) + PHY_TX_PKT_BUF_PHY_HDR_OFFSET + 3);

		//Set HTSIG bytes
	    // PHY logic fills in bytes 4 and 5; ok to ignore here
	    // Going byte-by-byte for now - maybe optimize to (unaligned) 32-bit write later
	    htsig_ptr[0] = mcs & 0x3F; //MSB is channel bandwidth; 0=20MHz
	    htsig_ptr[1] = length & 0xFF;
	    htsig_ptr[2] = (length >> 8) & 0xFF;
	    htsig_ptr[3] = 0x7; //smoothing=1, not-sounding=1, reserved=1, aggregation=STBC=FEC=short_GI=0
	}


    return;
}

/*****************************************************************************/
/**
 * Transmission debug methods
 *
 * @param   Variable
 *
 * @return  Variable
 *
 * @note    These function is for debug use only.
 *
 *****************************************************************************/
inline void wlan_tx_start() {
    // Start the PHY Tx immediately; this bypasses the mac_hw MPDU Tx state machine
    //     This should only be used for debug - normal transmissions should use mac_hw
    //
    REG_SET_BITS(WLAN_TX_REG_START, WLAN_TX_REG_START_VIA_RC);
    REG_CLEAR_BITS(WLAN_TX_REG_START, WLAN_TX_REG_START_VIA_RC);

    return;
}

/*****************************************************************************/
/**
 * Calculates duration of an OFDM waveform.
 *
 * This function assumes every OFDM symbol is the same duration. Duration
 * calculation for short guard interval (SHORT_GI) waveforms is not supported.
 *
 * @param   length         - Length of MAC payload in bytes
 * @param   mcs            - MCS index
 * @param   phy_mode       - PHY waveform mode - either PHY_MODE_NONHT (11a/g) or PHY_MODE_HTMF (11n)
 * @param   phy_samp_rate  - PHY sampling rate - one of (PHY_10M, PHY_20M, PHY_40M)
 *
 * @return  u16              - Duration of transmission in microseconds
 *                             (See IEEE 802.11-2012 18.4.3 and 20.4.3)
 *****************************************************************************/
inline u16 wlan_ofdm_calc_txtime(u16 length, u8 mcs, u8 phy_mode, phy_samp_rate_t phy_samp_rate) {

    u16 num_ht_preamble_syms, num_payload_syms;

    u16 t_preamble;
    u16 t_sym;

    // Note: the t_ext signal extension represent the value used in the standard, which in turn
    // is the value expected by other commercial WLAN devices. By default, the signal extensions
    // programmed into the PHY match this value.
    u16 t_ext = 6;

    // Set OFDM symbol duration in microseconds; only depends on PHY sampling rate
    switch(phy_samp_rate) {
        case PHY_40M:
            t_sym = 2;
        break;

        default:
        case PHY_20M:
            t_sym = 4;
        break;

        case PHY_10M:
            t_sym = 8;
        break;
    }

    // PHY preamble common to NONHT and HTMF waveforms consists of 5 OFDM symbols
    //  4 symbols for STF/LTF
    //  1 symbol for SIGNAL/L-SIG
    t_preamble = 5*t_sym;

    // Only HTMF waveforms have HT-SIG, HT-STF and HT-LTF symbols
    if(phy_mode == PHY_MODE_HTMF) {
    	num_ht_preamble_syms = 4;
    } else {
    	num_ht_preamble_syms = 0;
    }

    num_payload_syms = wlan_ofdm_calc_num_payload_syms(length, mcs, phy_mode);

    // Sum each duration and return
    return (t_preamble + (t_sym * (num_ht_preamble_syms + num_payload_syms)) + t_ext);
}


/*****************************************************************************/
/**
 * Calculates number of payload OFDM symbols in a packet. Implements
 *  ceil(payload_length_bits / num_bits_per_ofdm_sym)
 *   where num_bits_per_ofdm_sym is a function of the MCS index and PHY mode
 *
 * @param   length         - Length of MAC payload in bytes
 * @param   mcs            - MCS index
 * @param   phy_mode       - PHY waveform mode - either PHY_MODE_NONHT (11a/g) or PHY_MODE_HTMF (11n)
 *
 * @return  u16            - Number of OFDM symbols
 *****************************************************************************/
inline u16 wlan_ofdm_calc_num_payload_syms(u16 length, u8 mcs, u8 phy_mode) {
	u16 num_payload_syms;
	u32 num_payload_bits;
	u16 n_dbps;

    // Payload consists of:
    //  16-bit SERVICE field
    //  'length' byte MAC payload
    //  6-bit TAIL field
    num_payload_bits = 16 + (8 * length) + 6;

    // Num payload syms is ceil(num_payload_bits / N_DATA_BITS_PER_SYM). The ceil()
    //  operation implicitly accounts for any PAD bits in the waveform. The PHY inserts
    //  PAD bits to fill the final OFDM symbol. A waveform always consists of an integer
    //  number of OFDM symbols, so the actual number of PAD bits is irrelevant here
    n_dbps = wlan_mac_low_mcs_to_n_dbps(mcs, phy_mode);
    num_payload_syms = num_payload_bits / n_dbps;

	// Apply ceil()
	//  Integer div above implies floor(); increment result if floor() changed result
	if( (n_dbps * num_payload_syms) != num_payload_bits ) {
		num_payload_syms++;
	}

     return num_payload_syms;
}
