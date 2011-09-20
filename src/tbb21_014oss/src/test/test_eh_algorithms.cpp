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

#include "stddef.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_exception.h"
#include "tbb/task.h"
#include "tbb/atomic.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"
#include "tbb/parallel_do.h"
#include "tbb/pipeline.h"
#include "tbb/blocked_range.h"

#include <cmath>
#include <typeinfo>

#include "harness.h"
#include "harness_trace.h"


#if __TBB_EXCEPTIONS

//------------------------------------------------------------------------
// Utility definitions
//------------------------------------------------------------------------

#define ITER_RANGE  100000
#define ITER_GRAIN  1000
#define NESTING_RANGE  100
#define NESTING_GRAIN  10
#define NESTED_RANGE  (ITER_RANGE / NESTING_RANGE)
#define NESTED_GRAIN  (ITER_GRAIN / NESTING_GRAIN)
#define EXCEPTION_DESCR "Test exception"

namespace internal = tbb::internal;
using internal::intptr;

namespace util {

#if _WIN32 || _WIN64

    typedef DWORD tid_t;

    tid_t get_my_tid () { return GetCurrentThreadId(); }

    void sleep ( int ms ) { ::Sleep(ms); }

#else /* !WIN */

    typedef pthread_t tid_t;

    tid_t get_my_tid () { return pthread_self(); }

    void sleep ( int ms ) {
        timespec  requested = { ms / 1000, (ms % 1000)*1000000 };
        timespec  remaining = {0};
        nanosleep(&requested, &remaining);
    }

#endif /* !WIN */

inline intptr num_subranges ( intptr length, intptr grain ) {
    return (intptr)std::pow(2., std::ceil(std::log((double)length / grain) / std::log(2.)));
}

} // namespace util

int g_max_concurrency = 0;
int g_num_threads = 0;

inline void yield_if_singlecore() { if ( g_max_concurrency == 1 ) __TBB_Yield(); }


class test_exception : public std::exception
{
    const char* my_description;
public:
    test_exception ( const char* description ) : my_description(description) {}

    const char* what() const throw() { return my_description; }
};

class solitary_test_exception : public test_exception
{
public:
    solitary_test_exception ( const char* description ) : test_exception(description) {}
};


tbb::atomic<intptr> g_cur_executed, // number of times a body was requested to process data
                    g_exc_executed, // snapshot of execution statistics at the moment of the 1st exception throwing
                    g_catch_executed, // snapshot of execution statistics at the moment when the 1st exception is caught
                    g_exceptions, // number of exceptions exposed to TBB users (i.e. intercepted by the test code)
                    g_added_tasks_count; // number of tasks added by parallel_do feeder

util::tid_t  g_master = 0;

volatile intptr g_exception_thrown = 0;
volatile bool g_throw_exception = true;
volatile bool g_no_exception = true;
volatile bool g_unknown_exception = false;
volatile bool g_task_was_cancelled = false;

bool    g_exception_in_master = false;
bool    g_solitary_exception = true;
volatile bool   g_wait_completed = false;

void reset_globals () {
    g_cur_executed = g_exc_executed = g_catch_executed = 0;
    g_exceptions = 0;
    g_exception_thrown = 0;
    g_throw_exception = true;
    g_no_exception = true;
    g_unknown_exception = false;
    g_task_was_cancelled = false;
    g_wait_completed = false;
    g_added_tasks_count = 0;
}

void throw_test_exception ( intptr throw_threshold ) {
    if ( !g_throw_exception  ||  g_exception_in_master ^ (util::get_my_tid() == g_master) )
        return; 
    if ( !g_solitary_exception ) {
        __TBB_CompareAndSwapW(&g_exc_executed, g_cur_executed, 0);
        TRACE ("About to throw one of multiple test_exceptions (thread %08x):", util::get_my_tid());
        g_exception_thrown = 1;
        throw (test_exception(EXCEPTION_DESCR));
    }
    while ( g_cur_executed < throw_threshold )
        yield_if_singlecore();
    if ( __TBB_CompareAndSwapW(&g_exception_thrown, 1, 0) == 0 ) {
        g_exc_executed = g_cur_executed;
        TRACE ("About to throw solitary test_exception... :");
        throw (solitary_test_exception(EXCEPTION_DESCR));
    }
}

#define TRY()   \
    bool no_exception = true, unknown_exception = false;    \
    try {

#define CATCH()     \
    } catch ( tbb::captured_exception& e ) {     \
        g_catch_executed = g_cur_executed;  \
        ASSERT (strcmp(e.name(), (g_solitary_exception ? typeid(solitary_test_exception) : typeid(test_exception)).name() ) == 0, "Unexpected original exception name");    \
        ASSERT (strcmp(e.what(), EXCEPTION_DESCR) == 0, "Unexpected original exception info");   \
        TRACE ("Executed at throw moment %d; upon catch %d", (intptr)g_exc_executed, (intptr)g_catch_executed);  \
        g_no_exception = no_exception = false;   \
        ++g_exceptions; \
    }   \
    catch ( ... ) { \
        g_no_exception = false;   \
        g_unknown_exception = unknown_exception = true;   \
    }

#define ASSERT_EXCEPTION()     \
    ASSERT (g_exception_thrown ? !g_no_exception : g_no_exception, "throw without catch or catch without throw");   \
    ASSERT (!g_no_exception, "no exception occurred");    \
    ASSERT (!g_unknown_exception, "unknown exception was caught");

#define CATCH_AND_ASSERT()     \
    CATCH() \
    ASSERT_EXCEPTION()

#define ASSERT_TEST_POSTCOND()


//------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------

typedef size_t count_type;
typedef tbb::blocked_range<count_type> range_type;


template<class Body>
intptr test_num_subranges_calculation ( intptr length, intptr grain, intptr nested_length, intptr nested_grain ) {
    reset_globals();
    g_throw_exception = false;
    intptr  nesting_body_calls = util::num_subranges(length, grain),
            nested_body_calls = util::num_subranges(nested_length, nested_grain),
            calls_in_normal_case = nesting_body_calls * (nested_body_calls + 1);
    tbb::parallel_for( range_type(0, length, grain), Body() );
    ASSERT (g_cur_executed == calls_in_normal_case, "Wrong estimation of bodies invocation count");
    return calls_in_normal_case;
}

class no_throw_pfor_body {
public:
    void operator()( const range_type& r ) const {
        volatile long x;
        count_type end = r.end();
        for( count_type i=r.begin(); i<end; ++i )
            x = 0;
    }
};

void Test0 () {
    TRACEP ("");
    reset_globals();
    tbb::simple_partitioner p;
    for( size_t i=0; i<10; ++i ) {
        tbb::parallel_for( range_type(0, 0, 1), no_throw_pfor_body() );
        tbb::parallel_for( range_type(0, 0, 1), no_throw_pfor_body(), p );
        tbb::parallel_for( range_type(0, 128, 8), no_throw_pfor_body() );
        tbb::parallel_for( range_type(0, 128, 8), no_throw_pfor_body(), p );
    }
} // void Test0 ()


class simple_pfor_body {
public:
    void operator()( const range_type& r ) const {
        volatile long x;
        count_type end = r.end();
        for( count_type i=r.begin(); i<end; ++i )
            x = 0;
        ++g_cur_executed;
        if ( g_exception_in_master ^ (util::get_my_tid() == g_master) )
        {
            //while ( g_no_exception ) __TBB_Yield();
            // Make absolutely sure that worker threads on multicore machines had a chance to steal something
            util::sleep(10);
        }
        throw_test_exception(1);
    }
};

