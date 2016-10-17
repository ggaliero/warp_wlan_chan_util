################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_common.c \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_mailbox_util.c \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_pkt_buf_util.c \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_sysmon_util.c \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_time_util.c \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_userio_util.c 

OBJS += \
./wlan_mac_common/wlan_mac_common.o \
./wlan_mac_common/wlan_mac_mailbox_util.o \
./wlan_mac_common/wlan_mac_pkt_buf_util.o \
./wlan_mac_common/wlan_mac_sysmon_util.o \
./wlan_mac_common/wlan_mac_time_util.o \
./wlan_mac_common/wlan_mac_userio_util.o 

C_DEPS += \
./wlan_mac_common/wlan_mac_common.d \
./wlan_mac_common/wlan_mac_mailbox_util.d \
./wlan_mac_common/wlan_mac_pkt_buf_util.d \
./wlan_mac_common/wlan_mac_sysmon_util.d \
./wlan_mac_common/wlan_mac_time_util.d \
./wlan_mac_common/wlan_mac_userio_util.d 


# Each subdirectory must supply rules for building sources it contributes
wlan_mac_common/wlan_mac_common.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_common.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

wlan_mac_common/wlan_mac_mailbox_util.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_mailbox_util.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

wlan_mac_common/wlan_mac_pkt_buf_util.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_pkt_buf_util.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

wlan_mac_common/wlan_mac_sysmon_util.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_sysmon_util.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

wlan_mac_common/wlan_mac_time_util.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_time_util.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

wlan_mac_common/wlan_mac_userio_util.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2_sniffer_ctrl/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_common/wlan_mac_userio_util.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2_sniffer_ctrl\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '


