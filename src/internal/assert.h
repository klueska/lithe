#ifndef PARLIB_INTERNAL_ASSERT_H
#define PARLIB_INTERNAL_ASSERT_H

#include <assert.h>

#ifndef DEBUG
# undef assert
# define assert(x) (x)
#endif

#endif
