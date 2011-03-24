# Makefile for parlib tests

TESTSNAME = parlib_tests
EXECS = ht_test generic_test bthread_test
CFLAGS = -O2 -Wall -std=gnu99 -MMD -MP
LDFLAGS = -L../obj -L../ht/obj -lpthread -lht -lparlib
V ?= @

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

SRCDIR := 
OBJDIR := $(SRCDIR)../obj
INCDIRS := . ..

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
	$(V)$(CC) -o $@ $< $(CFLAGS) $(INCS) $(LDFLAGS)

clean:
	@echo + clean [$(call uc, $(TESTSNAME))]
	$(V)rm -rf $(EXECS) $(DEPS)

.PHONY: all clean