void Test1 () {
    TRACEP ("");
    reset_globals();
    TRY();
        tbb::parallel_for( range_type(0, ITER_RANGE, ITER_GRAIN), simple_pfor_body() ); // , tbb::simple_partitioner()
    CATCH_AND_ASSERT();
    ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d; number of exceptions", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test1 ()


class nesting_pfor_body {
public:
    void operator()( const range_type& ) const {
        ++g_cur_executed;
        if ( util::get_my_tid() == g_master )
            yield_if_singlecore();
        tbb::parallel_for( tbb::blocked_range<size_t>(0, NESTED_RANGE, NESTED_GRAIN), simple_pfor_body() );
    }
};

//! Uses parallel_for body containing a nested parallel_for with the default context not wrapped by a try-block.
/** Nested algorithms are spawned inside the new bound context by default. Since 
    exceptions thrown from the nested parallel_for are not handled by the caller 
    (nesting parallel_for body) in this test, they will cancel all the sibling nested 
    algorithms. **/
void Test2 () {
    TRACEP ("");
    reset_globals();
    TRY();
        tbb::parallel_for( range_type(0, NESTING_RANGE, NESTING_GRAIN), nesting_pfor_body() );
    CATCH_AND_ASSERT();
    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    //if ( g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test2 ()


class nesting_pfor_with_isolated_context_body {
public:
    void operator()( const range_type& ) const {
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
//        util::sleep(1); // Give other threads a chance to steal their first tasks
        __TBB_Yield();
        tbb::parallel_for( tbb::blocked_range<size_t>(0, NESTED_RANGE, NESTED_GRAIN), simple_pfor_body(), tbb::simple_partitioner(), ctx );
    }
};

//! Uses parallel_for body invoking a nested parallel_for with an isolated context without a try-block.
/** Even though exceptions thrown from the nested parallel_for are not handled 
    by the caller in this test, they will not affect sibling nested algorithms 
    already running because of the isolated contexts. However because the first 
    exception cancels the root parallel_for only the first g_num_threads subranges
    will be processed (which launch nested parallel_fors) **/
void Test3 () {
    TRACEP ("");
    reset_globals();
    typedef nesting_pfor_with_isolated_context_body body_type;
    intptr  nested_body_calls = util::num_subranges(NESTED_RANGE, NESTED_GRAIN),
            min_num_calls = (g_num_threads - 1) * nested_body_calls;
    TRY();
        tbb::parallel_for( range_type(0, NESTING_RANGE, NESTING_GRAIN), body_type() );
    CATCH_AND_ASSERT();
    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    if ( g_solitary_exception ) {
        ASSERT (g_cur_executed > min_num_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= min_num_calls + (g_catch_executed + g_num_threads), "Too many tasks survived exception");
    }
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test3 ()


class nesting_pfor_with_eh_body {
public:
    void operator()( const range_type& ) const {
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
        TRY();
            tbb::parallel_for( tbb::blocked_range<size_t>(0, NESTED_RANGE, NESTED_GRAIN), simple_pfor_body(), tbb::simple_partitioner(), ctx );
        CATCH();
    }
};

//! Uses parallel_for body invoking a nested parallel_for (with default bound context) inside a try-block.
/** Since exception(s) thrown from the nested parallel_for are handled by the caller 
    in this test, they do not affect neither other tasks of the the root parallel_for 
    nor sibling nested algorithms. **/
void Test4 () {
    TRACEP ("");
    reset_globals();
    intptr  nested_body_calls = util::num_subranges(NESTED_RANGE, NESTED_GRAIN),
            nesting_body_calls = util::num_subranges(NESTING_RANGE, NESTING_GRAIN),
            calls_in_normal_case = nesting_body_calls * (nested_body_calls + 1);
    TRY();
        tbb::parallel_for( range_type(0, NESTING_RANGE, NESTING_GRAIN), nesting_pfor_with_eh_body() );
    CATCH();
    ASSERT (no_exception, "All exceptions must have been handled in the parallel_for body");
    TRACE ("Executed %d (normal case %d), exceptions %d, in master only? %d", (intptr)g_cur_executed, calls_in_normal_case, (intptr)g_exceptions, g_exception_in_master);
    intptr  min_num_calls = 0;
    if ( g_solitary_exception ) {
        min_num_calls = calls_in_normal_case - nested_body_calls;
        ASSERT (g_exceptions == 1, "No exception registered");
        ASSERT (g_cur_executed <= min_num_calls + g_num_threads, "Too many tasks survived exception");
    }
    else if ( !g_exception_in_master ) {
        // Each nesting body + at least 1 of its nested body invocations
        min_num_calls = 2 * nesting_body_calls;
        ASSERT (g_exceptions > 1 && g_exceptions <= nesting_body_calls, "Unexpected actual number of exceptions");
        ASSERT (g_cur_executed >= min_num_calls + (nesting_body_calls - g_exceptions) * nested_body_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived multiple exceptions");
        // Additional nested_body_calls accounts for the minimal amount of tasks spawned 
        // by not throwing threads. In the minimal case it is either the master thread or the only worker.
        ASSERT (g_cur_executed <= min_num_calls + (nesting_body_calls - g_exceptions + 1) * nested_body_calls + g_exceptions + g_num_threads, "Too many tasks survived exception");
    }
} // void Test4 ()


class my_cancellator_task : public tbb::task
{
    tbb::task_group_context &my_ctx_to_cancel;
    intptr my_cancel_threshold;

    tbb::task* execute () {
        s_cancellator_ready = true;
        while ( g_cur_executed < my_cancel_threshold )
            yield_if_singlecore();
        my_ctx_to_cancel.cancel_group_execution();
        g_catch_executed = g_cur_executed;
        return NULL;
    }
public:
    my_cancellator_task ( tbb::task_group_context& ctx, intptr threshold ) 
        : my_ctx_to_cancel(ctx), my_cancel_threshold(threshold)
    {}

    static volatile bool s_cancellator_ready;
};

volatile bool my_cancellator_task::s_cancellator_ready = false;

class pfor_body_to_cancel {
public:
    void operator()( const range_type& ) const {
        ++g_cur_executed;
        do {
            __TBB_Yield();
        } while( !my_cancellator_task::s_cancellator_ready );
    }
};

template<class B>
class my_worker_task : public tbb::task
{
    tbb::task_group_context &my_ctx;

    tbb::task* execute () {
        tbb::parallel_for( range_type(0, ITER_RANGE, ITER_GRAIN), B(), tbb::simple_partitioner(), my_ctx );
        return NULL;
    }
public:
    my_worker_task ( tbb::task_group_context& ctx ) : my_ctx(ctx) {}
};


//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
void Test5 () {
    TRACEP ("");
    reset_globals();
    g_throw_exception = false;
    intptr  threshold = util::num_subranges(ITER_RANGE, ITER_GRAIN) / 4;
    tbb::task_group_context  ctx;
    ctx.reset();
    my_cancellator_task::s_cancellator_ready = false;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_task(ctx, threshold) );
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_task<pfor_body_to_cancel>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");
    //ASSERT_WARNING (g_catch_executed < threshold + 2 * g_num_threads, "Too many tasks were executed between reaching threshold and signaling cancellation");
    ASSERT (g_cur_executed < g_catch_executed + g_num_threads, "Too many tasks were executed after cancellation");
} // void Test5 ()

class my_cancellator_2_task : public tbb::task
{
    tbb::task_group_context &my_ctx_to_cancel;

    tbb::task* execute () {
        util::sleep(20);  // allow the first workers to start
        my_ctx_to_cancel.cancel_group_execution();
        g_catch_executed = g_cur_executed;
        return NULL;
    }
public:
    my_cancellator_2_task ( tbb::task_group_context& ctx ) : my_ctx_to_cancel(ctx) {}
};

class pfor_body_to_cancel_2 {
public:
    void operator()( const range_type& ) const {
        ++g_cur_executed;
        // The test will hang (and be timed out by the test system) if is_cancelled() is broken
        while( !tbb::task::self().is_cancelled() ) __TBB_Yield();
    }
};

//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
/** This version also tests task::is_cancelled() method. **/
void Test6 () {
    TRACEP ("");
    reset_globals();
    tbb::task_group_context  ctx;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_2_task(ctx) );
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_task<pfor_body_to_cancel_2>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");
    ASSERT_WARNING (g_catch_executed < g_num_threads, "Somehow worker tasks started their execution before the cancellator task");
    ASSERT (g_cur_executed <= g_catch_executed, "Some tasks were executed after cancellation");
} // void Test6 ()

////////////////////////////////////////////////////////////////////////////////
// Regression test based on the contribution by the author of the following forum post:
// http://softwarecommunity.intel.com/isn/Community/en-US/forums/thread/30254959.aspx

#define LOOP_COUNT 16
#define MAX_NESTING 3
#define REDUCE_RANGE 1024 
#define REDUCE_GRAIN 256

class my_worker_t {
public:
    void doit (int & result, int nest);
};

class reduce_test_body_t {
    my_worker_t * my_shared_worker;
    int my_nesting_level;
    int my_result;
public:
    reduce_test_body_t ( reduce_test_body_t& src, tbb::split )
        : my_shared_worker(src.my_shared_worker)
        , my_nesting_level(src.my_nesting_level)
        , my_result(0)
    {}
    reduce_test_body_t ( my_worker_t *w, int nesting )
        : my_shared_worker(w)
        , my_nesting_level(nesting)
        , my_result(0)
    {}

    void operator() ( const tbb::blocked_range<size_t>& r ) {
        for (size_t i = r.begin (); i != r.end (); ++i) {
            int result = 0;
            my_shared_worker->doit (result, my_nesting_level);
            my_result += result;
        }
    }
    void join (const reduce_test_body_t & x) {
        my_result += x.my_result;
    }
    int result () { return my_result; }
};

void my_worker_t::doit ( int& result, int nest ) {
    ++nest;
    if ( nest < MAX_NESTING ) {
        reduce_test_body_t rt (this, nest);
        tbb::parallel_reduce (tbb::blocked_range<size_t>(0, REDUCE_RANGE, REDUCE_GRAIN), rt);
        result = rt.result ();
    }
    else
        ++result;
}

//! Regression test for hanging that occurred with the first version of cancellation propagation
void Test7 () {
    TRACEP ("");
    my_worker_t w;
    int result = 0;
    w.doit (result, 0);
    ASSERT ( result == 1048576, "Wrong calculation result");
}

void RunTests () {
    tbb::task_scheduler_init init (g_num_threads);
    g_master = util::get_my_tid();
    
    Test0();
#if !(__GLIBC__==2&&__GLIBC_MINOR__==3)
    Test1();
    Test3();
    Test4();
#endif
    Test5();
    Test6();
    Test7();
}

// Parallel_do testing

#define ITER_TEST_NUMBER_OF_ELEMENTS 1000

// Forward iterator type
template <class T>
class ForwardIterator {
    size_t * my_ptr;
public:
    typedef std::forward_iterator_tag iterator_category;
    typedef T value_type;
    typedef typename std::allocator<T>::difference_type difference_type;
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::reference reference;
   
    ForwardIterator ( size_t * ptr ) : my_ptr(ptr){}
    
    ForwardIterator ( const ForwardIterator& r ) : my_ptr(r.my_ptr){}
    
    size_t& operator* () { return *my_ptr; }
    
    ForwardIterator& operator++ () { ++my_ptr; return *this; }

    bool operator== ( const ForwardIterator& r ) { return my_ptr == r.my_ptr; }
};


template <class T>
class RandomIterator {
    size_t * my_ptr;
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef T value_type;
    typedef typename std::allocator<T>::difference_type difference_type;
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::reference reference;
   
    RandomIterator ( size_t * ptr ) : my_ptr(ptr){}
    
    RandomIterator ( const RandomIterator& r ) : my_ptr(r.my_ptr){}
    
    size_t& operator* () { return *my_ptr; }
    
    RandomIterator& operator++ () { ++my_ptr; return *this; }

    bool operator== ( const RandomIterator& r ) { return my_ptr == r.my_ptr; }
    size_t operator- (const RandomIterator &r) {return my_ptr - r.my_ptr;}
    RandomIterator operator+ (size_t n) {return RandomIterator(my_ptr + n);}
};

// Simple functor object, no exception throwing
class nothrow_pdo_body
{
public:
    //! This form of the function call operator can be used when the body needs to add more work during the processing
    void operator() ( size_t &value ) const {
        value = value + 1000;
    }
}; // class nothrow_pdo_body

// Simple functor object with feeder, no exception throwing
class nothrow_pdo_body_with_feeder
{
public:
    //! This form of the function call operator can be used when the body needs to add more work during the processing
    void operator() ( size_t &value, tbb::parallel_do_feeder<size_t>& feeder ) const {
        value = value + 1000;
        
        if (g_added_tasks_count < 500 )
        {
            g_added_tasks_count ++;
            feeder.add(0);
        }
    }
}; // class nothrow_pdo_body_with_feeder

// Test parallel_do without exceptions throwing
template <class Iterator> void Test0_parallel_do () {
    TRACEP ("");
    reset_globals();
    size_t values[ITER_TEST_NUMBER_OF_ELEMENTS];

    for (int i =0; i < ITER_TEST_NUMBER_OF_ELEMENTS; i++) values[i] = i;

    Iterator begin(&values[0]);
    Iterator end(&values[ITER_TEST_NUMBER_OF_ELEMENTS - 1]);
    tbb::parallel_do<Iterator, nothrow_pdo_body>(begin, end, nothrow_pdo_body());    
    g_added_tasks_count = 0;
    tbb::parallel_do<Iterator, nothrow_pdo_body_with_feeder>(begin, end, nothrow_pdo_body_with_feeder());    
} // void Test0_parallel_do_forward ()

// Simple functor object with exception
class simple_pdo_body
{
public:
    //! This form of the function call operator can be used when the body needs to add more work during the processing
    void operator() ( size_t &value ) const {
        value = value + 1000;

        ++g_cur_executed;
        if ( g_exception_in_master ^ (util::get_my_tid() == g_master) )
        {            
            // Make absolutely sure that worker threads on multicore machines had a chance to steal something
            util::sleep(10);
        }
        throw_test_exception(1);
    }
}; // class simple_pdo_body

// Simple functor object with exception and feeder
class simple_pdo_body_feeder
{
public:
    //! This form of the function call operator can be used when the body needs to add more work during the processing
    void operator() ( size_t &value, tbb::parallel_do_feeder<size_t> &feeder ) const {
        value = value + 1000;
        if (g_added_tasks_count < 500) {
            g_added_tasks_count++;
            feeder.add(0);
        }

        ++g_cur_executed;
        if ( g_exception_in_master ^ (util::get_my_tid() == g_master) )
        {            
            // Make absolutely sure that worker threads on multicore machines had a chance to steal something
            util::sleep(10);
        }
        throw_test_exception(1);
    }
}; // class simple_pdo_body_feeder

// Tests exceptions without nesting
template <class Iterator, class simple_body> void Test1_parallel_do () {
    TRACEP ("");
    reset_globals();
    
    size_t test_vector[ITER_TEST_NUMBER_OF_ELEMENTS + 1];

    for (int i =0; i < ITER_TEST_NUMBER_OF_ELEMENTS; i++) test_vector[i] = i;

    Iterator begin(&test_vector[0]);
    Iterator end(&test_vector[ITER_TEST_NUMBER_OF_ELEMENTS]);    

    g_added_tasks_count = 0;

    TRY();
        tbb::parallel_do<Iterator, simple_body>(begin, end, simple_body() );
    CATCH_AND_ASSERT();

    ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d; number of exceptions", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");

} // void Test1_parallel_do ()


template <class Iterator> class nesting_pdo_body {
public:  
    void operator()( size_t& /*value*/ ) const {
        ++g_cur_executed;
        if ( util::get_my_tid() == g_master )
            yield_if_singlecore();

        size_t test_vector[101];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);    
 
        tbb::parallel_do<Iterator, simple_pdo_body>(begin, end, simple_pdo_body());
    }
};

