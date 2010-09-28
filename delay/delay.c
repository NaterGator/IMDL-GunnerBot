#include <avr/io.h>
#include <avr/interrupt.h>
#include "delay.h"


void delayInit(void)
{
	TCF1_CTRLA = 0x01;						//set clock/1
	TCF1_CTRLB = 0x31;						//enable COMA and COMB, set to FRQ
	TCF1_INTCTRLB = 0x00;					//turn off interrupts for COMA and COMB
	SREG |= CPU_I_bm;						//enable all interrupts
	PMIC_CTRL |= 0x01;						//enable all low priority interrupts
}

void delay_ms(int cnt)
{
	delaycnt = 0;							//set count value
	TCF1_CCA = 32000;						//set COMA to be 1ms delay
	TCF1_CNT = 0;							//reset counter
	TCF1_INTCTRLB = 0x01;					//enable low priority interrupt for delay
	while (cnt != delaycnt);				//delay
	TCF1_INTCTRLB = 0x00;					//disable interrupts
}

void delay_us(int cnt)
{
	delaycnt = 0;							//set counter
	TCF1_CCA = 32;							//set COMA to be 1us delay
	TCF1_CNT = 0;							//reset counter
	TCF1_INTCTRLB = 0x01;					//enable low priority interrupt for delay
	while (cnt != delaycnt);				//delay
	TCF1_INTCTRLB = 0x00;					//disable interrupts
}

SIGNAL(TCF1_CCB_vect)
{
	delaycnt++;
}

SIGNAL(TCF1_CCA_vect)
{
	delaycnt++;
}
