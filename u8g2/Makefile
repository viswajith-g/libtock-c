include Makefile.version

# Base folder definitions
TOCK_USERLAND_BASE_DIR ?= ..
LIBNAME := u8g2
$(LIBNAME)_DIR := $(TOCK_USERLAND_BASE_DIR)/$(LIBNAME)
LIB_SRC_DIR := $($(LIBNAME)_DIR)/u8g2-$(U8G2_VERSION_HASH)

# List all C and Assembly files
$(LIBNAME)_SRCS_ALL  += $(wildcard $(LIB_SRC_DIR)/csrc/u8g2*.c)
$(LIBNAME)_SRCS_ALL  += $(wildcard $(LIB_SRC_DIR)/csrc/u8x8*.c)
$(LIBNAME)_SRCS_ALL  += $(wildcard $(LIB_SRC_DIR)/csrc/mui*.c)

$(LIBNAME)_SRCS_ALL  += $($(LIBNAME)_DIR)/u8g2-tock.c

# Remove the buffer file, we need to create our own tock-specific version.
$(LIBNAME)_SRCS := $(filter-out $(LIB_SRC_DIR)/csrc/u8g2_buffer.c,$($(LIBNAME)_SRCS_ALL))

# Need libtock headers
override CPPFLAGS += -I$(TOCK_USERLAND_BASE_DIR)
override CPPFLAGS += -I$(TOCK_USERLAND_BASE_DIR)/libtock
override CPPFLAGS += -I$(LIB_SRC_DIR)/csrc

# Avoid failing in CI due to warnings in the library.
override CPPFLAGS_$(LIBNAME) += -Wno-error

# Include the rules to build the library.
include $(TOCK_USERLAND_BASE_DIR)/TockLibrary.mk