template <class Iterator> class nesting_pdo_body_feeder {
public:  
    void operator()( size_t& /*value*/, tbb::parallel_do_feeder<size_t>& feeder ) const {
        ++g_cur_executed;

        if (g_added_tasks_count < 500) {
            g_added_tasks_count++;
            feeder.add(0);
        }

        if ( util::get_my_tid() == g_master )
            yield_if_singlecore();

        size_t test_vector[101];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);    
 
        tbb::parallel_do<Iterator, simple_pdo_body>(begin, end, simple_pdo_body());
    }
};

//! Uses parallel_do body containing a nested parallel_do with the default context not wrapped by a try-block.
/** Nested algorithms are spawned inside the new bound context by default. Since 
    exceptions thrown from the nested parallel_do are not handled by the caller 
    (nesting parallel_do body) in this test, they will cancel all the sibling nested 
    algorithms. **/
template <class Iterator, class nesting_body> void Test2_parallel_do () {
    TRACEP ("");
    reset_globals();

    size_t test_vector[ITER_TEST_NUMBER_OF_ELEMENTS + 1];

    for (int i =0; i < ITER_TEST_NUMBER_OF_ELEMENTS; i++) test_vector[i] = i;

    Iterator begin(&test_vector[0]);
    Iterator end(&test_vector[ITER_TEST_NUMBER_OF_ELEMENTS]);    

    g_added_tasks_count = 0;

    TRY();
        tbb::parallel_do<Iterator, nesting_body >(begin, end, nesting_body() );
    CATCH_AND_ASSERT();

    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    //if ( g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test2_parallel_do ()

template <class Iterator> class nesting_pdo_with_isolated_context_body {
public:
    void operator()( size_t& /*value*/ ) const {
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;

        __TBB_Yield();
        size_t test_vector[101];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);    

        tbb::parallel_do<Iterator, simple_pdo_body>(begin, end, simple_pdo_body(), ctx);
    }
};

