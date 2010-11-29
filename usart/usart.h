/*
 * usart.h
 *
 *  Created on: Sep 28, 2010
 *      Author: nate
 */

#ifndef USART_H_
#define USART_H_
#include <avr/io.h>

struct USARTconfig {
	USART_t* 	USARTmap;
	PORT_t*		PORTmap;
	char *pszDataOut;
	struct USARTconfig* this;
};

extern void initUsart(struct USARTconfig *conf);
extern void enableTxInt( struct USARTconfig *conf);
extern void disableTxInt( struct USARTconfig *conf);
extern void writeData( struct USARTconfig *conf,  char *dataIn);


#endif /* USART_H_ */
