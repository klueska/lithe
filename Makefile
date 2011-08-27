
SRCDIR := $(PWD)/src
OBJDIR := $(PWD)/obj

include src/Makefile
TESTS := $(patsubst %, %_tests, $(LIBS))
TESTS += $(patsubst %, %_tests, $(SCHEDULERS))

tests: all $(TESTS)

tests_clean: $(patsubst %, %_clean, $(TESTS)) 

clean_top:
	@rm -rf $(OBJDIR)

clean: clean_top

.PHONY: tests tests_clean
