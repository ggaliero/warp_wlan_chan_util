Search.setIndex({envversion:47,filenames:["device","index","log_entry_types","log_overview","log_util","log_util_hdf","ltg","node","node_ap","node_ibss","node_sta","wlan_exp","wlan_exp_log","wlan_exp_util"],objects:{"wlan_exp.device":{WlanDevice:[0,0,1,""]},"wlan_exp.log.entry_types":{WlanExpLogEntryType:[2,0,1,""],np_array_add_fields:[2,2,1,""]},"wlan_exp.log.entry_types.WlanExpLogEntryType":{add_gen_numpy_array_callback:[2,1,1,""],append_field_defs:[2,1,1,""],deserialize:[2,1,1,""],get_entry_type_id:[2,1,1,""],get_field_names:[2,1,1,""],get_field_struct_formats:[2,1,1,""],modify_field_def:[2,1,1,""],serialize:[2,1,1,""]},"wlan_exp.log.util":{calc_next_entry_offset:[4,2,1,""],calc_tx_time:[4,2,1,""],convert_datetime_to_log_time_str:[4,2,1,""],convert_log_time_str_to_datetime:[4,2,1,""],filter_log_index:[4,2,1,""],find_overlapping_tx_low:[4,2,1,""],gen_raw_log_index:[4,2,1,""],get_now_as_log_time_str:[4,2,1,""],log_data_to_np_arrays:[4,2,1,""],merge_log_indexes:[4,2,1,""],overwrite_entries_with_null_entry:[4,2,1,""],overwrite_payloads:[4,2,1,""],print_log_index_summary:[4,2,1,""]},"wlan_exp.log.util_hdf":{HDF5LogContainer:[5,0,1,""],hdf5_close_file:[5,2,1,""],hdf5_open_file:[5,2,1,""],hdf5_to_attr_dict:[5,2,1,""],hdf5_to_log_data:[5,2,1,""],hdf5_to_log_index:[5,2,1,""],log_data_to_hdf5:[5,2,1,""],np_arrays_to_hdf5:[5,2,1,""]},"wlan_exp.log.util_hdf.HDF5LogContainer":{get_attr_dict:[5,1,1,""],get_log_data:[5,1,1,""],get_log_data_size:[5,1,1,""],get_log_index:[5,1,1,""],is_valid:[5,1,1,""],write_attr_dict:[5,1,1,""],write_log_data:[5,1,1,""],write_log_index:[5,1,1,""]},"wlan_exp.ltg":{FlowConfig:[6,0,1,""],FlowConfigAllAssocCBR:[6,0,1,""],FlowConfigCBR:[6,0,1,""],FlowConfigRandomRandom:[6,0,1,""],Payload:[6,0,1,""],PayloadAllAssocFixed:[6,0,1,""],PayloadFixed:[6,0,1,""],PayloadUniformRandom:[6,0,1,""],Schedule:[6,0,1,""],SchedulePeriodic:[6,0,1,""],ScheduleUniformRandom:[6,0,1,""]},"wlan_exp.node":{WlanExpNode:[7,0,1,""]},"wlan_exp.node.WlanExpNode":{configure_bss:[7,1,1,""],configure_pkt_det_min_power:[7,1,1,""],enable_beacon_mac_time_update:[7,1,1,""],enable_dsss:[7,1,1,""],get_bss_info:[7,1,1,""],get_mac_time:[7,1,1,""],get_network_list:[7,1,1,""],get_rx_ant_mode:[7,1,1,""],get_station_info:[7,1,1,""],get_system_time:[7,1,1,""],get_tx_ant_mode_multicast_data:[7,1,1,""],get_tx_ant_mode_multicast_mgmt:[7,1,1,""],get_tx_ant_mode_unicast:[7,1,1,""],get_tx_power:[7,1,1,""],get_tx_power_multicast_data:[7,1,1,""],get_tx_power_multicast_mgmt:[7,1,1,""],get_tx_power_unicast:[7,1,1,""],get_tx_rate_multicast_data:[7,1,1,""],get_tx_rate_multicast_mgmt:[7,1,1,""],get_tx_rate_unicast:[7,1,1,""],get_txrx_counts:[7,1,1,""],get_wlan_mac_address:[7,1,1,""],identify:[7,1,1,""],is_scanning:[7,1,1,""],log_configure:[7,1,1,""],log_get:[7,1,1,""],log_get_all_new:[7,1,1,""],log_get_capacity:[7,1,1,""],log_get_flags:[7,1,1,""],log_get_indexes:[7,1,1,""],log_get_size:[7,1,1,""],log_is_full:[7,1,1,""],log_write_exp_info:[7,1,1,""],log_write_time:[7,1,1,""],ltg_configure:[7,1,1,""],ltg_get_status:[7,1,1,""],ltg_remove:[7,1,1,""],ltg_remove_all:[7,1,1,""],ltg_start:[7,1,1,""],ltg_start_all:[7,1,1,""],ltg_stop:[7,1,1,""],ltg_stop_all:[7,1,1,""],ping:[7,1,1,""],queue_tx_data_purge_all:[7,1,1,""],reset:[7,1,1,""],reset_all:[7,1,1,""],send_user_command:[7,1,1,""],set_dcf_param:[7,1,1,""],set_low_param:[7,1,1,""],set_low_to_high_rx_filter:[7,1,1,""],set_mac_time:[7,1,1,""],set_phy_samp_rate:[7,1,1,""],set_print_level:[7,1,1,""],set_radio_channel:[7,1,1,""],set_random_seed:[7,1,1,""],set_rx_ant_mode:[7,1,1,""],set_scan_parameters:[7,1,1,""],set_tx_ant_mode_multicast_data:[7,1,1,""],set_tx_ant_mode_multicast_mgmt:[7,1,1,""],set_tx_ant_mode_unicast:[7,1,1,""],set_tx_power:[7,1,1,""],set_tx_power_ctrl:[7,1,1,""],set_tx_power_multicast_data:[7,1,1,""],set_tx_power_multicast_mgmt:[7,1,1,""],set_tx_power_unicast:[7,1,1,""],set_tx_rate_multicast_data:[7,1,1,""],set_tx_rate_multicast_mgmt:[7,1,1,""],set_tx_rate_unicast:[7,1,1,""],start_network_scan:[7,1,1,""],stop_network_scan:[7,1,1,""]},"wlan_exp.node_ap.WlanExpNodeAp":{add_association:[8,1,1,""],configure_bss:[8,1,1,""],disassociate:[8,1,1,""],disassociate_all:[8,1,1,""],enable_DTIM_multicast_buffering:[8,1,1,""],enable_beacon_mac_time_update:[8,1,1,""],is_associated:[8,1,1,""],set_authentication_address_filter:[8,1,1,""]},"wlan_exp.node_ibss.WlanExpNodeIBSS":{configure_bss:[9,1,1,""],get_txrx_counts:[9,1,1,""]},"wlan_exp.node_sta.WlanExpNodeSta":{configure_bss:[10,1,1,""],disassociate:[10,1,1,""],is_associated:[10,1,1,""],is_joining:[10,1,1,""],join_network:[10,1,1,""],set_aid:[10,1,1,""]},"wlan_exp.util":{broadcast_cmd_set_mac_time:[13,2,1,""],broadcast_cmd_write_exp_info_to_logs:[13,2,1,""],broadcast_cmd_write_time_to_logs:[13,2,1,""],byte_str_to_mac_addr:[13,2,1,""],channel_info_to_str:[13,2,1,""],check_bss_membership:[13,2,1,""],create_locally_administered_bssid:[13,2,1,""],debug_here:[13,2,1,""],filter_nodes:[13,2,1,""],get_channel_info:[13,2,1,""],get_rate_info:[13,2,1,""],init_nodes:[13,2,1,""],int_to_ip:[13,2,1,""],ip_to_int:[13,2,1,""],mac_addr_broadcast:[13,3,1,""],mac_addr_desc:[13,2,1,""],mac_addr_desc_map:[13,3,1,""],mac_addr_local_mask:[13,3,1,""],mac_addr_mcast_mask:[13,3,1,""],mac_addr_to_byte_str:[13,2,1,""],mac_addr_to_str:[13,2,1,""],node_type_to_str:[13,2,1,""],phy_modes:[13,4,1,""],rate_info_to_str:[13,2,1,""],sn_to_str:[13,2,1,""],str_to_mac_addr:[13,2,1,""],uart_print_levels:[13,4,1,""],wlan_channels:[13,4,1,""],wlan_rx_ant_modes:[13,3,1,""],wlan_tx_ant_modes:[13,3,1,""]}},objnames:{"0":["py","class","Python class"],"1":["py","method","Python method"],"2":["py","function","Python function"],"3":["py","attribute","Python attribute"],"4":["py","data","Python data"]},objtypes:{"0":"py:class","1":"py:method","2":"py:function","3":"py:attribute","4":"py:data"},terms:{"0x00":7,"0x000000000000":[0,8],"0x0001":7,"0x0002":7,"0x0004":7,"0x0008":7,"0x0010":7,"0x01":7,"0x012345000000":8,"0x0123456789ab":[0,8],"0x02":7,"0x400000000000":8,"0x40d855042000":8,"0xff0000000000":8,"0xffffff000000":8,"0xfffffffff000":8,"0xffffffffffff":8,"11a":13,"11n":[0,7,13],"20mhz":13,"4ghz":13,"5ghz":13,"__eq__":3,"__hash__":3,"__repr__":3,"_info":5,"boolean":[3,8,9,10,13],"byte":[3,4,5,6,7,9,13],"case":[3,4,7,8,13],"char":7,"default":[2,3,4,5,7,8,9,10,13],"final":0,"float":[6,7,10,13],"function":1,"import":[2,3,7,13],"int":[0,2,3,4,5,6,7,8,9,10,13],"long":[3,4,7,13],"new":[2,3,5,7,8],"null":[4,7,10,13],"return":[2,3,4,5,7,8,9,10,13],"short":[7,13],"static":[3,7,9],"super":4,"true":[2,3,5,7,8,9,10,13],"try":[7,10,13],"while":[0,3,7],abil:7,abl:[4,7],about:[3,7,13],absolut:4,accept:13,access:[0,1,3,7],account:4,ack:7,across:13,activ:[7,8,10],actual:[3,6,13],add:[2,5,7,8,13],add_associ:8,add_gen_numpy_array_callback:2,addit:[1,3,7,13],addr_mask:13,addr_valu:13,address:[0,1,2,6,7,8,9,10,11],adjac:2,adjust:[7,10],administ:[9,13],administerd:13,adress:7,affect:[7,8,10],after:[2,3,4,10],aid:10,air:8,align:7,all:[0,3,4,5,6,7,8,9,10,13],all_mpdu:7,alloc:7,allow:[3,7,8,10],alomst:3,alreadi:[5,7,13],also:[0,3,4,7,8,10,13],alter:7,alwai:[3,4,7],amount:[4,7],analysi:[3,7],ani:[3,4,5,7,8,9,10,13],anoth:3,ant_mod:7,antenna:1,appear:5,append:[0,5],append_field_def:2,applic:[3,8,9,10],approach:3,appropri:4,approx:7,arbitrari:[3,7,13],archiv:1,area:3,arg:7,argument:[3,4,7,13],arrai:[1,2],assign:10,assocait:8,associ:[0,1,6],assum:[3,4,7,8],attain:8,attempt:[7,9,10],attr_dict:5,attribut:[3,5,7],authent:[8,10],authenticaion:8,auto:7,auto_start:7,automat:[3,4,7,10],avail:7,avoid:2,await:7,backlog:7,band:[7,13],banner:13,base:[1,4,6],basic:[3,4,7],beacon:[7,8,9,10],beacon_interv:[7,8,9,10],becaus:4,been:[3,4,7,13],befor:[7,10,13],begin:4,behavior:[1,3,4,5,7,11],below:[3,7,8,9,10],besid:4,between:[6,7],bin:5,binari:[2,4,5],bit:[0,3,6,7,8,9,10,13],bitstream:13,bitwis:13,blank:7,blink:7,blue:3,board:13,bool:[0,2,3,5,7,8,9,10,13],boot:7,both:[4,7],bound:7,boundari:[4,7],broadcast:[7,13],broadcast_cmd_set_mac_tim:13,broadcast_cmd_write_exp_info_to_log:13,broadcast_cmd_write_time_to_log:13,bss:[7,8,9,10,13],bss_info:7,bssid:[7,8,9,10,13],bssinfo:7,buf:2,buffer:[2,7,8],build:[3,7],built:[3,13],bw20:7,bw40_sec_abov:7,bw40_sec_below:7,bypass:8,byte_offset:4,byte_str_to_mac_addr:13,bytearrai:[2,3,7],calc_next_entry_offset:4,calc_tx_tim:4,calcsiz:4,calcul:[3,4],call:[3,4,5,7],callback:2,can:[0,2,3,4,5,7,8,10,11,13],cannot:4,capabl:[0,7],capac:7,carrier:7,caus:[7,10],caveat:3,cbr:7,center:[7,13],chang:[3,4,7,10],channel:[1,7,8,9,10,11],channel_info:13,channel_info_to_str:13,channel_list:7,channel_typ:7,charact:[8,9,10],check:[4,5,7,8,10,13],check_bss_membership:13,checksum:7,chosen:[7,13],clariti:3,client:[0,8],clobber:2,close:[5,7,13],cmd:7,cmd_id:7,cmd_param_low_param_:7,code:[1,3,4,6,7,10,13],collect:2,colloqui:[7,8,9,10],colon:[9,10,13],com:1,combin:4,come:7,command:[3,7,8,13],commmand:7,common:[1,4],commun:[1,11,13],compar:13,complet:[7,10],complex:3,compon:1,compos:3,compress:5,concept:3,condit:13,config:1,configur:1,configure_bss:[7,8,9,10],configure_pkt_det_min_pow:7,connect:[0,7],consid:[3,7],constant:[6,7,13],construct:3,consum:[4,7],contain:[0,1,2,3,4],content:[1,3,7,11,12],continu:[4,10],control:[0,1,7,11,13],conveni:2,convent:7,convert:[3,4,7,13],convert_datetime_to_log_time_str:4,convert_log_time_str_to_datetim:4,copi:[2,3,5],copyright:1,correspond:[3,4,7,10,13],cosmet:10,could:7,count:1,counts_dictionari:[7,9],cours:3,cpu:[6,7,8,9,10,13],creat:[0,3,4,5,6,7,8,9,13],create_locally_administered_bssid:[9,13],creation:[3,6,7],ctrl:7,curr_assoc:7,current:[4,5,6,7,8,9,10,13],custom:[7,13],cw_exp_max:7,cw_exp_min:7,cycl:7,data:[1,2],data_num_rx_byt:[7,9],data_num_rx_bytes_tot:7,data_num_rx_packet:[7,9],data_num_tx_attempt:[7,9],data_num_tx_bytes_success:[7,9],data_num_tx_bytes_tot:[7,9],data_num_tx_packets_success:[7,9],data_num_tx_packets_tot:[7,9],dataset:[3,5],datatyp:3,date:[3,5],datetim:4,datetime_obj:4,dbm:7,dcf:[7,13],deal:7,deauthent:7,debbug:13,debug:[7,13],debug_her:13,dedic:7,defalt:13,defin:[0,2,3,4,5,6,7,8,13],definit:[0,1,2,3,11],deivc:6,delimit:[3,4,9,10,13],deliveri:8,depend:[3,4,7,10,13],desc:13,desc_map:13,descprit:13,describ:[1,2,3,7,8,9,10,12,13],descript:[0,7,9,13],descritpion:13,deseri:2,design:0,desir:4,dest_addr:[6,7],dest_index:4,destim:7,destin:[4,6,7],destroi:[5,7],destruct:7,detail:[1,3,7,12,13],detector:7,determin:[4,7,10,13],device_list:[7,8,9],device_typ:[0,7],dict:[2,4,5,13],dictionari:[2,3,4,5,7,9,13],differ:[3,4,9,13],differenti:7,digit:[7,13],dimens:[3,4],directli:[3,7],disabl:[3,7,8,9,10],disable_timeout:8,disassoci:[7,8,10],disassociate_al:8,discard:7,discov:7,disk:5,displai:[7,10,13],disrupt:7,distinct:3,distribut:1,distrupt:7,doc:[2,6],doc_str:2,docs_onli:2,doe:[3,4,5,7,8,10,13],done:3,dot:13,downstream:3,dram:[3,7],drop:7,dsss:[7,13],dtim:8,dtype:[2,3],duplic:7,durat:[4,6],dure:[7,10,13],dynam:13,each:[0,2,3,4,5,7,8,9,10,13],easier:[3,7],eeprom:7,effect:7,effici:3,either:[4,5,7,8,9,10,13],element:3,empti:[3,4,7,13],enabl:[1,7,8],enable_beacon_mac_time_upd:[7,8],enable_dsss:7,enable_dtim_multicast_buff:8,encod:[3,13],end:[3,4,7],enqueu:[6,7],ensur:7,entir:7,entri:1,entry_head:4,entry_length:4,entry_list:2,entry_rx_dsss:3,entry_rx_ofdm:3,entry_tx_high:3,entry_typ:[3,4],entry_type_id:[2,3,4],entry_type_rx_dsss:3,entry_type_rx_ofdm:3,entry_type_tx_high:3,environ:7,equal:[4,8],equival:[7,13],error:[7,13],especi:3,ess:7,etc:[3,4],eth_b:7,ethernet:7,even:[3,8,13],event:1,everi:[3,4,6,7,8],evolv:3,exceed:10,exclud:[3,4,7],exclus:7,execut:2,exhaust:3,exist:[0,4,5,7,9],exp:[1,11],exp_info:13,expens:3,experi:0,experiment:3,experimentor:[7,13],explictli:7,expon:7,express:0,extend:[2,7],extern:7,extra:[3,5],extract:[5,7],extrem:13,fact:4,fail:[7,10,13],failur:10,fals:[0,2,3,4,5,7,8,9,10,13],fast:[3,6],faster:3,few:[3,7],field:[2,3,4,7,9,13],field_desc:2,field_info:2,field_nam:2,field_type_numpi:2,field_type_struct:2,file:1,file_handl:5,filenam:[3,5],fill:7,filter:1,filter_log_index:[3,4],filter_nod:13,find:[3,4,5,10],find_overlapping_tx_low:4,first:[0,7,13],five:13,fix:6,flag:7,flow:[1,3,4],flowconfig:[6,7],flowconfigallassoccbr:[6,7],flowconfigcbr:[6,7],flowconfigrandomrandom:[6,7],fmt_log_hdr:4,follow:[3,4,7,9,13],forc:8,forev:10,form:[2,3,4,9,10,13],format:[0,1,2],former:10,forum:7,found:[2,7,11],fpga:7,fpga_dna:7,fragement:7,fragment:7,frame:7,framework:0,framwork:7,freq:13,frequenc:[7,13],from:[1,2,3,4,5,7,8,9,10,12,13],full:[3,7],further:3,futur:3,garbag:3,gather:[7,11],gen_index:[3,5],gen_numpy_array_callback:2,gen_random:7,gen_raw_log_index:[3,4],gener:[0,1,2,3,4,5,6],get:[2,4,5,7,9,13],get_attr_dict:5,get_bss_info:[7,8,9,10],get_channel_info:13,get_entry_type_id:2,get_field_nam:2,get_field_struct_format:2,get_log_data:5,get_log_data_s:5,get_log_index:5,get_mac_tim:7,get_network_list:7,get_now_as_log_time_str:4,get_rate_info:13,get_rx_ant_mod:7,get_station_info:7,get_system_tim:7,get_tx_ant_mode_multicast_data:7,get_tx_ant_mode_multicast_mgmt:7,get_tx_ant_mode_unicast:7,get_tx_pow:7,get_tx_power_multicast_data:7,get_tx_power_multicast_mgmt:7,get_tx_power_unicast:7,get_tx_rate_multicast_data:7,get_tx_rate_multicast_mgmt:7,get_tx_rate_unicast:7,get_txrx_count:[7,9],get_wlan_mac_address:7,ghz:7,gigabyt:3,give:3,given:[0,4,5,6,7,8,10,13],global:13,goal:7,good:[5,7],green:3,group:[3,5],group_nam:5,guarante:13,guard:13,guarente:[4,7],guid:[1,2,3,6,12],h5py:[3,5],had:8,halt:10,handl:[4,5],handshak:[7,8,10],hardwar:[7,13],have:[3,4,5,7,9,10,13],hdf5:1,hdf5_close_fil:5,hdf5_open_fil:5,hdf5_to_attr_dict:5,hdf5_to_log_data:[3,5],hdf5_to_log_index:[3,5],hdf5logcontain:5,header:[2,3,4,6,7],heap:7,here:[3,7,9,12],hex:[7,10],hierarchi:3,high:[6,7,8,9,10,13],high_se:7,hoc:[1,7],homogen:3,host:[7,13],how:[1,4,7,13],howev:[3,4,7,10],ht_capabl:[0,7,8,9,10],htmf:[7,8,9,10,13],http:[1,5,7,8,9,10,11,13],hw_gener:13,hw_ver:7,ibss:[1,7],ideal:7,ident:[3,7],identif:7,identifi:[7,13],ignor:[3,4,7],illustr:[3,7],immedi:[7,10],implement:[1,3,6,7,8,9,13],inact:8,inc:1,includ:[3,4,7,8,9],include_onli:[3,4],incom:[7,8],incomplet:7,inconveni:3,indefinit:7,independ:8,index:1,indic:0,individu:7,info:[5,7,13],info_typ:[7,13],inform:[2,5,6,7,8,9,10,11,13],infrastructur:13,inherit:7,inidc:10,init:13,init_nod:13,initi:[3,7,13],input:[3,4,5,7,13],insert:[7,9],instanc:[3,4,7],instanti:7,instead:[4,7,9],int_to_ip:13,int_tx_low:4,integ:[2,3,4,7,8,9,10,13],integr:[1,7],interact:[0,5,7,13],interfac:[3,7,8,9,10,13],intern:7,interpret:[4,7,13],interv:[6,7,8,9,10,13],invalid:7,ip_address:13,ip_to_int:13,is_associ:[0,8,10],is_join:10,is_scan:7,is_valid:5,issu:[7,13],item:7,iter:[4,13],itself:[3,4,7,12],join:[0,10],join_network:10,keep:[3,4,7,8,13],kei:[2,3,4,5],kept:7,keyboard:13,know:[4,7],known:[7,10,13],larg:[3,7],larger:3,largest:3,last:[7,9],latest_beacon_rx_pow:7,latest_beacon_rx_tim:7,latest_txrx_timestamp:[7,9],len:0,length:[3,4,6,7,8,9,10],less:[7,8,9,10],let:7,level:[1,3,5,7,9,11],licens:1,like:[3,5,7],limit:7,list:[0,2,3,4,7,8,9,13],list_of_wlan_channel:13,load:7,local:1,locat:[3,8],log:1,log_capture_continu:7,log_configur:7,log_data:[3,4,5],log_data_to_hdf5:[3,5],log_data_to_np_arrai:[3,4],log_en:7,log_full_payload:7,log_get:7,log_get_all_new:[3,7],log_get_capac:7,log_get_flag:7,log_get_index:7,log_get_indexex:7,log_get_s:7,log_index:[3,4,5],log_is_ful:7,log_max_s:7,log_next_read_index:7,log_node_a:5,log_node_b:5,log_num_wrap:7,log_tail_pad:7,log_time_str:4,log_total_bytes_read:7,log_txrx_ctrl:7,log_txrx_mpdu:7,log_wrap_en:7,log_write_exp_info:[7,13],log_write_tim:[7,13],logic:[8,9,10],long_retry_limit:7,longer:7,lookup:13,low:[1,3,7,9,13],low_se:7,lower:7,ltg:[1,2,3],ltg_configur:7,ltg_get_statu:7,ltg_id:7,ltg_id_list:7,ltg_remov:7,ltg_remove_al:7,ltg_start:7,ltg_start_al:7,ltg_stop:7,ltg_stop_al:7,mac:[0,1,2,3,4,6,7,8,9,10,11],mac_addr:[2,7,9,13],mac_addr_broadcast:13,mac_addr_desc:13,mac_addr_desc_map:13,mac_addr_local_mask:13,mac_addr_mcast_mask:13,mac_addr_to_byte_str:13,mac_addr_to_str:13,mac_address:[0,13],mac_head:7,mac_high:13,mac_low:13,machin:[3,7,10],magic:3,mai:[5,7,10],maintain:7,major:[3,7],make:7,manag:[7,9],mangocomm:1,mani:[3,4],map:[7,8,13],mask:[8,13],match:[3,4,5,7,8,13],materi:7,matlab:13,max:7,max_interv:[6,7],max_length:6,max_payload_length:[6,7],max_req_s:7,max_tx_power_dbm:7,maximum:[6,7],mbp:13,mean:[2,3,7,13],medium:[0,7],member:[0,7,8,9,10,13],membership:13,memori:7,merg:[3,4],merge_log_index:4,messag:[5,7,13],met:13,method:[1,3,4,5],mgmt_num_rx_byt:[7,9],mgmt_num_rx_bytes_tot:7,mgmt_num_rx_packet:[7,9],mgmt_num_rx_packets_tot:7,mgmt_num_tx_attempt:[7,9],mgmt_num_tx_bytes_success:[7,9],mgmt_num_tx_bytes_tot:[7,9],mgmt_num_tx_packets_success:[7,9],mgmt_num_tx_packets_tot:[7,9],mhz:[7,13],microsecond:[6,7,8,9,10,13],might:4,million:3,mimic:13,min_interv:[6,7],min_length:6,min_payload_length:[6,7],min_tx_power_dbm:7,minimum:[6,7,8,9,10],minor:[3,7],misc:1,mismatch:7,mode:[1,4,5],modif:4,modifi:[2,4,5,7],modify_field_def:2,modul:[1,2,3,4,7,13],more:[2,3,5,6,7,8,9,10,12,13],most:3,move:4,mpdu:7,mpdu_to_m:7,msp:7,much:[3,7],multi:3,multicast:[7,8,13],multipl:[1,3,4,7,13],must:[0,2,3,4,5,7,8,9,10,13],mutual:7,my_raw_log_index:3,n_ap:[0,8],name:[0,1,2,3,4,5,7,8,9,10],ndbp:13,necessari:[4,13],necessarili:4,need:[7,10],network:[0,1,3],network_config:[7,13],network_list:7,network_reset:13,networkconfigur:[7,13],new_assoc:7,next:[4,7],next_index:7,nnumber:4,node:[0,1,3,5],node_a_log_fil:5,node_b_log_fil:5,node_factori:13,node_id:7,node_info:3,node_typ:[7,13],node_type_to_str:13,nodes_config:13,nodesconfigur:13,nomac:13,non:[0,7,13],none:[0,2,4,5,6,7,8,9,10,13],nonht:[7,8,9,10,13],normal:[3,7,8,10],notat:13,note:[7,9],noth:7,notic:3,notimplementederror:8,now:[3,4],np_arr:2,np_arr_orig:2,np_arrai:[2,4],np_array_add_field:2,np_array_of_rx_etri:5,np_array_of_rx_etries_a:5,np_array_of_rx_etries_b:5,np_array_of_tx_entri:5,np_array_of_tx_entries_a:5,np_array_of_tx_entries_b:5,np_arrays_to_hdf5:5,np_log_dict:5,nullifi:7,num_byt:7,num_join:0,num_probe_tx_per_chan:7,num_probe_tx_per_channel:7,num_wrap:7,number:[2,3,4,5,7,8,9,10,13],numpi:[1,2],numpy_typ:2,object:[3,4,5,7,13],observ:7,occupi:7,occur:[4,7,13],ofdm:[5,13],off:7,offset:[3,4,7],often:3,oldest:7,oldest_index:7,omit:3,onc:10,onli:[2,3,4,5,7,8,10,13],opaqu:3,open:[3,5],oper:[2,4,7,8,9,10],opposit:3,option:[2,3,4,5,6,7,8,9,13],order:[2,3,7,8,9,11,13],org:[1,5,7,8,9,10,11,13],origin:[2,3,5],ota:7,other:[3,4,7],otherwis:[5,10],our:3,out:[4,7],output:[3,4,7,13],over:[4,8],overhead:[4,7],overheard:7,overlap:4,overload:[3,7],overview:1,overwrit:[4,5],overwrite_entries_with_null_entri:4,overwrite_payload:4,overwritten:7,own:7,pack:[2,3],packag:[1,3,12],packet:[1,2,3,4,5,6],page:1,pair:3,param:7,param_id:7,param_nam:7,param_v:7,param_valu:7,paramet:[0,2,4,5,6,7,8,9,10,13],paramt:7,parent:0,pars:[3,4],part:[3,4,7,13],particip:[7,13],particular:3,pass:[3,7,8,13],passiv:7,path:3,pattern:13,payload:[1,3,4],payload_length:[4,6,7],payload_offset:4,payloadallassocfix:6,payloadfix:6,payloaduniformrandom:6,per:[2,3,7,13],period:[6,7,8,10],phy:[0,1,4,7,8,9,10,13],phy_cs_thresh:7,phy_mdo:7,phy_mod:[4,7,13],phy_rat:13,phy_samp_r:[4,7,13],phy_sample_r:4,physic:[3,7],piec:4,ping:7,pipe:7,place:[4,7,13],pleas:[6,7,12],plu:[4,8,9,10],point:[1,7],poll:8,portion:7,possibl:[3,4,7,13],potenti:4,power:[1,6],power_level:7,pre:[8,13],preambl:4,preced:[4,7],predefin:8,present:3,previou:13,previous:[3,4,10],primari:7,print:[0,1,4,5,7,11],print_log_index_summari:4,print_warn:5,printabl:13,priori:13,privaci:7,probe:[7,10],problem:10,process:1,processs:3,produc:[3,7],promiscu:7,prompt:13,proper:[7,9],properli:7,properti:4,provi:5,provid:[0,3,4,5,7,9,10,12,13],providi:7,pull:7,purg:7,purpos:13,python:[1,2,3,4,7,12,13],qam:13,quantiz:6,queri:[7,10],queue:[7,8],queue_data:7,queue_tx_data_purge_al:7,quit:3,rais:8,ram:3,random:[6,7,13],rang:13,rate:[0,1,4,6],rate_info:13,rate_info_to_str:13,raw:[1,2],raw_log_index:[3,4,5],reach:7,read:1,readonli:5,real:1,reason:7,receiv:[1,4,7,9,11],recent:8,recept:[4,5,7,8,9],recommend:[3,7],record:[3,4,5,7,9],red:3,refer:0,rel:4,relev:7,reli:4,rememb:3,remot:[7,9],remov:[0,4,7,8],replac:[3,5],repres:[0,3,7,13],represent:13,request:[3,4,5,7],requir:[3,4,7,8,9,10],research:3,reset:[7,8,10,13],reset_al:7,resolut:7,resourc:3,resp_arg:7,respond:7,respons:[6,7],result:[4,7,13],retain:[3,7],retransmiss:7,retri:7,retriev:[3,7,9,13],retrieval_timestamp:[7,9],return_zeroed_counts_if_non:[7,9],rev:3,revis:7,rf_a:7,rf_b:7,root:[3,5],rssi:7,rts_thresh:7,rule:[3,4],run:[1,2,7,8,9,10,13],runtim:7,rx_all:4,rx_dsss:[3,4],rx_ofdm:[3,4,5],safe:7,same:[2,3,7,8,9,10,13],sampl:[4,7,13],save:3,scalaer:4,scalar:[3,7],scan:1,schedul:1,scheduleperiod:6,scheduler_resolut:7,scheduleuniformrandom:6,scheme:[4,7,13],scratch:3,script:[3,7,13],search:1,sec:7,second:[6,7],see:[7,8,9,10,13],seed:7,seen:7,select:[3,7],send:[7,13],send_user_command:7,sens:7,sent:7,separ:13,sequenc:3,serial:[2,5,7,13],serial_numb:[7,13],serv:13,servic:7,set:[3,4,5,6,7,8,9,10,13],set_aid:10,set_authentication_address_filt:8,set_dcf_param:7,set_low_param:7,set_low_to_high_rx_filt:7,set_mac_tim:7,set_phy_samp_r:7,set_print_level:[7,13],set_radio_channel:7,set_random_se:7,set_rx_ant_mod:7,set_scan_paramet:[7,10],set_tim:13,set_tx_ant_mode_multicast_data:7,set_tx_ant_mode_multicast_mgmt:7,set_tx_ant_mode_unicast:7,set_tx_pow:7,set_tx_power_ctrl:7,set_tx_power_multicast_data:7,set_tx_power_multicast_mgmt:7,set_tx_power_unicast:7,set_tx_rate_multicast_data:7,set_tx_rate_multicast_mgmt:7,set_tx_rate_unicast:7,setup:0,short_gi:13,short_retry_limit:7,should:[0,3,4,7],show:[3,7],signal:4,similar:7,simpl:3,simplest:7,sinc:[3,4,7],singl:[3,4,7,8,9,13],size:[3,4,5,6,7],sleep:[7,8],smaller:7,sn_to_str:13,snap:4,snippet:3,solicit:7,some:7,some_log_fil:5,sometim:4,sourc:[0,2,4,5,6,7,8,9,10,13],specif:[3,6,7,8,9,10],specifc:7,specifi:[0,2,3,4,7,8,9,10,13],spend:7,src_index:4,src_tx_low:4,ssid:[7,8,9,10,13],sta:[1,7,8],staion:7,staistic:9,standard:[5,7,8],start:[3,7,8,10],start_network_scan:7,start_timestamp:7,state:1,station:[1,7,8],station_info:13,statist:3,statoin:7,statu:[7,10,11],step:3,still:[7,10],stop:[7,10],stop_network_scan:7,stop_timestamp:7,store:[3,5,7],str:[0,2,4,5,7,8,9,10,13],str_to_mac_addr:13,strict:3,string:[0,2,4,7,8,9,10,13],struct:[2,4],struct_typ:2,structur:1,sub:[1,3],subclass:[7,8,9,10,13],submit:7,subnet:13,subset:3,substitut:8,success:10,successfulli:[3,7,9,10],suffix:5,suit:3,summar:3,summari:4,suppli:[3,7],support:[3,7,8,9,10,13],symbol:13,system:[7,9,12],take:[3,4,7,13],taken:3,target:[7,8,10],technic:3,ten:3,term:3,termin:10,test:[3,7,10],than:[3,7],thankfulli:10,the_kei:5,thei:[4,7,8,10],them:[0,3,4],themselv:3,therefor:[3,4,7],thi:[0,1,2,3,4,5,6,7,8,9,10,13],thing:6,those:[4,8],three:[3,6,7],threshold:7,through:[4,7,13],tightli:3,time:[1,3,4,5,6,7,8,9,10,13],time_id:[7,13],time_info:[7,13],time_per_channel:7,time_unit:[8,9,10],timeout:[7,8,10],timer:6,timestamp:[7,9,13],timezon:4,titl:4,tmp_devic:0,togeth:3,tool:1,top:5,total:[7,9],total_devic:0,trac:[1,7,8,9,10,11],track:[7,8,13],traffic:[1,6],traffic_flow:7,trafic_flow:7,translat:1,transmiss:[3,4,7,8,9],transmit:[1,7,9,10,11],transport:7,transport_broadcast:7,treat:[3,8],trial:3,trigger:[7,8],tu_:[8,9,10],tune:7,tupl:[0,2,3,4,7,8,13],two:[3,5,6,7,13],tx_high:[3,5],tx_high_ltg:3,tx_low:4,tx_power:7,txrx_count:7,txrxcount:7,type:[0,1,2],typedef:4,typic:[3,4],u16:4,u32:[4,7,13],uart:[1,7,11],uart_print_level:[7,13],uint32:3,uint64:[0,3],unassoci:[7,8,9,10],unchang:3,under:1,underli:[2,3,5,7],understand:7,understood:3,unfortun:7,unicast:7,uniformli:6,uniqu:[0,5,7],unit:[7,8,9,10],unix:3,unknown:13,unless:5,unpack:[2,4],unsign:13,until:[7,10],updat:[2,7,8,9,10],usec:6,user:[0,1,2,3,5,6,7,10,12],utf:7,util:[1,3],util_hdf:3,valid:[4,5,7,8,9,10,13],valu:[0,2,3,4,5,7,8,9,10,13],variabl:[0,4,7,8,9,10],varieti:3,variou:[1,2,7,13],verbos:[4,7,13],veri:[3,7],version:[3,7,13],via:[7,8,10,13],virtual:2,visibl:1,voidn:3,wai:3,wait:[0,7],want:4,warn:[4,5,7,13],warp:[0,7,11,13],warpproject:[1,7,8,9,10,11],warpv3:13,well:[3,7],were:[4,13],what:[4,7],whatev:7,when:[3,4,5,7,8,9,13],where:[2,3,7,13],wherev:3,whether:[7,8,9,10,13],which:[0,3,4,5,7,8,9,10,13],whose:[7,9],wide:3,wiki:[1,2,7,8,9,10,11,13],wikipedia:[8,9,10,13],wildcard:7,window:7,wireless:[0,7,8],within:[0,3,5],without:[4,7],wlan_channel:[7,13],wlan_devic:0,wlan_device_list:0,wlan_exp:[0,1,3,4,5,7,8,9,10,11,12,13],wlan_exp_high_ap:13,wlan_exp_high_ibss:13,wlan_exp_high_sta:13,wlan_exp_log:3,wlan_exp_log_data_contain:3,wlan_exp_low_dcf:13,wlan_exp_low_nomac:13,wlan_exp_v:3,wlan_exp_ver_major:7,wlan_exp_ver_minor:7,wlan_exp_ver_revis:7,wlan_mac_address:[0,7,8,9],wlan_mac_high:7,wlan_rx_ant_mod:13,wlan_tx_ant_mod:13,wlandevic:[0,7,8,9,10],wlanexplogentrytyp:[2,3,4],wlanexpnod:[4,5,7,8,9,10,13],wlanexpnodeap:8,wlanexpnodefactori:13,wlanexpnodeibss:9,wlanexpnodesta:10,would:[3,4],wrap:[5,7],write:1,write_:5,write_attr_dict:5,write_log_data:5,write_log_index:5,written:[3,5,7],wrote:3,www:5,xxxxx:13,yellow:3,you:[5,7],zero:[4,7,9,13]},titles:["WLAN Device","Mango 802.11 Reference Design Experiments Framework Documentation","Log Entry","Event Log Overview","Event Log Utilities","HDF5 Utilities","LTG Flow Configurations","Node Classes","Access Point (AP) Node","Ad hoc (IBSS) Node","Station (STA) Node","Framework","Event Log","WLAN Exp Utilities"],titleterms:{"class":[0,2,5,7],"function":[2,4,5,13],access:8,address:13,antenna:[7,13],archiv:3,arrai:3,associ:7,base:7,channel:13,common:7,compon:3,config:6,configur:6,contain:5,count:7,data:3,definit:13,design:1,devic:0,document:1,entri:[2,3],event:[3,4,12],exampl:0,exp:13,experi:1,file:3,filter:3,flow:6,format:3,framework:[1,11],gener:7,hdf5:[3,5],hoc:9,ibss:9,index:3,indic:1,level:13,local:7,log:[2,3,4,7,12],ltg:[6,7],mac:13,mango:1,method:[7,8,9,10],misc:[4,13],mode:[7,13],network:7,node:[7,8,9,10,13],numpi:3,overview:3,packet:7,payload:6,point:8,power:7,print:13,process:3,rate:[7,13],raw:3,read:3,receiv:13,refer:1,scan:7,schedul:6,sta:10,state:7,station:10,structur:3,sub:7,tabl:1,tool:3,traffic:7,translat:3,transmit:13,type:3,uart:13,util:[4,5,13],wlan:[0,13],write:3}})