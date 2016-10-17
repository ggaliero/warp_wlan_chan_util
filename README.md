# warp_wlan_chan_util

This is a description for all functionalities that were developed during the research project.


- Mango_802.11_RefDes_v1.3.0
  Consists of the source code of Reference Design 802.11 developed by Mango Communications customized to include several
  functionalities.
  
    1. The wireless channel at which the AP operates may be changed by the UART by pushing c/C on the keyboard. In addition,
     WiFi channels for 5GHz band (Unlicensed) are now accessible. WiFi channels available 1-11, 36, 40, 44, 48.
    2. The RX OFDM entry structured was modified to include the RTP sequence number of a frame generated by a traffic 
     generation application. This was done to calculate the unheard frames by the sniffer when running different kind of 
     traffic flows between a WARP AP and STA using the experimental framework.

- Mango_802.11_RefDes_v1.3.0_sniffer
  Consist of the source code of Reference Design 802.11 developed by Mango Communications. This is built up from the 
  previous software at Mango_802.11_RefDes_v1.3.0.
  It implements a SNIFFER at the AP subproject of Reference Design 802.11. The sniffer presumably captures most of the WiFi
  traffic (802.11a/g amendments) going over the operation channel. It has two operation modes that may be enabled by 
  selecting the correspondent option indicated at the UART menu.
  
            ********************** AP Menu **********************
            [1] - Interactive AP Status
            [2] - Print Queue Status
            [3] - Print all Observed Statistics
            [4] - Scan channel traffic
            [5] - Channel Utilization

            [c/C] - change channel (note: changing channel will
                    purge any associations, forcing stations to
                    join the network again)
            [r/R] - change default unicast rate
            [s]   - change SSID (note: changing SSID will purge)
                    any associations)
            *****************************************************
  
  Both [4] and [5] enable the sniffer mode of the device, for which the WARP board captured all WiFi frames transmitted 
  along 1 second.
  
  [4] - Scan channel traffic: This mode will show a menu with WiFi channels, 1-11, 36, 40, 44 and 48. For each channel it 
  displays the total number of captured MPDUs (Medium Protocol Data Unit) at the last measurement time period and the 
  cumulative number of MPDUs.
  
  The device performs a sweep over all WiFi channels (2.4 GHz, 5GHz bands). Each sweep lasts 1 second. Then it prints the 
  observed statistics. The sweeps start at CH 1 and loops over the rest to start again at CH 1.
        
        ---------------TRAFFIC PER CHANNEL---------------
        -----------(MPDUs/sec)------(Total MPDUs)--------
        CH 1*   |       13       |       22         |
        -------------------------------------------------
        CH 2    |       22       |       51         |
        -------------------------------------------------
        CH 3    |        3       |        5         |
        -------------------------------------------------
        CH 4    |        2       |        2         |
        -------------------------------------------------
        CH 5    |       11       |       24         |
        -------------------------------------------------
        CH 6    |       25       |       50         |
        -------------------------------------------------
        CH 7    |       12       |       23         |
        -------------------------------------------------
        CH 8    |        7       |       19         |
        -------------------------------------------------
        CH 9    |        5       |       15         |
        -------------------------------------------------
        CH 10   |        9       |       19         |
        -------------------------------------------------
        CH 11   |       37       |       54         |
        -------------------------------------------------
        CH 36   |        6       |        6         |
        -------------------------------------------------
        CH 40   |        17      |       17         |
        -------------------------------------------------
        CH 44   |        3       |        3         |
        -------------------------------------------------
        CH 48   |      216       |      216         |
        -------------------------------------------------
        Scanning channel 11...
        
        [r] - reset statistics
        [w] - increase channel
        [s] - decrease channel
        [c] - confirm channel selection
        
  [r] - Resets the cumulative number of captured MPDUs
  [w] and [s] commands increase or decrease the selected channel with the star '*'. When pushing [c] confirm, the current
  operation stops and then [5] - Channel Utilization menu runs over the channel selected by the star '*'. In this example
  case the selected channel would be CH 1.
        
  [5] - Channel Utilization: This mode will loop over 1 second measurements, for which it captures frames transmitted over  
  the AP operation channel. Then data is processed to print a list with the users which are accessing the channel, showing 
  MAC addresses. The following information is shown for each user:
  
    - Distinction between AP (marked with a star '*') or STA. 
    - Number of captured frames over the measurement time.
    - Transmission time for the frames captured at the measurement time. In other words, how much time was the
      the channel occupied.
    - Cumulative number of captured frames.
    - Power level of the last captured frame.
    - Bad FCS frames: This are the frames which contain bit errors. The information cannot be correctly decoded, but still
      the transmission time that those frames occupy the channel is taken into account.
    
  Eventually it displays: 
    - Total number of captured frames over the measurement time (contribution of all users).
    - TX_TIME.      Total transmission time for all captured frames (contribution of all users) including transmission time 
                    for ACKs and time parameters of the MAC protocol, IFS.
    - TIME_ELAPSED. Time elapsed, which means measurement time.
    - CH_UTIL.       Channel Utilization (%)
    
  The channel utilization is calculated as follows:
  
        CH_UTIL = (TX_TIME / TIME_ELAPSED) * 100 [%]
  
  For each Channel Utilization calculation, the wireless channel state is assigned based on the following thresholds:
  
      - IDLE: CH_UTIL < 5%
      - LIGHT TRAFFIC: 5% < CH_UTIL < 60%
      - HEAVY TRAFFIC: CH_UTIL > 60%
      
  Here is a capture of a real measurement in the Channel Utilization menu:
  
        ---------------------------------Channel Utilization---------------------------------
        Channel:      48
        Users/sec:    2
        -------------------------------------------------------------------------------------
        User MAC                MPDUs/sec       Time (us)         Power (dBm)       Tot MPDUs
        80:49:71:13:BA:29       750             232839            -54               5572
        40:D8:55:04:24:B8(*)    7               1665              -49               144
        Bad FCS                 1               196               -49               2
        -------------------------------------------------------------------------------------
        Total                   758             234700
        -------------------------------------------------------------------------------------
        Time elapsed                            10000361
        Channel Utilization                     23.4615 %
        -------------------------------------------------------------------------------------
        (*) Access Point

        [m] - sort users by number of captured MPDUs
        [p] - sort users by received power level
        [r] - reset channel utilization statistics

        Ch State: LIGHT TRAFFIC
        
  [m] - The users are sorted in descendent order or captured MPDUs.
  [p] - The users are sorted in descendent power level values.
  [r] - All values are reset. It is interesting only for cumulative number of captured MPDUs.


