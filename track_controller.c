#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define F_CPU 16000000
#include <util/delay.h>

#include "lcd.h"
#include "uart.h"
#include "pwm.h"
#include "timer.h"
#include "states.h"

#define BUTTON_1    0x80
#define BUTTON_2    0x40
#define BUTTON_3    0x20
#define BUTTON_4    0x10

const char track_order[10] = {0,1,2,3,4,5,6,7,8,9};

volatile char counter = 0;
char current_state = STATE_INIT;
volatile char sensor_hit = 0;

volatile char seconds = 0;
volatile char minutes = 0;
char min_sec_message[12] = "00:00      "; // Needs the 12th for \0 terminator 
char time_output[20]; // to send the time down uart 

// these will be calculated later 
int timer_value = 0;
char miliseconds = 0;

int main(void)
{
    // DDR setup 
    DDRA = 0xFF; // Sensor outputs
    DDRB = 0xFB; // PWM output, and a couple of sensors. Interrupt pin is input
	DDRC = 0xFF; // LCD outputs 
	DDRD = 0x02; // The only output is the TX pin, the rest are all inputs for the buttons
	
    lcd_init(LCD_DISP_ON); // set up the LCD on PORTC 
    uart_init(0,103); // Set UART to 9600 (p168 datasheet)
    pwm_init(26); // count up to 26 before firing interrupt 
    timer_init(); // for counting the seconds 

	uart_tx_string("Hello!");
	
	sei(); // Enable interrupts 
	
	while(1){ // Run forever
        current_state = STATE_WAIT_START; 
        lcd_clrscr();
        lcd_puts("Speed Beast MKII!   \nPress Go!");
        while (PIND & BUTTON_1); // While the button is not pressed
        
        // Set up the LCD 
        lcd_clrscr();
        lcd_gotoxy(0,0);
        lcd_puts("Time: ");
        
        // Time to start the track timing 
        pwm_start();
        timer_start();
        current_state = STATE_RUNNING;
        _delay_ms(200);
        int i;
        for (i=0; i<10; i++) {
            // Turn the sensor on
            if ( (track_order[i] == 0) || (track_order[i] == 1) ) {
                PORTB |= (1 << track_order[i]);
            } else {
                PORTA |= (1 << (track_order[i]-2) );
            }
            
            GIFR |= (1 << INTF2); // Clear the external interrupt flags
            GICR |= (1 << INT2); // Enable INT2
			
            while (!sensor_hit){
                lcd_gotoxy(0,1);
                sprintf(min_sec_message, "%i:%i  ", minutes, seconds);
                lcd_puts( min_sec_message );
            } // Wait until the INT2 signal is caught
            GICR &= ~(1 << INT2); // Disable INT2

            sensor_hit = 0;
			uart_tx('h');            

            // Disable the sensor
            if ( (track_order[i] == 0) || (track_order[i] == 1) ) {
                PORTB &= ~(1 << track_order[i]);
            } else {
                PORTA &= ~(1 << (track_order[i]-2) );
            }
        }
        /* TODO: Logic */

		// We need to disable the interrupts before a 16bit register read or bad things can happen 
		cli(); 
		timer_value = TCNT1;
		sei();
        
		sprintf(time_output, "Time: %i:%i:%i\n", minutes, seconds, timer_value);
		uart_tx_string(time_output);

        // Empty all the variables
		timer_value = 0;
        seconds = 0;
        minutes = 0;
		sprintf(time_output, " ");
		        
        timer_stop();
        pwm_stop();
        current_state = STATE_FINISHED;
        
        lcd_gotoxy(0,2);
        lcd_puts("Finished - Press Go!");
        while (PIND & BUTTON_1); // Wait for the Go button
	}
}

ISR(TIMER0_COMP_vect) { // 1 compare is 27us (37kHz) 
    
	counter++;
	if (counter == 23) { // 4320us
		TCCR0 = (1<<WGM01) | // Set waveform mode to CTC
			(1<<COM01) |  // clear OC0 pin on compare match
			(1<<CS01); // clk/8 prescaler
	} else if (counter == 56) { // 6480us; 2160 difference
		TCCR0 = (1<<WGM01) | // Set waveform mode to CTC
			(1<<COM00) |  // toggle OC0 pin on compare match
			(1<<CS01); // clk/8 prescaler
		counter = 0;
	}
}

ISR(TIMER1_COMPA_vect) { // Every second
    seconds++;
    if (seconds == 60) {
        seconds = 0;
        minutes++;
    }
    /* Before clearing the timer we need to disable interrupts
     This is because it is a 16bit write and if it gets blocked then very bad things will happen */
	cli();
	TCNT1 = 0;
    sei();
}

ISR(USART_RXC_vect) { // On UART receive 
    char rx_byte = UDR; // Get the byte out of the register 
    switch (rx_byte){
        
        case 'X': // This is the test case; if the computer sends this it is trying to check if the controller is alive 
            uart_tx('x'); // Send back 'x' to show that it is still alive 
            break; 
    }
}

ISR(INT2_vect) { // On INT2 low
    int i; 
    char low = 0;
    /* This is a digital filter; it vastly decreases the likelyhood that this will trigger on noise by continuously checking the pin
     for the correct value */
    for (i=0; i<200; i++) {
        if (!(PINB & (1<<2))) {
            low++;
        }
    }
    if (low > 200*.9) sensor_hit = 1; // Let the other functions know that the sensor has been hit
}
