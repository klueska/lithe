/*
    Copyright 2005-2008 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_profiling_H
#define __TBB_profiling_H

// Check if the tools support is enabled
#if (_WIN32||_WIN64||__linux__) && TBB_USE_THREADING_TOOLS && __TBB_NAMING_API_SUPPORT

#if _WIN32||_WIN64
#include <cstdlib>  /* mbstowcs */
#endif
#include "tbb/tbb_stddef.h"

namespace tbb {
    namespace internal {
#if _WIN32||_WIN64
        void __TBB_EXPORTED_FUNC itt_set_sync_name_v3( void *obj, const wchar_t* name ); 
#else
        void __TBB_EXPORTED_FUNC itt_set_sync_name_v3( void *obj, const char* name ); 
#endif
    } // namespace internal
} // namespace tbb

//! Macro __TBB_DEFINE_PROFILING_SET_NAME(T) defines "set_name" methods for sync objects of type T
/** Should be used in the "tbb" namespace only. 
    Don't place semicolon after it to avoid compiler warnings. **/
#if _WIN32||_WIN64
    #define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type)    \
        namespace profiling {                                                       \
            inline void set_name( sync_object_type& obj, const wchar_t* name ) {    \
                tbb::internal::itt_set_sync_name_v3( &obj, name );             \
            }                                                                       \
            inline void set_name( sync_object_type& obj, const char* name ) {       \
                size_t len = mbstowcs(NULL, name, 0) + 1;                           \
                wchar_t *wname = new wchar_t[len];                                  \
                mbstowcs(wname, name, len);                                         \
                set_name( obj, wname );                                             \
                delete wname;                                                       \
            }                                                                       \
        }
#else /* !WIN */
    #define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type)    \
        namespace profiling {                                                       \
            inline void set_name( sync_object_type& obj, const char* name ) {       \
                tbb::internal::itt_set_sync_name_v3( &obj, name );             \
            }                                                                       \
        }
#endif /* !WIN */

#else /* no tools support */

#if _WIN32||_WIN64
    #define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type)    \
        namespace profiling {                                               \
            inline void set_name( sync_object_type&, const wchar_t* ) {}    \
            inline void set_name( sync_object_type&, const char* ) {}       \
        }
#else /* !WIN */
    #define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type)    \
        namespace profiling {                                               \
            inline void set_name( sync_object_type&, const char* ) {}       \
        }
#endif /* !WIN */

#endif /* no tools support */

#endif /* __TBB_profiling_H */