- Mango_802.11_RefDes_v1.5.2
  It is a newer version of the Reference Design 802.11 by Mango Communications. It includes the same functionalities added
  into the software Mango_802.11_RefDes_v1.3.0, adapted to the new software architecture.
  
  AP Reference Design Modification:
  
  A new functionality is included in the AP UART menu
  
                    ********************** AP Menu **********************
                    [1]   - Interactive AP Status
                    [2]   - Print Queue Status
                    [3]   - Print all Observed Statistics

                    [a]   - Display Network List
                    [c/C] - Change radio channel
                    [t]   - Toggle radio channel 36/48
                    *****************************************************
  
  [t] - Toggle radio channel 36/48. The AP changes its operation channel from CH 36 to CH 48 and vice versa. If the 
  operation is neither CH 36 nor CH 48, when pushing 't' in the keyboard, the AP will change to CH 36.
  This is implemented because we will have a preferred channel (CH 36) to which allocate all clients, but we will
  move the AP to CH 48, in certain situations. This is only used for demonstration purposes.
  
  STA Reference Design Modification:
  
  In addition, a new funtionality is included into the STA subproject. In case the AP changes its operation channel, the 
  commercial STAs, previously associated, will automatically re-associate to the same AP at the new channel. This may be 
  done following two different approaches:
  
      - Channel Switch Assignment (CSA) protocol, defined by the standard IEEE802.11
      - Establishing a preferred SSID to associate with
      
   For the purpose of this study, the easiest solution was to establish a preferred SSID or BSSID to associate with.
   The WARP STA always tries a startup association with a default SSID defined into the code. If this association is
   successful, then it will retrieve also the AP BSSID.
   
   The STA UART menu can be seen below:
   
                    ********************** Station Menu **********************
                    [1] - Interactive Station Status
                    [2] - Print all Observed Statistics
                    [3] - Enable/Disable automatic association

                    [a] - Display network list
                    [j] - Join a network
                    [s] - Toggle active scan
                    **********************************************************
                    
    [3] - Enable/Disable automatic association was added. This schedules a function to check every two seconds that STA is
    associated with an AP. In case that STA is not associated to any AP, it will perform different tasks based on different
    situations:
    
        - If there was a startup association, the AP BSSID was retrieved. With the SSID, BSSID and the WiFi channel of
          the AP the STA perform a passive scan to re-associate with the AP.
          
        - If there was a startup association, but we do not know to what WiFi channel the AP switched to, therefore the STA 
          performs an active scan to re-associate with the AP.
    
    The demonstration of this functionality with traffic flows can be seen at the link below:
    https://www.youtube.com/watch?v=ZKTEMk8PVzA
    
