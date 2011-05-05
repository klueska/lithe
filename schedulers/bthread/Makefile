# Makefile for bthread library

LIBNAME = bthread
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
V ?= @

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

# Needed to point to installation of $(PARLIB_*) in case this library is not
# already installed in the cross compiler standard locations.
-include Makelocal

SRCDIR := 
OBJDIR := $(SRCDIR)obj
INCDIRS := . $(PARLIB_INCDIR)

INCS = $(patsubst %, -I%, $(INCDIRS))
FINALLIB = $(OBJDIR)/lib$(LIBNAME).a

uc = $(shell echo $(1) | tr a-z A-Z)

LIBUCNAME := $(call uc, $(LIBNAME))
HEADERS := $(shell find $(INCDIRS) -name "*.h")
CFILES  := $(wildcard $(SRCDIR)*.c)
OBJS    := $(patsubst %.c, $(OBJDIR)/%.o, $(CFILES))

all: $(FINALLIB)

$(OBJDIR)/%.o: $(SRCDIR)%.c $(HEADERS)
	@echo + cc [$(LIBUCNAME)] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(CFLAGS) $(INCS) -o $@ -c $<

$(FINALLIB): $(OBJS)
	@echo + ar [$(LIBUCNAME)] $@
	@mkdir -p $(@D)
	$(V)$(AR) rc $@ $(OBJS)

tests: all
	$(V)V=$(V) make -C tests

clean: 
	@echo + clean [$(LIBUCNAME)]
	$(V)rm -rf $(OBJDIR)
	
