# Require a minimum autoconf version
AC_PREREQ([2.65])

# Initialize autoconf
# Specify package's name, version number, and bug-report address.
AC_INIT([Lithe], [1.0], [klueska@cs.berkeley.edu])

# Directory containing any external m4 macros
AC_CONFIG_MACRO_DIR([m4])

# Auxiliary scripts such as install-sh and depcomp should be in DIRECTORY
AC_CONFIG_AUX_DIR([build-aux])

# Make the default prefix /usr instead of /usr/local
AC_PREFIX_DEFAULT([/usr])

# Initialize automake
# Turn on all Automake warnings and report them as errors.
# This is a foreign package
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror -Wno-portability])

# Checks for a C compiler
AC_PROG_CC

# Checks for a C++ compiler
AC_PROG_CXX

# Check for an assembler
#AM_PROG_AS

# Checks for libtool
AC_PROG_LIBTOOL

# Check for HEADERS and #define HAVE_HEADER_H for each header found
#AC_CHECK_HEADERS([HEADERS ...])

# Output the following to config.h 
#AC_DEFINE(VARIABLE, VALUE, DESCRIPTION)

# Declare config.h as the output header
AC_CONFIG_HEADERS([config.h])

# Declare Makefiles as output files
AC_CONFIG_FILES([
  Makefile 
])

# A safety check. FILE is a distributed source file, making sure that
# 'configure' is not run from outerspace.
AC_CONFIG_SRCDIR([src/lithe.c])

CFLAGS="                   \
  -g -O2 -Wall             \
  -Wno-unused-function     \
  -Wno-unused-value        \
  -Wno-missing-braces      \
  -fno-exceptions          \
  -fno-strict-aliasing     \
"

# Set the common AM_CFLAGS for all Makefile.am files
AC_SUBST([AM_CFLAGS],["-std=gnu99 $CFLAGS"])
AC_SUBST([AM_CXXFLAGS],["$CFLAGS"])

# Set up some global variables for use in the makefile
SRCDIR=src
TESTSDIR=tests
AC_SUBST([SRCDIR])
AC_SUBST([TESTSDIR])
AM_SUBST_NOTMAKE([SRCDIR])
AM_SUBST_NOTMAKE([TESTSDIR])

# Check whether LIBRARY exists and contains FUNCT
# Execute ACT-IF-FOUND if it does.  ACT-IF-NOT otherwise.
#AC_CHECK_LIB(LIBRARY, FUNCT, [ACT-IF-FOUND], [ACT-IF-NOT])

# Actually output all declared files
AC_OUTPUT
