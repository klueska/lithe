# Makefile for parlib library

MAKE := $(MAKE) -s
V ?= @

uc = $(shell echo $(1) | tr a-z A-Z)
lc = $(shell echo $(1) | tr A-Z a-z)
strip_tests = $(subst _tests,,$(1))

LIBS := ht parlib lithe
SCHEDULERS := rrthread 
STATICLIBS := yes

OS := $(call lc, $(shell uname -s))
ARCH := $(shell uname -m)
SYSDEP := unix/sysv/$(OS)/$(ARCH)
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -Wno-unused-function 
CFLAGS += -Wno-unused-value 

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

all: $(LIBS) 

LIBINCDIR = $(SRCDIR)
SCHEDSDIR = $(SRCDIR)/schedulers
LIBOBJDIRS = $(patsubst %, $(OBJDIR)/%, $(LIBS))
include $(patsubst %, $(SRCDIR)/%/Makefrag, $(LIBS))
include $(SCHEDSDIR)/Makefrag
	
