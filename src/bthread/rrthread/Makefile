# Makefile for rrthread library

V ?= @
SCHEDNAME = rrthread

SCHEDDIR ?= $(PWD)/..
BUILDDIR ?= $(PWD)/build
BUILDINCDIR ?= $(BUILDDIR)/include
BUILDOBJDIR ?= $(BUILDDIR)/obj
BUILDLIBDIR ?= $(BUILDDIR)/lib
LIBBUILDDIR ?= $(PWD)/../../../build
LIBINCDIR ?= $(LIBBUILDDIR)/include
LIBOBJDIR ?= $(LIBBUILDDIR)/obj
LIBLIBDIR ?= $(LIBBUILDDIR)/lib
TESTSDIR ?= $(SCHEDDIR)/tests

# Needed to point to installation of the scheduler directory where the
# Makevars file is located.
include $(SCHEDDIR)/Makevars

# Needed to point to installation of the scheduler directory where the
# Maketargets file is located.
include $(SCHEDDIR)/Maketargets

# Needed to point to installation of the scheduler directory where the 
# tests subfolder is located, to build all bthread tests for this scheduler
include $(TESTSDIR)/Makerules

