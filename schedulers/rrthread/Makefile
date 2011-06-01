# Makefile for rruthread library

V ?= @
LIBNAME = rrthread

PARLIB_DIR := /home/klueska/parlab/lithe/linux_parlib
SCHEDS_DIR := $(PARLIB_DIR)/schedulers
PARLIB_LDDIR := $(PARLIB_DIR)/obj

TESTS_DIR := $(SCHEDS_DIR)/tests
TESTS_LDLIBS := -l$(LIBNAME) -lpthread -lparlib

# Needed to point to installation of the scheduler directory where the
# Makerules file is located.
include $(SCHEDS_DIR)/Makerules

# Needed to point to installation of the scheduler directory where the 
# tests subfolder is located, to build all bthread tests for this scheduler
include $(TESTS_DIR)/Makerules

