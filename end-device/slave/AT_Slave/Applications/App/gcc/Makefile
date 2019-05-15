# STM32 Makefile for GNU toolchain and openocd
#
# Usage:
#	make		Compile code
#	make clean	Clean build files
#	make flash	Flash program in the board, given by BOARD_PATH

# A name common to all output files (elf, map, hex, bin, lst)
TARGET     = AT_Slave
BOARD_PATH = /dev/ttyACM0/

# Take a look into $(CUBE_DIR)/Drivers/BSP for available BSPs
# name needed in upper case and lower case
BOARD      = B-L072Z-LRWAN1
BOARD_UC   = B-L072Z-LRWAN1
BOARD_MC   = B-L072Z-LRWAN1
BOARD_LC   = b-l072z-lrwan1
RADIO      = sx1276
COMPONENT  = mlm32l0xx
COMP_MC    = MLM32L07X01
COMP_LC    = mlm32l07x01
BSP_BASE   = $(BOARD)

APP_ROOT   = ..

# MCU family and type in various capitalizations o_O
MCU_FAMILY = stm32l0xx
MCU_FAM_MC = STM32L0xx
MCU_LC     = stm32l072xx
MCU_MC     = STM32L072xx
MCU_UC     = STM32L072CZ

# path of the ld-file inside the example directories
LDFILE     = ../../SW4STM32/$(COMP_LC)/$(MCU_UC)Yx_FLASH.ld

# C files from the /src directory
SRCS       = main.c
SRCS      += ../../../Core/src/$(COMPONENT)_it.c
SRCS      += system_$(MCU_FAMILY).c
SRCS      += ../../../Core/src/$(COMPONENT)_hw.c
SRCS      += at.c
SRCS      += command.c
SRCS      += debug.c
SRCS      += hw_gpio.c
SRCS      += hw_rtc.c
SRCS      += hw_spi.c
SRCS      += vcom.c
SRCS      += lora.c
SRCS      += test_rf.c
SRCS      += tiny_sscanf.c
SRCS      += tiny_vsnprintf.c
SRCS      += eeprom.c

# Basic HAL libraries
SRCS      += ../../../Core/src/$(COMPONENT)_hal_msp.c
SRCS      += $(MCU_FAMILY)_hal_spi.c
SRCS      += $(MCU_FAMILY)_hal_uart.c
SRCS      += $(MCU_FAMILY)_hal_uart_ex.c
SRCS      += $(MCU_FAMILY)_hal_usart.c
SRCS      += $(MCU_FAMILY)_hal_adc.c
SRCS      += $(MCU_FAMILY)_hal_adc_ex.c
SRCS      += $(MCU_FAMILY)_hal_rcc.c
SRCS      += $(MCU_FAMILY)_hal_rcc_ex.c
SRCS      += $(MCU_FAMILY)_hal.c
SRCS      += $(MCU_FAMILY)_hal_cortex.c
SRCS      += $(MCU_FAMILY)_hal_gpio.c
SRCS      += $(MCU_FAMILY)_hal_dma.c
SRCS      += $(MCU_FAMILY)_hal_pwr.c
SRCS      += $(MCU_FAMILY)_hal_pwr_ex.c
SRCS      += $(MCU_FAMILY)_hal_rtc.c
SRCS      += $(MCU_FAMILY)_hal_rtc_ex.c
SRCS      += $(MCU_FAMILY)_hal_tim.c
SRCS      += $(MCU_FAMILY)_hal_tim_ex.c
SRCS      += $(MCU_FAMILY)_hal_flash.c
SRCS      += $(MCU_FAMILY)_hal_flash_ex.c
SRCS      += $(MCU_FAMILY)_hal_i2c.c
SRCS      += $(MCU_FAMILY)_ll_dma.c

