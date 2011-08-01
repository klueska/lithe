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
#include "tbb/cache_aligned_allocator.h"
#include "itt_notify.h"

namespace tbb {

namespace internal {

//! This structure is used to store task information in ordered buffer
struct task_info{
    void* my_object;
    Token my_token;
    bool my_token_ready;
    bool is_valid;
};

//! A buffer of ordered items.
/** Each item is a task, inserted into a position in the buffer corrsponding to a Token. */
class ordered_buffer {
    typedef  Token  size_type;

    //! Array of deferred tasks that cannot yet start executing. 
    task_info* array;

    //! Size of array
    /** Always 0 or a power of 2 */
    size_type array_size;

    //! Lowest token that can start executing.
    /** All prior Token have already been seen. */
    Token low_token;

    //! Serializes updates.
    spin_mutex array_mutex;

    //! Resize "array".
    /** Caller is responsible to acquiring a lock on "array_mutex". */
    void grow( size_type minimum_size );

    //! Initial size for "array"
    /** Must be a power of 2 */
    static const size_type initial_buffer_size = 4;

    //! Used only for out of order buffer.
    Token high_token;

    //! True for ordered filter, false otherwise. 
    bool is_ordered;
public:
    //! Construct empty buffer.
    ordered_buffer( bool is_ordered_ ) : array(NULL), array_size(0),
                                         low_token(0), high_token(0), is_ordered(is_ordered_) {
        grow(initial_buffer_size);
        __TBB_ASSERT( array, NULL );
    }

    //! Destroy the buffer.
    ~ordered_buffer() {
        __TBB_ASSERT( array, NULL );
        cache_aligned_allocator<task_info>().deallocate(array,array_size);
        poison_pointer( array );
    }

    //! Put a token into the buffer.
    /** If task information was placed into buffer, returns true;
        otherwise returns false, informing the caller to create and spawn a task.
    */
    // Using template to avoid explicit dependency on stage_task
    template<typename StageTask>
    bool put_token( StageTask& putter ) {
        {
            spin_mutex::scoped_lock lock( array_mutex );
            Token token = is_ordered ? putter.next_token_number() : high_token++;
            if( token!=low_token ) {
                // Trying to put token that is beyond low_token.
                // Need to wait until low_token catches up before dispatching.
                __TBB_ASSERT( (tokendiff_t)(token-low_token)>0, NULL );
                if( token-low_token>=array_size ) 
                    grow( token-low_token+1 );
                ITT_NOTIFY( sync_releasing, this );
                putter.put_task_info(array[token&array_size-1]);
                return true;
            }
        }
        return false;
    }

