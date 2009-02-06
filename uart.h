
/* 
	Functions to initialise and use the uart registers.
*/

void uart_init(char ubrrh, char ubrrl);
void uart_tx(char byte);
void uart_tx_string(char *string);