template <class Iterator> class nesting_pdo_with_isolated_context_body_and_feeder {
public:
    void operator()( size_t& /*value*/, tbb::parallel_do_feeder<size_t> &feeder ) const {
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;

        if (g_added_tasks_count < 50) {
            g_added_tasks_count++;
            feeder.add(0);
        }

        __TBB_Yield();
        size_t test_vector[101];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);    

        tbb::parallel_do<Iterator, simple_pdo_body>(begin, end, simple_pdo_body(), ctx);
    }
};

//! Uses parallel_do body invoking a nested parallel_do with an isolated context without a try-block.
/** Even though exceptions thrown from the nested parallel_do are not handled 
    by the caller in this test, they will not affect sibling nested algorithms 
    already running because of the isolated contexts. However because the first 
    exception cancels the root parallel_do only the first g_num_threads subranges
    will be processed (which launch nested parallel_dos) **/
template <class Iterator, class nesting_body> void Test3_parallel_do () {
    TRACEP ("");    
    reset_globals();    
    intptr  nested_body_calls = 100,
            min_num_calls = (g_num_threads - 1) * nested_body_calls;
    
    size_t test_vector[101];

    for (int i =0; i < 100; i++) test_vector[i] = i;

    Iterator begin(&test_vector[0]);
    Iterator end(&test_vector[100]);    

    TRY();
        tbb::parallel_do<Iterator, nesting_body >(begin, end,nesting_body());
    CATCH_AND_ASSERT();

    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    if ( g_solitary_exception ) {
        ASSERT (g_cur_executed > min_num_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= min_num_calls + (g_catch_executed + g_num_threads), "Too many tasks survived exception");
    }
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
} // void Test3_parallel_do ()


template <class Iterator> class nesting_pdo_with_eh_body {
public:
    void operator()( size_t& /*value*/ ) const {
        size_t test_vector[100];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);            

        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
        TRY();
            tbb::parallel_do<Iterator, simple_pdo_body>(begin, end, simple_pdo_body(), ctx);            
        CATCH();
    }
};

template <class Iterator> class nesting_pdo_with_eh_body_and_feeder: NoAssign {
public:
    void operator()( size_t &/*value*/, tbb::parallel_do_feeder<size_t> &feeder ) const {
        size_t test_vector[100];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);    

        if (g_added_tasks_count < 5) {
            g_added_tasks_count++;
            feeder.add(0);
        }

        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
        TRY();
            tbb::parallel_do<Iterator, simple_pdo_body>(begin, end, simple_pdo_body(), ctx);            
        CATCH();
    }
};


//! Uses parallel_for body invoking a nested parallel_for (with default bound context) inside a try-block.
/** Since exception(s) thrown from the nested parallel_for are handled by the caller 
    in this test, they do not affect neither other tasks of the the root parallel_for 
    nor sibling nested algorithms. **/

