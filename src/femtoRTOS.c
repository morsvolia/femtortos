/*
 * Copyright (c) 2006, Alexander Yaworsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <femtoRTOS.h>

/* External declarations. */
extern const struct task_group task_groups[];
extern const unsigned char number_of_task_groups;
extern const struct task *task_lists[];

/* Current task. */
const struct task * volatile current_task;

/* The list of delayed tasks */
const struct task *delayed_task_list;

/* This variable tracks the priority of current task. */
unsigned char current_priority;

/* Max priority of the ready-to-run task.
 * This variable may be increased when a task with a higher than current
 * priority becomes ready to run but context cannot be switched immediately
 * (critical section is owned or interrupts are disabled).
 */
unsigned char max_priority_ready_to_run;

unsigned char critical_section;

/*****************************************************************************
 * rtos_main
 *
 * The main function of RTOS.
 */
void rtos_main()
{
    unsigned char i;

    TRACE( "enter and never return\n" );

    TRACE( "initialize task_lists\n" );
    for( i = 0; i < number_of_task_groups; i++ )
        task_lists[ i ] = task_groups[ i ].start_of_array;

#   ifdef USE_TASK_LISTS
        TRACE( "initialize ready lists\n" );
        for( i = 0; i < number_of_task_groups; i++ )
        {
            const struct task *task = task_groups[ i ].start_of_array;
            const struct task *end_of_array = task_groups[ i ].end_of_array;
            const struct task * volatile *task_list_ptr = &task_lists[ i ];

            while( task != end_of_array )
            {
                struct task_control_block *tcb = task->tcb;
                tcb->next = task + 1;
                tcb->prev = task - 1;
                tcb->list = task_list_ptr;
                task++;
            }
            task = task_groups[ i ].start_of_array;
            end_of_array--;
            task->tcb->prev = end_of_array;
            end_of_array->tcb->next = task;
        }
#   endif

    TRACE( "initialize current_task and current_priority\n" );
    current_task = task_lists[ number_of_task_groups - 1 ];
    current_priority = current_task->priority;
    TRACE( "current_task=%p, current_priority=%u\n", current_task, current_priority );

    TRACE( "initialize variables\n" );
    delayed_task_list = NULL;
    max_priority_ready_to_run = 0;
    critical_section = 0;

    TRACE( "initialize stacks and TCBs\n" );
    for( i = 0; i < number_of_task_groups; i++ )
    {
        const struct task *task = task_groups[ i ].start_of_array;
        const struct task *end_of_array = task_groups[ i ].end_of_array;

        while( task != end_of_array )
        {
            struct task_control_block *tcb = task->tcb;
            tcb->next_delayed_task = NULL;
            tcb->wait_countdown = 0;
            tcb->saved_top_of_stack = init_stack( task->stack, task->stack_size, task->function );
            task++;
        }
    }

    TRACE( "voila\n" );
    restore_context();

    /* because instruction pointer on the virgin stack points to the task function,
     * we'll never get here
     * but... who knows ;))
     */
    ASSERT( FALSE, "never get here\n" );
}

#ifdef USE_TASK_LISTS
/*****************************************************************************
 * remove_task_from_list
 *
 * Remove item from the list.
 * Interrupts must be disabled before calling this function.
 */
static inline void remove_task_from_list( const struct task *task,
                                          struct task_control_block *tcb )
{
    if( tcb->next == task )
    {
        /* the item was last in the list */
        if( tcb->list )
            *tcb->list = NULL;
    }
    else
    {
        tcb->next->tcb->prev = tcb->prev;
        tcb->prev->tcb->next = tcb->next;
        if( *tcb->list == task )
            *tcb->list = tcb->next;
    }
}

/*****************************************************************************
 * move_task_to_head_of_list
 *
 * Move task from one list to other one.
 * The task becomes the next after head of destination list.
 * Interrupts must be disabled before calling this function.
 */
static inline void move_task_to_head_of_list( const struct task* task,
                                              const struct task* volatile* destination )
{
    struct task_control_block *tcb = task->tcb;

    remove_task_from_list( task, tcb );

    if( ! *destination )
        *destination = task;
    tcb->prev = *destination;
    tcb->next = tcb->prev->tcb->next;
    tcb->prev->tcb->next = task;
    tcb->next->tcb->prev = task;
    tcb->list = destination;
}

