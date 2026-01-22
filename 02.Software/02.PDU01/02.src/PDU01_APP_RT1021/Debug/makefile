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
-include user_code/HAL/hal_eeprom/subdir.mk
-include user_code/HAL/hal_can/subdir.mk
-include user_code/HAL/hal_adc/subdir.mk
-include user_code/HAL/flash_data/subdir.mk
-include user_code/HAL/factory_test/subdir.mk
-include user_code/HAL/calculate_crc/subdir.mk
-include user_code/HAL/boot/subdir.mk
-include user_code/HAL/SignalManage/subdir.mk
-include user_code/APP/uart_comm/subdir.mk
-include user_code/APP/relay_ctrl/subdir.mk
-include user_code/APP/poll_adc/subdir.mk
-include user_code/APP/pdu_can/subdir.mk
-include user_code/APP/data_sample/subdir.mk
-include user_code/APP/average_filter/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include freertos/freertos-kernel/portable/MemMang/subdir.mk
-include freertos/freertos-kernel/portable/GCC/ARM_CM4F/subdir.mk
-include freertos/freertos-kernel/subdir.mk
-include drivers/freertos/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
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


BUILD_ARTIFACT_NAME := PDU01_APP_RT1021
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: PDU01_APP_RT1021.axf

# Tool invocations
PDU01_APP_RT1021.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -no-warn-rwx-segments -Xlinker -Map="PDU01_APP_RT1021.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -T PDU01_APP_RT1021_Debug.ld -o "PDU01_APP_RT1021.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) PDU01_APP_RT1021.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "PDU01_APP_RT1021.axf"; # arm-none-eabi-objcopy -v -O binary "PDU01_APP_RT1021.axf" "PDU01_APP_RT1021.bin" ; # checksum -p MIMXRT1021xxxxx -d "PDU01_APP_RT1021.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