template <class Iterator, class nesting_body_with_eh> void Test4_parallel_do () {
    TRACEP ("");
    reset_globals();

    intptr  nested_body_calls = 100,
        nesting_body_calls = 10,
        calls_in_normal_case = nesting_body_calls * nested_body_calls;

    size_t test_vector[10];

    for (int i =0; i < 10; i++) test_vector[i] = i;

    Iterator begin(&test_vector[0]);
    Iterator end(&test_vector[10]);

    TRY();
        tbb::parallel_do<Iterator, nesting_body_with_eh>(begin, end, nesting_body_with_eh());
    CATCH();
    ASSERT (no_exception, "All exceptions must have been handled in the parallel_do body");
    TRACE ("Executed %d (normal case %d), exceptions %d, in master only? %d", (intptr)g_cur_executed, calls_in_normal_case, (intptr)g_exceptions, g_exception_in_master);
    intptr  min_num_calls = 0;
    if ( g_solitary_exception ) {
        min_num_calls = calls_in_normal_case - nested_body_calls;
        ASSERT (g_exceptions == 1, "No exception registered");
        ASSERT (g_cur_executed <= min_num_calls + g_num_threads, "Too many tasks survived exception");
    }
    else if ( !g_exception_in_master ) {
        // Each nesting body + at least 1 of its nested body invocations
        nesting_body_calls += g_added_tasks_count;
        min_num_calls = 2 * nesting_body_calls;        
        ASSERT (g_exceptions > 1 && g_exceptions <= nesting_body_calls, "Unexpected actual number of exceptions");
        ASSERT (g_cur_executed >= min_num_calls + (nesting_body_calls - g_exceptions) * nested_body_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived multiple exceptions");
        // Additional nested_body_calls accounts for the minimal amount of tasks spawned 
        // by not throwing threads. In the minimal case it is either the master thread or the only worker.
        ASSERT (g_cur_executed <= min_num_calls + (nesting_body_calls - g_exceptions + 1) * nested_body_calls + g_exceptions + g_num_threads, "Too many tasks survived exception");
    }
} // void Test4_parallel_do ()

class pdo_body_to_cancel {
public:
    void operator()( size_t& /*value*/ ) const {
        ++g_cur_executed;
        do {
            util::sleep(10);
            __TBB_Yield();
        } while( !my_cancellator_task::s_cancellator_ready );
    }
};

class pdo_body_to_cancel_with_feeder {
public:
    void operator()( size_t& /*value*/, tbb::parallel_do_feeder<size_t> &feeder ) const {
        ++g_cur_executed;

        if (g_added_tasks_count < 50) {
            g_added_tasks_count++;
            feeder.add(0);
        }

        do {
            util::sleep(10);
            __TBB_Yield();
        } while( !my_cancellator_task::s_cancellator_ready );
    }
};

template<class B, class Iterator>
class my_worker_pdo_task : public tbb::task
{
    tbb::task_group_context &my_ctx;

    tbb::task* execute () {
        size_t test_vector[100];

        for (int i =0; i < 100; i++) test_vector[i] = i;

        Iterator begin(&test_vector[0]);
        Iterator end(&test_vector[100]);

        tbb::parallel_do<Iterator, B>( begin, end, B(), my_ctx );
        return NULL;
    }
public:
    my_worker_pdo_task ( tbb::task_group_context& ctx ) : my_ctx(ctx) {}
};

//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
template <class Iterator, class body_to_cancel> void Test5_parallel_do () {
    TRACEP ("");
    reset_globals();
    g_throw_exception = false;
    intptr  threshold = 10;
    tbb::task_group_context  ctx;
    ctx.reset();
    my_cancellator_task::s_cancellator_ready = false;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_task(ctx, threshold) );
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_pdo_task<body_to_cancel, Iterator>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");    
    ASSERT (g_cur_executed < g_catch_executed + g_num_threads, "Too many tasks were executed after cancellation");
} // void Test5_parallel_do ()


class pdo_body_to_cancel_2 {
public:
    void operator()( size_t& /*value*/ ) const {
        ++g_cur_executed;
        // The test will hang (and be timed out by the tesst system) if is_cancelled() is broken
        while( !tbb::task::self().is_cancelled() ) __TBB_Yield();
    }
};

class pdo_body_to_cancel_2_with_feeder {
public:
    void operator()( size_t& /*value*/, tbb::parallel_do_feeder<size_t> &feeder ) const {
        ++g_cur_executed;

        if (g_added_tasks_count < 50) {
            g_added_tasks_count++;
            feeder.add(0);
        }

        // The test will hang (and be timed out by the tesst system) if is_cancelled() is broken
        while( !tbb::task::self().is_cancelled() ) __TBB_Yield();
    }
};

//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
/** This version also tests task::is_cancelled() method. **/
template <class Iterator, class body_to_cancel> void Test6_parallel_do () {
    TRACEP ("");
    reset_globals();
    tbb::task_group_context  ctx;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_2_task(ctx) );    
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_pdo_task<body_to_cancel, Iterator>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");    
    ASSERT (g_cur_executed <= g_catch_executed, "Some tasks were executed after cancellation");
} // void Test6_parallel_do ()

// This body throws an exception only if the task was added by feeder
class pdo_body_exception_only_in_task_added_by_feeder
{
public:
    //! This form of the function call operator can be used when the body needs to add more work during the processing
    void operator() ( size_t &value, tbb::parallel_do_feeder<size_t> &feeder ) const {
        ++g_cur_executed;

        if (g_added_tasks_count < 50) {
            g_added_tasks_count++;
            feeder.add(1);
        }

        if (value == 1) throw_test_exception(1);
    }
}; // class pdo_body_exception_only_in_task_added_by_feeder

// Test exception in task, which was added by feeder.
template <class Iterator> void Test8_parallel_do () {
    TRACEP ("");
    reset_globals();
    size_t values[ITER_TEST_NUMBER_OF_ELEMENTS];

    for (int i =0; i < ITER_TEST_NUMBER_OF_ELEMENTS; i++) values[i] = 0;

    Iterator begin(&values[0]);
    Iterator end(&values[ITER_TEST_NUMBER_OF_ELEMENTS - 1]);
    TRY();
        tbb::parallel_do<Iterator, pdo_body_exception_only_in_task_added_by_feeder>(begin, end, pdo_body_exception_only_in_task_added_by_feeder());    
    CATCH();    

    TRACE ("Executed at the end of test %d; number of exceptions", (intptr)g_cur_executed);
    if (g_solitary_exception)
        ASSERT (!no_exception, "At least one exception should occur");
} // void Test8_parallel_do ()


