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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <femtoRTOS.h>

/*****************************************************************************
 * enable_interrupts
 */
void enable_interrupts()
{
}

/*****************************************************************************
 * disable_interrupts
 */
void disable_interrupts()
{
}

/*****************************************************************************
 * enter_critical_section
 */
void enter_critical_section()
{
    critical_section++;
}

/*****************************************************************************
 * get_tos_placeholder
 *
 * Helper function for save_context() and restore_context().
 * Return address of memory where top of stack is saved or should be saved.
 */
static void* get_tos_placeholder()
{
    return &current_task->tcb->saved_top_of_stack;
}

/*****************************************************************************
 * idle_handler
 *
 * Helper function for restore_context().
 * Run tick_handler().
 */
static void idle_handler()
{
    TRACE( "enter\n" );
    for(;;)
    {
        if( tick_handler() )
        {
            TRACE( "restoring context of %p\n", current_task );
            restore_context();
            ASSERT( FALSE, "never get here\n" );
        }
        TRACE( "\n" );
    }
}

/*****************************************************************************
 * restore_context
 */
#ifdef __i386__
void asm_dummy_restore_context()
{
    asm volatile (
        ".globl restore_context\n"
    "restore_context:\n\t"
        "popl %%eax\n\t"
        "cmpl $0, current_task\n\t"
        "jnz do_restore\n\t"
        "jmp idle_handler\n\t"
    "do_restore:\n\t"
        "call get_tos_placeholder\n\t"
        "movl (%%eax), %%esp\n\t"
        "popl %%ebp\n\t"
        "popl %%edi\n\t"
        "popl %%esi\n\t"
        "popl %%edx\n\t"
        "popl %%ecx\n\t"
        "popl %%ebx\n\t"
        "popl %%eax\n\t"
        "ret"
        : :
    );

    /* make compiler happy */
    idle_handler();
    get_tos_placeholder();
}
#else
#    error Unsupported architecture
#endif

/*****************************************************************************
 * yield
 */
#ifdef __i386__
void asm_dummy_yield()
{
    asm volatile (
        ".globl yield\n"
    "yield:\n\t"
        "cmpl $0, current_task\n\t"
        "jz save_context_done\n\t"
        "pushl %%eax\n\t"
        "pushl %%ebx\n\t"
        "pushl %%ecx\n\t"
        "pushl %%edx\n\t"
        "pushl %%esi\n\t"
        "pushl %%edi\n\t"
        "pushl %%ebp\n\t"
        "call get_tos_placeholder\n\t"
        "movl %%esp, (%%eax)\n\t"
    "save_context_done:"
        : :
    );
    switch_context();
    restore_context();
}
#else
#    error Unsupported architecture
#endif

/*****************************************************************************
 * init_stack
 */
void* init_stack( char* stack, int stack_size, void (*task_function)() )
{
    void **p = (void*)(stack + stack_size);

#   ifdef __i386__
    *--p = task_function;  /* eip */
    *--p = (void*) 6;  /* eax */
    *--p = (void*) 5;  /* ebx */
    *--p = (void*) 4;  /* ecx */
    *--p = (void*) 3;  /* edx */
    *--p = (void*) 2;  /* esi */
    *--p = (void*) 1;  /* edi */
    *--p = (void*) 0;  /* ebp */
#   else
#       error Unsupported architecture
#   endif
    return p;
}

/*****************************************************************************
 * rtos_trace
 *
 * Write message to stderr.
 */
void rtos_trace( const char* file, int line, const char* func, const char* fmt, ... )
{
    int file_len, func_len, fmt_len;
    char *format_str;
    va_list ap;

    file_len = strlen( file );
    func_len = strlen( func );
    fmt_len = strlen( fmt );
    format_str = malloc( file_len + func_len + fmt_len + 64 );
    sprintf( format_str, "%s:%d:%s: %s", file, line, func, fmt );
    va_start( ap, fmt );
    vfprintf( stderr, format_str, ap );
    va_end( ap );
    free( format_str );
}

/*****************************************************************************
 * rtos_assert
 *
 * Write message to stderr and exit.
 */
void rtos_assert( const char* file, int line, const char* func,
                  const char* expression_str, const char* fmt, ... )
{
    int file_len, func_len, expr_len, fmt_len;
    char *format_str;
    va_list ap;

    file_len = strlen( file );
    func_len = strlen( func );
    expr_len = strlen( expression_str );
    fmt_len = strlen( fmt );
    format_str = malloc( file_len + func_len + expr_len + fmt_len + 64 );
    sprintf( format_str, "%s:%d:%s: %s; %s", file, line, func, expression_str, fmt );
    va_start( ap, fmt );
    vfprintf( stderr, format_str, ap );
    va_end( ap );
    free( format_str );
    exit(1);
}
