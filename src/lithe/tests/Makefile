# Makefile for parlib tests

MAKE = $(MAKE) -s
V ?= @

TESTSNAME = lithe_tests
LIBDEPS = lithe parlib ht pthread 

SRCDIR ?= $(PWD)
OBJDIR ?= $(PWD)/obj
LIBINCDIR ?= $(PWD)/../.. 
LIBTOPOBJDIR ?= $(PWD)/../../obj
LIBINCDIRS ?= $(LIBINCDIR) $(patsubst %, $(LIBINCDIR)/%, $(LIBDEPS))
LIBOBJDIRS ?= $(patsubst %, $(LIBTOPOBJDIR)/%, $(LIBDEPS))
EXT_DEPS ?= 

CFLAGS += -O2 -Wall -std=gnu99 -MMD -MP
CFLAGS += -fno-strict-aliasing
CFLAGS += -Wno-unused-function 
CXXFLAGS = $(filter-out -std=gnu99, $(CFLAGS))
LDFLAGS += $(patsubst %, -L%, $(LIBOBJDIRS))
LDFLAGS += $(patsubst %, -l%, $(LIBDEPS))

INCDIRS := $(LIBINCDIRS) 
INCS := $(patsubst %, -I%, $(INCDIRS))
CFILES := $(notdir $(shell find $(SRCDIR) -name "*.c"))
CXXFILES := $(notdir $(shell find $(SRCDIR) -name "*.cc"))
EXECS := $(patsubst %.c, $(OBJDIR)/%, $(CFILES))
EXECS += $(patsubst %.cc, $(OBJDIR)/%_cc, $(CXXFILES))

uc = $(shell echo $(1) | tr a-z A-Z)

all: $(EXECS) 

DEPS = $(patsubst %, %.d, $(EXECS))
-include $(DEPS)

$(OBJDIR)/%: $(SRCDIR)/%.c $(EXT_DEPS)
	@echo + cc [$(call uc, $(TESTSNAME))] $(<F)
	@mkdir -p $(@D)
	$(V)$(CC) -o $@ $< $(CFLAGS) $(INCS) $(LDFLAGS)

$(OBJDIR)/%_cc: $(SRCDIR)/%.cc $(EXT_DEPS)
	@echo + cx [$(call uc, $(TESTSNAME))] $(<F)
	@mkdir -p $(@D)
	$(V)$(CXX) -o $@ $< $(CXXFLAGS) $(INCS) $(LDFLAGS)

clean:
	@echo + clean [$(call uc, $(TESTSNAME))]
	$(V)rm -rf $(EXECS) $(DEPS)
	$(V)if [ -d $(OBJDIR) ]; then \
	  [ -z $$(ls $(OBJDIR)) ] && rm -rf $(OBJDIR) || echo "" > /dev/null; \
	fi

.PHONY: all clean
