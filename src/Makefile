# Makefile for parlib library

MAKE := $(MAKE) -s
V ?= @

uc = $(shell echo $(1) | tr a-z A-Z)
lc = $(shell echo $(1) | tr A-Z a-z)
strip_tests = $(subst _tests,,$(1))

LIBS := ht parlib lithe bthread
STATICLIBS := no
TESTS := $(patsubst %, %_tests, $(LIBS))

SRCDIR ?= $(PWD)
BUILDDIR ?= $(SRCDIR)/build
BUILDOBJDIR ?= $(BUILDDIR)/obj
BUILDINCDIR ?= $(BUILDDIR)/include
BUILDLIBDIR ?= $(BUILDDIR)/lib

OS := $(call lc, $(shell uname -s))
ARCH := $(shell uname -m)
SYSDEP := unix/sysv/$(OS)/$(ARCH)
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -Wno-unused-function 
CFLAGS += -Wno-unused-value 
CFLAGS += -Wno-missing-braces 

GCCPREFIX ?= 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

all: $(LIBS)

include $(patsubst %, $(SRCDIR)/%/Makefrag, $(LIBS))

tests: all $(TESTS)

tests_clean: $(patsubst %, %_tests_clean, $(LIBS))

clean: $(patsubst %, %_clean, $(LIBS)) 

clean: $(patsubst %, %_clean, $(TESTS)) 

clean: tests_clean
	$(V)if [ -d "$(BUILDINCDIR)" ]; then \
	  [ -z "$$(ls $(BUILDINCDIR))" ] && rm -rf $(BUILDINCDIR) || echo "" > /dev/null; \
	fi
	$(V)if [ -d "$(BUILDOBJDIR)" ]; then \
	  [ -z "$$(ls $(BUILDOBJDIR))" ] && rm -rf $(BUILDOBJDIR) || echo "" > /dev/null; \
	fi
	$(V)if [ -d "$(BUILDLIBDIR)" ]; then \
	  [ -z "$$(ls $(BUILDLIBDIR))" ] && rm -rf $(BUILDLIBDIR) || echo "" > /dev/null; \
	fi
	$(V)if [ -d $(BUILDDIR) ]; then \
	  [ -z "$$(ls $(BUILDDIR))" ] && rm -rf $(BUILDDIR) || echo "" > /dev/null; \
	fi
