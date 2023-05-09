################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
A51_UPPER_SRCS += \
../src/SILABS_STARTUP.A51 \
../src/adc_ref_const.A51 

C_SRCS += \
../src/InitDevice.c \
../src/Interrupts.c \
../src/SolarMpptCharger_main.c \
../src/adc.c \
../src/buck.c \
../src/charge.c \
../src/led.c \
../src/param.c \
../src/power.c \
../src/smbus.c \
../src/temp.c \
../src/timer.c \
../src/watchdog.c 

OBJS += \
./src/InitDevice.OBJ \
./src/Interrupts.OBJ \
./src/SILABS_STARTUP.OBJ \
./src/SolarMpptCharger_main.OBJ \
./src/adc.OBJ \
./src/adc_ref_const.OBJ \
./src/buck.OBJ \
./src/charge.OBJ \
./src/led.OBJ \
./src/param.OBJ \
./src/power.OBJ \
./src/smbus.OBJ \
./src/temp.OBJ \
./src/timer.OBJ \
./src/watchdog.OBJ 


# Each subdirectory must supply rules for building sources it contributes
src/%.OBJ: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Keil 8051 Compiler'
	wine "/Applications/Simplicity Studio.app/Contents/Eclipse/developer/toolchains/keil_8051/9.60/BIN/C51" "@$(patsubst %.OBJ,%.__i,$@)" || $(RC)
	@echo 'Finished building: $<'
	@echo ' '

src/InitDevice.OBJ: /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/InitDevice.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/Interrupts.OBJ: /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/%.OBJ: ../src/%.A51
	@echo 'Building file: $<'
	@echo 'Invoking: Keil 8051 Assembler'
	wine "/Applications/Simplicity Studio.app/Contents/Eclipse/developer/toolchains/keil_8051/9.60/BIN/AX51" "@$(patsubst %.OBJ,%.__ia,$@)" || $(RC)
	@echo 'Finished building: $<'
	@echo ' '

src/SolarMpptCharger_main.OBJ: /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/InitDevice.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/adc.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/buck.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/charge.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/led.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/power.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/temp.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/timer.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/watchdog.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/adc.OBJ: /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/toolchains/keil_8051/9.60/INC/INTRINS.H /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/adc.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/buck.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/watchdog.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/buck.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/buck.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/adc.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/charge.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/charge.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/adc.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/buck.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/power.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/temp.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/led.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/led.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/charge.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/power.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/temp.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/timer.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/param.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/power.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/adc.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/power.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/charge.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/smbus.OBJ: /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/buck.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/charge.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/power.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/temp.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/watchdog.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/temp.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/temp.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/adc.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/config.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/toolchains/keil_8051/9.60/INC/MATH.H /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/param.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/timer.OBJ: /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/timer.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h

src/watchdog.OBJ: /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Register_Enums.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/watchdog.h /Users/danjulio/SimplicityStudio/v4_workspace/SolarMpptCharger/inc/smbus.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/EFM8SB1/inc/SI_EFM8SB1_Defs.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/si_toolchain.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdint.h /Applications/Simplicity\ Studio.app/Contents/Eclipse/developer/sdks/8051/v4.1.7/Device/shared/si8051Base/stdbool.h


