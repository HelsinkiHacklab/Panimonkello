/*
 * Panimonkello.cpp
 *
 * Created: 16.7.2015 23:39:23
 *  Author: martti
 */ 


#include <avr/io.h>

#include <avr/interrupt.h>

#define F_CLK 16000000L
#define TIM1_COMPVAL_H 0x3e	// 16000
#define TIM1_COMPVAL_L 0x80
#define PULSE_MS 200
#define INTERVAL_MS 1000

#define BOUNCE_CNT 100

uint16_t ms_count, min_count;
uint16_t pulse_count, interval_count;
bool reset;

uint8_t btn, prev_btn;
uint8_t btnstate, prev_btnstate;
uint16_t debouncer;

uint8_t db;
uint32_t idlecounter;
uint8_t a;



int main(void) {
	
	PORTA = 0x00;
	DDRA = 0x81;
	
	// system clock
	CLKPR = 1 << CLKPCE;		// enable prescaler;
	CLKPR = 0x00;				// prescaler = 1;

	// Timer 1
	TCCR1A = 0x00;
	TCCR1B = 0x09;				// Clear on OCR1A match; prescaler = 1
	OCR1AH = TIM1_COMPVAL_H;	// set TIMER1 compare reg A to get 1 ms timer interrupts on 16 MHz clock
	OCR1AL = TIM1_COMPVAL_L;
	TIMSK1 = 1 << OCIE1A;		// enable compare match A interrupts
	
	ms_count = 0;
	min_count = 0;
	pulse_count = 0;
	interval_count = 0;
	reset = false;
	debouncer = 0;
	
	idlecounter = 0;
	
//	sei();
	__asm__ __volatile__ ("sei" ::: "memory");
	for ( ; ; ) {
		idlecounter++;
		if ( idlecounter > 1000 ) {
			a++;
		}
	}
}

// Timer 1 compare match A interrupt handler
ISR(TIM1_COMPA_vect) {
	// debug pulse PA0; change state at every iteration
	db = !db & 0x01;
	PORTA = ( PINA & 0xfe ) | db;
	
	// timing for the minutes counter
	ms_count++;
	if ( ms_count == 60000 ) {
		ms_count = 0;
		min_count++;
	}

	// timing for solenoid pulse inititation interval (enforce minimum interval)
	if ( interval_count ) {
		interval_count--;
	}
	else if ( min_count ) {
		min_count--;
		pulse_count = PULSE_MS;
		PORTA = PINA | 0x80;	// set solenoid output for another clock tick
	}
	
	// timing for the clock solenoid on-pulse
	if ( pulse_count ) {
		pulse_count--;
		if ( pulse_count == 1 ) reset = true;
	}
	else if ( reset ) {
		PORTA = PINA & 0x7f;	// reset solenoid output when pulse has expired
		interval_count = INTERVAL_MS;
		reset = false;
	}

	// timing for debouncing the adjustment button
	btn = ( PINA & 0x08 ) >> 3;
	if ( btn == prev_btn ) {
		if ( debouncer < BOUNCE_CNT ) debouncer++;
	}
	else debouncer = 0;
	prev_btn = btn;
	if ( debouncer == BOUNCE_CNT ) {
		prev_btnstate = btnstate;
		btnstate = btn;
	}
	
	// manual adjustment
	if ( ( btnstate == 0 ) && ( prev_btnstate == 1 ) ) {
		ms_count = 0;
		min_count++;
	}
}