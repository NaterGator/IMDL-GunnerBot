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

#include "FetchGunner.h"
#define SRCALLBACK(name) void name(char *pszFrame)


int main(void) {
	motors.max = 9000;
	motors.min = 800;


	initXmega();
	ADCAInit();
	ServoCInit();
	initTCF0();

	botState.mode = MODE_SEEKING;
	botState.phoneLooking = false;


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




	//Set PORTB for use with motor controllers and bluetooth monitoring
	ADCB_CTRLA = 0x0;
	PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm;
	PORTB_INTCTRL = PORT_INT0LVL_MED_gc;
	PORTB_INT0MASK = PIN5_bm;

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
	smirfSart.this = &smirfSart;
	USARTE = &smirfSart;

	initUsart( smirfSart.this );
	/*
	 * Serialstream/Bluetooth USART initialization complete.
	 */



	/*
	 * Add callbacks for serialstream processing inside the main behavior loop
	 */
	serialstreamAddCallbackPair( blueSmirfStream, "setL", &csetSpeedL );
	serialstreamAddCallbackPair( blueSmirfStream, "setR", &csetSpeedR );
	serialstreamAddCallbackPair( blueSmirfStream, "dirL", &csetDirL );
	serialstreamAddCallbackPair( blueSmirfStream, "dirR", &csetDirR );
	serialstreamAddCallbackPair( blueSmirfStream, "cir", &readCircle );
	serialstreamAddCallbackPair( blueSmirfStream, "tm", &timeMotor );
	serialstreamAddCallbackPair( blueSmirfStream, "move", &move );
	serialstreamAddCallbackPair( blueSmirfStream, "td", &cturnD );


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
			case MODE_SEEKING:
				if(botState.phoneLooking == false) {
					//Instruct phone to look for the ball
					//TODO: Implement this!
					if(botState.bluetoothState == false) {
						clearLCD(LCD);
						sendStringToLCD(LCD, "bluetooth disconnected");
						_delay_ms(3000);
						continue;
					}
					botState.phoneLooking = true;
					writeData(smirfSart.this, "$FSTART$getImg$FEND$ ");
					botState.lookingCount = 0;
					clearLCD(LCD);
					sendStringToLCD(LCD, "Running look mode");
					runTCF0(&idleCamera, TURN_DELAY);
					continue;
				} /*else {
					if(botState.lookingCount == 24) {
						clearLCD(LCD);
						sendStringToLCD(LCD, "Ball not found.");
						botState.mode = MODE_WAITING;
					}
					_delay_ms(5000);
					if(botState.phoneLooking == true){
						turnD(30);
						botState.lookingCount++;
					}

				}*/



				break;
			case MODE_PICKUP:
				if(botState.phoneLooking == false) {
					botState.phoneLooking = true;
					writeData(smirfSart.this, "$FSTART$getImg$FEND$ ");
				}
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
	uint8_t volatile saved_sreg = SREG;
	cli();

	//Disable JTAG so the rest of PORTB can be used.
	CCP = 0xD8;
	MCU_MCUCR = MCU_JTAGD_bm;

	CCP = 0xD8;
	CLK_PSCTRL = 0x00;
	//setup oscilllator
	OSC_CTRL = 0x02;						//enable 32MHz internal clock
	while ((OSC_STATUS & 0x02) == 0);		//wait for oscillator to be ready
	CCP = 0xD8;								//write signature to CCP
	CLK_CTRL = 0x01;

	SREG = saved_sreg;
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


SRCALLBACK(timeMotor) {
	char *pTok = strtok(pszFrame, ",");
	if(pTok == NULL) return;
	int speedl = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int speedr = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int time = atoi(pTok);

	advance(speedl, speedr, time);

}

SRCALLBACK(readCircle) {

	//X is left/right
	//Y is near/far
	pauseTCF0();
	botState.mode = MODE_PICKUP;
	char *pTok = strtok(pszFrame, ",");
	if(pTok == NULL) return;
	int x = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int y = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int angle = atoi(pTok);
	if(y > 320)
		brush(1);
	else
		brush(0);

	clearLCD(LCD);
	sendStringToLCD(LCD, "Circle: {");
	sendIntToLCD(LCD, x);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, y);
	sendCharToLCD(LCD, '}');
	setLCDCursor(LCD, LCD->config.lineLength);
	sendStringToLCD(LCD, "Angle: ");
	sendIntToLCD(LCD, angle);
	if(abs(angle) > 1 ) {
		turnD(angle);

		botState.phoneLooking = false;
		return;
	}

	setDir(1);
	//advance(8000, 8000, 500-y);
	brush(1);
	moveTo(0, y);
	_delay_ms(2500);
	brush(0);
	botState.phoneLooking = false;
	return;

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

void turn(int magnitude, unsigned int duration) {
	unsigned int halfMag = (int)(abs(magnitude)/2.0);
	if(magnitude > 0) {
		//turn right
		setDirR(-1);
		setDirL(1);

	} else {
		//turn left
		setDirR(1);
		setDirL(-1);

	}
	sendStringToLCD(LCD, " M");
	sendIntToLCD(LCD, halfMag);
	setSpeed(halfMag);
	if(duration != 0) {
		_delay_ms(duration);
		setSpeed(0);
	}
}

void advance(int l, int r, unsigned int duration) {
	//setSpeed(magnitude);
	setSpeedL(l);
	setSpeedR(r);
	if(duration != 0){
		_delay_ms(duration);
		setSpeed(0);
	}

}


void brush(unsigned onoff){
	if(onoff == 1)
		PORTB_OUT |= PIN4_bm;
	else
		PORTB_OUT &= ~PIN4_bm;
}

void moveTo( int x, int y){
	const unsigned int pwm = 5000;
	// lead biasing
	// inches per second = 6.5352431 ln( pwm speed ) - 40.6344299
	double speed = 6.185 * log( (double) pwm ) - 41.635;
	clearLCD(LCD);
	sendStringToLCD(LCD, "di: ");
	double distance = 8.5595812*pow(1.0048532,((double) abs(480-y)));
	sendIntToLCD(LCD, (int) distance);
	double duration = 1000*(distance)/speed; //duration in seconds

/*	//turn biasing
	// pwm speed = 516.206014*1.162575^inches offset
	double turnpwm = 0;
	if(x != 0)
		turnpwm = 516.206014 * pow(1.162575,abs(x)/(duration/1000));

	turnpwm /= 2;
	int l = (x < 0?-1:1)*turnpwm + pwm ;
	int r = (x < 0?1:-1)*turnpwm + pwm ;
	//clearLCD(LCD);*/
	sendStringToLCD(LCD, "du: ");
	sendIntToLCD(LCD, (int) duration);
	setLCDCursor(LCD, LCD->config.lineLength);

	advance( pwm+1620, pwm-1620, ((unsigned int) duration));

}

void turnD(int degrees) {
	const unsigned int pwm = 5000;
	// angular velocity calculation
	// degrees per second = 35.710404 ln(x) - 192.689120
	double aVel = 35.710404*log((double) pwm) - 192.689120;
	unsigned int duration = 1100*((double) abs(degrees))/aVel;
	//at the high end of pwm speed angle is off with longer spins (about 20 degs at 360 rotation)

	//fudgefactor is inversely proportional to set speed.
	//unsigned int fudgeFactor = (9000-pwm)/9000;
	unsigned oldDL = getDirL();
	unsigned oldDR = getDirR();

	setDirL(degrees < 0?-1:1);
	setDirR(degrees < 0?1:-1);
	setSpeed(pwm);

	_delay_ms(duration);
	setSpeed(0);

	setDirL(oldDL);
	setDirR(oldDR);
	return;

}


SRCALLBACK(move) {
	char *pTok = strtok(pszFrame, ",");
	if(pTok == NULL) return;
	int x = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int y = atoi(pTok);

	moveTo(x, y);


}

SRCALLBACK(cturnD) {
	int deg = atoi(pszFrame);
	turnD(deg);
}


void sendPendingChar(struct USARTconfig* conf){
	if(conf->pszDataOut != NULL) {
		if(*(conf->pszDataOut)){
			conf->USARTmap->DATA = *(conf->pszDataOut++);
		}else
			conf->pszDataOut = NULL;
	} else
		disableTxInt(conf);

	return;
}





ISR( USARTE1_DRE_vect ) {
//	sendStringToLCD(LCD, "Inside DRE.");
	PORTQ.OUT = 1 & (PORTQ.OUT ^ 1);
	sendPendingChar(USARTE);
}

ISR( PORTB_INT0_vect ){
	botState.bluetoothState = PORTB_IN & PIN5_bm;
	clearLCD(LCD);
	sendStringToLCD(LCD, "bluetoothState change: ");
	sendIntToLCD(LCD, botState.bluetoothState);
	return;
}
