################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include xip/subdir.mk
-include utilities/subdir.mk
-include user_code/Signal/subdir.mk
-include user_code/HAL/hal_uart/subdir.mk
-include user_code/HAL/hal_sys/subdir.mk
-include user_code/HAL/hal_ext_rtc/subdir.mk
-include user_code/HAL/hal_eth/subdir.mk
-include user_code/HAL/hal_eeprom/subdir.mk
-include user_code/HAL/hal_cd4052b/subdir.mk
-include user_code/HAL/hal_can/subdir.mk
-include user_code/HAL/hal_cJSON/subdir.mk
-include user_code/HAL/hal_adc/subdir.mk
-include user_code/HAL/flash_data/subdir.mk
-include user_code/HAL/calculate_crc/subdir.mk
-include user_code/HAL/boot/subdir.mk
-include user_code/HAL/SignalManage/subdir.mk
-include user_code/APP/uart_comm/subdir.mk
-include user_code/APP/secc_logAndupgrade/subdir.mk
-include user_code/APP/rgb_pwm/subdir.mk
-include user_code/APP/rfid/subdir.mk
-include user_code/APP/pos/subdir.mk
-include user_code/APP/poll_adc/subdir.mk
-include user_code/APP/meter/subdir.mk
-include user_code/APP/led/subdir.mk
-include user_code/APP/imd/subdir.mk
-include user_code/APP/hmi/subdir.mk
-include user_code/APP/fault/subdir.mk
-include user_code/APP/fan/subdir.mk
-include user_code/APP/eth/subdir.mk
-include user_code/APP/datasample/subdir.mk
-include user_code/APP/cm_backtrace/subdir.mk
-include user_code/APP/client/subdir.mk
-include user_code/APP/cig/subdir.mk
-include user_code/APP/charge_process/subdir.mk
-include user_code/APP/charge_price/subdir.mk
-include user_code/APP/charge_control/subdir.mk
-include user_code/APP/charge_can/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include lwip/src/netif/ppp/subdir.mk
-include lwip/src/netif/subdir.mk
-include lwip/src/core/ipv6/subdir.mk
-include lwip/src/core/ipv4/subdir.mk
-include lwip/src/core/subdir.mk
-include lwip/src/api/subdir.mk
-include lwip/port/subdir.mk
-include lwip/contrib/apps/tcpecho/subdir.mk
-include freertos/freertos-kernel/portable/MemMang/subdir.mk
-include freertos/freertos-kernel/portable/GCC/ARM_CM4F/subdir.mk
-include freertos/freertos-kernel/subdir.mk
-include drivers/freertos/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/silicon_id/subdir.mk
-include component/serial_manager/subdir.mk
-include component/phy/subdir.mk
-include component/lists/subdir.mk
-include component/gpio/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := CCU01_APP_RT1021
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: CCU01_APP_RT1021.axf

# Tool invocations
CCU01_APP_RT1021.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -no-warn-rwx-segments -Xlinker -Map="CCU01_APP_RT1021.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -T CCU01_APP_RT1021_Debug.ld -o "CCU01_APP_RT1021.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) CCU01_APP_RT1021.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "CCU01_APP_RT1021.axf"; # arm-none-eabi-objcopy -v -O binary "CCU01_APP_RT1021.axf" "CCU01_APP_RT1021.bin" ; # checksum -p MIMXRT1021xxxxx -d "CCU01_APP_RT1021.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
