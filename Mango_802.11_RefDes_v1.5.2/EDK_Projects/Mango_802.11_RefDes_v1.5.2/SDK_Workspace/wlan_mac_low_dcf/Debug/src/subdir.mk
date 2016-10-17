################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_dcf/wlan_mac_dcf.c 

LD_SRCS += \
C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_dcf/lscript.ld 

OBJS += \
./src/wlan_mac_dcf.o 

C_DEPS += \
./src/wlan_mac_dcf.d 


# Each subdirectory must supply rules for building sources it contributes
src/wlan_mac_dcf.o: C:/Users/egagali/software/Mango_802.11_RefDes_v1.5.2/EDK_Projects/Mango_802.11_RefDes_v1.5.2/SDK_Workspace/svn_src/wlan_mac_low_dcf/wlan_mac_dcf.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -Os -fdata-sections -ffunction-sections -g -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_dcf\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.5.2\EDK_Projects\Mango_802.11_RefDes_v1.5.2\SDK_Workspace\svn_src\wlan_mac_low_framework\include" -I../../wlan_bsp_cpu_low/mb_low/include -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '


