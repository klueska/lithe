# Makefile for parlib library

MAKE := $(MAKE) -s
V ?= @

uc = $(shell echo $(1) | tr a-z A-Z)
lc = $(shell echo $(1) | tr A-Z a-z)
strip_tests = $(subst _tests,,$(1))

LIBS := ht parlib lithe
SCHEDULERS := rrthread 
STATICLIBS := yes

SRCDIR ?= $(PWD)
OBJDIR ?= $(SRCDIR)/obj

OS := $(call lc, $(shell uname -s))
ARCH := $(shell uname -m)
SYSDEP := unix/sysv/$(OS)/$(ARCH)
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -Wno-unused-function 
CFLAGS += -Wno-unused-value 

GCCPREFIX ?= 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

all: $(LIBS) 

include $(patsubst %, $(SRCDIR)/%/Makefrag, $(LIBS))

SCHEDSDIR = $(SRCDIR)/schedulers
include $(SCHEDSDIR)/Makefrag
	
TESTS := $(patsubst %, %_tests, $(LIBS))
TESTS += $(patsubst %, %_tests, $(SCHEDULERS))

tests: all $(TESTS)

tests_clean: $(patsubst %, %_clean, $(TESTS)) 

clean: tests_clean
	@rm -rf $(OBJDIR)