void RunParallelDoTests() {
    tbb::task_scheduler_init init (g_num_threads);
    g_master = util::get_my_tid();

    Test0_parallel_do<RandomIterator<size_t> >();
    Test0_parallel_do<ForwardIterator<size_t> >();
#if !(__GLIBC__==2&&__GLIBC_MINOR__==3)
    Test1_parallel_do<RandomIterator<size_t>, simple_pdo_body >();
    Test1_parallel_do<RandomIterator<size_t>, simple_pdo_body_feeder >();
    Test1_parallel_do<ForwardIterator<size_t>, simple_pdo_body >();
    Test1_parallel_do<ForwardIterator<size_t>, simple_pdo_body_feeder >();    
    Test2_parallel_do<RandomIterator<size_t>, nesting_pdo_body<RandomIterator<size_t> > >();
    Test2_parallel_do<RandomIterator<size_t>, nesting_pdo_body_feeder<RandomIterator<size_t> > >();
    Test2_parallel_do<ForwardIterator<size_t>, nesting_pdo_body<ForwardIterator<size_t> > >();
    Test2_parallel_do<ForwardIterator<size_t>, nesting_pdo_body_feeder<ForwardIterator<size_t> > >();
    Test3_parallel_do<ForwardIterator<size_t>, nesting_pdo_with_isolated_context_body<ForwardIterator<size_t> > >();
    Test3_parallel_do<RandomIterator<size_t>, nesting_pdo_with_isolated_context_body<RandomIterator<size_t> > >();    
    Test3_parallel_do<ForwardIterator<size_t>, nesting_pdo_with_isolated_context_body_and_feeder<ForwardIterator<size_t> > >();
    Test3_parallel_do<RandomIterator<size_t>, nesting_pdo_with_isolated_context_body_and_feeder<RandomIterator<size_t> > >(); 
    Test4_parallel_do<ForwardIterator<size_t>, nesting_pdo_with_eh_body<ForwardIterator<size_t> > >();    
    Test4_parallel_do<RandomIterator<size_t>, nesting_pdo_with_eh_body<RandomIterator<size_t> > >();    
    Test4_parallel_do<ForwardIterator<size_t>, nesting_pdo_with_eh_body_and_feeder<ForwardIterator<size_t> > >();    
    Test4_parallel_do<RandomIterator<size_t>, nesting_pdo_with_eh_body_and_feeder<RandomIterator<size_t> > >();
#endif        
    Test5_parallel_do<ForwardIterator<size_t>, pdo_body_to_cancel >();
    Test5_parallel_do<RandomIterator<size_t>, pdo_body_to_cancel >();
    Test5_parallel_do<ForwardIterator<size_t>, pdo_body_to_cancel_with_feeder >();
    Test5_parallel_do<RandomIterator<size_t>, pdo_body_to_cancel_with_feeder >();    
    Test6_parallel_do<ForwardIterator<size_t>, pdo_body_to_cancel_2 >();
    Test6_parallel_do<RandomIterator<size_t>, pdo_body_to_cancel_2 >();
    Test6_parallel_do<ForwardIterator<size_t>, pdo_body_to_cancel_2_with_feeder >();
    Test6_parallel_do<RandomIterator<size_t>, pdo_body_to_cancel_2_with_feeder >();
#if !(__GLIBC__==2&&__GLIBC_MINOR__==3)
    Test8_parallel_do<ForwardIterator<size_t> >();
    Test8_parallel_do<RandomIterator<size_t> >();
#endif
}

// Pipeline testing
#define TEST_PROLOGUE() \
    TRACEP ("");    \
    {   \
    tbb::task_scheduler_init init (g_num_threads);  \
    g_master = util::get_my_tid();  \
    reset_globals();    \

#define TEST_EPILOGUE() \
    }   \

const size_t buffer_size = 100,
             data_end_tag = size_t(~0);

size_t g_num_tokens = 0;

// Simple input filter class, it assigns 1 to all array members
// It stops when it receives item equal to -1
class input_filter: public tbb::filter {
    tbb::atomic<size_t> my_item;
    size_t my_buffer[buffer_size + 1];

public:    
    input_filter() : tbb::filter(parallel) {
        my_item = 0;
        for (size_t i = 0; i < buffer_size; ++i ) {
            my_buffer[i] = 1;
        }
        my_buffer[buffer_size] = data_end_tag;
    }

    void* operator()( void* ) {
        size_t item = my_item.fetch_and_increment();
        if ( item >= buffer_size ) { // end of input
            return NULL;
        }
        size_t &value = my_buffer[item];
        value = 1;
        return &value;
    }

    size_t* buffer() { return my_buffer; }
}; // class input_filter

// Pipeline filter, without exceptions throwing
class no_throw_filter: public tbb::filter {
    size_t my_value;
public:
    enum operation {
        addition,
        subtraction,
        multiplication,
    } my_operation;

    no_throw_filter(operation _operation, size_t value, bool is_parallel) 
        : filter(is_parallel? tbb::filter::parallel : tbb::filter::serial_in_order),
        my_value(value), my_operation(_operation)
    {}
    void* operator()(void* item) {
        size_t &value = *(size_t*)item;
        ASSERT(value != data_end_tag, "terminator element is being processed");
        switch (my_operation){
            case addition:
                value += my_value;
                break;
            case subtraction:
                value -= my_value;
                break;
            case multiplication:
                value *= my_value;
                break;
            default:
                ASSERT(0, "Wrong operation parameter passed to no_throw_filter");
        } // switch (my_operation)
        return item;
    }    
};

// Test pipeline without exceptions throwing
void Test0_pipeline () {
    TEST_PROLOGUE()

    // Run test when serial filter is the first non-input filter
    input_filter my_input_filter;
    no_throw_filter my_filter_1(no_throw_filter::addition, 99, false);
    no_throw_filter my_filter_2(no_throw_filter::subtraction, 90, true);
    no_throw_filter my_filter_3(no_throw_filter::multiplication, 5, false);
    // Result should be 50 for all items except the last

    tbb::pipeline my_pipeline;
    my_pipeline.add_filter(my_input_filter);
    my_pipeline.add_filter(my_filter_1);
    my_pipeline.add_filter(my_filter_2);
    my_pipeline.add_filter(my_filter_3);
    my_pipeline.run(8);    
    
    for (size_t i = 0; i < buffer_size; ++i) {
        ASSERT(my_input_filter.buffer()[i] == 50, "pipeline didn't process items properly");
    }
    TEST_EPILOGUE()
} // void Test0_pipeline ()

// Simple filter with exception throwing
class simple_filter : public tbb::filter
{
public:
    simple_filter (tbb::filter::mode _mode ) : filter (_mode) {}

    void* operator()(void* item) {        
        ++g_cur_executed;
        if ( g_exception_in_master ^ (util::get_my_tid() == g_master) )
        {            
            // Make absolutely sure that worker threads on multicore machines had a chance to steal something
            util::sleep(10);
        }
        throw_test_exception(1);

        return item;
    }
}; // class simple_filter

// This enumeration represents filters order in pipeline
enum filter_set {
    parallel__parallel=0,
    parallel__serial=1,
    parallel__serial_out_of_order=2,
    serial__parallel=4,
    serial__serial=5,
    serial__serial_out_of_order=6,
    serial_out_of_order__parallel=8,
    serial_out_of_order__serial=9,
    serial_out_of_order__serial_out_of_order=10
};

// The function returns filter type using filter number in set
tbb::filter::mode filter_mode (filter_set set, int number)
{
    size_t tmp = set << (2 * (2 - number));
    switch (tmp&12){
        case 0:
            return tbb::filter::parallel;
        case 4:
            return tbb::filter::serial_in_order;
        case 8:
            return tbb::filter::serial_out_of_order;
    }
    ASSERT(0, "Wrong filter set passed to get_filter_type");
    return tbb::filter::parallel; // We should never get here, just to prevent compiler warnings
}


template<typename InputFilter, typename Filter>
class custom_pipeline : protected tbb::pipeline {
    typedef tbb::pipeline base;
    InputFilter my_input_filter;
    Filter my_filter_1;
    Filter my_filter_2;

public:
    custom_pipeline( filter_set filter_set )
        : my_filter_1(filter_mode(filter_set, 1))
        , my_filter_2(filter_mode(filter_set, 2))
    {
       add_filter(my_input_filter);
       add_filter(my_filter_1);
       add_filter(my_filter_2);
    }
    void run () { base::run(g_num_tokens); }
    void run ( tbb::task_group_context& ctx ) { base::run(g_num_tokens, ctx); }
    using base::add_filter;
};

typedef custom_pipeline<input_filter, simple_filter> simple_pipeline;

// Tests exceptions without nesting
void Test1_pipeline ( filter_set mode ) {
    TEST_PROLOGUE()
    
    simple_pipeline test_pipeline(mode);
    TRY();
        test_pipeline.run();
        if ( g_cur_executed == 2 * buffer_size ) {
            // In case of all serial filters they might be all executed in the thread(s)
            // where exceptions are not allowed by the common test logic. So we just quit.
            return;
        }
    CATCH_AND_ASSERT();

    ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d; number of exceptions", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");

    TEST_EPILOGUE()
} // void Test1_pipeline ()

