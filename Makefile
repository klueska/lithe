
SRCDIR := $(PWD)/src
BUILDDIR := $(PWD)/build

include src/Makefile

clean_top:
	$(V)if [ -d $(BUILDDIR) ]; then \
	  [ -z "$$(ls $(BUILDDIR))" ] && rm -rf $(BUILDDIR) || echo "" > /dev/null; \
	fi

clean: clean_top

.PHONY: tests tests_clean
