/*
 * xmega_test1.c
 *
 *  Created on: Sep 26, 2010
 *      Author: nate
 */

#include <avr/io.h>
#include <delay.h>
#include <lcdiface.h>

int main(void) {
	struct LCDinfo avrLCD = {0};
	struct LCDconfig twoLines = {0};
	unsigned int dir = 0;

	avrLCD.this = &avrLCD;

	CCP = 0xD8;
	CLK_PSCTRL = 0x00;
	//setup oscilllator
	OSC_CTRL = 0x02;						//enable 32MHz internal clock
	while ((OSC_STATUS & 0x02) == 0);		//wait for oscillator to be ready
	CCP = 0xD8;								//write signature to CCP
	CLK_CTRL = 0x01;

	twoLines.cursorInc = 1;
	twoLines.displayEnable = 1;
	twoLines.cursorBlink = 1;
	twoLines.lineLength = 16;
	twoLines.twoLines = 1;

	//Apply the configuration profile
	avrLCD.config = twoLines;
	avrLCD.pLCDDataBus = (volatile int *) &PORTK_OUT;
	delayInit();

	PORTK.DIRSET = 0xFF;
	PORTQ.DIRSET = 1;
	initLCD( avrLCD.this, 1 );

	while(1){
		delay_ms(1000);
		sendStringToLCD(avrLCD.this, "This is some content. It will line wrap.");

		if(dir == 1) PORTQ.OUTSET = 1;
		else	PORTQ.OUTCLR = 1;
		dir = !dir;
	}
	return 0;
}
