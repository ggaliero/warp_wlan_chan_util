################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/wlan_mac_dcf.c 

LD_SRCS += \
../src/lscript.ld 

OBJS += \
./src/wlan_mac_dcf.o 

C_DEPS += \
./src/wlan_mac_dcf.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -O2 -g3 -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.3.0_sniffer\EDK_Projects\Mango_802.11_RefDes_v1.3.0\SDK_Workspace\wlan_mac_shared\wlan_mac_common\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.3.0_sniffer\EDK_Projects\Mango_802.11_RefDes_v1.3.0\SDK_Workspace\wlan_mac_shared\wlan_mac_low_framework\include" -I"C:\Users\egagali\software\Mango_802.11_RefDes_v1.3.0_sniffer\EDK_Projects\Mango_802.11_RefDes_v1.3.0\SDK_Workspace\wlan_mac_low_dcf\src\include" -c -fmessage-length=0 -Wl,--no-relax -I../../wlan_bsp_cpu_low/mb_low/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '


