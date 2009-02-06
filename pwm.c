#include <avr/io.h>
#include "pwm.h"

void pwm_init(char register_value) {
	TIMSK = (1<<OCIE0);
    OCR0 = register_value;
}

void pwm_start() {
    TCCR0 = (1<<WGM01) | /* Set waveform mode to CTC */
			(1<<COM00) |  /* Toggle OC0 pin on compare match */
			(1<<CS01); /* clk/8 prescaler */
}

void pwm_stop() {
    TCCR0 = 0x00; /* Set prescaler to zero; stop clock */
}