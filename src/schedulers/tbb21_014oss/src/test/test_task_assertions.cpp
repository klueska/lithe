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

// to avoid usage of #pragma comment
#define __TBB_NO_IMPLICIT_LINKAGE 1
#define __TBB_TASK_CPP_DIRECTLY_INCLUDED 1
#include "../tbb/task.cpp"

//------------------------------------------------------------------------
// Test that important assertions in class task fail as expected.
//------------------------------------------------------------------------

#define HARNESS_NO_PARSE_COMMAND_LINE 1
#include "harness.h"
#include "harness_bad_expr.h"

//! Task that will be abused.
tbb::task* volatile AbusedTask;

//! Number of times that AbuseOneTask
int AbuseOneTaskRan;

#if USE_LITHE
void __apply(void *arg);
#endif /* USE_LITHE */

//! Body used to create task in thread 0 and abuse it in thread 1.
struct AbuseOneTask {
    void apply(int) const {
        // Thread 1 attempts to incorrectly use the task created by thread 0.
        TRY_BAD_EXPR(AbusedTask->spawn(*AbusedTask),"owne");
        TRY_BAD_EXPR(AbusedTask->spawn_and_wait_for_all(*AbusedTask),"owne");
        TRY_BAD_EXPR(tbb::task::spawn_root_and_wait(*AbusedTask),"owne");

        // Try variant that operate on a tbb::task_list
        tbb::task_list list;
        TRY_BAD_EXPR(AbusedTask->spawn(list),"owne");
        TRY_BAD_EXPR(AbusedTask->spawn_and_wait_for_all(list),"owne");
        // spawn_root_and_wait over empty list should vacuously succeed.
        tbb::task::spawn_root_and_wait(list);

        // Check that spawn_root_and_wait fails on non-empty list. 
        list.push_back(*AbusedTask);
        TRY_BAD_EXPR(tbb::task::spawn_root_and_wait(list),"owne");

        TRY_BAD_EXPR(AbusedTask->destroy(*AbusedTask),"owne");
        TRY_BAD_EXPR(AbusedTask->wait_for_all(),"owne");

        // Try abusing recycle_as_continuation
        TRY_BAD_EXPR(AbusedTask->recycle_as_continuation(), "execute" );
        TRY_BAD_EXPR(AbusedTask->recycle_as_safe_continuation(), "execute" );
        TRY_BAD_EXPR(AbusedTask->recycle_to_reexecute(), "execute" );

        // Check correct use of depth parameter
        tbb::task::depth_type depth = AbusedTask->depth();
        ASSERT( depth==0, NULL );
        for( int k=1; k<=81; k*=3 ) {
            AbusedTask->set_depth(depth+k);
            ASSERT( AbusedTask->depth()==depth+k, NULL );
            AbusedTask->add_to_depth(k+1);
            ASSERT( AbusedTask->depth()==depth+2*k+1, NULL );
        }
        AbusedTask->set_depth(0);

        // Try abusing the depth parameter
        TRY_BAD_EXPR(AbusedTask->set_depth(-1),"negative");
        TRY_BAD_EXPR(AbusedTask->add_to_depth(-1),"negative");

        ++AbuseOneTaskRan;
    }

    void operator()( int ) const {
#if USE_LITHE
	tbb::task_scheduler_init init(__apply, (void *) this);
#else
        tbb::task_scheduler_init init;
	apply();
#endif /* USE_LITHE */
    }
};

#if USE_LITHE
void __apply(void *arg) {
    struct AbuseOneTask *__this = static_cast<struct AbuseOneTask *>(arg);
    __this->apply(0);
}

void __TestTaskAssertions(void *) {
#if TBB_USE_ASSERT
    // Create task to be abused
    AbusedTask = new( tbb::task::allocate_root() ) tbb::empty_task;
    NativeParallelFor( 1, AbuseOneTask() );
    ASSERT( AbuseOneTaskRan==1, NULL );
    AbusedTask->destroy(*AbusedTask);
    // Restore normal assertion handling
    tbb::set_assertion_handler( NULL );
#endif /* TBB_USE_ASSERT */
}
#endif /* USE_LITHE */

//! Test various __TBB_ASSERT assertions related to class tbb::task.
void TestTaskAssertions() {
#if TBB_USE_ASSERT
    // Catch assertion failures
    tbb::set_assertion_handler( AssertionFailureHandler );
#if USE_LITHE
    tbb::task_scheduler_init init(__TestTaskAssertions, NULL);
#else
    tbb::task_scheduler_init init;
    // Create task to be abused
    AbusedTask = new( tbb::task::allocate_root() ) tbb::empty_task;
    NativeParallelFor( 1, AbuseOneTask() );
    ASSERT( AbuseOneTaskRan==1, NULL );
    AbusedTask->destroy(*AbusedTask);
    // Restore normal assertion handling
    tbb::set_assertion_handler( NULL );
#endif /* USE_LITHE */
#endif /* TBB_USE_ASSERT */
}

#if USE_LITHE
extern "C" {
#endif /* USE_LITHE */

//------------------------------------------------------------------------
int main(int argc, char* argv[]) {
#if __GLIBC__==2&&__GLIBC_MINOR__==3
    printf("skip\n");
#else
    TestTaskAssertions();
    printf("done\n");
#endif
    return 0;
}

#if USE_LITHE
}
#endif /* USE_LITHE */
