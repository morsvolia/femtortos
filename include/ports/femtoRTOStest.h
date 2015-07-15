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

#ifndef FEMTO_RTOS_TEST_H
#define FEMTO_RTOS_TEST_H

#include <stdlib.h>

/* Just declare the necessary functions.
 * See tests for their implementation.
 */
extern void enable_interrupts();
extern void disable_interrupts();
extern void enter_critical_section();
extern void yield();
extern void restore_context();
extern void* init_stack( char* stack, int stack_size, void (*task_function)() );

/* Define TRACE, ASSERT macros and underlying functions
 */
#define TRACE( ... ) rtos_trace( __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__ )
#define ASSERT( expression, ... ) \
    do { \
        if( !(expression) ) \
            rtos_assert( __FILE__, __LINE__, __FUNCTION__, #expression, __VA_ARGS__ ); \
    } while(0)

extern void rtos_trace( const char* file, int line, const char* func, const char* fmt, ... );
extern void rtos_assert( const char* file, int line, const char* func,
                         const char* expression_str, const char* fmt, ... );

/* Test framework
 */
#define TEST_RESULT( expression, comment ) \
    do { \
        int ok = expression; \
        printf( "%s: %s\n\t%s\n", ok? "PASSED" : "FAILED", #expression, comment ); \
        exit( ok? 0 : 1 ); \
    } while(0)

#endif /* FEMTO_RTOS_TEST_H */
