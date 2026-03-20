# Makefile for user application

# Specify this directory relative to the current application.
TOCK_USERLAND_BASE_DIR = ../../../../..

# Which files to compile.
C_SRCS := $(wildcard *.c)

STACK_SIZE = 2048
APP_HEAP_SIZE = 1024

PACKAGE_NAME = buggy-temp

include $(TOCK_USERLAND_BASE_DIR)/AppMakefile.mk
