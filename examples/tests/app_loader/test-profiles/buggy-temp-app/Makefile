# Makefile for user application

# Specify this directory relative to the current application.
TOCK_USERLAND_BASE_DIR = ../../../../..

# Which files to compile.
C_SRCS := $(wildcard *.c)

STACK_SIZE = 2048
APP_HEAP_SIZE = 1024

PACKAGE_NAME = buggy-temp
# ELF2TAB_ARGS += --ecdsa-nist-p256-private $(TOCK_USERLAND_BASE_DIR)/examples/tutorials/dynamic-apps-and-policies/keys/ec-secp256r1-tutorial-key.private.p8

include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk
