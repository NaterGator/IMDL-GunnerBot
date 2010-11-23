/*
 * FetchGunner.c
 *
 *  Created on: Oct 19, 2010
 *      Author: nate
 *
 *      Behavior modes:
 *      1) Wait for Gunner to approach
 *      2) Instruct Gunner to drop tennis ball, navigate to and pick up tennis ball
 *      3) Launch tennis ball, return to mode 1
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_x.h>
#include <lcdiface.h>
#include <usart.h>
#include <serialstream.h>
#include <string.h>
#include <stdlib.h>
#include "FetchGunner.h"

int main(void) {

	initXmega();
	ADCAInit();
	ServoCInit();



	struct FGoperator botState = {0};


	/*
	 * LCD Configuration and initialization
	 */
	struct LCDinfo avrLCD = {0};
	struct LCDconfig twoLines = {0};
	avrLCD.this = &avrLCD;
	LCD = &avrLCD;


	twoLines.cursorInc = 1;		//LCD cursor should increment forward
	twoLines.displayEnable = 1; //LCD display should be active
	twoLines.cursorBlink = 0;	//LCD cursor should not blink
	twoLines.lineLength = 16;	//LCD line length is 16 characters
	twoLines.twoLines = 1;		//LCD contains two lines

	//Apply the configuration profile
	avrLCD.config = twoLines;
	//Map the correct port
	avrLCD.pLCDDataBus = (volatile int *) &PORTK_OUT;

	//Set PORTK for output.
	PORTK.DIRSET = 0xFF;
	initLCD( avrLCD.this, 1 );
	/*
	 * LCD Configuration and initialization complete
	 */




	//Set PORTB for use with motor controllers
	PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;

	//PORTQ debug light, used to notify of a USART buffer overrun
	PORTQ.DIRSET = 1;



	/*
	 * USART/Serial Stream Configuration and Initialization
	 */
	struct USARTconfig smirfSart = {0};
	struct serialstream_struct bluetoothData = {0};

	//Set stream framing words to delimit incoming/outgoing packets
	struct ss_framing frameSeq = {"$FB$", "$FE$", "|"};

	//Set introspective reference
	smirfSart.this = &smirfSart;
	bluetoothData.framePairs = frameSeq;
	//Set global reference to serialstream data
	blueSmirfStream = &bluetoothData;

	//Identify PORTS to be used with this serialStream object.
	//BlueSMiRF is connected to PORTE
	smirfSart.USARTmap = &USARTE1;
	smirfSart.PORTmap = &PORTE;
	initUsart( smirfSart.this );
	/*
	 * Serialstream/Bluetooth USART initialization complete.
	 */



	/*
	 * Add callbacks for serialstream processing inside the main behavior loop
	 */
	serialstreamAddCallbackPair( blueSmirfStream, "setL", &setSpeedL );
	serialstreamAddCallbackPair( blueSmirfStream, "setR", &setSpeedR );
	serialstreamAddCallbackPair( blueSmirfStream, "dirL", &setDirL );
	serialstreamAddCallbackPair( blueSmirfStream, "dirR", &setDirR );
	serialstreamAddCallbackPair( blueSmirfStream, "cir", &readCircle );


	NEW_SMOOTH(ADCAsmoother, 20);
	NEW_SMOOTH(SonarASmooth, 15);
	initPWMread();
	while(1){
		//Act on any queued packets from bluetooth
		runSerialStreamCallbackQueue(&bluetoothData);
/*
		if((TCD0_INTFLAGS & TC0_CCAIF_bm) == TC0_CCAIF_bm ) {
			setLCDCursor(LCD, 0);
			sendStringToLCD(LCD, "Val: ");
			sendUIntToLCD(LCD, (TCD0_CCA*2)/58); //58uS per cm.
										//2uS per clock cycle
			sendStringToLCD(LCD, "   ");
		}
		if((TCD0_INTFLAGS & TC0_CCBIF_bm) == TC0_CCBIF_bm ) {
					setLCDCursor(LCD, LCD->config.lineLength);
					sendStringToLCD(LCD, "Val: ");
					sendIntToLCD(LCD, adcSmooth(&SonarASmooth, (unsigned int) (TCD0_CCB*2)/58));//58uS per cm.
														//2uS per clock cycle
														// Valid range is from 20-765 cm
					sendStringToLCD(LCD, "   ");

		}*/

		switch(botState.mode) {
			case MODE_PICKUP:
				if(botState.phoneLooking == false) {
					//Instruct phone to look for the ball
					//TODO: Implement this!
					continue;
				}



				break;
			case MODE_LAUNCHING:
				break;
			case MODE_WAITING:
			default:
				break;

		}

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

void initPWMread(void) {
	PORTD_PIN0CTRL = PORT_ISC_BOTHEDGES_gc;
	PORTD_PIN1CTRL = PORT_ISC_BOTHEDGES_gc;
	PORTD_DIRCLR = PIN0_bm | PIN1_bm;

	EVSYS_CH0MUX = EVSYS_CHMUX_PORTD_PIN0_gc;
	EVSYS_CH1MUX = EVSYS_CHMUX_PORTD_PIN1_gc;

	TCD0_CTRLD = TC_EVACT_PW_gc | TC_EVSEL_CH0_gc;
	TCD0_CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm;
	TCD0_CTRLA = TC_CLKSEL_DIV64_gc;

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

SRCALLBACK(readCircle) {
	char *pTok = strtok(pszFrame, ",");
	if(pTok == NULL) return;
	int x = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int y = atoi(pTok);
	clearLCD(LCD);
	sendStringToLCD(LCD, "Circle at: {");
	sendIntToLCD(LCD, x);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, y);
	sendCharToLCD(LCD, '}');
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
