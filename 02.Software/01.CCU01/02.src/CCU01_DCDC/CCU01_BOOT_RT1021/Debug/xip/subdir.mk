################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../xip/evkmimxrt1020_flexspi_nor_config.c \
../xip/fsl_flexspi_nor_boot.c 

C_DEPS += \
./xip/evkmimxrt1020_flexspi_nor_config.d \
./xip/fsl_flexspi_nor_boot.d 

OBJS += \
./xip/evkmimxrt1020_flexspi_nor_config.o \
./xip/fsl_flexspi_nor_boot.o 


# Each subdirectory must supply rules for building sources it contributes
xip/%.o: ../xip/%.c xip/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_MIMXRT1021DAG5A -DCPU_MIMXRT1021DAG5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DMCUXPRESSO_SDK -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\source" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\utilities" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\drivers" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\device" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\component\uart" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\component\lists" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\xip" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\CMSIS" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-xip

clean-xip:
	-$(RM) ./xip/evkmimxrt1020_flexspi_nor_config.d ./xip/evkmimxrt1020_flexspi_nor_config.o ./xip/fsl_flexspi_nor_boot.d ./xip/fsl_flexspi_nor_boot.o

.PHONY: clean-xip

