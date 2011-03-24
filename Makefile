# Makefile for hard threads library

LIBNAME = ht
CFLAGS = -g -O2 -Wall -std=gnu99 -MMD -MP
V ?= @

GCCPREFIX := 
CC := $(GCCPREFIX)gcc
GCC_ROOT := $(shell which $(CC) | xargs dirname)/../

SRCDIR := 
OBJDIR := $(SRCDIR)obj
INCDIR = .

INCS = -I. -I$(INCDIR) 
FINALLIB = $(OBJDIR)/lib$(LIBNAME).a

uc = $(shell echo $(1) | tr a-z A-Z)

LIBUCNAME := $(call uc, $(LIBNAME))
HEADERS := $(shell find . $(INCDIR) -name "*.h")
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
	$(MAKE) -C tests

clean: 
	@echo + clean [$(LIBUCNAME)]
	$(MAKE) -C tests clean
	$(V)rm -rf $(FINALLIB)
	$(V)rm -rf $(OBJDIR)
	
