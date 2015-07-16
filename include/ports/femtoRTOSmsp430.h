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

#ifndef FEMTO_RTOS_MSP430_H
#define FEMTO_RTOS_MSP430_H

#include <msp430.h>

#define enable_interrupts()  asm volatile ( "eint" )
#define disable_interrupts() asm volatile ( "dint" )

#define enter_critical_section() asm volatile ( "inc.b critical_section" )

#define save_context() \
    asm volatile ( \
        "cmp.w #0, current_task \n\t" \
        "jnz 1f \n\t" \
        "add.w #4, r1 \n\t" \
        "jmp 2f \n" \
    "1: \n\t" \
        "push r4 \n\t" \
        "push r5 \n\t" \
        "push r6 \n\t" \
        "push r7 \n\t" \
        "push r8 \n\t" \
        "push r9 \n\t" \
        "push r10 \n\t" \
        "push r11 \n\t" \
        "push r12 \n\t" \
        "push r13 \n\t" \
        "push r14 \n\t" \
        "push r15 \n\t" \
        "mov.w current_task, r4 \n\t" \
        "mov.w @r4, r4 \n\t" \
        "mov.w r1, @r4 \n" \
    "2:" \
    )

#define restore_context() \
    asm volatile ( \
        "mov.w current_task, r4 \n\t" \
        "cmp.w #0, r4 \n\t" \
        "jnz 1f \n\t" \
        "bis.w #0x0018, r2 /* GIE | CPUOFF */ \n" \
    "1: \n\t" \
        "mov.w @r4, r4 \n\t" \
        "mov.w @r4, r1 \n\t" \
        "pop r15 \n\t" \
        "pop r14 \n\t" \
        "pop r13 \n\t" \
        "pop r12 \n\t" \
        "pop r11 \n\t" \
        "pop r10 \n\t" \
        "pop r9 \n\t" \
        "pop r8 \n\t" \
        "pop r7 \n\t" \
        "pop r6 \n\t" \
        "pop r5 \n\t" \
        "pop r4 \n\t" \
        "bic.w #0x0010, @r1 /* CPUOFF */ \n\t" \
        "bis.w #0x0008, @r1 /* GIE */ \n\t" \
        "reti" \
    )

extern void yield();
extern void* init_stack( char* stack, int stack_size, void (*task_function)() );

#endif /* FEMTO_RTOS_MSP430_H */
