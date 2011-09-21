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

#include "tbb/pipeline.h"
#include "tbb/spin_mutex.h"
#include <cstdlib>
#include <cstdio>
#include "harness.h"

struct Buffer {
    int id;
    //! True if Buffer is in use.
    bool is_busy;
    int sequence_number;
    Buffer() : id(-1), is_busy(false), sequence_number(-1) {}
};

class waiting_probe {
    size_t check_counter;
public:
    waiting_probe() : check_counter(0) {}
    bool required( ) {
        ++check_counter;
        return !((check_counter+1)&size_t(0x7FFF));
    }
    void probe( ); // defined below
};

static const int MaxStreamSize = 1<<12;
//! Maximum number of filters allowed
static const int MaxFilters = 5;
static int StreamSize;
static const size_t MaxBuffer = 8;
static bool Done[MaxFilters][MaxStreamSize];
static waiting_probe WaitTest;
static size_t out_of_order_count;

class BaseFilter: public tbb::filter {
    bool* const my_done;
    const bool my_is_last;  
    bool my_is_running;
public:
    tbb::atomic<int> current_token;
    BaseFilter( tbb::filter::mode type, bool done[], bool is_last ) : 
        filter(type),
        my_done(done),
        my_is_last(is_last),
        my_is_running(false),
        current_token()
    {}
    virtual Buffer* get_buffer( void* item ) {
        current_token++;
        return static_cast<Buffer*>(item);
    } 
    /*override*/void* operator()( void* item ) {
        if( is_serial() )
            ASSERT( !my_is_running, "premature entry to serial stage" );
        my_is_running = true;
        Buffer* b = get_buffer(item);
        if( b ) {
            if( is_ordered() ) {
                if( b->sequence_number == -1 ) 
                    b->sequence_number = current_token-1;
                else 
                    ASSERT( b->sequence_number==current_token-1, "item arrived out of order" );
            } else if( is_serial() ) {
                    if( b->sequence_number != current_token-1 && b->sequence_number != -1 )
                        out_of_order_count++;
            }
            ASSERT( 0<=b->id && b->id<StreamSize, NULL ); 
            ASSERT( !my_done[b->id], "duplicate processing of token?" ); 
            ASSERT( b->is_busy, NULL );
            my_done[b->id] = true;
            if( my_is_last ) {
                b->id = -1;
                b->sequence_number = -1;
                __TBB_store_with_release(b->is_busy, false);
            }
        }
        my_is_running = false;
        return b;  
    }
};
class InputFilter: public BaseFilter {
    tbb::spin_mutex input_lock;
    Buffer buffer[MaxBuffer];
    const size_t my_number_of_tokens;
public:
    InputFilter( tbb::filter::mode type, size_t ntokens, bool done[], bool is_last ) :
        BaseFilter(type, done, is_last),
        my_number_of_tokens(ntokens)
    {}
    /*override*/Buffer* get_buffer( void* ) {
        int next_input;
        size_t free_buffer = 0; 
        { // lock protected scope
            tbb::spin_mutex::scoped_lock lock(input_lock);
            if( current_token>=StreamSize )
                return NULL;
            next_input = current_token++; 
            // once in a while, emulate waiting for input; this only makes sense for serial input
            if( is_serial() && WaitTest.required() )
                WaitTest.probe( );
            while( free_buffer<MaxBuffer )
                if( __TBB_load_with_acquire(buffer[free_buffer].is_busy) )
                    ++free_buffer;
                else {
                    buffer[free_buffer].is_busy = true;
                    break;
                }
        }
        ASSERT( free_buffer<my_number_of_tokens, "premature reuse of buffer" );
        Buffer* b = &buffer[free_buffer]; 
        ASSERT( &buffer[0] <= b, NULL ); 
        ASSERT( b <= &buffer[MaxBuffer-1], NULL ); 
        ASSERT( b->id == -1, NULL);
        b->id = next_input;
        ASSERT( b->sequence_number == -1, NULL);
        return b;
    }
};

//! The struct below repeats layout of tbb::pipeline.
struct hacked_pipeline {
    tbb::filter* filter_list;
    tbb::filter* filter_end;
    tbb::empty_task* end_counter;
    tbb::internal::Token input_tokens;
    tbb::internal::Token token_counter;
    bool end_of_input;

    virtual ~hacked_pipeline();
};

//! The struct below repeats layout of tbb::internal::ordered_buffer.
struct hacked_ordered_buffer {
    void* array; // This should be changed to task_info* if ever used
    tbb::internal::Token array_size;
    tbb::internal::Token low_token;
    tbb::spin_mutex array_mutex;
    tbb::internal::Token high_token;
    bool is_ordered;
};

