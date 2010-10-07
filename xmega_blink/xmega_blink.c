/*
 * xmega_test1.c
 *
 *  Created on: Sep 26, 2010
 *      Author: nate
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_x.h>
#include <lcdiface.h>
#include <usart.h>
#include <serialstream.h>
#include <string.h>
#include <stdlib.h>
#include "xmega_blink.h"

struct LCDinfo* LCD;
struct serialstream_struct *blueSmirfStream;

char data='A';



int main(void) {
	struct LCDinfo avrLCD = {0};
	struct LCDconfig twoLines = {0};
	struct USARTconfig smirfSart = {0};
	struct serialstream_struct serialData = {0};
	struct ss_framing frameSeq = {"$FB$", "$FE$", "|"};
	NEW_SMOOTH(ADCAsmooth, 20);

	unsigned int dir = 0;

	initXmega();
	ADCAInit();

	avrLCD.this = &avrLCD;
	LCD = &avrLCD;
	smirfSart.this = &smirfSart;
	serialData.framePairs = frameSeq;
	blueSmirfStream = &serialData;

	twoLines.cursorInc = 1;
	twoLines.displayEnable = 1;
	twoLines.cursorBlink = 0;
	twoLines.lineLength = 16;
	twoLines.twoLines = 1;

	//Apply the configuration profile
	avrLCD.config = twoLines;
	avrLCD.pLCDDataBus = (volatile int *) &PORTK_OUT;

	PORTK.DIRSET = 0xFF;
	PORTQ.DIRSET = 1;
	initLCD( avrLCD.this, 1 );

	smirfSart.USARTmap = &USARTE1;
	smirfSart.PORTmap = &PORTE;
	initUsart( smirfSart.this );

//	sendStringToLCD(avrLCD.this, "CdS ADC Value: ");
	while(1){
		_delay_ms(10);

		//Move cursor back to start of 2nd line
	//	setLCDCursor(avrLCD.this, twoLines.lineLength);

		//sendIntToLCD(avrLCD.this, adcSmooth(&ADCAsmooth, ADCA0()));
		//sendStringToLCD(avrLCD.this, "   "); //clears any leftover digits (no zero padding)
		if(dir == 1) PORTQ.OUTSET = 1;
		else	PORTQ.OUTCLR = 1;
		dir = !dir;
	}
	return 0;
}

void initXmega( void ) {
	CCP = 0xD8;
	CLK_PSCTRL = 0x00;
	//setup oscilllator
	OSC_CTRL = 0x02;						//enable 32MHz internal clock
	while ((OSC_STATUS & 0x02) == 0);		//wait for oscillator to be ready
	CCP = 0xD8;								//write signature to CCP
	CLK_CTRL = 0x01;
}

ISR( USARTE1_RXC_vect ){
	blueSmirfStream->cIn = USARTE1_DATA;
	int retCode = serialStreamProcessChar( blueSmirfStream );

	if( retCode == 1 ) {
		clearLCD(LCD);
		sendStringToLCD(LCD, blueSmirfStream->pszFrame);
	}


}



void ADCAInit(void)
{

	ADCA_CTRLB = 0x00;				//12bit, right adjusted
	ADCA_REFCTRL = 0x10;			//set to Vref = Vcc/1.6 = 2.0V (approx)
	ADCA_CH0_CTRL = 0x01;			//set to single-ended
	ADCA_CH0_INTCTRL = 0x00;		//set flag at conversion complete.  Disable interrupt
	ADCA_CH0_MUXCTRL = 0x08;		//set to Channel 1
	ADCA_CTRLA |= 0x01;				//Enable ADCA
}


unsigned int ADCA0(void)
{
	ADCA_CH0_MUXCTRL = 0x00;		//Set to Pin 0
	ADCA_CTRLA |= 0x04;				//Start Conversion on ADCA Channel 0
	while ((ADCA_CH0_INTFLAGS & 0x01) != 0x01);	//wait for conversion to complete
	_delay_ms(5);
	unsigned int value = ADCA_CH0_RES;		//grab result
	return value;					//return result
}

unsigned int adcSmooth(struct adcSmooth_struct *smoother, unsigned int iNewVal) {
	if(smoother->index == smoother->depth) smoother->index = 0;
	int tempVal = smoother->prevVals[smoother->index];
	int tmpAvg = smoother->avg - tempVal / smoother->depth;
	tmpAvg += iNewVal / smoother->depth;
	smoother->avg = tmpAvg;
	smoother->prevVals[smoother->index++] = iNewVal;
	return smoother->avg;
}
