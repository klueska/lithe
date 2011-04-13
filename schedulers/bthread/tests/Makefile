# Makefile for parlib tests

TESTSNAME = bthread_tests
EXECS = bthread_test
CFLAGS = -O2 -Wall -std=gnu99 -MMD -MP
V ?= @

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

# Needed to point to installation of $(PARLIB_*) in case this library is not
# already installed in the cross compiler standard locations.
-include ../Makelocal

SRCDIR := 
OBJDIR := $(SRCDIR)../obj
INCDIRS := . .. $(PARLIB_INCDIR)
LDFLAGS = -L../obj -L$(PARLIB_LDDIR) -lbthread $(PARLIB_LDLIBS)

INCS = $(patsubst %, -I%, $(INCDIRS))
EXECS := $(patsubst %, $(OBJDIR)/%, $(EXECS))

uc = $(shell echo $(1) | tr a-z A-Z)

HEADERS := $(shell find $(INCDIRS) -name "*.h")
CFILES  := $(wildcard $(SRCDIR)*.c)

all: $(EXECS)

DEPS = $(patsubst %, %.d, $(EXECS))
-include $(DEPS)

$(OBJDIR)/%: %.c  $(HEADERS)
	@echo + cc [$(call uc, $(TESTSNAME))] $<
	@mkdir -p $(@D)
	$(V)$(CC) $< $(CFLAGS) $(INCS) $(LDFLAGS) -o $@

clean:
	@echo + clean [$(call uc, $(TESTSNAME))]
	$(V)rm -rf $(EXECS) $(DEPS)

.PHONY: all clean
