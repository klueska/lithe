# Makefile for parlib tests

MAKE = $(MAKE) -s
V ?= @

TESTSNAME = lithe_tests
LIBDEPS = lithe parlib pthread 
#LIBDEPS = lithe parlib #ht pthread 

SRCDIR ?= $(PWD)
BUILDDIR ?= $(PWD)/build
LIBBUILDDIR ?= $(PWD)/../../../build
LIBINCDIRS ?= $(LIBBUILDDIR)/include
LIBLIBDIRS ?= $(LIBBUILDDIR)/lib
EXT_DEPS ?= 

CFLAGS += -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -fno-strict-aliasing
CFLAGS += -Wno-unused-function 
CXXFLAGS = $(filter-out -std=gnu99, $(CFLAGS))
LDFLAGS += $(patsubst %, -L%, $(LIBLIBDIRS))
LDFLAGS += $(patsubst %, -l%, $(LIBDEPS))

INCS := $(patsubst %, -I%, $(LIBINCDIRS))
CFILES := $(notdir $(shell find $(SRCDIR) -name "*.c"))
CXXFILES := $(notdir $(shell find $(SRCDIR) -name "*.cc"))
EXECS := $(patsubst %.c, $(BUILDDIR)/%, $(CFILES))
EXECS += $(patsubst %.cc, $(BUILDDIR)/%_cc, $(CXXFILES))

uc = $(shell echo $(1) | tr a-z A-Z)

all: $(EXECS) 

DEPS = $(patsubst %, %.d, $(EXECS))
-include $(DEPS)

$(BUILDDIR)/%: $(SRCDIR)/%.c $(EXT_DEPS)
	@echo + cc [$(call uc, $(TESTSNAME))] $(<F)
	@mkdir -p $(@D)
	$(V)$(CC) -o $@ $< $(CFLAGS) $(INCS) $(LDFLAGS)

$(BUILDDIR)/%_cc: $(SRCDIR)/%.cc $(EXT_DEPS)
	@echo + cx [$(call uc, $(TESTSNAME))] $(<F)
	@mkdir -p $(@D)
	$(V)$(CXX) -o $@ $< $(CXXFLAGS) $(INCS) $(LDFLAGS)

clean:
	@echo + clean [$(call uc, $(TESTSNAME))]
	$(V)rm -rf $(EXECS) $(DEPS)
	$(V)if [ -d $(BUILDDIR) ]; then \
	  [ -z $$(ls $(BUILDDIR)) ] && rm -rf $(BUILDDIR) || echo "" > /dev/null; \
	fi

.PHONY: all clean
