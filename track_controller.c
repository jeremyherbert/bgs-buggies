#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "uart.h"
#include "pwm.h"
#include "timer.h"
#include "states.h"

#define BUTTON_1    0x80
#define BUTTON_2    0x40
#define BUTTON_3    0x20
#define BUTTON_4    0x10

volatile char counter = 0;
char current_state = STATE_INIT;

volatile char seconds = 0;
volatile char minutes = 0;

/* these will be calculated later */
int timer_value = 0;
char miliseconds = 0;

int main(void)
{
    /* DDR setup */
    DDRA = 0xFF; /* Sensor outputs */
    DDRB = 0xFB; /* PWM output, and a couple of sensors. Interrupt pin is input */
	DDRC = 0xFF; /* LCD outputs */
	DDRD = 0x02; /* The only output is the TX pin, the rest are all inputs for the buttons */
	
    lcd_init(LCD_DISP_ON); /* set up the LCD on PORTC */
    uart_init(0,103); /* Set UART to 9600 (p168 datasheet) */
    pwm_init(26); /* count up to 26 before firing interrupt */
    timer_init(); /* for counting the seconds */
	
	sei(); /* Enable interrupts */
	
	while(1){ /* Run forever */
        current_state = STATE_WAIT_START; 
        lcd_clrscr();
        lcd_puts("BGS-Buggies!\nv0.1\nPress Go!");
        while (PIND & BUTTON_1); /* While the button is not pressed */
        
        /* Time to start the track timing */
        pwm_start();
        timer_start();
        current_state = STATE_RUNNING;
        
        /* TODO: Logic */
        
        timer_stop();
        pwm_stop();
        current_state = STATE_FINISHED;
	}
}

ISR(TIMER0_COMP_vect) { /* 1 compare is 27us (37kHz) */
    
	counter++;
	if (counter == 23) { // 4320us
		TCCR0 = (1<<WGM01) | /* Set waveform mode to CTC */
			(1<<COM01) |  /* clear OC0 pin on compare match */
			(1<<CS01); /* clk/8 prescaler */
	} else if (counter == 56) { // 6480us; 2160 difference
		TCCR0 = (1<<WGM01) | /* Set waveform mode to CTC */
			(1<<COM00) |  /* toggle OC0 pin on compare match */
			(1<<CS01); /* clk/8 prescaler */
		counter = 0;
	}
}

ISR(TIMER1_COMPA_vect) { /* Every second */
    seconds++;
    if (seconds == 60) {
        seconds = 0;
        minutes++;
    }
}

ISR(USART_RXC_vect) { /* On UART receive */
    char rx_byte = UDR; /* Get the byte out of the register */
    switch (rx_byte){
        
        case 'T': /* This is the test case; if the computer sends this it is trying to check if the controller is alive */
            uart_tx('t'); /* Send back 't' to show that it is still alive */
            break; 
    }
}