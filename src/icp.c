// derived from AVR App Note 135

#include <avr/io.h>
#include <avr/interrupt.h>
#include "icp.h"

/*
 * Definitions for the ICP pin; for this example we use timer 1
 */
#define	ICP_PIN		PIND			/* ICP1 GPIO value	*/
#define	ICP_PORT	PORTD			/* ICP1 GPIO port	*/
#define	ICP_DDR		DDRD			/* ICP1 GPIO DDR	*/
#define	ICP_BIT		PD4				/* ICP1 GPIO pin	*/

/*
 * Definitions for ICP timer (1) setup.
 */
#define	ICP_OCR		OCR1A			/* ICP1 Output Compare register		*/
#define	ICP_OC_IE	OCIE1A			/* ICP1 timer Output Compare enable */
#define	ICP_OC_IF	OCF1A			/* ICP1 timer Output Compare flag	*/
#define	ICP_IE		ICIE1			/* ICP1 interrupt enable			*/
#define	ICP_IF		ICF1			/* ICP1 interrupt flag				*/
#define	ICP_CTL_A	TCCR1A			/* ICP1 timer control				*/
#define	ICP_CTL		TCCR1B			/* ICP1 interrupt control			*/
#define	ICP_SENSE	ICES1			/* ICP1 interrupt sense (rising/falling) */
#define	ICP_PRESCALE ((0 << CS12) | (1 << CS11) | (1 << CS10))	/* prescale /64  => gives 8us/tick */

#define	ICP_START_SENSE	(0 << ICP_SENSE)	/* start with rising edge	*/

/**
 * icp_start_time, icp_stop_time, icp_period
 *
 * State variables for the Demodulator.
 *
 * start_time, stop_time and period all have the same type as the
 * respective TCNT register.
 */
typedef unsigned int icp_timer_t;			/* same as TCNT1		*/

icp_timer_t icp_start_time, icp_stop_time;
const icp_timer_t icp_period = 18750; // 8us ticks in 150ms (maximum pulse width is hypothetically only 90ms

/**
 * icp_rx_q[]
 *
 * Stores demodulated samples.
 *
 * The rx_q elements have the same type as the duty-cycle result.
 */
icp_sample_t icp_rx_q[ICP_RX_QSIZE];

/**
 * icp_rx_tail, icp_rx_head
 *
 * Queue state variables for icp_rx_q.
 *
 * The rx_head and rx_tail indices need to be wide enough to
 * accomodate [0:ICP_RX_QSIZE). Since QSIZE should generally
 * not be very large, these are hard-coded as single bytes,
 * which gets around certain atomicity concerns.
 */
unsigned char icp_rx_tail;		/* icp_rx_q insertion index */
unsigned char icp_rx_head;		/* icp_rx_q retrieval index */


/**
 * icp_enq()
 *
 * Stores a new sample into the Rx queue.
 */
__inline__ static void icp_enq(icp_sample_t sample)
{
	unsigned char t;

	t = icp_rx_tail;

	icp_rx_q[t] = sample;
	if (++t >= ICP_RX_QSIZE)
		t = 0;

	if (t != icp_rx_head)		/* digital: Check for Rx overrun */
		icp_rx_tail = t;
	return;
}

/**
 * TIMER1_COMPA()
 *
 * ICP timer Output Compare ISR.
 *
 * The OC interrupt indicates that some edge didn't arrive as
 * expected. This happens when the duty cycle is either 0% (no
 * pulse at all) or 100% (pulse encompasses the entire period).
 */
ISR(TIMER1_COMPA_vect, ISR_NOBLOCK){
	icp_sample_t sample;

	ICP_OCR += icp_period;		/* slide timeout window forward */
	sample = 0;					/* assume 0%	*/
	if ((ICP_CTL & (1 << ICP_SENSE)) != ICP_START_SENSE)
		sample = icp_period;	/* 100%	*/

	icp_enq(sample);

	return;
}

/**
 * TIMER1_CAPT()
 *
 * ICP capture interrupt.
 */
