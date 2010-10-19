#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

void initUsart( struct USARTconfig *conf) {
	cli();
	//USART0 Port E Initialization
	//while(! (conf->USARTmap->STATUS & USART_RXCIF_bm ) );	//wait for USART to be ready to go
	conf->PORTmap->DIR |= PIN7_bm;	//set the pin to output
	conf->PORTmap->OUT |= PIN7_bm;	//set the pin high
	conf->USARTmap->BAUDCTRLA = 34;
	conf->USARTmap->BAUDCTRLB = 0;
	/*
	 *
	 *  Baud rate 57,600
	 *  8 bits
	 *  No Parity
	 *  1 stop bit
	 *  Hardware flow control enabled
	 *
	 */
	conf->USARTmap->CTRLC 	|= 	USART_CMODE_ASYNCHRONOUS_gc
							|	USART_PMODE_DISABLED_gc
							|	USART_CHSIZE_8BIT_gc;

	conf->USARTmap->CTRLB	|= USART_RXEN_bm | USART_TXEN_bm;
	conf->USARTmap->CTRLA	|= USART_RXCINTLVL_MED_gc;
	PMIC_CTRL |= PMIC_MEDLVLEN_bm;
	sei();
}

