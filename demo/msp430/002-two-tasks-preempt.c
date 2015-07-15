/*
 * Two tasks, preemptive switching; it was tested on MSP430F123.
 */

#include <femtoRTOS.h>
#include <signal.h>

static void task_1_func()
{
	for(;;)	{
		P3OUT |= BIT0;
	}
}

static void task_2_func()
{
	for(;;) {
		P3OUT &= ~BIT0;
	}
}

TCB( task_1, 32 );
TCB( task_2, 32 );

TASK_GROUP( tasks ) {
	TASK( 0, task_1, task_1_func ),
	TASK( 0, task_2, task_2_func )
};

TASKS_BEGIN
	TASKS_ITEM( tasks )
TASKS_END;

interrupt (WDT_VECTOR) tickISR() __attribute__ ( ( naked ) );
interrupt (WDT_VECTOR) tickISR()
{
	save_context();
	if( !tick_handler() )
		switch_context();
	restore_context();
}

int main()
{
        /* init ports */
	P1OUT = 0;
	P2OUT = 0;
	P3OUT = BIT2;  /* TPS60211 SNOOZE off */

	P1DIR = 0xFE;  /* all outputs but button 2 */
	P2DIR = 0xE7;  /* all outputs but comparator */
	P3DIR = 0x7F;  /* all outputs but button 1 */

	/* use watchdog timer as interval timer, SMCLK/8192 */
	WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL | WDTIS0;
	IE1 |= WDTIE;

	/* start the scheduler */
	rtos_main();

	/* never get here */
	return 0;
}
