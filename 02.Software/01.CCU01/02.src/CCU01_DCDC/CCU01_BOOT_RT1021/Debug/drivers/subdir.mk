################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_cache.c \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_common_arm.c \
../drivers/fsl_gpio.c \
../drivers/fsl_lpuart.c \
../drivers/fsl_romapi.c 

C_DEPS += \
./drivers/fsl_cache.d \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_common_arm.d \
./drivers/fsl_gpio.d \
./drivers/fsl_lpuart.d \
./drivers/fsl_romapi.d 

OBJS += \
./drivers/fsl_cache.o \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_common_arm.o \
./drivers/fsl_gpio.o \
./drivers/fsl_lpuart.o \
./drivers/fsl_romapi.o 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c drivers/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_MIMXRT1021DAG5A -DCPU_MIMXRT1021DAG5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DMCUXPRESSO_SDK -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\source" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\utilities" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\drivers" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\device" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\component\uart" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\component\lists" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\xip" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\CMSIS" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_BOOT_RT1021\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-drivers

clean-drivers:
	-$(RM) ./drivers/fsl_cache.d ./drivers/fsl_cache.o ./drivers/fsl_clock.d ./drivers/fsl_clock.o ./drivers/fsl_common.d ./drivers/fsl_common.o ./drivers/fsl_common_arm.d ./drivers/fsl_common_arm.o ./drivers/fsl_gpio.d ./drivers/fsl_gpio.o ./drivers/fsl_lpuart.d ./drivers/fsl_lpuart.o ./drivers/fsl_romapi.d ./drivers/fsl_romapi.o

.PHONY: clean-drivers

