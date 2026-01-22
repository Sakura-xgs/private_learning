################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../user_code/APP/imd/GY_imd.c \
../user_code/APP/imd/imd.c 

C_DEPS += \
./user_code/APP/imd/GY_imd.d \
./user_code/APP/imd/imd.d 

OBJS += \
./user_code/APP/imd/GY_imd.o \
./user_code/APP/imd/imd.o 


# Each subdirectory must supply rules for building sources it contributes
user_code/APP/imd/%.o: ../user_code/APP/imd/%.c user_code/APP/imd/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1021DAG5A -DCPU_MIMXRT1021DAG5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DMCUXPRESSO_SDK -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DCHECKSUM_GEN_ICMP6=1 -DCHECKSUM_CHECK_ICMP6=1 -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE -DUSE_RTOS=1 -I"D:\svn\02.src\CCU01_APP_RT1021\component\lists" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\eth" -I"D:\svn\02.src\CCU01_APP_RT1021\component\uart" -I"D:\svn\02.src\CCU01_APP_RT1021\component\serial_manager" -I"D:\svn\02.src\CCU01_APP_RT1021\device" -I"D:\svn\02.src\CCU01_APP_RT1021\drivers" -I"D:\svn\02.src\CCU01_APP_RT1021\xip" -I"D:\svn\02.src\CCU01_APP_RT1021\utilities" -I"D:\svn\02.src\CCU01_APP_RT1021\freertos\freertos-kernel\include" -I"D:\svn\02.src\CCU01_APP_RT1021\freertos\freertos-kernel\portable\GCC\ARM_CM4F" -I"D:\svn\02.src\CCU01_APP_RT1021\CMSIS" -I"D:\svn\02.src\CCU01_APP_RT1021\drivers\freertos" -I"D:\svn\02.src\CCU01_APP_RT1021\component\silicon_id" -I"D:\svn\02.src\CCU01_APP_RT1021\component\phy" -I"D:\svn\02.src\CCU01_APP_RT1021\lwip\port" -I"D:\svn\02.src\CCU01_APP_RT1021\component\gpio" -I"D:\svn\02.src\CCU01_APP_RT1021\lwip\contrib\apps\tcpecho" -I"D:\svn\02.src\CCU01_APP_RT1021\lwip\src" -I"D:\svn\02.src\CCU01_APP_RT1021\lwip\src\include" -I"D:\svn\02.src\CCU01_APP_RT1021\source" -I"D:\svn\02.src\CCU01_APP_RT1021\lwip\template" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_cd4052b" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_sys" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_eth" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\uart_comm" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\poll_adc" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\rgb_pwm" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_uart" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\charge_can" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_can" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_ext_rtc" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_eeprom" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\flash_data" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\calculate_crc" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\Signal" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\SignalManage" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\boot" -I"D:\svn\02.src\CCU01_APP_RT1021\board" -I"D:\svn\02.src\CCU01_APP_RT1021\lwip\src\include\lwip" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_cJSON" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\secc_logAndupgrade" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\cm_backtrace" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\cm_backtrace\Languages\en-US" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\cm_backtrace\Languages\zh-CN" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\pos" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\client" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\HAL\hal_adc" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\datasample" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\fan" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\cig" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\charge_control" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\fault" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\charge_process" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\imd" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\meter" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\led" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\hmi" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\charge_price" -I"D:\svn\02.src\CCU01_APP_RT1021\user_code\APP\rfid" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-user_code-2f-APP-2f-imd

clean-user_code-2f-APP-2f-imd:
	-$(RM) ./user_code/APP/imd/GY_imd.d ./user_code/APP/imd/GY_imd.o ./user_code/APP/imd/imd.d ./user_code/APP/imd/imd.o

.PHONY: clean-user_code-2f-APP-2f-imd

