################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_adc.c \
../drivers/fsl_adc_etc.c \
../drivers/fsl_cache.c \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_common_arm.c \
../drivers/fsl_dmamux.c \
../drivers/fsl_edma.c \
../drivers/fsl_enet.c \
../drivers/fsl_flexcan.c \
../drivers/fsl_gpio.c \
../drivers/fsl_lpi2c.c \
../drivers/fsl_lpi2c_edma.c \
../drivers/fsl_lpuart.c \
../drivers/fsl_lpuart_edma.c \
../drivers/fsl_qtmr.c \
../drivers/fsl_romapi.c \
../drivers/fsl_xbara.c 

C_DEPS += \
./drivers/fsl_adc.d \
./drivers/fsl_adc_etc.d \
./drivers/fsl_cache.d \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_common_arm.d \
./drivers/fsl_dmamux.d \
./drivers/fsl_edma.d \
./drivers/fsl_enet.d \
./drivers/fsl_flexcan.d \
./drivers/fsl_gpio.d \
./drivers/fsl_lpi2c.d \
./drivers/fsl_lpi2c_edma.d \
./drivers/fsl_lpuart.d \
./drivers/fsl_lpuart_edma.d \
./drivers/fsl_qtmr.d \
./drivers/fsl_romapi.d \
./drivers/fsl_xbara.d 

OBJS += \
./drivers/fsl_adc.o \
./drivers/fsl_adc_etc.o \
./drivers/fsl_cache.o \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_common_arm.o \
./drivers/fsl_dmamux.o \
./drivers/fsl_edma.o \
./drivers/fsl_enet.o \
./drivers/fsl_flexcan.o \
./drivers/fsl_gpio.o \
./drivers/fsl_lpi2c.o \
./drivers/fsl_lpi2c_edma.o \
./drivers/fsl_lpuart.o \
./drivers/fsl_lpuart_edma.o \
./drivers/fsl_qtmr.o \
./drivers/fsl_romapi.o \
./drivers/fsl_xbara.o 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c drivers/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT1021DAG5A -DCPU_MIMXRT1021DAG5A_cm7 -DSDK_DEBUGCONSOLE=1 -DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DMCUXPRESSO_SDK -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DCHECKSUM_GEN_ICMP6=1 -DCHECKSUM_CHECK_ICMP6=1 -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE -DUSE_RTOS=1 -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\component\lists" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\eth" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\component\uart" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\component\serial_manager" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\device" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\drivers" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\xip" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\utilities" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\freertos\freertos-kernel\include" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\freertos\freertos-kernel\portable\GCC\ARM_CM4F" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\CMSIS" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\drivers\freertos" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\component\silicon_id" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\component\phy" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\lwip\port" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\component\gpio" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\lwip\contrib\apps\tcpecho" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\lwip\src" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\lwip\src\include" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\source" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\lwip\template" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_cd4052b" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_sys" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_eth" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\uart_comm" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\poll_adc" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\rgb_pwm" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_uart" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\charge_can" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_can" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_ext_rtc" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_eeprom" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\flash_data" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\calculate_crc" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\Signal" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\SignalManage" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\boot" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\board" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\lwip\src\include\lwip" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_cJSON" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\secc_logAndupgrade" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\cm_backtrace" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\cm_backtrace\Languages\en-US" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\cm_backtrace\Languages\zh-CN" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\pos" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\client" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\HAL\hal_adc" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\datasample" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\fan" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\cig" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\charge_control" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\fault" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\charge_process" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\imd" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\meter" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\led" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\hmi" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\charge_price" -I"D:\SVN_code\02.Software\01.CCU01\02.src\CCU01_DCDC\CCU01_APP_RT1021\user_code\APP\rfid" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-drivers

clean-drivers:
	-$(RM) ./drivers/fsl_adc.d ./drivers/fsl_adc.o ./drivers/fsl_adc_etc.d ./drivers/fsl_adc_etc.o ./drivers/fsl_cache.d ./drivers/fsl_cache.o ./drivers/fsl_clock.d ./drivers/fsl_clock.o ./drivers/fsl_common.d ./drivers/fsl_common.o ./drivers/fsl_common_arm.d ./drivers/fsl_common_arm.o ./drivers/fsl_dmamux.d ./drivers/fsl_dmamux.o ./drivers/fsl_edma.d ./drivers/fsl_edma.o ./drivers/fsl_enet.d ./drivers/fsl_enet.o ./drivers/fsl_flexcan.d ./drivers/fsl_flexcan.o ./drivers/fsl_gpio.d ./drivers/fsl_gpio.o ./drivers/fsl_lpi2c.d ./drivers/fsl_lpi2c.o ./drivers/fsl_lpi2c_edma.d ./drivers/fsl_lpi2c_edma.o ./drivers/fsl_lpuart.d ./drivers/fsl_lpuart.o ./drivers/fsl_lpuart_edma.d ./drivers/fsl_lpuart_edma.o ./drivers/fsl_qtmr.d ./drivers/fsl_qtmr.o ./drivers/fsl_romapi.d ./drivers/fsl_romapi.o ./drivers/fsl_xbara.d ./drivers/fsl_xbara.o

.PHONY: clean-drivers

