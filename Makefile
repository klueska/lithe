# Makefile for parlib library

MAKE := $(MAKE) -s
LIBNAME = parlib
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
V ?= @

SCHEDULERS := rrthread

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

SRCDIR := 
OBJDIR := $(SRCDIR)obj
INCDIRS := .. .

INCS = $(patsubst %, -I%, $(INCDIRS))
FINALLIB = $(OBJDIR)/lib$(LIBNAME).a

uc = $(shell echo $(1) | tr a-z A-Z)
strip_tests = $(subst _tests,,$(1))

LIBUCNAME := $(call uc, $(LIBNAME))
HEADERS := $(shell find $(INCDIRS) -name "*.h")
CFILES  := $(wildcard $(SRCDIR)*.c)
OBJS    := $(patsubst %.c, $(OBJDIR)/%.o, $(CFILES))

all: ht $(FINALLIB)

include ht/Makefrag

$(OBJDIR)/%.o: $(SRCDIR)%.c $(HEADERS)
	@echo + cc [$(LIBUCNAME)] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(CFLAGS) $(INCS) -o $@ -c $<

$(FINALLIB): $(OBJS)
	@echo + ar [$(LIBUCNAME)] $@
	@mkdir -p $(@D)
	$(V)$(AR) rc $@ $(OBJS)

tests: all
	$(V)$(MAKE) -C tests

$(SCHEDULERS): 
	$(V)V=$(V) $(MAKE) -C schedulers/$@
	$(V)mkdir -p $(OBJDIR)/schedulers
	$(V)rm -rf $(OBJDIR)/schedulers/$@
	$(V)ln -s ../../schedulers/$@/obj $(OBJDIR)/schedulers/$@

$(patsubst %, %_tests, $(SCHEDULERS)): all 
	$(V)V=$(V) $(MAKE) $(subst _tests,,$@)
	$(V)V=$(V) $(MAKE) -C schedulers/$(subst _tests,,$@) tests

$(patsubst %, %_clean, $(SCHEDULERS)):
	$(V)V=$(V) $(MAKE) -C schedulers/$(subst _clean,,$@) clean

clean: $(patsubst %, %_clean, $(SCHEDULERS))
	@echo + clean [$(HT_LIBUCNAME)]
	@echo + clean [$(LIBUCNAME)]
	$(V)rm -rf $(OBJDIR)
	
