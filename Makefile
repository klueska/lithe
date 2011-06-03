
SRCDIR := $(PWD)/src
OBJDIR := $(PWD)/obj
TESTS_DIR := $(PWD)/tests

include src/Makefile
OTHER_TESTS := $(patsubst %, %_tests, $(INTERMLIBS))
OTHER_TESTS += $(patsubst %, %_tests, $(SCHEDULERS))

top_tests: all 
	$(V)V=$(V) SRCDIR=$(TESTS_DIR) OBJDIR=$(OBJDIR)/tests \
	  PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) EXT_DEPS=$(FINALLIB)\
	  $(MAKE) -C tests

top_tests_clean:
	$(V)V=$(V) SRCDIR=$(TESTS_DIR) OBJDIR=$(OBJDIR)/tests \
	  PARLIB_DIR=$(SRCDIR) PARLIB_LDDIR=$(OBJDIR) EXT_DEPS=$(FINALLIB)\
	  $(MAKE) -C tests clean

tests: all $(OTHER_TESTS) top_tests

tests_clean: $(patsubst %, %_clean, $(OTHER_TESTS)) top_tests_clean

clean: top_tests_clean

.PHONY: tests tests_clean
