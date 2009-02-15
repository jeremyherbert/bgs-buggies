#include <avr/io.h>
#include "timer.h"

void timer_init() {
    OCR1A = 16000000/1024; /* Output compare at exactly 1 second */
    TIMSK = (1 << OCIE1A); /* Enable compare match interrupt */
}

void timer_start() {
    TCCR1B = (1 << CS12) | (1 << CS10); /* Set prescaler to clk/1024 */
}

void timer_stop() {
    TCCR1B = 0x00; /* Stop timer from running */
}