# LoRa
SRCS      += lora.c
SRCS      += lora-test.c
SRCS      += LoRaMac.c
SRCS      += LoRaMacAdr.c
SRCS      += LoRaMacClassB.c
SRCS      += LoRaMacCrypto.c
SRCS      += LoRaMacSerializer.c
SRCS      += LoRaMacCommands.c
SRCS      += LoRaMacConfirmQueue.c
SRCS      += LoRaMacFCntHandler.c
SRCS      += LoRaMacParser.c
SRCS      += Region.c
#SRCS      += RegionAS923.c
#SRCS      += RegionAU915.c
#SRCS      += RegionCN470.c
#SRCS      += RegionCN779.c
SRCS      += RegionCommon.c
#SRCS      += RegionEU433.c
#SRCS      += RegionEU868.c
#SRCS      += RegionIN865.c
#SRCS      += RegionKR920.c
SRCS      += RegionUS915.c
SRCS      += low_power_manager.c
SRCS      += timeServer.c
SRCS      += utilities.c
SRCS      += queue.c
SRCS      += systime.c
SRCS      += trace.c
SRCS      += aes.c
SRCS      += cmac.c
SRCS      += soft-se.c

# Drivers/BSP/Components
SRCS      += HTS221_Driver.c
SRCS      += HTS221_Driver_HL.c
SRCS      += LPS25HB_Driver.c
SRCS      += LPS25HB_Driver_HL.c
SRCS      += LPS22HB_Driver.c
SRCS      += LPS22HB_Driver_HL.c
SRCS      += $(RADIO).c

# Drivers/BSP/...
SRCS      += $(BOARD_LC).c
SRCS      += ../$(CUBE_DIR)/Drivers/BSP/CMWX1ZZABZ-0xx/$(COMP_LC).c

# Directories
CUBE_DIR   = ../../..

BSP_DIR    = $(CUBE_DIR)/Drivers/BSP/$(COMP_MC)
HAL_DIR    = $(CUBE_DIR)/Drivers/$(MCU_FAM_MC)_HAL_Driver
CMSIS_DIR  = $(CUBE_DIR)/Drivers/CMSIS

DEV_DIR    = $(CMSIS_DIR)/Device/ST/$(MCU_FAM_MC)

COMP_DIR   = $(CUBE_DIR)/Drivers/BSP/Components
MWARE_DIR  = $(CUBE_DIR)/Middlewares/Third_Party

###############################################################################
# Toolchain
PREFIX     = arm-none-eabi
CC         = $(PREFIX)-gcc
AR         = $(PREFIX)-ar
OBJCOPY    = $(PREFIX)-objcopy
OBJDUMP    = $(PREFIX)-objdump
SIZE       = $(PREFIX)-size
GDB        = $(PREFIX)-gdb

###############################################################################

# Defines
DEFS       = -D$(MCU_MC) -DUSE_HAL_DRIVER

# LoRa
DEFS       += -DUSE_B_L072Z_LRWAN1
DEFS       += -DREGION_US915
DEFS       += -DUSE_BAND_915

# Debug specific definitions for semihosting
DEFS       += -DUSE_DBPRINTF

# Include search paths (-I)
INCS       = -I$(APP_ROOT)/inc
INCS      += -I$(BSP_DIR)
INCS      += -I$(CUBE_DIR)/Drivers/BSP/$(BOARD_MC)
INCS      += -I$(CUBE_DIR)/Drivers/BSP/CMWX1ZZABZ-0xx
INCS      += -I$(CMSIS_DIR)/Include
INCS      += -I$(DEV_DIR)/Include
INCS      += -I$(HAL_DIR)/Inc

INCS      += -I$(COMP_DIR)/Common
INCS      += -I$(COMP_DIR)/hts221
INCS      += -I$(COMP_DIR)/lps22hb
INCS      += -I$(COMP_DIR)/lps25hb
INCS      += -I$(COMP_DIR)/$(RADIO)

# LoRa
INCS      += -I$(MWARE_DIR)/LoRaWAN/Crypto
INCS      += -I$(MWARE_DIR)/LoRaWAN/Core
INCS      += -I$(MWARE_DIR)/LoRaWAN/Mac
INCS      += -I$(MWARE_DIR)/LoRaWAN/Mac/region
INCS      += -I$(MWARE_DIR)/LoRaWAN/Phy
INCS      += -I$(MWARE_DIR)/LoRaWAN/Utilities
INCS      += -I../../Core/inc
INCS      += -I../../Core/src

