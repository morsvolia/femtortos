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
#include <limits.h>

static int task_1_iteration = 0, task_2_iteration = 0, max_iterations = 0;

static void task_1_func()
{
    TRACE( "enter\n" );
    for( ; task_1_iteration < max_iterations; task_1_iteration++ )
    {
        TRACE( "%d\n", task_1_iteration );
        yield();
    }
    TRACE( "task_1_iteration=%d, task_2_iteration=%d\n",
           task_1_iteration, task_2_iteration );

    TEST_RESULT( task_1_iteration == max_iterations
                 && task_2_iteration == max_iterations - 1,
                 "task_1_iteration is greater because of post-increment in for() loop" );
}

static void task_2_func()
{
    TRACE( "enter\n" );
    for( ;; task_2_iteration++ )
    {
        TRACE( "%d\n", task_2_iteration );
        yield();
    }
}

TCB( task_1, 4096 );
TCB( task_2, 4096 );

TASK_GROUP( tasks ) {
    TASK( 0, task_1, task_1_func ),
    TASK( 0, task_2, task_2_func )
};

TASKS_BEGIN
    TASKS_ITEM( tasks )
TASKS_END;

int main( int argc, char* argv[] )
{
    if( argc > 1 )
    {
        max_iterations = atoi( argv[1] );
    }

    if( max_iterations == 0 )
        max_iterations = 10;
    else if( max_iterations < 0 )
        max_iterations = INT_MAX;

    rtos_main();
    return 0;
}
