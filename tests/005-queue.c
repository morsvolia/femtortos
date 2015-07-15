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

QUEUE( queue, 5, sizeof(int) );

static void task_1_func()
{
    int value;

    TRACE( "enter\n" );

    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 1, "bad first item: %d\n", value );

    TRACE( "sleep and let task_2 to fill queue and enter idle state\n" );
    sleep(1);

    TRACE( "ok, read from queue\n" );

    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 2, "bad item 2: %d\n", value );
    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 3, "bad item 3: %d\n", value );
    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 4, "bad item 4: %d\n", value );
    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 5, "bad item 5: %d\n", value );
    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 6, "bad item 6: %d\n", value );
    TRACE( "the next queue_pop should release pending push in task_2\n" );
    queue_pop( &queue, &value, INFINITE_TIMEOUT );
    ASSERT( value == 7, "bad item 7: %d\n", value );

    TEST_RESULT( TRUE, "all right" );
}

static void task_2_func()
{
    int value;

    TRACE( "enter\n" );

    value = 1;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    TRACE( "pushed item 1, let task_1 to read it\n" );
    yield();

    TRACE( "fill queue\n" );
    value = 2;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    value = 3;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    value = 4;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    value = 5;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    value = 6;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    TRACE( "now we must enter idle state\n" );
    value = 7;
    queue_push( &queue, &value, INFINITE_TIMEOUT );
    TRACE( "let task_1 to complete queue_pop\n" );
    yield();
    ASSERT( FALSE, "never get here\n" );
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
    rtos_main();
    return 0;
}