- Mango_802.11_RefDes_v1.5.2_sniffer
  This software implements exactly the same as Mango_802.11_RefDes_v1.3.0_sniffer adapted to the new software architecture.
  It also extends the user for amendment 802.11n.
  
- Mango_802.11_RefDes_v1.5.2_sniffer_dca
  This software integrates a simplified version of the sniffer at Mango_802.11_RefDes_v1.5.2_sniffer, within the AP. So 
  that, the AP is able to run in the background measurements and CH_UTIL assesment. There is not user distinction.
  
  The AP UART menu is depicted below:
  
                    ****************** AP/Sniffer Menu ******************
                    [1]   - Interactive AP Status
                    [2]   - Print Queue Status
                    [3]   - Print all Observed Statistics
                    [4]   - Scan channel traffic
                    [5]   - Channel Utilization
                    [6]   - Enable/Disable channel allocation
                    
                    [a]   - Display Network List
                    [c/C] - Change radio channel
                    [t]   - Toggle radio channel 36/48
                    *****************************************************
                    
  [5] - Channel Utilization. This will activate the sniffer mode, so in addition to serve as AP, the device will analyze
  the channel traffic conditions, each 1 second. Next it is a capture of a real measurement when option [5] is pushed.
  
                    ----------------------------------------
                    Time elapsed:               1000056 us
                    Channel utilization:        40.24 %
                    Channel:                    36
                    ----------------------------------------
                    CH UTIL: 40, 40, 40
                    Channel Status: CH_TRAFFIC_LIGHT
                    
   For each three Channel Utilization calculations, the wireless channel state is assigned based on the following   
   thresholds:
  
      - IDLE: CH_UTIL < 5%
      - LIGHT TRAFFIC: 5% < CH_UTIL < 60%
      - HEAVY TRAFFIC: CH_UTIL > 60% 
        In this case the AP will automatically migrate to another channel that we know it does not have traffic congestion,
        for this solution, from CH 36 to CH 48 or vice versa.
      
  The channel state does not change unless the traffic is stable during at least 3 seconds. This is to avoid that the AP 
  migrates to another channel in the presence of traffic peaks along short periods of time.
  
- Mango_802.11_RefDes_v1.5.2_sniffer_ctrl
  This software implements exactly the same as Mango_802.11_RefDes_v1.5.2_sniffer, but it has a difference with respect to 
  it.
  
  This sniffer includes also captures of control frames into the channel utilization calculation (ACKs). In the previous 
  design the calculations of this metric always assumed that whenever a DATA frame was captured, the 
  transmission time of an ACK was added to the total time for that the channel is occuppied. On the contrary, this design 
  takes into account the channel occupancy time of a control frame only if it is captured.
  
- Mango_802.11_RefDes_v1.5.2_sniffer_dca_ctrl
  This software is the same as the one defined in Mango_802.11_RefDes_v1.5.2_sniffer_dca, but it also takes into account the
  channel occupancy time of a control frame only if it is captured.
  
  
The main purpose of this project was to build up a WiFi prototype device compatible with 802.11n/a/g which is able to measure the percentage of a WiFi channel being used at a certain time instant. Then the next step was to find an application
to this metric. 



The basic scenario to use the sniffer capabilities is depicted below:


--------------                         ------------
|  WARPv1.3  |                         | WARPv1.3 |
| AP+SNIFFER |        <-------->       |   STA    |
--------------                         ------------

                    --------------
                    |  WARPv1.3  |
                    |  SNIFFER   |
                    --------------
                    
--------------                         --------------
| COMMERCIAL |                         | COMMERCIAL |
|     AP     |        <-------->       |     STA    |
--------------                         --------------                    
