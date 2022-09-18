########################################################################################################################
# Build constants
########################################################################################################################

CC 			:= ccache clang
LD			:= clang

# Build in debug or release mode
DEBUG		:= 0

OUT_DIR		:= out
BIN_DIR		:= $(OUT_DIR)/bin
BUILD_DIR	:= $(OUT_DIR)/build

#-----------------------------------------------------------------------------------------------------------------------
# General configurations
#-----------------------------------------------------------------------------------------------------------------------

CFLAGS 		:= -target x86_64-pc-linux-elf
CFLAGS		+= -Werror -std=gnu17
CFLAGS 		+= -Wno-unused-label
CFLAGS 		+= -Wno-address-of-packed-member

ifeq ($(DEBUG),1)
	CFLAGS	+= -Og -g
	CFLAGS	+= -fsanitize=undefined
	CFLAGS 	+= -fno-sanitize=alignment
	CFLAGS 	+= -fstack-protector-all
else
	CFLAGS	+= -O3 -g -flto
	CFLAGS 	+= -DNDEBUG
endif

CFLAGS 		+= -mtune=skylake -march=skylake
CFLAGS		+= -static -fshort-wchar
CFLAGS 		+= -Ilib -Isrc -I$(BUILD_DIR)

SRCS 		:= $(shell find src -name '*.c')

MCDATA_BASE	:= minecraft-data/data/pc
MCDATA_VER	:= 1.16.2
MCDATA_PATH	:= $(MCDATA_BASE)/$(MCDATA_VER)

MCDATA_LOGIN_PACKET := $(MCDATA_PATH)/loginPacket.json

#-----------------------------------------------------------------------------------------------------------------------
# Flecs config
#-----------------------------------------------------------------------------------------------------------------------

SRCS		+= lib/flecs/flecs.c

########################################################################################################################
# Targets
########################################################################################################################

LDFLAGS		:= $(CFLAGS) -luring -ljson-c -luuid

all: $(BIN_DIR)/mminecraft.elf

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:%.o=%.d)
BINS ?=
-include $(DEPS)

GENERATED_HEADERS 	:= $(BUILD_DIR)/nbt/dimension_codec.nbt.h
GENERATED_HEADERS	+= $(BUILD_DIR)/nbt/overworld.nbt.h
GENERATED_HEADERS	+= $(BUILD_DIR)/nbt/the_nether.nbt.h
GENERATED_HEADERS	+= $(BUILD_DIR)/nbt/the_end.nbt.h

$(BIN_DIR)/mminecraft.elf: $(OBJS) | Makefile
	@echo LD $@
	@mkdir -p $(@D)
	@$(LD) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c | $(GENERATED_HEADERS)
	@echo CC $@
	@mkdir -p $(@D)
	@$(CC) -Wall $(CFLAGS) -MMD -c $< -o $@

#-----------------------------------------------------------------------------------------------------------------------
# NBT generation
#-----------------------------------------------------------------------------------------------------------------------

# TODO: create better rules, I am too lazy for now

$(BUILD_DIR)/nbt/dimension_codec.nbt: $(MCDATA_LOGIN_PACKET) | ./scripts/generate_dimesion_codec.py
	@mkdir -p $(@D)
	./scripts/generate_dimesion_codec.py $^ $(dir $@)

$(BUILD_DIR)/nbt/overworld.nbt: $(BUILD_DIR)/nbt/dimension_codec.nbt
$(BUILD_DIR)/nbt/the_nether.nbt: $(BUILD_DIR)/nbt/dimension_codec.nbt
$(BUILD_DIR)/nbt/the_end.nbt: $(BUILD_DIR)/nbt/dimension_codec.nbt

$(BUILD_DIR)/nbt/dimension_codec.nbt.h: $(BUILD_DIR)/nbt/dimension_codec.nbt
	@mkdir -p $(@D)
	xxd -i $^ $^.h

$(BUILD_DIR)/nbt/overworld.nbt.h: $(BUILD_DIR)/nbt/overworld.nbt
	@mkdir -p $(@D)
	xxd -i $^ $^.h

$(BUILD_DIR)/nbt/the_nether.nbt.h: $(BUILD_DIR)/nbt/the_nether.nbt
	@mkdir -p $(@D)
	xxd -i $^ $^.h

$(BUILD_DIR)/nbt/the_end.nbt.h: $(BUILD_DIR)/nbt/the_end.nbt
	@mkdir -p $(@D)
	xxd -i $^ $^.h

clean:
	rm -rf out