//! The struct below repeats layout of tbb::filter.
struct hacked_filter {
    tbb::filter* next_filter_in_pipeline;
    hacked_ordered_buffer* input_buffer;
    unsigned char my_filter_mode;
    tbb::filter* prev_filter_in_pipeline;
    tbb::pipeline* my_pipeline;

    virtual ~hacked_filter();
};

const tbb::internal::Token tokens_before_wraparound = 0xF;

void TestTrivialPipeline( size_t nthread, int number_of_filters ) {
    // There are 3 filter types: parallel, serial_in_order and serial_out_of_order 
    static const tbb::filter::mode filter_table[] = { tbb::filter::parallel, tbb::filter::serial_in_order, tbb::filter::serial_out_of_order}; 
    const int number_of_filter_types = int(sizeof(filter_table)/sizeof(filter_table[0]));
    if( Verbose ) 
        printf("testing with %d threads and %d filters\n", int(nthread), number_of_filters );
    ASSERT( number_of_filters<=MaxFilters, "too many filters" );
    ASSERT( sizeof(hacked_pipeline) == sizeof(tbb::pipeline), "layout changed for tbb::pipeline?" );
    ASSERT( sizeof(hacked_filter) == sizeof(tbb::filter), "layout changed for tbb::filter?" );
    size_t ntokens = nthread<MaxBuffer ? nthread : MaxBuffer;
    // Count maximum iterations number
    int limit = 1;
    for( int i=0; i<number_of_filters; ++i)
        limit *= number_of_filter_types;
    // Iterate over possible filter sequences
    for( int numeral=0; numeral<limit; ++numeral ) {
        // Build pipeline
        tbb::pipeline pipeline;
        // A private member of pipeline is hacked there for sake of testing wrap-around immunity.
        ((hacked_pipeline*)(void*)&pipeline)->token_counter = ~tokens_before_wraparound;
        tbb::filter* filter[MaxFilters];
        int temp = numeral;
        for( int i=0; i<number_of_filters; ++i, temp/=number_of_filter_types ) {
            tbb::filter::mode filter_type = filter_table[temp%number_of_filter_types];
            const bool is_last = i==number_of_filters-1;
            if( i==0 )
                filter[i] = new InputFilter(filter_type,ntokens,Done[i],is_last);
            else
                filter[i] = new BaseFilter(filter_type,Done[i],is_last);
            pipeline.add_filter(*filter[i]);
            // The ordered buffer of serial filters is hacked as well.
            if ( filter[i]->is_serial() ) {
                ((hacked_filter*)(void*)filter[i])->input_buffer->low_token = ~tokens_before_wraparound;
                if ( !filter[i]->is_ordered() )
                    ((hacked_filter*)(void*)filter[i])->input_buffer->high_token = ~tokens_before_wraparound;
            }
        }
        for( StreamSize=0; StreamSize<=MaxStreamSize; StreamSize += StreamSize/3+1 ) {
            memset( Done, 0, sizeof(Done) );
            for( int i=0; i<number_of_filters; ++i ) {
                static_cast<BaseFilter*>(filter[i])->current_token=0;
            }
            pipeline.run( ntokens );
            for( int i=0; i<number_of_filters; ++i )
                ASSERT( static_cast<BaseFilter*>(filter[i])->current_token==StreamSize, NULL );
            for( int i=0; i<MaxFilters; ++i )
                for( int j=0; j<StreamSize; ++j ) {
                    ASSERT( Done[i][j]==(i<number_of_filters), NULL );
                }
        }
        for( int i=number_of_filters; --i>=0; ) {
            delete filter[i];
            filter[i] = NULL;
        }
        pipeline.clear();
    }
}

#include "harness_cpu.h"

static int nthread; // knowing number of threads is necessary to call TestCPUUserTime

void waiting_probe::probe( ) {
    if( nthread==1 ) return;
    if( Verbose ) printf("emulating wait for input\n");
    // Test that threads sleep while no work.
    // The master doesn't sleep so there could be 2 active threads if a worker is waiting for input
    TestCPUUserTime(nthread, 2);
}

#include "tbb/task_scheduler_init.h"

int main( int argc, char* argv[] ) {
    // Default is at least one thread.
    MinThread = 1;
    out_of_order_count = 0;
    ParseCommandLine(argc,argv);
    if( MinThread<1 ) {
        printf("must have at least one thread");
        exit(1);
    }

    // Test with varying number of threads.
    for( nthread=MinThread; nthread<=MaxThread; ++nthread ) {
        // Initialize TBB task scheduler
        tbb::task_scheduler_init init(nthread);

        // Test pipelines with n filters
        for( int n=0; n<=5; ++n )
            TestTrivialPipeline(size_t(nthread),n);

        // Test that all workers sleep when no work
        TestCPUUserTime(nthread);
    }
    if( !out_of_order_count )
        printf("Warning: out of order serial filter received tokens in order\n");
    printf("done\n");
    return 0;
}
