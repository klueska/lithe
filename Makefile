
SRCDIR := $(PWD)/src
OBJDIR := $(PWD)/obj

include src/Makefile

clean_top:
	@rm -rf $(OBJDIR)

clean: clean_top

.PHONY: tests tests_clean
