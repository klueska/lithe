# Makefile for parlib library

MAKE := $(MAKE) -s
V ?= @

uc = $(shell echo $(1) | tr a-z A-Z)
lc = $(shell echo $(1) | tr A-Z a-z)

LIBNAME = parlib
INTERMLIBS := ht lithe
SCHEDULERS := rrthread
SYSDEP := unix/sysv/$(call lc, $(shell uname -s))/$(shell uname -m)
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -Wno-unused-function 
CFLAGS += -Wno-unused-value 

SRCDIR ?= $(PWD)
OBJDIR ?= $(SRCDIR)/obj
SYSDEPDIR := $(SRCDIR)/sysdeps/$(SYSDEP)
INCDIRS := $(SRCDIR)

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

INCS = $(patsubst %, -I%, $(INCDIRS))
FINALLIB = $(OBJDIR)/lib$(LIBNAME).a

uc = $(shell echo $(1) | tr a-z A-Z)
strip_tests = $(subst _tests,,$(1))

LIBUCNAME := $(call uc, $(LIBNAME))
CFILES  := $(notdir $(wildcard $(SRCDIR)/*.c))
CFILES  += $(notdir $(wildcard $(SYSDEPDIR)/*.c))
SFILES  := $(notdir $(wildcard $(SRCDIR)/*.S))
SFILES  += $(notdir $(wildcard $(SYSDEPDIR)/*.S))
OBJS    := $(patsubst %.c, $(OBJDIR)/%.o, $(CFILES))
OBJS    += $(patsubst %.S, $(OBJDIR)/%.o, $(SFILES))
DEPS    := $(patsubst %.o, %.d, $(OBJS))

all: $(INTERMLIBS) $(FINALLIB)

-include $(DEPS)
include $(patsubst %, $(SRCDIR)/%/Makefrag, $(INTERMLIBS))

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo + cc [$(LIBUCNAME)] $(<F)
	@mkdir -p $(@D)
	$(V)$(CC) $(CFLAGS) $(INCS) -o $@ -c $<

$(FINALLIB): $(OBJS)
	@echo + ar [$(LIBUCNAME)] $(@F)
	@mkdir -p $(@D)
	$(V)$(AR) rc $@ $(OBJS)

$(SCHEDULERS): 
	$(V)V=$(V) PARLIB_DIR=$(SRCDIR) $(MAKE) -C $(SRCDIR)/schedulers/$@
	$(V)mkdir -p $(OBJDIR)/schedulers
	$(V)rm -rf $(OBJDIR)/schedulers/$@
	$(V)ln -s $(SRCDIR)/schedulers/$@/obj $(OBJDIR)/schedulers/$@

$(patsubst %, %_tests, $(SCHEDULERS)): all 
	$(V)V=$(V) PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) \
	  $(MAKE) $(subst _tests,,$@)
	$(V)V=$(V) PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) \
	  $(MAKE) -C $(SRCDIR)/schedulers/$(subst _tests,,$@) tests
	$(V)mkdir -p $(OBJDIR)/tests/schedulers
	$(V)rm -rf $(OBJDIR)/tests/schedulers/$(subst _tests,,$@)
	$(V)ln -s $(SRCDIR)/schedulers/$(subst _tests,,$@)/obj/tests $(OBJDIR)/tests/schedulers/$(subst _tests,,$@)

$(patsubst %, %_clean, $(SCHEDULERS)): 
	$(V)V=$(V) PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) \
	  $(MAKE) -C $(SRCDIR)/schedulers/$(subst _clean,,$@) clean
	$(V)rm -rf $(OBJDIR)/schedulers/$(subst _clean,,$@)
	$(V)if [ -d $(OBJDIR)/schedulers ]; then \
	  [ -z $$(ls $(OBJDIR)/schedulers) ] && rm -rf $(OBJDIR)/schedulers || echo "" > /dev/null; \
	fi

$(patsubst %, %_tests_clean, $(SCHEDULERS)):
	$(V)V=$(V) PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) \
	  $(MAKE) -C $(SRCDIR)/schedulers/$(subst _tests_clean,,$@) tests_clean
	$(V)rm -rf $(OBJDIR)/tests/schedulers/$(subst _tests_clean,,$@)
	$(V)if [ -d $(OBJDIR)/tests/schedulers ]; then \
	  [ -z $$(ls $(OBJDIR)/tests/schedulers) ] && rm -rf $(OBJDIR)/tests/schedulers || echo "" > /dev/null; \
	fi

clean: $(patsubst %, %_clean, $(INTERMLIBS)) $(patsubst %, %_clean, $(SCHEDULERS))
	@echo + clean [$(LIBUCNAME)]
	$(V)rm -rf $(OBJDIR)
	
