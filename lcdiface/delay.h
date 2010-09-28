/*
 * delay.h
 *
 *  Created on: Sep 27, 2010
 *      Author: nate
 */

#ifndef DELAY_H_
#define DELAY_H_
#endif /* DELAY_H_ */

volatile int delaycnt;

void delayInit(void);

void delay_ms(int cnt);

void delay_us(int cnt);
