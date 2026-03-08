################################################################################
# RF24 library subdir.mk — STM32 HAL nRF24L01(+) driver
#
# This file is part of the reusable RF24 library and is NOT auto-generated.
# It is safe to edit. If CubeIDE regenerates the project, re-add the
# -include line to Debug/makefile and the SUBDIRS entry to Debug/sources.mk.
################################################################################

C_SRCS += \
../Drivers/RF24/Src/RF24_STM32.c

OBJS += \
./Drivers/RF24/Src/RF24_STM32.o

C_DEPS += \
./Drivers/RF24/Src/RF24_STM32.d

CYCLO_FILES += \
./Drivers/RF24/Src/RF24_STM32.cyclo

SU_FILES += \
./Drivers/RF24/Src/RF24_STM32.su

# Build rule — include all paths needed by the RF24 source
Drivers/RF24/Src/%.o Drivers/RF24/Src/%.su Drivers/RF24/Src/%.cyclo: \
        ../Drivers/RF24/Src/%.c \
        Drivers/RF24/Src/subdir.mk
	@mkdir -p Drivers/RF24/Src
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 \
	  -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB \
	  -c \
	  -I../Drivers/RF24/Inc \
	  -I../Core/Inc \
	  -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy \
	  -I../Drivers/STM32F1xx_HAL_Driver/Inc \
	  -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include \
	  -I../Drivers/CMSIS/Include \
	  -O0 -ffunction-sections -fdata-sections -Wall \
	  -fstack-usage -fcyclomatic-complexity \
	  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" \
	  --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-RF24-2f-Src

clean-Drivers-2f-RF24-2f-Src:
	-$(RM) \
	  ./Drivers/RF24/Src/RF24_STM32.cyclo \
	  ./Drivers/RF24/Src/RF24_STM32.d \
	  ./Drivers/RF24/Src/RF24_STM32.o \
	  ./Drivers/RF24/Src/RF24_STM32.su
