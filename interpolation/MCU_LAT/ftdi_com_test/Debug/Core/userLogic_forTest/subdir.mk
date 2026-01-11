################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/userLogic_forTest/ipol.c \
../Core/userLogic_forTest/logic_test.c 

OBJS += \
./Core/userLogic_forTest/ipol.o \
./Core/userLogic_forTest/logic_test.o 

C_DEPS += \
./Core/userLogic_forTest/ipol.d \
./Core/userLogic_forTest/logic_test.d 


# Each subdirectory must supply rules for building sources it contributes
Core/userLogic_forTest/%.o Core/userLogic_forTest/%.su Core/userLogic_forTest/%.cyclo: ../Core/userLogic_forTest/%.c Core/userLogic_forTest/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-userLogic_forTest

clean-Core-2f-userLogic_forTest:
	-$(RM) ./Core/userLogic_forTest/ipol.cyclo ./Core/userLogic_forTest/ipol.d ./Core/userLogic_forTest/ipol.o ./Core/userLogic_forTest/ipol.su ./Core/userLogic_forTest/logic_test.cyclo ./Core/userLogic_forTest/logic_test.d ./Core/userLogic_forTest/logic_test.o ./Core/userLogic_forTest/logic_test.su

.PHONY: clean-Core-2f-userLogic_forTest

