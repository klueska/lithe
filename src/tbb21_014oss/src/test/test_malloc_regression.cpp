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

#define __TBB_NO_IMPLICIT_LINKAGE 1
#define HARNESS_NO_PARSE_COMMAND_LINE 1

#include <stdio.h>
#include "tbb/scalable_allocator.h"

class minimalAllocFree {
public:
    void operator()(int size) const {
        tbb::scalable_allocator<char> a;
        char* str = a.allocate( size );
        a.deallocate( str, size );
    }
};

#include "harness.h"

template<typename Body, typename Arg>
void RunThread(const Body& body, const Arg& arg) {
    NativeParallelForTask<Arg,Body> job(arg, body);
    job.start();
    job.wait_to_finish();
}

#include "harness_memory.h"

// The regression test for bug #1518 where thread boot strap allocations "leaked"
bool test_bootstrap_leak(void) {
    // Check whether memory usage data can be obtained; if not, skip the test.
    if( !GetMemoryUsage() )
        return true;

    /* In the bug 1518, each thread leaked ~384 bytes.
       Initially, scalable allocator maps 1MB. Thus it is necessary to take out most of this space.
       1MB is chunked into 16K blocks; of those, one block is for thread boot strap, and one more 
       should be reserved for the test body. 62 blocks left, each can serve 15 objects of 1024 bytes.
    */
    const int alloc_size = 1024;
    const int take_out_count = 15*62;

    tbb::scalable_allocator<char> a;
    char* array[take_out_count];
    for( int i=0; i<take_out_count; ++i )
        array[i] = a.allocate( alloc_size );

    RunThread( minimalAllocFree(), alloc_size ); // for threading library to take some memory
    size_t memory_in_use = GetMemoryUsage();
    // Wait for memory usage data to "stabilize". The test number (1000) has nothing underneath.
    for( int i=0; i<1000; i++) {
        if( GetMemoryUsage()!=memory_in_use ) {
            memory_in_use = GetMemoryUsage();
            i = -1;
        }
    }

    // Notice that 16K boot strap memory block is enough to serve 42 threads.
    const int num_thread_runs = 200;
    for( int i=0; i<num_thread_runs; ++i )
        RunThread( minimalAllocFree(), alloc_size );

    ptrdiff_t memory_leak = GetMemoryUsage() - memory_in_use;
    if( memory_leak>0 ) { // possibly too strong?
        printf( "Error: memory leak of up to %ld bytes\n", static_cast<long>(memory_leak));
    }

    for( int i=0; i<take_out_count; ++i )
        a.deallocate( array[i], alloc_size );

    return memory_leak<=0;
}

int main( int /*argc*/, char* argv[] ) {
    bool passed = true;

    passed &= test_bootstrap_leak();

    if(passed) printf("done\n");
    else       printf("%s failed\n", argv[0]);

    return passed?0:1;
}