ISR(TIMER1_CAPT_vect, ISR_NOBLOCK)
{
	icp_timer_t icr, delta;
	unsigned char tccr1b;

	/*
	 * Capture the ICR and then reverse the sense of the capture.
	 * These must be done in this order, since as soon as the
	 * sense is reversed it is possible for ICR to be updated again.
	 */
	icr = ICR1;							/* capture timestamp	*/

	do
	{
		tccr1b = ICP_CTL;
		ICP_CTL = tccr1b ^ (1 << ICP_SENSE);		/* reverse sense		*/
	
		/*
		 * If we were waiting for a start edge, then this is the
		 * end/beginning of a period.
		 */
		if ((tccr1b & (1 << ICP_SENSE)) == ICP_START_SENSE)
		{
			/*
			 * Beginning of pulse: Compute length of preceding period,
			 * and thence the duty cycle of the preceding pulse.
			 */
			delta = icp_stop_time - icp_start_time; /* Length of previous pulse */
			icp_start_time = icr;				/* Start of new pulse/period */

			/*
			 * Update the timeout based on the new period. (The new period
			 * is probably the same as the old, give or take clock drift.)
			 * We add 1 to make fairly sure that, in case of competition,
			 * the PWM edge takes precedence over the timeout.
			 */
			ICP_OCR = icr + icp_period + 1;		/* Move timeout window		*/
			TIFR1 = (1 << ICP_OC_IF);			/* Clear in case of race	*/

			/*
			 * store the new reading.
			 */
			icp_enq(delta);
	
			/*
			 * Check for a race condition where a (very) short pulse
			 * ended before we could reverse the sense above.
			 * If the ICP pin is still high (as expected) OR the IF is
			 * set (the falling edge has happened, but we caught it),
			 * then we won the race, so we're done for now.
			 */
			if ((ICP_PIN & (1 << ICP_BIT)) || (TIFR1 & (1 << ICP_IF)))
				break;
		}
		else
		{
			/*
			 * Falling edge detected, so this is the end of the pulse.
			 * The time is simply recorded here; the final computation
			 * will take place at the beginning of the next pulse.
			 */
			icp_stop_time = icr;		/* Capture falling-edge time */

			/*
			 * If the ICP pin is still low (as expected) OR the IF is
			 * set (the transition was caught anyway) we won the race,
			 * so we're done for now.
			 */
			if ((!(ICP_PIN & (1 << ICP_BIT))) || (TIFR1 & (1 << ICP_IF)))
				break;
		}
		/*
		 * If we got here, we lost the race with a very short/long pulse.
		 * We now loop, pretending (as it were) that we caught the transition.
		 * The Same ICR value is used, so the effect is that we declare
		 * the duty cycle to be 0% or 100% as appropriate.
		 */
	} while (1);

	return;
}

/**
 * icp_rx()
 *
 * Fetch a sample from the queue. For analog mode, this is a moving
 * average of the last QSIZE readings. For digital, it is the oldest
 * reading.
 */
icp_sample_t icp_rx(void)
{
	icp_sample_t r;
	unsigned char h;

	h = icp_rx_head;
	if (h == icp_rx_tail)				/* if head == tail, queue is empty */
		r = (icp_sample_t)-1;
	else
	{
		r = icp_rx_q[h];				/* fetch next entry				*/
		if (++h >= ICP_RX_QSIZE)		/* increment head, modulo QSIZE	*/
			h = 0;
		icp_rx_head = h;
	}

	return(r);
}

/**
 * icp_init()
 *
 * Set up the ICP timer.
 */
void icp_init(void)
{
	/*
	 * Nothing interesting to set in TCCR1A
	 */
	ICP_CTL_A = 0;

	/*
	 * Setting the OCR (timeout) to 0 allows the full TCNT range for
	 * the initial period.
	 */
	ICP_OCR	= 0;

	/*
	 * Set the interrupt sense and the prescaler
	 */
	ICP_CTL	= ICP_START_SENSE | ICP_PRESCALE;

	/*
	 *	Enable both the Input Capture and the Output Capture interrupts.
	 *	The latter is used for timeout (0% and 100%) checking.
	 */
	TIMSK1	|= (1 << ICP_IE) | (1 << ICP_OC_IE);

	return;
}
