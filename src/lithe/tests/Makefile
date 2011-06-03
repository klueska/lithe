# Makefile for parlib tests

MAKE = $(MAKE) -s
V ?= @

TESTSNAME = lithe_tests

SRCDIR ?= $(PWD)
OBJDIR ?= $(PWD)/obj
PARLIB_DIR ?= $(PWD)/../src
PARLIB_LDDIR ?= $(PWD)/../obj
EXT_DEPS ?= 

CFLAGS += -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -fno-strict-aliasing
LDFLAGS += -L$(PARLIB_LDDIR) -lpthread -lparlib

INCDIRS := $(SRCDIR) $(PARLIB_DIR)
INCS := $(patsubst %, -I%, $(INCDIRS))
CFILES := $(notdir $(shell find $(SRCDIR) -name "*.c"))
EXECS := $(patsubst %.c, $(OBJDIR)/%, $(CFILES))

uc = $(shell echo $(1) | tr a-z A-Z)

all: $(EXECS) 

DEPS = $(patsubst %, %.d, $(EXECS))
-include $(DEPS)

$(OBJDIR)/%: $(SRCDIR)/%.c $(EXT_DEPS)
	@echo + cc [$(call uc, $(TESTSNAME))] $(<F)
	@mkdir -p $(@D)
	$(V)$(CC) -o $@ $< $(CFLAGS) $(INCS) $(LDFLAGS)

clean:
	@echo + clean [$(call uc, $(TESTSNAME))]
	$(V)rm -rf $(EXECS) $(DEPS)
	$(V)if [ -d $(OBJDIR) ]; then \
	  [ -z $$(ls $(OBJDIR)) ] && rm -rf $(OBJDIR) || echo "" > /dev/null; \
	fi

.PHONY: all clean
