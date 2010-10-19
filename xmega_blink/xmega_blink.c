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
	ServoCInit();

	avrLCD.this = &avrLCD;
	LCD = &avrLCD;
	smirfSart.this = &smirfSart;
	serialData.framePairs = frameSeq;

	blueSmirfStream = &serialData;
	//serialstreamAddCallbackPair( blueSmirfStream, "print2", &frameHandlerLower );
	//printCallback.tag="print2";
	//printCallback.dataCallback=&frameHandlerLower;
	//serialstreamAddCallbackPair( blueSmirfStream, &printCallback );

	twoLines.cursorInc = 1;
	twoLines.displayEnable = 1;
	twoLines.cursorBlink = 0;
	twoLines.lineLength = 16;
	twoLines.twoLines = 1;

	//Apply the configuration profile
	avrLCD.config = twoLines;
	avrLCD.pLCDDataBus = (volatile int *) &PORTK_OUT;

	PORTK.DIRSET = 0xFF;
	PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
	PORTQ.DIRSET = 1;
	initLCD( avrLCD.this, 1 );
	//sendIntToLCD(avrLCD.this, serialstreamAddCallbackPair( blueSmirfStream, &printCallback ));
	serialstreamAddCallbackPair( blueSmirfStream, "setL", &setSpeedL );
	serialstreamAddCallbackPair( blueSmirfStream, "setR", &setSpeedR );
	serialstreamAddCallbackPair( blueSmirfStream, "dirL", &setDirL );
	serialstreamAddCallbackPair( blueSmirfStream, "dirR", &setDirR );
	smirfSart.USARTmap = &USARTE1;
	smirfSart.PORTmap = &PORTE;
	initUsart( smirfSart.this );

//	sendStringToLCD(avrLCD.this, "CdS ADC Value: ");
	while(1){
		runSerialStreamCallbackQueue(&serialData);

		//Move cursor back to start of 2nd line
	//	setLCDCursor(avrLCD.this, twoLines.lineLength);

		//sendIntToLCD(avrLCD.this, adcSmooth(&ADCAsmooth, ADCA0()));
		//sendStringToLCD(avrLCD.this, "   "); //clears any leftover digits (no zero padding)
		//if(dir == 1) PORTQ.OUTSET = 1;
		//else	PORTQ.OUTCLR = 1;
		//dir = !dir;
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
	//Check for buffer overflow, toggle debug LED if it happens. Shame on you!
	if( (USARTE1.STATUS & USART_BUFOVF_bm) == 8) PORTQ.OUT = 1 & (PORTQ.OUT ^ 1);
	blueSmirfStream->cIn = USARTE1_DATA;
	serialStreamProcessChar( blueSmirfStream );

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

void setSpeedL( char *pszFrame ) {
	unsigned int iVal = atoi(pszFrame);
	clearLCD(LCD);
	sendStringToLCD(LCD, "Set L to: ");
	sendIntToLCD(LCD, iVal);
	TCC0.CCA = iVal;
}

void setSpeedR( char *pszFrame ) {
	//setLCDCursor(LCD, LCD->config.lineLength);
	//sendStringToLCD(LCD, pszFrame);
	unsigned int iVal = atoi(pszFrame);
	clearLCD(LCD);
	sendStringToLCD(LCD, "Set R to: ");
	sendIntToLCD(LCD, iVal);
	TCC0.CCB = iVal;
}

void setDirL( char *pszFrame ) {
	//setLCDCursor(LCD, LCD->config.lineLength);
	//sendStringToLCD(LCD, pszFrame);
	int prevSpeed = TCC0_CCA;
	if(prevSpeed != 0 ){
		TCC0_CCA = 0;
		_delay_ms(200);
	}

	clearLCD(LCD);
	if(strcmp(pszFrame,"F") == 0) {
		PORTB_OUT = (PORTB_OUT & 0xFC) | PIN0_bm;
		sendStringToLCD(LCD, "Set L to fwd");
	} else if(strcmp(pszFrame,"R") == 0) {
		PORTB_OUT = (PORTB_OUT & 0xFC) | PIN1_bm;
		sendStringToLCD(LCD, "Set L to rev");
	} else {
		sendStringToLCD(LCD, "Could not understand Ldir");
	}
	if(prevSpeed != 0 && TCC0_CCA == 0)
		TCC0_CCA = prevSpeed;
}

void setDirR( char *pszFrame ) {
	//setLCDCursor(LCD, LCD->config.lineLength);
	//sendStringToLCD(LCD, pszFrame);
	int prevSpeed = TCC0_CCB;
	if(prevSpeed != 0 ){
		TCC0_CCB = 0;
		_delay_ms(200);
	}

	clearLCD(LCD);
	if(strcmp(pszFrame,"F") == 0) {
		PORTB_OUT = (PORTB_OUT & 0xF3) | PIN2_bm;
		sendStringToLCD(LCD, "Set R to fwd");
	} else if(strcmp(pszFrame,"R") == 0) {
		PORTB_OUT = (PORTB_OUT & 0xF3) | PIN3_bm;
		sendStringToLCD(LCD, "Set R to rev");
	} else {
		sendStringToLCD(LCD, "Could not understand Rdir");
	}
	if(prevSpeed != 0 && TCC0_CCB == 0)
			TCC0_CCB = prevSpeed;
}


void ServoCInit(void)
{
	TCC0_CTRLA = 0x05;				//set TCC0_CLK to CLK/64
	TCC0_CTRLB = 0xF3;				//Enable OC A, B, C, and D.  Set to Single Slope PWM
									//OCnX = 1 from Bottom to CCx and 0 from CCx to Top
	TCC0_PER = 10000;				//20ms / (1/(32MHz/64)) = 10000.  PER = Top
	TCC1_CTRLA = 0x05;				//set TCC1_CLK to CLK/64
	TCC1_CTRLB = 0x33;				//Enable OC A and B.  Set to Single Slope PWM
									//OCnX = 1 from Bottom to CCx and 0 from CCx to Top
	TCC1_PER = 10000;				//20ms / (1/(32MHz/64)) = 10000.  PER = Top
	PORTC_DIR = 0x3F;				//set PORTC5:0 to output
	TCC0_CCA = 0;					//PWMC0 off
	TCC0_CCB = 0;					//PWMC1 off
	TCC0_CCC = 0;					//PWMC2 off
	TCC0_CCD = 0;					//PWMC3 off
	TCC1_CCA = 0;					//PWMC4 off
	TCC1_CCB = 0;					//PWMC5 off
}
