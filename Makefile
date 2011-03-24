# Makefile for parlib library

CFLAGS = -O2 -std=gnu99 -static -fomit-frame-pointer 
LIBNAME = parlib
CFLAGS = -g -O0 -Wall -std=gnu99 -MMD -MP
V ?= @

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

SRCDIR := 
OBJDIR := $(SRCDIR)obj
INCDIRS := .. .

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
	$(MAKE) -C ht
	$(MAKE) -C tests

clean: 
	@echo + clean [$(LIBUCNAME)]
	$(V)rm -rf $(OBJDIR)
	
