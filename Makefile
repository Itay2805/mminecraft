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
CFLAGS		+= -Werror -std=gnu11
CFLAGS 		+= -Wno-unused-label
CFLAGS 		+= -Wno-address-of-packed-member

ifeq ($(DEBUG),1)
	CFLAGS	+= -O0 -g
	CFLAGS	+= -fsanitize=undefined
	CFLAGS 	+= -fno-sanitize=alignment
	CFLAGS 	+= -fstack-protector-all
else
	CFLAGS	+= -O3 -g0 -flto
	CFLAGS 	+= -DNDEBUG
endif

CFLAGS 		+= -mtune=skylake -march=skylake
CFLAGS		+= -static -fshort-wchar
CFLAGS 		+= -Ilib -Isrc

LDFLAGS		:= $(CFLAGS) -luring -ljson-c

SRCS 		:= $(shell find src -name '*.c')

########################################################################################################################
# Targets
########################################################################################################################

all: $(BIN_DIR)/mminecraft.elf

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:%.o=%.d)
BINS ?=
-include $(DEPS)

$(BIN_DIR)/mminecraft.elf: $(OBJS) | Makefile
	@echo LD $@
	@mkdir -p $(@D)
	@$(LD) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@echo CC $@
	@mkdir -p $(@D)
	@$(CC) -Wall $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -rf out
