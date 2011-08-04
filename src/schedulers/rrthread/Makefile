# Makefile for rruthread library

V ?= @
LIBNAME = rrthread

PARLIB_DIR ?= $(PWD)/../..
PARLIB_LDDIR ?= $(PARLIB_DIR)/obj
SCHEDS_DIR := $(PARLIB_DIR)/schedulers

TESTS_DIR := $(SCHEDS_DIR)/tests
TESTS_LDLIBS := -l$(LIBNAME) -lpthread -lparlib

# Needed to point to installation of the scheduler directory where the
# Makevars file is located.
include $(SCHEDS_DIR)/Makevars

# Needed to point to installation of the scheduler directory where the
# Maketargets file is located.
include $(SCHEDS_DIR)/Maketargets

# Needed to point to installation of the scheduler directory where the 
# tests subfolder is located, to build all bthread tests for this scheduler
include $(TESTS_DIR)/Makerules