/*****************************************************************************
 * move_task_to_end_of_list
 *
 * Move task from one list to other one.
 * The task is added to the end of destination list.
 * Interrupts must be disabled before calling this function.
 */
static inline void move_task_to_end_of_list( const struct task* task,
                                             const struct task* volatile* destination )
{
    struct task_control_block *tcb = task->tcb;

    remove_task_from_list( task, tcb );

    if( ! *destination )
        *destination = task;
    tcb->next = *destination;
    tcb->prev = tcb->next->tcb->prev;
    tcb->next->tcb->prev = task;
    tcb->prev->tcb->next = task;
    tcb->list = destination;
}
#endif /* USE_TASK_LISTS */

/*****************************************************************************
 * tick_handler
 *
 * Must be invoked from some timer ISR.
 * The ISR must be a "naked" function with save_context() in prologue
 * and restore_context() in epilogue.
 * Interrupts must be disabled before calling this function.
 * Decrease wait_countdown of delayed tasks.
 * Return TRUE if some task becomes ready.
 * In this case current_task is set and context should be restored immediately.
 */
int tick_handler()
{
    const struct task *task = delayed_task_list;
    const struct task **prev_task_holder = &delayed_task_list;
    const struct task *ready_task = NULL;

    while( task )
    {
        struct task_control_block *tcb = task->tcb;

        if( --tcb->wait_countdown == 0 )
        {
            unsigned char priority = task->priority;

            /* the task becomes ready to run;
             * increase max_priority_ready_to_run, if required;
             * priority of ready_task must correspond the max_priority_ready_to_run
             */
            if( max_priority_ready_to_run < priority )
            {
                max_priority_ready_to_run = priority;
                ready_task = task;
            }
            else if( !ready_task )
                ready_task = task;

#           ifdef USE_TASK_LISTS
                /* move the task to the end of the ready list */
                move_task_to_end_of_list( task, task_lists + priority );
#           endif

            /* and remove the task from the delayed list */
            TRACE( "ready to run: %p\n", task );
            task = tcb->next_delayed_task;
            tcb->next_delayed_task = NULL;
            *prev_task_holder = task;
        }
        else
        {
            task = tcb->next_delayed_task;
            prev_task_holder = &tcb->next_delayed_task;
        }
    }

    if( NULL == current_task && ready_task )
    {
        current_task = ready_task;
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 * switch_context
 *
 * This function changes current_task pointer.
 * If the preemptive multitasking is required, this function must be invoked
 * from some timer ISR.
 * Usually it should be called from the same ISR after tick handler,
 * but this does not matters.
 * The ISR must be a "naked" function with save_context() in prologue
 * and restore_context() in epilogue.
 * Interrupts must be disabled before calling this function.
 * If there's no task to run, current_task is set NULL.
 */
void switch_context()
{
    TRACE_SWITCH_CONTEXT( "enter; current_task=%p\n", current_task );

    if( NULL == current_task )
    {
        TRACE_SWITCH_CONTEXT( "still no task to run\n" );
        return;
    }

    if( critical_section )
    {
        TRACE_SWITCH_CONTEXT( "critical section is owned\n" );
        return;
    }

    /* adjust current priority */
    if( max_priority_ready_to_run > current_priority )
        current_priority = max_priority_ready_to_run;

#   ifdef USE_TASK_LISTS

    /* Start from current priority, walk through the list,
     * if the list is empty, step down to the lower priority.
     */
    for(;;)
    {
        const struct task *task = task_lists[ current_priority ];

        if( task )
        {
            /* take the next task */
            current_task = task->tcb->next;
            task_lists[ current_priority ] = current_task;
            TRACE_SWITCH_CONTEXT( "exit; current_task=%p\n", current_task );
            return;
        }

        /* no ready tasks of current priority */
        if( 0 == current_priority )
        {
            current_task = NULL;
            TRACE_SWITCH_CONTEXT( "no task to run\n" );
            return;
        }

        /* step down to the lower priority */
        current_priority--;
        max_priority_ready_to_run = current_priority;
    }

#   else /* do not USE_TASK_LISTS */

    /* Start from current priority, walk through the tasks in the group,
     * if the group contains no ready tasks, step down to the lower priority.
     */
    for(;;)
    {
        const struct task *start_task, *end_of_array, *task;

        TRACE_SWITCH_CONTEXT( "find the next ready task of priority %u\n", current_priority );

        start_task = task_lists[ current_priority ];
        end_of_array = task_groups[ current_priority ].end_of_array;
        for( task = start_task + 1;; task++ )
        {
            if( task == end_of_array )
                /* wrap around */
                task = task_groups[ current_priority ].start_of_array;

            if( 0 == task->tcb->wait_countdown )
            {
                /* found the task */
                task_lists[ current_priority ] = task;
                current_task = task;
                TRACE_SWITCH_CONTEXT( "exit; current_task=%p\n", current_task );
                return;
            }

            if( task == start_task )
            {
                /* no ready tasks of current priority */
                if( 0 == current_priority )
                {
                    current_task = NULL;
                    TRACE_SWITCH_CONTEXT( "no task to run\n" );
                    return;
                }

                /* step down to the lower priority */
                current_priority--;
                max_priority_ready_to_run = current_priority;
                break;
            }
        }
    }
#   endif /* USE_TASK_LISTS */
}

/*****************************************************************************
 * delay_current_task
 *
 * Interrupts must be disabled before calling this function.
 */
static inline void delay_current_task( tick_t timeout )
{
    /* set idle state */
    current_task->tcb->wait_countdown = timeout;

    if( timeout != 0 && timeout != INFINITE_TIMEOUT )
    {
        /* add the current task to the delayed_task_list */
        current_task->tcb->next_delayed_task = delayed_task_list;
        delayed_task_list = current_task;
    }
}

/*****************************************************************************
 * sleep
 *
 * Sleep for the specified time.
 * If timeout is 0, simply switch context.
 * This function has no effect if called inside critical section.
 */
void sleep( tick_t timeout )
{
    TRACE( "enter; timeout=%u, current_task=%p\n", timeout, current_task );
    if( critical_section )
    {
        TRACE( "cannot sleep inside critical section\n" );
        return;
    }
    disable_interrupts();

#   ifdef USE_TASK_LISTS
    {
        /* remove task from the ready list */
        struct task_control_block *tcb = current_task->tcb;

        remove_task_from_list( current_task, tcb );

        tcb->prev = current_task;
        tcb->next = current_task;
        tcb->list = NULL;
    }
#   endif

    delay_current_task( timeout );
    yield();
    TRACE( "exit\n" );
}

/*****************************************************************************
 * leave_critical_section
 *
 * Check whether context switch is required after leaving and switch if so.
 */
void leave_critical_section()
{
    disable_interrupts();
    if( --critical_section == 0 )
    {
        if( max_priority_ready_to_run > current_priority )
        {
            yield();
            return;
        }
    }
    enable_interrupts();
}

/*****************************************************************************
 * leave_critical_section_no_yield
 *
 * Just leave critical section with no check whether context switch is required.
 * Interrupts must be disabled before calling this function because on some
 * architectures this operation is not atomic.
 */
static inline void leave_critical_section_no_yield()
{
    --critical_section;
}

/*****************************************************************************
 * capture_pso
 *
 * Try to gain exclusive access to a protected shared object.
 * Critical section must be owned once before calling this function.
 * On exit critical section is still owned.
 * If exclusive access cannot be gained immediately, current task
 * is placed into the wait_list and the task is suspended for the specified
 * timeout.
 * opposite_wait_list is used to work around priority inversion.
 * Return FALSE if timeout is 0 or critical section is owned more than once.
 */
static int capture_pso( const struct task* volatile* wait_list,
                        const struct task* volatile* opposite_wait_list, tick_t* timeout )
{
    TRACE( "enter\n" );

    if( *timeout == 0 )
    {
        TRACE( "exit; queue is full, timeout expired\n" );
        return FALSE;
    }

    if( critical_section != 1 )
    {
        TRACE( "exit; cannot suspend inside critical section\n" );
        return FALSE;
    }

    TRACE( "suspending itself; timeout=%u, current_task=%p\n", *timeout, current_task );

#   ifdef USE_TASK_LISTS

        /* move itself from the ready list to the list of waiting (idle) tasks */
        disable_interrupts();
        move_task_to_end_of_list( current_task, wait_list );
        enable_interrupts();

#   else /* do not USE_TASK_LISTS */
        *wait_list = current_task;
#   endif

    /* leave critical section but disable interrupts first */
    disable_interrupts();
    leave_critical_section_no_yield();

    /* suspend */
    delay_current_task( *timeout );
    /* switch context */
    yield();

#   ifdef USE_TASK_LISTS

        disable_interrupts();
        *timeout = current_task->tcb->wait_countdown;
        /* move itself from the list of waiting tasks to the ready list */
        move_task_to_end_of_list( current_task, task_lists + current_priority );
        enable_interrupts();

#   else /* do not USE_TASK_LISTS */
        *wait_list = NULL;
#   endif

    enter_critical_section();
    TRACE( "exit; ok\n" );
    return TRUE;
}

/*****************************************************************************
 * resume_waiting_tasks
 *
 * After completed operation with protected shared object this function
 * should be called to set ready state of all waiting tasks.
 * Interrupts must be disabled before calling this function.
 * Return TRUE if current_task was NULL and some task becomes ready.
 * In this case current_task is set and context should be restored immediately.
 * Otherwise switch_context() must be called immediately to gain control to
 * resumed tasks.
 * FIXME: seems it is possible to disable interrupts only in critical parts of code
 */
static int resume_waiting_tasks( const struct task* volatile* opposite_wait_list )
{
    const struct task *task = *opposite_wait_list;
    const struct task *ready_task = NULL;

    TRACE( "enter\n" );

    if( !task )
    {
        TRACE( "exit; empty list\n" );
        return FALSE;
    }

#   ifdef USE_TASK_LISTS

    do {
        unsigned char task_priority = task->priority;

        TRACE( "resume %p\n", task );
        task->tcb->wait_countdown = 0;

#       ifdef PRIORITY_INVERSION_SUPPORT
            /* This looks like a hack: it exploits the fact that context switcher
             * does not use priority member of the task structure.
             */
            if( task_priority < current_priority )
            {
                TRACE( "task %p with priority %u inherits current priority %u\n",
                       task, task_priority, current_priority );
                task_priority = current_priority;
            }
#       endif

        /* priority of ready_task must correspond the max_priority_ready_to_run */
        if( max_priority_ready_to_run < task_priority )
        {
            max_priority_ready_to_run = task_priority;
            ready_task = task;
        }
        else if( !ready_task )
            ready_task = task;
        /* Move task to the head of ready list for faster, out-of-order execution. */
        move_task_to_head_of_list( task, task_lists + task_priority );
        /* continue until all tasks from opposite wait list are resumed */
        task = *opposite_wait_list;
    } while( task );

#   ifdef PRIORITY_INVERSION_SUPPORT
        /* Priority of current task may be inherited. Restore it. */
        if( current_task->priority != current_priority )
        {
            TRACE( "restoring priority %u of current task %p; current_priority=%u \n",
                   current_task->priority, current_task, current_priority );
            current_priority = current_task->priority;
            move_task_to_end_of_list( current_task, task_lists + current_priority );
        }
#   endif

#   else /* do not USE_TASK_LISTS */
    {
        unsigned char task_priority = task->priority;
        const struct task *prev_task;
        const struct task_group *task_group;

        TRACE( "resume %p\n", task );
        task->tcb->wait_countdown = 0;
        /* priority of ready_task must correspond the max_priority_ready_to_run */
        if( max_priority_ready_to_run < task_priority )
        {
            max_priority_ready_to_run = task_priority;
            ready_task = task;
        }
        else if( !ready_task )
            ready_task = task;
        *opposite_wait_list = NULL;

        /* Schedule the task for faster, out-of-order execution. */
        prev_task = task - 1;
        task_group = &task_groups[ task_priority ];
        if( prev_task < task_group->start_of_array )
            prev_task = task_group->end_of_array - 1;
        task_lists[ task_priority ] = prev_task;
    }
#   endif

    if( NULL == current_task && ready_task )
    {
        current_task = ready_task;
        TRACE( "exit; return TRUE (current_task is set)\n" );
        return TRUE;
    }
    TRACE( "exit; return FALSE\n" );
    return FALSE;
}

/*****************************************************************************
 * queue_push
 *
 * Push a new item into the queue.
 */
int queue_push( const struct queue* queue, void* buffer, tick_t timeout )
{
    struct queue_variable_part *qvp = queue->variable_part;

    TRACE( "enter; queue=%p, buffer=%p, timeout=%u\n", queue, buffer, timeout );

    enter_critical_section();

    /* if queue is full, wait */
    while( qvp->item_count == queue->capacity )
    {
        if( !capture_pso( &qvp->waiting_push, &qvp->waiting_pop, &timeout ) )
        {
            leave_critical_section();
            TRACE( "exit; return FALSE\n" );
            return FALSE;
        }
    }

    TRACE( "putting the item into the ring buffer\n" );

    /* leave critical section but disable interrupts first */
    disable_interrupts();
    leave_critical_section_no_yield();

    /* put item into the ring buffer */
    qvp->item_count++;
    memcpy( queue->buffer + qvp->tail, buffer, queue->item_size );
    qvp->tail += queue->item_size;
    if( qvp->tail == queue->buffer_size )
        qvp->tail = 0;

    resume_waiting_tasks( &qvp->waiting_pop );

    /* we leaved critical section with no check whether context switch
     * is required; do it now
     */
    if( max_priority_ready_to_run > current_priority )
        yield();
    else
        enable_interrupts();
    TRACE( "exit; ok\n" );
    return TRUE;
}

/*****************************************************************************
 * queue_push_from_isr
 *
 * Similar to queue_push() but never wait.
 * ISR must be a "naked" function with save_context()/restore_context() calls.
 */
enum pso_result queue_push_from_isr( const struct queue* queue, void* buffer )
{
    struct queue_variable_part *qvp = queue->variable_part;

    TRACE( "enter; queue=%p, buffer=%p\n", queue, buffer );

    if( qvp->item_count == queue->capacity )
    {
        TRACE( "exit; queue is full\n" );
        return PSO_BUSY;
    }

    /* put item into the ring buffer */
    qvp->item_count++;
    memcpy( queue->buffer + qvp->tail, buffer, queue->item_size );
    qvp->tail += queue->item_size;
    if( qvp->tail == queue->buffer_size )
        qvp->tail = 0;

    if( resume_waiting_tasks( &qvp->waiting_pop ) )
    {
        TRACE( "exit; return RESTORE_CONTEXT_NOW\n" );
        return RESTORE_CONTEXT_NOW;
    }

    TRACE( "exit; return SWITCH_CONTEXT_NOW\n" );
    return SWITCH_CONTEXT_NOW;
}

/*****************************************************************************
 * queue_pop
 *
 * Pop item from the queue.
 */
int queue_pop( const struct queue* queue, void* buffer, tick_t timeout )
{
    struct queue_variable_part *qvp = queue->variable_part;

    TRACE( "enter; queue=%p, buffer=%p, timeout=%u\n", queue, buffer, timeout );

    enter_critical_section();

    /* if queue is empty, wait */
    while( qvp->item_count == 0 )
    {
        if( !capture_pso( &qvp->waiting_pop, &qvp->waiting_push, &timeout ) )
        {
            leave_critical_section();
            TRACE( "exit; return FALSE\n" );
            return FALSE;
        }
    }

    TRACE( "extracting item from the ring buffer\n" );

    /* leave critical section but disable interrupts first */
    disable_interrupts();
    leave_critical_section_no_yield();

    /* extract item from the ring buffer */
    qvp->item_count--;
    memcpy( buffer, queue->buffer + qvp->head, queue->item_size );
    qvp->head += queue->item_size;
    if( qvp->head == queue->buffer_size )
        qvp->head = 0;

    resume_waiting_tasks( &qvp->waiting_push );

    if( max_priority_ready_to_run > current_priority )
        yield();
    else
        enable_interrupts();
    TRACE( "exit; ok\n" );
    return TRUE;
}

/*****************************************************************************
 * queue_pop_from_isr
 *
 * Similar to queue_pop() but never wait.
 * ISR must be a "naked" function with save_context()/restore_context() calls.
 */
enum pso_result queue_pop_from_isr( const struct queue* queue, void* buffer )
{
    struct queue_variable_part *qvp = queue->variable_part;

    TRACE( "enter; queue=%p, buffer=%p\n", queue, buffer );

    if( qvp->item_count == 0 )
    {
        TRACE( "exit; queue is empty\n" );
        return PSO_BUSY;
    }

    /* extract item from the ring buffer */
    qvp->item_count--;
    memcpy( buffer, queue->buffer + qvp->head, queue->item_size );
    qvp->head += queue->item_size;
    if( qvp->head == queue->buffer_size )
        qvp->head = 0;

    if( resume_waiting_tasks( &qvp->waiting_push ) )
    {
        TRACE( "exit; return RESTORE_CONTEXT_NOW\n" );
        return RESTORE_CONTEXT_NOW;
    }

    TRACE( "exit; return SWITCH_CONTEXT_NOW\n" );
    return SWITCH_CONTEXT_NOW;
}
