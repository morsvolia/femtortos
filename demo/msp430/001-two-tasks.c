/*
 * First simple demo; it was tested on MSP430F123.
 */

#include <femtoRTOS.h>

static void task_1_func()
{
	for(;;)	{
		P1OUT |= BIT0;
		yield();
		P1OUT &= ~BIT0;
		yield();
	}
}

static void task_2_func()
{
	for(;;) {
		P1OUT |= BIT6;
		yield();
		P1OUT &= ~BIT6;
		yield();
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

int main()
{
        /* init ports */
	P1DIR = 0xFF;
	P1OUT = BIT0 + BIT6;
	P2OUT = 0xFF;
	P2OUT = 0;
//	P3OUT = BIT2;  /* TPS60211 SNOOZE off */

//	P1DIR = 0xFE;  /* all outputs but button 2 */
//	P2DIR = 0xE7;  /* all outputs but comparator */
//	P3DIR = 0x7F;  /* all outputs but button 1 */

	/* start the scheduler */
	rtos_main();

	/* never get here */
	return 0;
}
