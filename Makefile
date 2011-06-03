
SRCDIR := $(PWD)/src
OBJDIR := $(PWD)/obj
TESTS_DIR := $(PWD)/tests

include src/Makefile

tests: all
	$(V)V=$(V) SRCDIR=$(TESTS_DIR) OBJDIR=$(OBJDIR)/tests \
	  PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) \
	  $(MAKE) -C tests

tests_clean:
	$(V)V=$(V) SRCDIR=$(TESTS_DIR) OBJDIR=$(OBJDIR)/tests \
	  PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) \
	  $(MAKE) -C tests clean

clean: tests_clean

.PHONY: tests tests_clean
