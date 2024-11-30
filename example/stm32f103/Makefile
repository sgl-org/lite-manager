TARGET    := demo
BUILD_DIR := build


# include configuration file for makefile
.PHONY: check_lmmk
ifneq ($(wildcard .lm.mk),)
-include .lm.mk
else
check_lmmk:
	@echo "Please run 'make config'"
endif


# toolchain
CC_PREFIX ?= arm-none-eabi-
CC = $(CC_PREFIX)gcc
AS = $(CC_PREFIX)gcc -x assembler-with-cpp
CP = $(CC_PREFIX)objcopy
SZ = $(CC_PREFIX)size
OD = $(CC_PREFIX)objdump
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S


CFLAGS    := $(C_PATH) $(C_DEFINE) $(C_FLAG) $(CPP_FLAG)
LDFLAGS   := $(LD_FLAG) $(LIB_PATH) -T$(LDS_SOURCE) -Wl,-M=$(BUILD_DIR)/$(TARGET).map


.PHONY: all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin elf_info


# list of c program objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCE:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCE)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCE:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCE)))


$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "CC   $<"
	@$(CC) -c $(CFLAGS) -MMD -MP \
		-MF  $(BUILD_DIR)/$(notdir $(<:.c=.d)) \
		-Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "AS   $<"
	@$(AS) -c $(CFLAGS) -MMD -MP  \
		-MF $(BUILD_DIR)/$(notdir $(<:.s=.d)) $< -o $@


$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "LD   $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@$(OD) $(BUILD_DIR)/$(TARGET).elf -xS > $(BUILD_DIR)/$(TARGET).s $@
	@printf "\n[32mBuild Successful! [0m \n"
	@echo "ELF   $@"


$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "HEX   $@"
	@$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "BIN   $@"
	@$(BIN) $< $@

elf_info: $(BUILD_DIR)/$(TARGET).elf
	@echo "=================================================================="
	@$(SZ) $<
	@echo "=================================================================="


$(BUILD_DIR):
	@mkdir $@


# Pseudo command
.PHONY: config clean

# Check if the proj.cfg file exists
config: proj.cfg
	@./lm.exe -c proj.cfg -f lm.cfg -o config.h -m 50
	@rm -rf $(BUILD_DIR)


# clean command, delete build directory
clean:
	@rm -rf $(BUILD_DIR)

