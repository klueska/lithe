# Makefile for rrthread library

V ?= @
SCHEDNAME = rrthread

SCHEDSDIR ?= $(PWD)/..
LIBINCDIRS ?= $(PWD)/../..
LIBTOPOBJDIR ?= $(PWD)/../../obj
TESTSDIR ?= $(SCHEDSDIR)/tests

# Needed to point to installation of the scheduler directory where the
# Makevars file is located.
include $(SCHEDSDIR)/Makevars

# Needed to point to installation of the scheduler directory where the
# Maketargets file is located.
include $(SCHEDSDIR)/Maketargets

# Needed to point to installation of the scheduler directory where the 
# tests subfolder is located, to build all bthread tests for this scheduler
include $(TESTSDIR)/Makerules

