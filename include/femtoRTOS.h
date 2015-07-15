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

#ifndef FEMTO_RTOS_H
#define FEMTO_RTOS_H

#include <stdio.h>

/*****************************************************************************
 * Include configuration parameters.
 */
#include "femtoRTOSconfig.h"

/*****************************************************************************
 * Definitions of miscellaneous important global variables required by ports.
 */

/* Critical section hold count. */
extern unsigned char critical_section;

/*****************************************************************************
 * Include hardware-specific definitions.
 */
#include <ports/femtoRTOSports.h>

/*****************************************************************************
 * Define missed macros.
 */
#ifndef TRACE
#   define TRACE(...)
#endif

#ifndef TRACE_SWITCH_CONTEXT
#   define TRACE_SWITCH_CONTEXT(...)
#endif

#ifndef ASSERT
#   define ASSERT(...)
#endif

/*****************************************************************************
 * Definitions of miscellaneous functions.
 */

/* Tick handler. */
extern int tick_handler();

/* Context switcher. */
extern void switch_context();

/* leave_critical_section */
extern void leave_critical_section();

/* The main function of RTOS */
extern void rtos_main();

/*****************************************************************************
 * Definitions of miscellaneous constants.
 */

#define INFINITE_TIMEOUT ((tick_t) -1)

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

/*****************************************************************************
 * Tasks
 *
 * Because microcontrollers have very limited amount of RAM, the structure
 * that describes the task is splitted into two parts to minimize RAM usage.
 */

/* Forward declaration. */
struct task;

/* Task control block must be placed in RAM. */
struct task_control_block
{
    /* This is used by context switcher to save stack pointer. */
    void *saved_top_of_stack;

    /* Tasks that are delayed for spefified time are linked in the
     * singly-linked delayed task list.
     * This list is used by tick handler.
     */
    const struct task *next_delayed_task;

    /* Explicit task state; 0 means READY state,
     * other value means IDLE state and is decremented by the tick handler.
     * (if not equals to INFINITE_TIMEOUT but in this case the task simply
     * is not included in this list)
     */
    volatile tick_t wait_countdown;

#   ifdef USE_TASK_LISTS

    /* At any given time the task may be in one of existing lists:
     * in the ready list of tasks with the same priority
     * or in the list of idle tasks waiting for the protected shared object
     * (currently, it is the writing end of a queue).
     * These pointers are used to make doubly-linked ring list.
     * If the list has only one item or this structure does
     * not belong to any list, these pointers initialized to the task
     * itself -- this facilitates insert and removal operations.
     */
    const struct task *next;
    const struct task *prev;

    /* We must know about the variable that referred to the list,
     * without this knowledge we cannot properly remove item from the list.
     */
    const struct task * volatile *list;

#   endif
};

/* Task definition may be placed in ROM. */
struct task
{
    struct task_control_block *tcb;
    void (*function)();
    void *stack;
    unsigned stack_size;  /* in bytes! */
    unsigned char priority;
};

/* Task control block and stack definition macro.
 * Reserves size * sizeof(void*) bytes of RAM for the stack.
 */
#define TCB( task_name, stack_size ) \
    struct task_control_block tcb_##task_name; \
    void *stack_##task_name[ stack_size ]

/* Tasks with the same priority are grouped together.
 */
#define TASK_GROUP( name ) \
    const struct task task_group_##name[] =

#define TASK( task_priority, task_name, task_function ) \
    { \
        &tcb_##task_name, \
        task_function, \
        stack_##task_name, \
        sizeof(stack_##task_name), \
        task_priority \
    }

/* Structure that describes group of tasks. */
struct task_group
{
    const struct task *start_of_array;
    const struct task *end_of_array;
};

/* Tasks definition macros.
 * IMPORTANT: the order of groups must be ascending by priority.
 * Priority numbers must start with 0 and must not have gaps.
 */
#define TASKS_BEGIN \
    /* task_groups contains descriptions of groups of tasks \
     * with the same priority. \
     */ \
    const struct task_group task_groups[] = {

#define TASKS_ITEM( task_group_name ) \
    { \
        task_group_##task_group_name, \
        &task_group_##task_group_name[ (sizeof(task_group_##task_group_name)/sizeof(task_group_##task_group_name[0])) ] \
    }

#define TASKS_END \
    }; \
    /* number of task groups */ \
    const unsigned char number_of_task_groups = sizeof(task_groups)/sizeof(task_groups[0]); \
    \
    /* task_lists contains pointers to the current task of each group. */ \
    const struct task *task_lists[ sizeof(task_groups)/sizeof(task_groups[0]) ]

/* Example: define four tasks.
 *
 * TCB( task_1, 48 );
 * TCB( task_2, 64 );
 * TCB( task_3, 32 );
 * TCB( task_4, 80 );
 * TASK_GROUP( idle_priority_tasks ) {
 *     TASK( 0, task_1, task_1_func ),
 *     TASK( 0, task_2, task_2_func )
 * };
 * TASK_GROUP( normal_priority_tasks ) {
 *     TASK( 1, task_3, task_3_func ),
 *     TASK( 1, task_4, task_4_func )
 * };
 * TASKS_BEGIN
 *     TASKS_ITEM( idle_priority_tasks ),
 *     TASKS_ITEM( normal_priority_tasks )
 * TASKS_END;
 */

/* Current task. */
extern const struct task * volatile current_task;

/* Task functions. */
extern void sleep( tick_t timeout );

/*****************************************************************************
 * Protected shared objects (PSO)
 */

/* return values for functions accessing PSO from interrupt service routine */
enum pso_result
{
    PSO_BUSY,              /* access to PSO would block */
    RESTORE_CONTEXT_NOW,   /* some task becomes ready, restore context immediately */
    SWITCH_CONTEXT_NOW     /* sitch_context() should be called to boost released tasks */
};

/*****************************************************************************
 * Queues
 *
 * Similar to tasks, queue structure is splitted into two parts.
 */

/* Queue variable part, must be placed in RAM */
struct queue_variable_part
{
    const struct task * volatile waiting_push;
    const struct task * volatile waiting_pop;
    unsigned char head;
    unsigned char tail;
    volatile unsigned char item_count;
};

/* Queue definition, may be placed in ROM */
struct queue
{
    struct queue_variable_part *variable_part;
    void *buffer;
    unsigned char capacity;
    unsigned char item_size;
    unsigned char buffer_size;
};

/* Queue definition macro. */
#define QUEUE( name, capacity, item_size ) \
    char queue_buffer_##name[ capacity * item_size ];  /* ring buffer */ \
    \
    struct queue_variable_part queue_variable_part_##name = { \
        NULL, /* waiting_task_push */ \
        NULL, /* waiting_task_pop */ \
        0,    /* head */ \
        0,    /* tail */ \
        0     /* item_count */ \
    }; \
    const struct queue name = { \
        &queue_variable_part_##name, \
        queue_buffer_##name, \
        capacity, \
        item_size, \
        item_size * capacity /* buffer_size */ \
    }

/* Example: define a queue.
 *
 * QUEUE( sample_queue, 2, 4 );
 */

/* Queue functions. */
extern int queue_push( const struct queue* queue, void* buffer, tick_t timeout );
extern enum pso_result queue_push_from_isr( const struct queue* queue, void* buffer );
extern int queue_pop( const struct queue* queue, void* buffer, tick_t timeout );
extern enum pso_result queue_pop_from_isr( const struct queue* queue, void* buffer );

#endif  /* FEMTO_RTOS_H */