# Library search paths
LIBS       = -L$(CMSIS_DIR)/Lib

# Compiler flags
CFLAGS     = -Wall -g -std=c99 -Os
CFLAGS    += -mcpu=cortex-m0plus -mthumb
#CFLAGS    += -Wextra
CFLAGS    += -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS    += -fmessage-length=0 -fno-exceptions -fno-builtin -funsigned-char
CFLAGS    += -MMD -fno-delete-null-pointer-checks -fomit-frame-pointer
#CFLAGS    += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS    += -ffunction-sections -fdata-sections
CFLAGS    += $(INCS) $(DEFS)

# Linker flags
LDFLAGS    = -Wl,--gc-sections -Wl,-Map=$(TARGET).map $(LIBS) -T$(LDFILE)

# Enable Semihosting
LDFLAGS   += --specs=rdimon.specs -lc -lrdimon

# Source search paths
VPATH      = $(APP_ROOT)/src
VPATH     += $(BSP_DIR)
VPATH     += $(CUBE_DIR)/Drivers/BSP/$(BOARD_MC)
VPATH     += $(HAL_DIR)/Src
VPATH     += $(DEV_DIR)/Source
VPATH     += $(DEV_DIR)/Source/Templates

# Components
VPATH     += $(COMP_DIR)/hts221
VPATH     += $(COMP_DIR)/lps25hb
VPATH     += $(COMP_DIR)/lps22hb
VPATH     += $(COMP_DIR)/$(RADIO)

# LoRa
VPATH     += $(MWARE_DIR)/LoRaWAN/Crypto
VPATH     += $(MWARE_DIR)/LoRaWAN/Core
VPATH     += $(MWARE_DIR)/LoRaWAN/Mac
VPATH     += $(MWARE_DIR)/LoRaWAN/Mac/region
VPATH     += $(MWARE_DIR)/LoRaWAN/Phy
VPATH     += $(MWARE_DIR)/LoRaWAN/Utilities

OBJS       = $(addprefix obj/,$(SRCS:.c=.o))
DEPS       = $(addprefix dep/,$(SRCS:.c=.d))

# Prettify output
V = 1
ifeq ($V, 0)
	Q = @
	P = > /dev/null
endif

###################################################

.PHONY: all dirs program debug template clean

all: $(TARGET).bin

-include $(DEPS)

dirs: dep obj
dep obj src:
	@echo "[MKDIR]   $@"
	$Qmkdir -p $@

obj/%.o : %.c | dirs
	@echo "[CC]      $(notdir $<)"
	$Q$(CC) $(CFLAGS) -c -o $@ $< -MMD -MF dep/$(*F).d

$(TARGET).elf: $(OBJS)
	@echo "[LD]      $(TARGET).elf"
	$Q$(CC) $(CFLAGS) $(LDFLAGS) ../../SW4STM32/startup_$(MCU_LC).s $^ -o $@
	@echo "[OBJDUMP] $(TARGET).lst"
	$Q$(OBJDUMP) -St $(TARGET).elf >$(TARGET).lst
	@echo "[SIZE]    $(TARGET).elf"
	$(SIZE) $(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	@echo "[OBJCOPY] $(TARGET).bin"
	$Q$(OBJCOPY) -O binary $< $@

clean:
	@echo "[RM]      $(TARGET).bin"; rm -f $(TARGET).bin
	@echo "[RM]      $(TARGET).elf"; rm -f $(TARGET).elf
	@echo "[RM]      $(TARGET).map"; rm -f $(TARGET).map
	@echo "[RM]      $(TARGET).lst"; rm -f $(TARGET).lst
	@echo "[RMDIR]   dep"          ; rm -fr dep
	@echo "[RMDIR]   obj"          ; rm -fr obj

flash:
	+@echo "Flashing board"
	+@make
	+@cp -r $(TARGET).bin $(BOARD_PATH)
	+@echo "Flashing done"
