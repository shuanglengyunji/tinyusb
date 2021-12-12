# ---------------------------------------
# Common make rules for all examples
# ---------------------------------------

# Set all as default goal
.DEFAULT_GOAL := all

# ---------------------------------------
# Compiler Flags
# ---------------------------------------

# libc
LIBS += -lgcc -lm -lnosys -lc

# TinyUSB Stack source
SRC_C += \
	src/tusb.c \
	src/common/tusb_fifo.c \
	src/device/usbd.c \
	src/device/usbd_control.c \
	src/class/audio/audio_device.c \
	src/class/cdc/cdc_device.c \
	src/class/dfu/dfu_device.c \
	src/class/dfu/dfu_rt_device.c \
	src/class/hid/hid_device.c \
	src/class/midi/midi_device.c \
	src/class/msc/msc_device.c \
	src/class/net/ecm_rndis_device.c \
	src/class/net/ncm_device.c \
	src/class/usbtmc/usbtmc_device.c \
	src/class/video/video_device.c \
	src/class/vendor/vendor_device.c

# TinyUSB stack include
INC += $(TOP)/src

CFLAGS += $(addprefix -I,$(INC))

# LTO makes it difficult to analyze map file for optimizing size purpose
# We will run this option in ci
ifeq ($(NO_LTO),1)
CFLAGS := $(filter-out -flto,$(CFLAGS))
endif

LDFLAGS += $(CFLAGS) -Wl,-T,$(TOP)/$(LD_FILE) -Wl,-Map=$@.map -Wl,-cref -Wl,-gc-sections
ifneq ($(SKIP_NANOLIB), 1)
LDFLAGS += -specs=nosys.specs -specs=nano.specs
endif

ASFLAGS += $(CFLAGS)

# Assembly files can be name with upper case .S, convert it to .s
SRC_S := $(SRC_S:.S=.s)

# Due to GCC LTO bug https://bugs.launchpad.net/gcc-arm-embedded/+bug/1747966
# assembly file should be placed first in linking order
# '_asm' suffix is added to object of assembly file
OBJ += $(addprefix $(BUILD)/obj/, $(SRC_S:.s=_asm.o))
OBJ += $(addprefix $(BUILD)/obj/, $(SRC_C:.c=.o))

# Verbose mode
ifeq ("$(V)","1")
$(info CFLAGS  $(CFLAGS) ) $(info )
$(info LDFLAGS $(LDFLAGS)) $(info )
$(info ASFLAGS $(ASFLAGS)) $(info )
endif

# ---------------------------------------
# Rules
# ---------------------------------------

all: $(BUILD)/$(PROJECT).bin $(BUILD)/$(PROJECT).hex size
	@echo Building $(PROJECT) in $(BUILD)

OBJ_DIRS = $(sort $(dir $(OBJ)))
$(OBJ): | $(OBJ_DIRS)
$(OBJ_DIRS):
	@$(MKDIR) -p $@

$(BUILD)/$(PROJECT).elf: $(OBJ)
	@echo LINK $@
	@$(CC) -o $@ $(LDFLAGS) $^ -Wl,--start-group $(LIBS) -Wl,--end-group

$(BUILD)/$(PROJECT).bin: $(BUILD)/$(PROJECT).elf
	@echo CREATE $@
	@$(OBJCOPY) -O binary $^ $@

$(BUILD)/$(PROJECT).hex: $(BUILD)/$(PROJECT).elf
	@echo CREATE $@
	@$(OBJCOPY) -O ihex $^ $@

# We set vpath to point to the top of the tree so that the source files
# can be located. By following this scheme, it allows a single build rule
# to be used to compile all .c files.
vpath %.c . $(TOP)
$(BUILD)/obj/%.o: %.c
	@echo CC $(notdir $@)
	@$(CC) $(CFLAGS) -c -MD -o $@ $<

# ASM sources lower case .s
vpath %.s . $(TOP)
$(BUILD)/obj/%_asm.o: %.s
	@echo AS $(notdir $@)
	@$(CC) -x assembler-with-cpp $(ASFLAGS) -c -o $@ $<

# ASM sources upper case .S
vpath %.S . $(TOP)
$(BUILD)/obj/%_asm.o: %.S
	@echo AS $(notdir $@)
	@$(CC) -x assembler-with-cpp $(ASFLAGS) -c -o $@ $<

size: $(BUILD)/$(PROJECT).elf
	-@echo ''
	@$(SIZE) $<
	-@echo ''

# linkermap must be install previously at https://github.com/hathach/linkermap
linkermap: $(BUILD)/$(PROJECT).elf
	@linkermap -v $<.map

.PHONY: clean
clean:
	$(RM) -rf $(BUILD)

# ---------------------------------------
# Flash Targets
# ---------------------------------------

# Jlink Interface
JLINK_IF ?= swd

# Flash using jlink
flash-jlink: $(BUILD)/$(PROJECT).hex
	@echo halt > $(BUILD)/$(BOARD).jlink
	@echo r > $(BUILD)/$(BOARD).jlink
	@echo loadfile $^ >> $(BUILD)/$(BOARD).jlink
	@echo r >> $(BUILD)/$(BOARD).jlink
	@echo go >> $(BUILD)/$(BOARD).jlink
	@echo exit >> $(BUILD)/$(BOARD).jlink
	JLinkExe -device $(JLINK_DEVICE) -if $(JLINK_IF) -JTAGConf -1,-1 -speed auto -CommandFile $(BUILD)/$(BOARD).jlink

# Flash STM32 MCU using stlink with STM32 Cube Programmer CLI
flash-stlink: $(BUILD)/$(PROJECT).elf
	STM32_Programmer_CLI --connect port=swd --write $< --go
