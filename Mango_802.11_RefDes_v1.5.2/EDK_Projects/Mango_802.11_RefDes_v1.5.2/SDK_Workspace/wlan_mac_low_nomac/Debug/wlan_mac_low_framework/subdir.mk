################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_framework/wlan_mac_low.c \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_framework/wlan_phy_util.c 

OBJS += \
./wlan_mac_low_framework/wlan_mac_low.o \
./wlan_mac_low_framework/wlan_phy_util.o 

C_DEPS += \
./wlan_mac_low_framework/wlan_mac_low.d \
./wlan_mac_low_framework/wlan_phy_util.d 


# Each subdirectory must supply rules for building sources it contributes
wlan_mac_low_framework/wlan_mac_low.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_framework/wlan_mac_low.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -O2 -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_nomac\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

wlan_mac_low_framework/wlan_phy_util.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_framework/wlan_phy_util.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -O2 -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_nomac\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '


