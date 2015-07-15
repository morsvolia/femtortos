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

void yield () __attribute__ ( ( naked ) );
void yield ()
{
    disable_interrupts();
    asm volatile ( "push r2" );  /* the stack should be like after entering ISR */
    save_context();
    switch_context();
    restore_context();
}

void* init_stack( char* stack, int stack_size, void (*task_function)() )
{
    void **p = (void**) (stack + stack_size);

    *--p = task_function;
    *--p = (void*) GIE;
    *--p = (void*) 0x4444;  /* r4 */
    *--p = (void*) 0x5555;  /* r5 */
    *--p = (void*) 0x6666;  /* r6 */
    *--p = (void*) 0x7777;  /* r7 */
    *--p = (void*) 0x8888;  /* r8 */
    *--p = (void*) 0x9999;  /* r9 */
    *--p = (void*) 0xAAAA;  /* r10 */
    *--p = (void*) 0xBBBB;  /* r11 */
    *--p = (void*) 0xCCCC;  /* r12 */
    *--p = (void*) 0xDDDD;  /* r13 */
    *--p = (void*) 0xEEEE;  /* r14 */
    *--p = (void*) 0xFFFF;  /* r15 */
    return p;
}