// Filter with nesting
class nesting_filter : public tbb::filter
{
public:
    nesting_filter(tbb::filter::mode _mode )
        : filter ( _mode)
    {}

    void* operator()(void* item) {        
        ++g_cur_executed;
        if ( util::get_my_tid() == g_master )
            yield_if_singlecore();

        simple_pipeline test_pipeline(serial__parallel);
        test_pipeline.run();        

        return item;
    }
}; // class nesting_filter

//! Uses pipeline containing a nested pipeline with the default context not wrapped by a try-block.
/** Nested algorithms are spawned inside the new bound context by default. Since 
    exceptions thrown from the nested pipeline are not handled by the caller 
    (nesting pipeline body) in this test, they will cancel all the sibling nested 
    algorithms. **/
void Test2_pipeline ( filter_set mode ) {
    TEST_PROLOGUE()

    custom_pipeline<input_filter, nesting_filter> test_pipeline(mode);
    TRY();
        test_pipeline.run();
    CATCH_AND_ASSERT();

    ASSERT (!no_exception, "No exception thrown from the nesting pipeline");
    //if ( g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TEST_EPILOGUE()
} // void Test2_pipeline ()

class nesting_filter_with_isolated_context : public tbb::filter
{
public:
    nesting_filter_with_isolated_context(tbb::filter::mode _mode )
        : filter ( _mode)
    {}

    void* operator()(void* item) {        
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
        __TBB_Yield();
        simple_pipeline test_pipeline(serial__parallel);
        test_pipeline.run(ctx);
        return item;
    }
}; // class nesting_filter_with_isolated_context

//! Uses pipeline invoking a nested pipeline with an isolated context without a try-block.
/** Even though exceptions thrown from the nested pipeline are not handled 
    by the caller in this test, they will not affect sibling nested algorithms 
    already running because of the isolated contexts. However because the first 
    exception cancels the root parallel_do only the first g_num_threads subranges
    will be processed (which launch nested pipelines) **/
void Test3_pipeline ( filter_set mode ) {
    TEST_PROLOGUE()
    intptr  nested_body_calls = 100,
            min_num_calls = (g_num_threads - 1) * nested_body_calls;
    
    custom_pipeline<input_filter, nesting_filter_with_isolated_context> test_pipeline(mode);
    TRY();
        test_pipeline.run();
    CATCH_AND_ASSERT();

    ASSERT (!no_exception, "No exception thrown from the nesting parallel_for");
    TRACE ("Executed at the end of test %d", (intptr)g_cur_executed);
    if ( g_solitary_exception ) {
        ASSERT (g_cur_executed > min_num_calls, "Too few tasks survived exception");
        ASSERT (g_cur_executed <= min_num_calls + (g_catch_executed + g_num_threads), "Too many tasks survived exception");
    }
    ASSERT (g_exceptions == 1, "No try_blocks in any body expected in this test");
    if ( !g_solitary_exception )
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived exception");
    TEST_EPILOGUE();
} // void Test3_pipeline ()

class nesting_filter_with_eh_body : public tbb::filter
{
public:
    nesting_filter_with_eh_body(tbb::filter::mode _mode )
        : filter ( _mode)
    {}

    void* operator()(void* item) {        
        tbb::task_group_context ctx(tbb::task_group_context::isolated);
        ++g_cur_executed;
        simple_pipeline test_pipeline(serial__parallel);
        TRY();
            test_pipeline.run(ctx);
        CATCH();
        return item;
    }
}; // class nesting_filter_with_eh_body

//! Uses pipeline body invoking a nested pipeline (with default bound context) inside a try-block.
/** Since exception(s) thrown from the nested pipeline are handled by the caller 
    in this test, they do not affect neither other tasks of the the root pipeline 
    nor sibling nested algorithms. **/

void Test4_pipeline ( filter_set mode ) {
    TEST_PROLOGUE()

    intptr  nested_body_calls = buffer_size + 1,
            nesting_body_calls = 2 * (buffer_size + 1),
            calls_in_normal_case = nesting_body_calls * nested_body_calls;

    custom_pipeline<input_filter, nesting_filter_with_eh_body> test_pipeline(mode);
    TRY();
        test_pipeline.run();
    CATCH_AND_ASSERT();


    ASSERT (no_exception, "All exceptions must have been handled in the parallel_do body");
    TRACE ("Executed %d (normal case %d), exceptions %d, in master only? %d", (intptr)g_cur_executed, calls_in_normal_case, (intptr)g_exceptions, g_exception_in_master);
    intptr  min_num_calls = 0;
    if ( g_solitary_exception ) {
        min_num_calls = calls_in_normal_case - nested_body_calls;
        ASSERT (g_exceptions == 1, "No exception registered");
        ASSERT (g_cur_executed <= min_num_calls + g_num_threads, "Too many tasks survived exception");
    }
    else if ( !g_exception_in_master ) {
        // Each nesting body + at least 1 of its nested body invocations
        nesting_body_calls += g_added_tasks_count;
        min_num_calls = 2 * nesting_body_calls;        
        ASSERT (g_exceptions > 1 && g_exceptions <= nesting_body_calls, "Unexpected actual number of exceptions");
        ASSERT (g_cur_executed <= g_catch_executed + g_num_threads, "Too many tasks survived multiple exceptions");
        // Additional nested_body_calls accounts for the minimal amount of tasks spawned 
        // by not throwing threads. In the minimal case it is either the master thread or the only worker.
        ASSERT (g_cur_executed <= min_num_calls + (nesting_body_calls - g_exceptions + 1) * nested_body_calls + g_exceptions + g_num_threads, "Too many tasks survived exception");
    }
    TEST_EPILOGUE()
} // void Test4_pipeline ()


class filter_to_cancel : public tbb::filter
{
public:
    filter_to_cancel(bool is_parallel) 
        : filter ( is_parallel ? tbb::filter::parallel : tbb::filter::serial_in_order)
    {}

    void* operator()(void* item) {
        ++g_cur_executed;
        do {
            util::sleep(10);
            __TBB_Yield();
        } while( !my_cancellator_task::s_cancellator_ready );
        return item;
    }
}; // class filter_to_cancel


template <class Filter_to_cancel> class my_worker_pipeline_task : public tbb::task
{
    tbb::task_group_context &my_ctx;

public:
    my_worker_pipeline_task ( tbb::task_group_context& ctx ) : my_ctx(ctx) {}

    tbb::task* execute () {
        // Run test when serial filter is the first non-input filter
        input_filter my_input_filter;
        Filter_to_cancel my_filter_to_cancel(true);        
        
        tbb::pipeline my_pipeline;
        my_pipeline.add_filter(my_input_filter);            
        my_pipeline.add_filter(my_filter_to_cancel);                

        my_pipeline.run(g_num_tokens, my_ctx);

        return NULL;
    }

};

//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
void Test5_pipeline () {
    TEST_PROLOGUE()

    g_throw_exception = false;
    intptr  threshold = 10;
    tbb::task_group_context ctx;
    ctx.reset();
    my_cancellator_task::s_cancellator_ready = false;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_task(ctx, threshold) );
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_pipeline_task<filter_to_cancel>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");    
    ASSERT (g_cur_executed < g_catch_executed + g_num_threads, "Too many tasks were executed after cancellation");
    TEST_EPILOGUE()
} // void Test5_pipeline ()

