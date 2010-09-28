/*
 * delay.h
 *
 *  Created on: Sep 27, 2010
 *      Author: nate
 */

#ifndef DELAY_H_
#define DELAY_H_

volatile int delaycnt;

extern void delayInit(void);

extern void delay_ms(int cnt);

extern void delay_us(int cnt);

#endif /* DELAY_H_ */
