#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

/* 
	Function to initialise uart given ubrrh and ubrrl. These are needed in order to set the baud
	rate correctly
*/
void uart_init(char ubrrh, char ubrrl)
{
	UCSRB = (1 << RXCIE) |  /* Enable interrupt on receive */
			(1 << RXEN) |  /* Enable receiver */
			(1 << TXEN); /* Enable transmitter */
	UBRRH = ubrrh & 0x0F; /* This is because we must set the higher 4 bits to zero (p164 datasheet) */
	UBRRL = ubrrl; 
}

/* 
	Function to transmit one byte over uart.
*/
void uart_tx(char byte)
{
	UDR = byte; /* Put the byte in the register so the avr can do its thing */
	while (!((UCSRA & 0x20) >> 5)); /* Wait until the byte is sent */
}

/* 
	Function to transmit a string. NOTE: the string *must* be null terminated or bad things 
	will happen.
*/
void uart_tx_string(char *string)
{
	char next_byte = string[0]; /* Get the first byte to send */
	int string_index = 0;

	while (next_byte != '\0') { /* Keep going until the null termination */
		uart_tx( next_byte );
		
		string_index++;
		next_byte = string[string_index];
	}
}
				
