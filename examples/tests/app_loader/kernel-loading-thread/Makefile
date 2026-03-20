# Makefile for user application (OpenThread MTD + CoAP)

# Path to libtock-c root relative to this app
TOCK_USERLAND_BASE_DIR = ../../../..

# Pull in OpenThread (MTD) integration for libtock-c
include $(TOCK_USERLAND_BASE_DIR)/libopenthread/libopenthread-mtd.mk

# App name (optional)
APP_NAME := ot_coap_downloader

# Larger stack for OpenThread
STACK_SIZE := 6000

# Your sources
C_SRCS := main.c

# Link the OpenThread CoAP library (the lib path is exported by the mk above)
# LIBRARIES += $(LIBOPENTHREAD_LIB_DIR)/libopenthread-coap.a

# Build with the standard libtock-c rules
include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk

# ===== paths =====
BUILD_DIR := build
BIN_NAME := button-press-loading
TAB_FILE  := $(BUILD_DIR)/$(BIN_NAME).tab
EXTRACT_DIR := $(BUILD_DIR)/flash_extract
APP_BIN := $(EXTRACT_DIR)/$(BIN_NAME).bin
APP_BIN_ABS := $(abspath $(APP_BIN))

# ===== placement =====
APP_FLASH_ADDRESS ?= 0x00040000   # nrf52dk's _sapps default
ERASE_LEN ?= 0x20000              # 128 KiB window

# ===== J-Link =====
JLINK_EXE    ?= JLinkExe
JLINK_DEVICE ?= NRF52840_XXAA
JLINK_IF     ?= SWD
JLINK_SPEED  ?= 4000

.PHONY: flash flash-jlink flash-jlink-addr clean-flash-extract
flash: flash-jlink

$(EXTRACT_DIR):
	mkdir -p $(EXTRACT_DIR)

# untar .tab, grab first .tbf, rename to .bin
$(APP_BIN): $(TAB_FILE) | $(EXTRACT_DIR)
	@echo "Extracting first .tbf from $(TAB_FILE) -> $(APP_BIN)"
	@set -e; \
	tar -xf $(TAB_FILE) -C $(EXTRACT_DIR); \
	TBF=$$(ls $(EXTRACT_DIR)/*.tbf | head -n1); \
	if [ -z "$$TBF" ]; then echo "No .tbf found in $(TAB_FILE)"; exit 1; fi; \
	cp -f "$$TBF" $(APP_BIN)

flash-jlink: $(APP_BIN)
	@echo "Flashing $(APP_BIN_ABS) to $(APP_FLASH_ADDRESS) with J-Link..."
	@SCRIPT=$$(mktemp /tmp/jlink_XXXXXX.jlink); \
	echo "halt"                                            >  $$SCRIPT; \
	echo "erase $(APP_FLASH_ADDRESS),$(ERASE_LEN)"         >> $$SCRIPT; \
	echo "loadbin $(APP_BIN_ABS), $(APP_FLASH_ADDRESS)"    >> $$SCRIPT; \
	echo "verifybin $(APP_BIN_ABS) $(APP_FLASH_ADDRESS)"   >> $$SCRIPT; \
	echo "g"                                               >> $$SCRIPT; \
	echo "q"                                               >> $$SCRIPT; \
	$(JLINK_EXE) -device $(JLINK_DEVICE) -if $(JLINK_IF) -speed $(JLINK_SPEED) -autoconnect 1 -CommandFile $$SCRIPT; \
	rm -f $$SCRIPT

flash-jlink-addr: $(APP_BIN)
	@if [ -z "$(ADDR)" ]; then echo "Usage: make $@ ADDR=0x3xxxx"; exit 1; fi
	$(MAKE) flash-jlink APP_FLASH_ADDRESS=$(ADDR)

clean-flash-extract:
	rm -rf $(EXTRACT_DIR)