class filter_to_cancel_2 : public tbb::filter {
public:
    filter_to_cancel_2(bool is_parallel) 
        : filter ( is_parallel ? tbb::filter::parallel : tbb::filter::serial_in_order)
    {}

    void* operator()(void* item) {
        ++g_cur_executed;
        // The test will hang (and be timed out by the tesst system) if is_cancelled() is broken
        while( !tbb::task::self().is_cancelled() ) __TBB_Yield();
        return item;
    }
};

//! Test for cancelling an algorithm from outside (from a task running in parallel with the algorithm).
/** This version also tests task::is_cancelled() method. **/
void Test6_pipeline () {
    TEST_PROLOGUE()

    tbb::task_group_context  ctx;
    tbb::empty_task &r = *new( tbb::task::allocate_root() ) tbb::empty_task;
    r.set_ref_count(3);
    r.spawn( *new( r.allocate_child() ) my_cancellator_2_task(ctx) );    
    __TBB_Yield();
    r.spawn( *new( r.allocate_child() ) my_worker_pipeline_task<filter_to_cancel_2>(ctx) );
    TRY();
        r.wait_for_all();
    CATCH();
    r.destroy(r);
    ASSERT (no_exception, "Cancelling tasks should not cause any exceptions");    
    ASSERT (g_cur_executed <= g_catch_executed, "Some tasks were executed after cancellation");
    TEST_EPILOGUE()
} // void Test6_pipeline ()

//! Testing filter::finalize method
const int FINALIZE_SIZE_OF_EACH_BUFFER = buffer_size + 1;
const int FINALIZE_NUMBER_OF_BUFFERS = 10000;
tbb::atomic<size_t> allocated_count; // Number of currently allocated buffers
tbb::atomic<size_t> total_count; // Total number of allocated buffers

//! Base class for all filters involved in finalize method testing
class finalize_base_filter: public tbb::filter{
public:
    finalize_base_filter (tbb::filter::mode _mode) 
        : filter ( _mode)
    {}

    // Deletes buffers if exception occured
    virtual void finalize( void* item ) {
        size_t* my_item = (size_t*)item;
        delete[] my_item;
        allocated_count--;
    }
};

//! Input filter to test finalize method
class finalize_input_filter: public finalize_base_filter {
public:
    finalize_input_filter() : finalize_base_filter(tbb::filter::serial)
    {
        total_count = 0;        
    }
    void* operator()( void* ){
        if (total_count == FINALIZE_NUMBER_OF_BUFFERS) {
            return NULL;
        }

        size_t* item = new size_t[FINALIZE_SIZE_OF_EACH_BUFFER];
        for (int i = 0; i < FINALIZE_SIZE_OF_EACH_BUFFER; i++)
            item[i] = 1;
        total_count++;
        allocated_count ++;
        return item;
    }
};

// The filter multiplies each buffer item by 10.
class finalize_process_filter: public finalize_base_filter {        
public:
    finalize_process_filter (tbb::filter::mode _mode) : finalize_base_filter (_mode) {}

    void* operator()( void* item){
        if (total_count > FINALIZE_NUMBER_OF_BUFFERS / 2)
            throw_test_exception(1);

        size_t* my_item = (size_t*)item;
        for (int i = 0; i < FINALIZE_SIZE_OF_EACH_BUFFER; i++)
            my_item[i] *= 10;

        return item;
    }
};

// Output filter deletes previously allocated buffer
class finalize_output_filter: public finalize_base_filter {        
public:
    finalize_output_filter (tbb::filter::mode _mode) : finalize_base_filter (_mode) {}

    void* operator()( void* item){
        size_t* my_item = (size_t*)item;
        delete[] my_item;
        allocated_count--;

        return NULL; // not used
    }
};

//! Tests filter::finalize method
void Test8_pipeline (filter_set mode) {
    TEST_PROLOGUE()

    allocated_count = 0;

    custom_pipeline<finalize_input_filter, finalize_process_filter> test_pipeline(mode);
    finalize_output_filter my_output_filter(tbb::filter::parallel);

    test_pipeline.add_filter(my_output_filter);
    TRY();
        test_pipeline.run();
        //TRACEP("Pipeline run finished\n");
    CATCH();

    TEST_EPILOGUE()

    ASSERT (allocated_count == 0, "Memory leak: Some my_object weren't destroyed");
} // void Test8_pipeline ()

// Tests pipeline function passed with different combination of filters
template<void pipeline_test(filter_set)> 
void TestWithDifferentFilters() {
    pipeline_test(parallel__parallel);
    pipeline_test(parallel__serial);
    pipeline_test(parallel__serial_out_of_order);
    pipeline_test(serial__parallel);
    pipeline_test(serial__serial);
    pipeline_test(serial__serial_out_of_order);
    pipeline_test(serial_out_of_order__parallel);
    pipeline_test(serial_out_of_order__serial);
    pipeline_test(serial_out_of_order__serial_out_of_order);
}

void RunPipelineTests() {
    g_num_tokens = 2 * g_num_threads;

    Test0_pipeline();
#if !(__GLIBC__==2&&__GLIBC_MINOR__==3)
    TestWithDifferentFilters<Test1_pipeline>();
    TestWithDifferentFilters<Test2_pipeline>();
#endif /* __GLIBC__ */
    Test5_pipeline();
    Test6_pipeline();
#if !(__GLIBC__==2&&__GLIBC_MINOR__==3)
    TestWithDifferentFilters<Test8_pipeline>();
#endif
}

#endif /* __TBB_EXCEPTIONS */


//------------------------------------------------------------------------
// Entry point
//------------------------------------------------------------------------

/** If min and max thread numbers specified on the command line are different, 
    the test is run only for 2 sizes of the thread pool (MinThread and MaxThread) 
    to be able to test the high and low contention modes while keeping the test reasonably fast **/
int main(int argc, char* argv[]) {
    ParseCommandLine( argc, argv );
    MinThread = min(MinThread, MaxThread);
    ASSERT (MinThread>=2, "Minimal number of threads must be 2 or more");
    ASSERT (ITER_RANGE >= ITER_GRAIN * MaxThread, "Fix defines");
#if __TBB_EXCEPTIONS
    int step = max(MaxThread - MinThread, 1);
    for ( g_num_threads = MinThread; g_num_threads <= MaxThread; g_num_threads += step ) {
        TRACE ("Number of threads %d", g_num_threads);
        g_max_concurrency = min(g_num_threads, tbb::task_scheduler_init::default_num_threads());
        // Execute in all the possible modes
        for ( size_t j = 0; j < 4; ++j ) {
            g_exception_in_master = (j & 1) == 1;
            g_solitary_exception = (j & 2) == 1;
            RunTests();
            RunParallelDoTests();
            RunPipelineTests();
        }
    }
#if __GLIBC__==2&&__GLIBC_MINOR__==3
    printf("Warning: Exception handling tests are skipped due to a known issue.\n");
#endif // workaround
    printf("done\n");
#else  /* __TBB_EXCEPTIONS */
    printf("skipped\n");
#endif /* __TBB_EXCEPTIONS */
    return 0;
}