    //! Note that processing of a token is finished.
    /** Fires up processing of the next token, if processing was deferred. */
    // Using template to avoid explicit dependency on stage_task
    template<typename StageTask>
    void note_done( Token token, StageTask& spawner ) {
        task_info wakee = {NULL, 0, 0, false};
        {
            spin_mutex::scoped_lock lock( array_mutex );
            if( !is_ordered || token==low_token ) {
                // Wake the next task
                task_info& item = array[++low_token & array_size-1];
                ITT_NOTIFY( sync_acquired, this );
                wakee = item;
                item.is_valid = false;
            }
        }
        if( wakee.is_valid )
            spawner.spawn_stage_task(wakee);
    }

#if __TBB_EXCEPTIONS
    //! The method destroys all data in filters to prevent memory leaks
    void clear( filter* my_filter ) {
        long t=low_token;
        for( size_type i=0; i<array_size; ++i, ++t ){
            task_info& temp = array[t&array_size-1];
            if (temp.is_valid ) {
                my_filter->finalize(temp.my_object);
                temp.is_valid = false;
            }
        }
    }
#endif
};

void ordered_buffer::grow( size_type minimum_size ) {
    size_type old_size = array_size;
    size_type new_size = old_size ? 2*old_size : initial_buffer_size;
    while( new_size<minimum_size ) 
        new_size*=2;
    task_info* new_array = cache_aligned_allocator<task_info>().allocate(new_size);
    task_info* old_array = array;
    for( size_type i=0; i<new_size; ++i )
        new_array[i].is_valid = false;
    long t=low_token;
    for( size_type i=0; i<old_size; ++i, ++t )
        new_array[t&new_size-1] = old_array[t&old_size-1];
    array = new_array;
    array_size = new_size;
    if( old_array )
        cache_aligned_allocator<task_info>().deallocate(old_array,old_size);
}

class stage_task: public task {
private:
    friend class tbb::pipeline;
    pipeline& my_pipeline;
    void* my_object;
    filter* my_filter;  
    //! Invalid until this task went through an ordered stage.
    Token my_token;
    //! False until my_token is set.
    bool my_token_ready;
    //! True if this task has not yet read the input.
    bool my_at_start;
public:
    //! Construct stage_task for first stage in a pipeline.
    /** Such a stage has not read any input yet. */
    stage_task( pipeline& pipeline ) : 
        my_pipeline(pipeline), 
        my_object(NULL),
        my_filter(pipeline.filter_list),
        my_token_ready(false),
        my_at_start(true)
    {}
    //! Construct stage_task for a subsequent stage in a pipeline.
    stage_task( pipeline& pipeline, filter* filter_list ) : 
        my_pipeline(pipeline), 
        my_filter(filter_list),
        my_token_ready(false),
        my_at_start(false)
    {}
    //! Requests the pipeline for the next token number
    /** It's not thread safe! Callers should guarantee exclusive execution */
    inline Token next_token_number () {
        if(!my_token_ready) {
            my_token = my_pipeline.token_counter++;
            my_token_ready = true;
        }
        return my_token;
    }
    //! The virtual task execution method
    /*override*/ task* execute();
#if __TBB_EXCEPTIONS
    ~stage_task()    
    {
        if (my_filter && my_object && (my_filter->my_filter_mode & filter::version_mask) >= __TBB_PIPELINE_VERSION(4)) {
            __TBB_ASSERT(is_cancelled(), "Tryning to finalize the task that wasn't cancelled");
            my_filter->finalize(my_object);
            my_object = NULL;
        }
    }
#endif // __TBB_EXCEPTIONS
    //! Creates and spawns stage_task from task_info
    void spawn_stage_task(const task_info& info)
    {
        stage_task* clone = new( allocate_additional_child_of(*my_pipeline.end_counter) ) stage_task( my_pipeline, my_filter );
        clone->my_token = info.my_token;
        clone->my_token_ready = info.my_token_ready;
        clone->my_object = info.my_object;                
        
        spawn(*clone);
    }
    //! Puts current task information
    void put_task_info(task_info &where_to_put ) {
        where_to_put.my_object = my_object;
        where_to_put.my_token = my_token;
        where_to_put.my_token_ready = my_token_ready;
        where_to_put.is_valid = true;
    }
};

task* stage_task::execute() {
    __TBB_ASSERT( !my_at_start || !my_object, NULL );
    if( my_at_start ) {
        if( my_filter->is_serial() ) {
            my_object = (*my_filter)(my_object);
            if( my_object ) {
                my_token = my_pipeline.token_counter++;
                my_token_ready = true;
                ITT_NOTIFY( sync_releasing, &my_pipeline.input_tokens );
                if( --my_pipeline.input_tokens>0 ) 
                    spawn( *new( allocate_additional_child_of(*my_pipeline.end_counter) ) stage_task( my_pipeline ) );
            } else {
                my_pipeline.end_of_input = true; 
                return NULL;
            }
        } else /*not is_serial*/ {
            if( my_pipeline.end_of_input )
                return NULL;
            ITT_NOTIFY( sync_releasing, &my_pipeline.input_tokens );
            if( --my_pipeline.input_tokens>0 )
                spawn( *new( allocate_additional_child_of(*my_pipeline.end_counter) ) stage_task( my_pipeline ) );
            my_object = (*my_filter)(my_object);
            if( !my_object ) {
                my_pipeline.end_of_input = true; 
                return NULL;
            }
        }
        my_at_start = false;
    } else {
        my_object = (*my_filter)(my_object);
        if( ordered_buffer* input_buffer = my_filter->input_buffer )
            input_buffer->note_done(my_token, *this);
    }
    task* next = NULL;
    my_filter = my_filter->next_filter_in_pipeline; 
    if( my_filter ) {
        // There is another filter to execute.
        // Crank up priority a notch.
        add_to_depth(1);
        if( ordered_buffer* input_buffer = my_filter->input_buffer ) {
            // The next filter must execute tokens in order.
            if (input_buffer->put_token(*this )){
                my_filter = NULL; // To prevent deleting my_object twice if exception occurs
                return NULL;
            }
        }
        /* A semi-hackish way to reexecute the same task object immediately without spawning.
           recycle_as_continuation marks the task for future execution,
           and then 'this' pointer is returned to bypass spawning. */
        recycle_as_continuation();
        next = this;
    } else {
        // Reached end of the pipe.  Inject a new token.
        // The token must be injected before execute() returns, in order to prevent the
        // end_counter task's reference count from prematurely reaching 0.
        set_depth( my_pipeline.end_counter->depth()+1 ); 
        if( ++my_pipeline.input_tokens==1 ) {
            ITT_NOTIFY( sync_acquired, &my_pipeline.input_tokens );
            if( !my_pipeline.end_of_input ) 
                spawn( *new( allocate_additional_child_of(*my_pipeline.end_counter) ) stage_task( my_pipeline ) );
        }
    }
    return next;
}

} // namespace internal

void pipeline::inject_token( task& ) {
    __TBB_ASSERT(0,"illegal call to inject_token");
}

#if __TBB_EXCEPTIONS
void pipeline::clear_filters() {
    for( filter* f = filter_list; f; f = f->next_filter_in_pipeline ) {
        if ((f->my_filter_mode & filter::version_mask) >= __TBB_PIPELINE_VERSION(4))
            if( internal::ordered_buffer* b = f->input_buffer )
                b->clear(f);
    }
}
#endif

pipeline::pipeline() : 
    filter_list(NULL),
    filter_end(NULL),
    end_counter(NULL),
    token_counter(0),
    end_of_input(false)
{
    input_tokens = 0;
}

pipeline::~pipeline() {
    clear();
}

void pipeline::clear() {
    filter* next;
    for( filter* f = filter_list; f; f=next ) {
        if( internal::ordered_buffer* b = f->input_buffer ) {
            delete b; 
            f->input_buffer = NULL;
        }
        next=f->next_filter_in_pipeline;
        f->next_filter_in_pipeline = filter::not_in_pipeline();
        if ( (f->my_filter_mode & filter::version_mask) >= __TBB_PIPELINE_VERSION(3) ) {
            f->prev_filter_in_pipeline = filter::not_in_pipeline();
            f->my_pipeline = NULL;
        }
    }
    filter_list = filter_end = NULL;
}

void pipeline::add_filter( filter& filter_ ) {
#if TBB_USE_ASSERT
    if ( (filter_.my_filter_mode & filter::version_mask) >= __TBB_PIPELINE_VERSION(3) ) 
        __TBB_ASSERT( filter_.prev_filter_in_pipeline==filter::not_in_pipeline(), "filter already part of pipeline?" );
    __TBB_ASSERT( filter_.next_filter_in_pipeline==filter::not_in_pipeline(), "filter already part of pipeline?" );
    __TBB_ASSERT( !end_counter, "invocation of add_filter on running pipeline" );
#endif    
    if( filter_.is_serial() ) {
        filter_.input_buffer = new internal::ordered_buffer( filter_.is_ordered() );
    }
    if ( (filter_.my_filter_mode & filter::version_mask) >= __TBB_PIPELINE_VERSION(3) ) {
        filter_.my_pipeline = this;
        filter_.prev_filter_in_pipeline = filter_end;
        if ( filter_list == NULL)
            filter_list = &filter_;
        else
            filter_end->next_filter_in_pipeline = &filter_;
        filter_.next_filter_in_pipeline = NULL;
        filter_end = &filter_;
    }
    else
    {
        if( !filter_end )
            filter_end = reinterpret_cast<filter*>(&filter_list);
        
        *reinterpret_cast<filter**>(filter_end) = &filter_;
        filter_end = reinterpret_cast<filter*>(&filter_.next_filter_in_pipeline);
        *reinterpret_cast<filter**>(filter_end) = NULL;
    }
}

void pipeline::remove_filter( filter& filter_ ) {
    if (&filter_ == filter_list) 
        filter_list = filter_.next_filter_in_pipeline;
    else {
        __TBB_ASSERT( filter_.prev_filter_in_pipeline, "filter list broken?" ); 
        filter_.prev_filter_in_pipeline->next_filter_in_pipeline = filter_.next_filter_in_pipeline;
    }
    if (&filter_ == filter_end)
        filter_end = filter_.prev_filter_in_pipeline;
    else {
        __TBB_ASSERT( filter_.next_filter_in_pipeline, "filter list broken?" ); 
        filter_.next_filter_in_pipeline->prev_filter_in_pipeline = filter_.prev_filter_in_pipeline;
    }
    if( internal::ordered_buffer* b = filter_.input_buffer ) {
        delete b; 
        filter_.input_buffer = NULL;
    }
    filter_.next_filter_in_pipeline = filter_.prev_filter_in_pipeline = filter::not_in_pipeline();
    filter_.my_pipeline = NULL;
}

void pipeline::run( size_t max_number_of_live_tokens
#if __TBB_EXCEPTIONS
    , tbb::task_group_context& context
#endif
    ) {
    __TBB_ASSERT( max_number_of_live_tokens>0, "pipeline::run must have at least one token" );
    __TBB_ASSERT( !end_counter, "pipeline already running?" );
    // The class destroys end_counter and clears all ordered buffers if pipeline was cancelled.
    class pipeline_cleaner: internal::no_copy {
        pipeline& my_pipeline;        
    public:
        pipeline_cleaner(pipeline& _pipeline) : my_pipeline(_pipeline)
        {}
        ~pipeline_cleaner(){
#if __TBB_EXCEPTIONS
            if (my_pipeline.end_counter->is_cancelled()) // Pipeline was cancelled
                my_pipeline.clear_filters(); 
#endif
            my_pipeline.end_counter->destroy(*my_pipeline.end_counter);
            my_pipeline.end_counter = NULL;            
        }
    };
    if( filter_list ) {
        if( filter_list->next_filter_in_pipeline || !filter_list->is_serial() ) {
            pipeline_cleaner my_pipeline_cleaner(*this);
            end_of_input = false;
#if __TBB_EXCEPTIONS            
            end_counter = new( task::allocate_root(context) ) empty_task;
#else
            end_counter = new( task::allocate_root() ) empty_task;
#endif
            // 2 = 1 for spawned child + 1 for wait
            end_counter->set_ref_count(2);
            input_tokens = internal::Token(max_number_of_live_tokens);
            // Prime the pump with the non-waiter
            end_counter->spawn_and_wait_for_all( *new( end_counter->allocate_child() ) internal::stage_task( *this ) );
        } else {
            // There are no filters, and thus no parallelism is possible.
            // Just drain the input stream.
            while( (*filter_list)(NULL) ) 
                continue;
        }
    } 
}

#if __TBB_EXCEPTIONS
void pipeline::run( size_t max_number_of_live_tokens ) {
    tbb::task_group_context context;
    run(max_number_of_live_tokens, context);
}
#endif // __TBB_EXCEPTIONS

filter::~filter() {
    if ( (my_filter_mode & version_mask) >= __TBB_PIPELINE_VERSION(3) ) {
        if ( next_filter_in_pipeline != filter::not_in_pipeline() ) { 
            __TBB_ASSERT( prev_filter_in_pipeline != filter::not_in_pipeline(), "probably filter list is broken" );
            my_pipeline->remove_filter(*this);
        } else 
            __TBB_ASSERT( prev_filter_in_pipeline == filter::not_in_pipeline(), "probably filter list is broken" );
    } else {
        __TBB_ASSERT( next_filter_in_pipeline==filter::not_in_pipeline(), "cannot destroy filter that is part of pipeline" );
    }
}

} // tbb

