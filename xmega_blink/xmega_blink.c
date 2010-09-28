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

struct LCDinfo* LCD;

char data='A';

void ADCAInit(void);
void initXmega( void );
int ADCA0(void);

int main(void) {
	struct LCDinfo avrLCD = {0};
	struct LCDconfig twoLines = {0};
	struct USARTconfig smirfSart = {0};
	unsigned int dir = 0;

	avrLCD.this = &avrLCD;
	LCD = &avrLCD;
	smirfSart.this = &smirfSart;

	initXmega();
	ADCAInit();
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
//	initUsart( smirfSart.this );

	sendStringToLCD(avrLCD.this, "CdS ADC Value: ");
	int i=0;
	while(1){
		_delay_ms(10);

		//Move cursor back to start of 2nd line
		setLCDCursor(avrLCD.this, twoLines.lineLength);
		sendIntToLCD(avrLCD.this, ADCA0());
		sendStringToLCD(avrLCD.this, "   "); //clears any leftover digits (no zero padding)
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
	//deal with rec.
	//sendStringToLCD(LCD, "got something");
	//sendCharToLCD(LCD, USARTE1_DATA);
	data = USARTE1.DATA;
	if( data == 0x8 ) backspace(LCD->this);
	else sendCharToLCD(LCD->this, data);
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


int ADCA0(void)
{
	ADCA_CH0_MUXCTRL = 0x00;		//Set to Pin 0
	ADCA_CTRLA |= 0x04;				//Start Conversion on ADCA Channel 0
	while ((ADCA_CH0_INTFLAGS & 0x01) != 0x01);	//wait for conversion to complete
	_delay_ms(5);
	int value = ADCA_CH0_RES;		//grab result
	return value;					//return result
}
