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

#include <femtoRTOS.h>

#ifndef USE_TASK_LISTS

int main( int argc, char* argv[] )
{
    TEST_RESULT( TRUE, "priority inversion cannot be tested in minimalistic configuration" );
    return 0;
}

#else

QUEUE( queue, 1, sizeof(int) );

static void high_priority_task_func()
{
    int value, ret;

    TRACE( "enter\n" );

    TRACE( "allow medium priority task to execute\n" );
    sleep(1);

    TRACE( "low priority task is idle here; it is on the waiting list\n" );
    TRACE( "medium priority task should be ready here\n" );
    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 1, "bad first item: %d\n", value );

    ret = queue_pop( &queue, &value, 1 );
    if( ret )
    {
        ASSERT( value == 2, "bad second item: %d\n", value );
    }

#   ifdef PRIORITY_INVERSION_SUPPORT
        TEST_RESULT( ret == TRUE, "queue_pop must extract item from queue" );
#   else
        TEST_RESULT( ret == FALSE, "queue_pop must be timed out because of priority inversion" );
#   endif
}

static void medium_priority_task_func()
{
    TRACE( "enter\n" );

    TRACE( "allow low priority task to execute\n" );
    sleep(1);

    for(;;)
    {
        TRACE( "running\n" );
        yield();
        TRACE( "high priority task is waiting, simulate ticks\n" );
        tick_handler();
    }
}

static void low_priority_task_func()
{
    int value;

    TRACE( "enter\n" );

    TRACE( "fill queue\n" );
    value = 1;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    TRACE( "wake up high and medium priority tasks\n" );
    tick_handler();
    TRACE( "here we should enter idle state inside capture_pso()\n" );
    value = 2;
    queue_push( &queue, &value, INFINITE_TIMEOUT );

    ASSERT( FALSE, "never get here; context switch should occur in queue_push because higher priority task is woken\n" );
}

TCB( low_priority_task, 4096 );
TCB( medium_priority_task, 4096 );
TCB( high_priority_task, 4096 );

TASK_GROUP( low_priority_tasks ) {
    TASK( 0, low_priority_task, low_priority_task_func )
};

TASK_GROUP( medium_priority_tasks ) {
    TASK( 1, medium_priority_task, medium_priority_task_func )
};

TASK_GROUP( high_priority_tasks ) {
    TASK( 2, high_priority_task, high_priority_task_func )
};

TASKS_BEGIN
    TASKS_ITEM( low_priority_tasks ),
    TASKS_ITEM( medium_priority_tasks ),
    TASKS_ITEM( high_priority_tasks )
TASKS_END;

int main( int argc, char* argv[] )
{
    rtos_main();
    return 0;
}

#endif /* USE_TASK_LISTS */
