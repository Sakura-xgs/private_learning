################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../user_code/APP/hmi/ATQ_hmi.c \
../user_code/APP/hmi/XRD_hmi.c \
../user_code/APP/hmi/hmi.c 

C_DEPS += \
./user_code/APP/hmi/ATQ_hmi.d \
./user_code/APP/hmi/XRD_hmi.d \
./user_code/APP/hmi/hmi.d 

OBJS += \
./user_code/APP/hmi/ATQ_hmi.o \
./user_code/APP/hmi/XRD_hmi.o \
./user_code/APP/hmi/hmi.o 


# Each subdirectory must supply rules for building sources it contributes
user_code/APP/hmi/%.o: ../user_code/APP/hmi/%.c user_code/APP/hmi/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1021DAG5A -DCPU_MIMXRT1021DAG5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DMCUXPRESSO_SDK -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DCHECKSUM_GEN_ICMP6=1 -DCHECKSUM_CHECK_ICMP6=1 -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE -DUSE_RTOS=1 -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\lists" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\uart" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\serial_manager" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\device" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\drivers" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\xip" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\utilities" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\freertos\freertos-kernel\include" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\freertos\freertos-kernel\portable\GCC\ARM_CM4F" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\CMSIS" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\drivers\freertos" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\silicon_id" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\phy" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\port" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\gpio" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\contrib\apps\tcpecho" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\src" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\src\include" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\source" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\template" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_cd4052b" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_sys" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_eth" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\uart_comm" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\poll_adc" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\rgb_pwm" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_uart" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\charge_can" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_can" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_ext_rtc" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_eeprom" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\flash_data" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\calculate_crc" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\Signal" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\SignalManage" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\boot" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-user_code-2f-APP-2f-hmi

clean-user_code-2f-APP-2f-hmi:
	-$(RM) ./user_code/APP/hmi/ATQ_hmi.d ./user_code/APP/hmi/ATQ_hmi.o ./user_code/APP/hmi/XRD_hmi.d ./user_code/APP/hmi/XRD_hmi.o ./user_code/APP/hmi/hmi.d ./user_code/APP/hmi/hmi.o

.PHONY: clean-user_code-2f-APP-2f-hmi

