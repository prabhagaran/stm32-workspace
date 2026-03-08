@echo off
setlocal enabledelayedexpansion
set CC=arm-none-eabi-gcc
set CP=arm-none-eabi-objcopy
set SZ=arm-none-eabi-size
set CFLAGS=-mcpu=cortex-m3 -mthumb -DUSE_HAL_DRIVER -DSTM32F103xB -ICore/Inc -IDrivers/STM32F1xx_HAL_Driver/Inc/Legacy -IDrivers/STM32F1xx_HAL_Driver/Inc -IDrivers/CMSIS/Device/ST/STM32F1xx/Include -IDrivers/CMSIS/Include -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2
if not exist build mkdir build














%CC% %OBJECTS% -mcpu=cortex-m3 -mthumb -specs=nano.specs -TSTM32F103XX_FLASH.ld -lc -lm -lnosys -Wl,-Map=build/led_links.map,--cref -Wl,--gc-sections -o build/led_links.elf || goto :err
n
n%SZ% build/led_links.elf
n%CP% -O ihex build/led_links.elf build/led_links.hex
n%CP% -O binary -S build/led_links.elf build/led_links.bin
necho Build complete
nexit /b 0
n:err
necho Build failed
nexit /b 1%CC% -x assembler-with-cpp %CFLAGS% -c startup_stm32f103xb.s -o build/startup_stm32f103xb.o || goto :err
n
necho Linking
nset OBJECTS=
nfor %%f in (build\*.o) do set OBJECTS=%%f !OBJECTS!%CC% %CFLAGS% -c Core/Src/syscalls.c -o build/syscalls.o || goto :err
n
necho Assembling startup%CC% %CFLAGS% -c Core/Src/sysmem.c -o build/sysmem.o || goto :err%CC% %CFLAGS% -c Core/Src/system_stm32f1xx.c -o build/system_stm32f1xx.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c -o build/stm32f1xx_hal_exti.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c -o build/stm32f1xx_hal_flash_ex.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c -o build/stm32f1xx_hal_flash.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c -o build/stm32f1xx_hal_pwr.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c -o build/stm32f1xx_hal_cortex.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c -o build/stm32f1xx_hal_dma.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c -o build/stm32f1xx_hal_gpio.o || goto :err%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c -o build/stm32f1xx_hal_rcc_ex.o || goto :errn
necho Compiling sources...
n%CC% %CFLAGS% -c Core/Src/main.c -o build/main.o || goto :err
n%CC% %CFLAGS% -c Core/Src/stm32f1xx_it.c -o build/stm32f1xx_it.o || goto :err
n%CC% %CFLAGS% -c Core/Src/stm32f1xx_hal_msp.c -o build/stm32f1xx_hal_msp.o || goto :err
n%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c -o build/stm32f1xx_hal_gpio_ex.o || goto :err
n%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c -o build/stm32f1xx_hal.o || goto :err
n%CC% %CFLAGS% -c Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c -o build/stm32f1xx_hal_rcc.o || goto :err