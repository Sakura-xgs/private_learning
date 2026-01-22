################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/netif/bridgeif.c \
../lwip/src/netif/bridgeif_fdb.c \
../lwip/src/netif/ethernet.c \
../lwip/src/netif/lowpan6.c \
../lwip/src/netif/lowpan6_ble.c \
../lwip/src/netif/lowpan6_common.c \
../lwip/src/netif/slipif.c \
../lwip/src/netif/zepif.c 

C_DEPS += \
./lwip/src/netif/bridgeif.d \
./lwip/src/netif/bridgeif_fdb.d \
./lwip/src/netif/ethernet.d \
./lwip/src/netif/lowpan6.d \
./lwip/src/netif/lowpan6_ble.d \
./lwip/src/netif/lowpan6_common.d \
./lwip/src/netif/slipif.d \
./lwip/src/netif/zepif.d 

OBJS += \
./lwip/src/netif/bridgeif.o \
./lwip/src/netif/bridgeif_fdb.o \
./lwip/src/netif/ethernet.o \
./lwip/src/netif/lowpan6.o \
./lwip/src/netif/lowpan6_ble.o \
./lwip/src/netif/lowpan6_common.o \
./lwip/src/netif/slipif.o \
./lwip/src/netif/zepif.o 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/netif/%.o: ../lwip/src/netif/%.c lwip/src/netif/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1021DAG5A -DCPU_MIMXRT1021DAG5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DMCUXPRESSO_SDK -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DCHECKSUM_GEN_ICMP6=1 -DCHECKSUM_CHECK_ICMP6=1 -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE -DUSE_RTOS=1 -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\lists" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\uart" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\serial_manager" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\device" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\drivers" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\xip" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\utilities" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\freertos\freertos-kernel\include" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\freertos\freertos-kernel\portable\GCC\ARM_CM4F" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\CMSIS" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\drivers\freertos" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\silicon_id" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\phy" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\port" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\component\gpio" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\contrib\apps\tcpecho" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\src" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\src\include" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\source" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\lwip\template" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_cd4052b" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_sys" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_eth" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\uart_comm" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\poll_adc" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\rgb_pwm" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_uart" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\APP\charge_can" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_can" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_ext_rtc" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\hal_eeprom" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\flash_data" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\calculate_crc" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\Signal" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\SignalManage" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\user_code\HAL\boot" -I"E:\0003.Charger_Software\02.PDU01\02.src\PDU01_APP_RT1021\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-lwip-2f-src-2f-netif

clean-lwip-2f-src-2f-netif:
	-$(RM) ./lwip/src/netif/bridgeif.d ./lwip/src/netif/bridgeif.o ./lwip/src/netif/bridgeif_fdb.d ./lwip/src/netif/bridgeif_fdb.o ./lwip/src/netif/ethernet.d ./lwip/src/netif/ethernet.o ./lwip/src/netif/lowpan6.d ./lwip/src/netif/lowpan6.o ./lwip/src/netif/lowpan6_ble.d ./lwip/src/netif/lowpan6_ble.o ./lwip/src/netif/lowpan6_common.d ./lwip/src/netif/lowpan6_common.o ./lwip/src/netif/slipif.d ./lwip/src/netif/slipif.o ./lwip/src/netif/zepif.d ./lwip/src/netif/zepif.o

.PHONY: clean-lwip-2f-src-2f-netif

