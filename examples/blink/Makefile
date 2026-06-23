# Makefile for user application

# Specify this directory relative to the current application.
TOCK_USERLAND_BASE_DIR = ../..

# Which files to compile.
C_SRCS := $(wildcard *.c)

ELF2TAB_ARGS += --app-version 3
ELF2TAB_ARGS += --ecdsa-nist-p256-private $(TOCK_USERLAND_BASE_DIR)/examples/tutorials/dynamic-apps-and-policies/keys/ec-secp256r1-tutorial-key.private.p8

# Include userland master makefile. Contains rules and flags for actually
# building the application.
include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk
