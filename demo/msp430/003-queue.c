#include <io.h>
#include <signal.h>
#include <femtoRTOS.h>

#define SMCLK_HZ  1000000
#define TICK_HZ   (SMCLK_HZ / 8192)

#define BUTTON_1  1
#define BUTTON_2  2

QUEUE( keypress_queue, 1, sizeof(unsigned char) );

/*****************************************************************************
 * timerA0_isr
 */
static int beep_half_period;

interrupt (TIMERA0_VECTOR) timerA0_isr( void )
{
	TACCR0 = TAR + beep_half_period;
	P1OUT ^= BIT3;
}

/*****************************************************************************
 * beep
 */
static void beep( int period )
{
	beep_half_period = period >> 1;
	TAR = 0;
	TACCR0 = beep_half_period;
	TACTL = TASSEL1 | MC1;     /* SMCLK, continuous mode */
	P1OUT |= BIT3;             /* buzzer output */
	TACCTL0 = CCIE;            /* enable interrupt when TAR == TACCR0 */

	/* delay 300 ms */
	sleep( 300 * TICK_HZ / 1000 );

	/* sound off */
	TACTL = 0;
	TACCTL0 = 0;
	P1OUT &= ~BIT3;
}

/*****************************************************************************
 * sample_task
 */
static void sample_task_func()
{
	for(;;)
	{
		unsigned char button_id;

		queue_pop( &keypress_queue, &button_id, INFINITE_TIMEOUT );

		switch( button_id )
		{
		case BUTTON_1:
			beep( SMCLK_HZ / 500 );
			break;

		case BUTTON_2:
			beep( SMCLK_HZ / 800 );
			break;
		}
	}
}

/*****************************************************************************
 * check_keypress
 */
static enum pso_result check_keypress( int pin, unsigned char* state,
				       unsigned char button_id )
{
	switch( *state )
	{
	case 0: /* waiting for keypress */
	case 1: /* pin should be stable across three calls */
	case 2:
		if( pin )
			*state = 0;
		else
			(*state)++;
		break;

	case 3: /* pressed */
		*state = 4;
		return queue_push_from_isr( &keypress_queue, &button_id );

	case 4: /* waiting for release */
	case 5: /* pin should be stable across three calls */
		if( !pin )
			*state = 4;
		else
			(*state)++;
		break;
	default:
		if( !pin )
			*state = 4;
		else
			*state = 0;
		break;
	}
	return PSO_BUSY;
}

/*****************************************************************************
 * tickISR
 *
 * Watchdog interrupt service routine.
 */
interrupt (WDT_VECTOR) tickISR() __attribute__ ( ( naked ) );
interrupt (WDT_VECTOR) tickISR()
{
	static unsigned char button1_state = 0;
	static unsigned char button2_state = 0;

	save_context();

	if( tick_handler() )
		goto done;

	if( check_keypress( P1IN & BIT0, &button1_state, BUTTON_1 ) == RESTORE_CONTEXT_NOW )
		goto done;

	if( check_keypress( P3IN & BIT7, &button2_state, BUTTON_2 ) == RESTORE_CONTEXT_NOW )
		goto done;

	switch_context();
done:
	restore_context();
}

/*****************************************************************************
 * tasks
 */

TCB( sample_task, 32 );

TASK_GROUP( tasks ) {
	TASK( 0, sample_task, sample_task_func )
};

TASKS_BEGIN
	TASKS_ITEM( tasks )
TASKS_END;

/*****************************************************************************
 * main
 */
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
