# Makefile for user application

# Specify this directory relative to the current application.
TOCK_USERLAND_BASE_DIR = ../../../../..

# Which files to compile.
C_SRCS := $(wildcard *.c)
# Specify this app depends on the MTD OpenThread library.
include $(TOCK_USERLAND_BASE_DIR)/libopenthread/libopenthread-ftd.mk

# set stack size to 8000 to support openthread app
STACK_SIZE:=16384
APP_HEAP_SIZE:=5000

PACKAGE_NAME = openthread-ipc-commisioner

# Include userland master makefile. Contains rules and flags for actually
# building the application.
include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk
