ST_FAMILY = f4
DEPS_SUBMODULES += lib/CMSIS_5 hw/mcu/st/cmsis_device_$(ST_FAMILY) hw/mcu/st/stm32$(ST_FAMILY)xx_hal_driver

ST_CMSIS = hw/mcu/st/cmsis_device_$(ST_FAMILY)
ST_HAL_DRIVER = hw/mcu/st/stm32$(ST_FAMILY)xx_hal_driver

LD_FILE = application/$(BOARD)/STM32F401VCTx_FLASH.ld

CFLAGS += \
  -flto \
  -mthumb \
  -mabi=aapcs \
  -mcpu=cortex-m4 \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 \
  -nostdlib -nostartfiles \
  -DCFG_TUSB_MCU=OPT_MCU_STM32F4 \
  -DSTM32F401xC

# suppress warning caused by vendor mcu driver
CFLAGS += -Wno-error=cast-align

SRC_S += $(ST_CMSIS)/Source/Templates/gcc/startup_stm32f401xc.s

SRC_C += \
	src/portable/synopsys/dwc2/dcd_dwc2.c \
	$(ST_CMSIS)/Source/Templates/system_stm32$(ST_FAMILY)xx.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_cortex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc_ex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_uart.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_gpio.c

INC += \
	$(TOP)/lib/CMSIS_5/CMSIS/Core/Include \
	$(TOP)/$(ST_CMSIS)/Include \
	$(TOP)/$(ST_HAL_DRIVER)/Inc

# For freeRTOS port source
FREERTOS_PORT = ARM_CM4F

# # For flash-jlink target
# JLINK_DEVICE = stm32f401cc

# # flash target using on-board stlink
# flash: flash-stlink

# # flash target ROM bootloader
# flash: $(BUILD)/$(PROJECT).bin
# 	dfu-util -R -a 0 --dfuse-address 0x08000000 -D $<