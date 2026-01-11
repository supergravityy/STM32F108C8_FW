################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/tester/logic_autoTester.c 

OBJS += \
./Core/tester/logic_autoTester.o 

C_DEPS += \
./Core/tester/logic_autoTester.d 


# Each subdirectory must supply rules for building sources it contributes
Core/tester/%.o Core/tester/%.su Core/tester/%.cyclo: ../Core/tester/%.c Core/tester/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-tester

clean-Core-2f-tester:
	-$(RM) ./Core/tester/logic_autoTester.cyclo ./Core/tester/logic_autoTester.d ./Core/tester/logic_autoTester.o ./Core/tester/logic_autoTester.su

.PHONY: clean-Core-2f-tester

