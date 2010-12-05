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

	ServoCInit();
	setHolder(ARM_HOLD);
	initTCF0();
	initTCF1();
//	runTCF1(&sensorWatch, 50);


	//PORTQ debug light, used to notify of a USART buffer overrun
	PORTQ_DIRSET |= PIN0_bm | PIN1_bm;
	PORTQ_OUTCLR |= PIN0_bm | PIN1_bm;
	_delay_ms(500);

	PORTQ_OUTSET |= PIN0_bm | PIN1_bm;
	_delay_us(40);
	PORTQ_DIRCLR |= PIN1_bm;
	PORTQ_OUTCLR |= PIN0_bm | PIN1_bm;


	setBotMode(MODE_SEEKING);
	botState.modeNeedInit = true;
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


	ADCAInit();

	//Set PORTB for use with motor controllers and bluetooth monitoring
	ADCB_CTRLA = 0x0;
	PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN6_bm | PIN7_bm;
	PORTB_INTCTRL = PORT_INT0LVL_MED_gc;
	PORTB_INT0MASK = PIN5_bm;



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
	serialstreamAddCallbackPair( blueSmirfStream, "setH", &csetHolder );
	serialstreamAddCallbackPair( blueSmirfStream, "dirL", &csetDirL );
	serialstreamAddCallbackPair( blueSmirfStream, "dirR", &csetDirR );
	serialstreamAddCallbackPair( blueSmirfStream, "cir", &readCircle );
	serialstreamAddCallbackPair( blueSmirfStream, "tm", &timeMotor );
	serialstreamAddCallbackPair( blueSmirfStream, "move", &move );
	serialstreamAddCallbackPair( blueSmirfStream, "td", &cturnD );


	NEW_SMOOTH(ADCAsmoother, 3);
	NEW_SMOOTH(Sonar0Smooth, 3);
	Sonar0 = &Sonar0Smooth;
	NEW_SMOOTH(Sonar1Smooth, 3);
	Sonar1 = &Sonar1Smooth;
	NEW_SMOOTH(Sonar2Smooth, 3);
	Sonar2 = &Sonar2Smooth;
	initPWMread();


	enableSonar();


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
				/*
				 * In MODE_SEEKING the robot is trying to find a tennis ball on the floor.
				 * It will request a circle location from the phone. If a circle is not found
				 * in TURN_DELAY milliseconds, the robot will rotate 30 degrees and rescan.
				 *
				 */

				if( botState.modeNeedInit == true) {
					//Initialize the lookingCount state var.
					//When it reaches 24 (nearly 2 turns) it will go into a sleep mode
					//TODO: Decide what to do after 24 turns.

					if(botState.bluetoothState == false && (PORTB_IN & PIN5_bm) == 0) {
						//Check if a bluetooth connection is active.
						//Wait and continue the loop.
						clearLCD(LCD);
						sendStringToLCD(LCD, "bluetooth disconnected");
						_delay_ms(3000);
						break;
					}

					stopTCF1();
					botState.lookingCount = 0;
					clearLCD(LCD);
					sendStringToLCD(LCD, "mode: seeking");

					//Run TCF0 with the idleCamera function pointer, delaying for TURN_DELAY ms.
					runTCF0(&idleCamera, TURN_DELAY);
					if(botState.phoneLooking == false) {
						//Set the state so we know we've asked the phone for a circle
						botState.phoneLooking = true;
						//Ask the phone for circle coordinates
						writeData(smirfSart.this, "$FSTART$getImg$FEND$ ");
						break;
					}
					botState.modeNeedInit = false;
				}

				/* In this area, we're waiting for the phone to ID a circle.
				 * When it does so, the serialstream callback will exec and
				 * move us on to the next state using interrupts
				 */
				break;
			case MODE_PICKUP:
				/* In this state, we've gotten a circle location from the phone.
				 * This means we're probably seeing a tennis ball, so it's time
				 * to stop turning around skeeing one and try to pick it up.
				 *
				 * We should have tried to align ourselves with the most recent
				 * callback execution (we get here from readCircle)
				 */
				if(botState.modeNeedInit == true) {
					//TODO: Run a timeout timer in case we "lose" the ball
					runTCF1(&lostBall, 15000);
					botState.modeNeedInit = false;
				}


				// We'll tell the phone to keep looking for the ball
				// We may need to line up with it again, or we may have just headed towards it
				// Here we notify the phone on the first time through, or if we processed
				// and incoming circle position and it didn't get into the hopper
				if(botState.phoneLooking == false) {
					botState.phoneLooking = true;
					writeData(smirfSart.this, "$FSTART$getImg$FEND$ ");
				}

				// Nothing much to do here. Wait to pick up the ball

				//botState.mode = MODE_LAUNCHING;

				break;
			case MODE_LAUNCHING:
				stopTCF1();
				clearLCD(LCD);
				sendStringToLCD(LCD, "mode: launching");

				botState.phoneLooking == false;

				if(botState.launcherOn == false) {
					// Turn that big beautiful motor on, let it spin up
					PORTB_OUT |= PIN6_bm;
					botState.launcherOn = true;
					_delay_s(2);
				}

				setHolder(ARM_RELEASE);
				_delay_s(4);
				setHolder(ARM_HOLD);
				// Turn off the motor
				PORTB_OUTCLR |= PIN7_bm | PIN6_bm;
				botState.launcherOn = false;

				setBotMode(MODE_WAITING);

				break;
			case MODE_WAITING:
				clearLCD(LCD);
				sendStringToLCD(LCD, "mode: waiting");
				_delay_s(5);
				//TODO: revert
				setBotMode(MODE_SEEKING);
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

	PMIC_CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;

	SREG = saved_sreg;

	sei();
}

ISR( USARTE1_RXC_vect ){
	//Check for buffer overflow, toggle debug LED if it happens. Shame on you!
	if( (USARTE1.STATUS & USART_BUFOVF_bm) == 8) PORTQ.OUT = 1 & (PORTQ.OUT ^ 1);
	blueSmirfStream->cIn = USARTE1_DATA;
	serialStreamProcessChar( blueSmirfStream );

}



void ADCAInit(void)
{
	ADCA_REFCTRL = 0x10;			//set to Vref = Vcc/1.6 = 2.0V (approx)
	ADCA_CH0_CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;			//set to single-ended
	ADCA.CMP = LIGHT_THRESHOLD;
	ADCA_CH0_INTCTRL = ADC_CH_INTMODE_BELOW_gc | ADC_CH_INTLVL_OFF_gc;		//set flag at conversion complete.  Disable interrupt
	ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN0_gc;
	cli();
	ADCA_CTRLB |= ADC_FREERUN_bm;				//12bit, right adjusted
	ADCA_CTRLA |= 0x01;				//Enable ADCA
	_delay_ms(1);
	ADCA_CH0_INTFLAGS |= ADC_CH_CHIF_bm;
	sei();

}

void initPWMread(void) {
	PORTD_PIN0CTRL = PORT_ISC_BOTHEDGES_gc;
	PORTD_PIN1CTRL = PORT_ISC_BOTHEDGES_gc;
	PORTD_PIN2CTRL = PORT_ISC_BOTHEDGES_gc;
	PORTD_DIRCLR = PIN0_bm | PIN1_bm | PIN2_bm;

	EVSYS_CH0MUX = EVSYS_CHMUX_PORTD_PIN0_gc;
	EVSYS_CH1MUX = EVSYS_CHMUX_PORTD_PIN1_gc;
	EVSYS_CH2MUX = EVSYS_CHMUX_PORTD_PIN2_gc;


	TCD0_CTRLD = TC_EVACT_PW_gc | TC_EVSEL_CH0_gc;
	TCD0_INTCTRLB |= TC_CCAINTLVL_LO_gc | TC_CCBINTLVL_LO_gc | TC_CCCINTLVL_LO_gc;
	TCD0_CTRLB = TC0_CCAEN_bm | TC0_CCBEN_bm | TC0_CCCEN_bm;
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

	if( botState.mode == MODE_LAUNCHING ) {
		botState.phoneLooking = false;
		return;
	}
	//X is left/right
	//Y is near/far


	if(botState.mode == MODE_SEEKING) {
		stopTCF0();
		enableTBDetect();
		setBotMode(MODE_PICKUP);
	}

	//This counter tracks the last time a ball was detected
	//If we don't reset this, our "give up" strategy will kick in
	isrPtrs.TCF1_milliloops = 15000;

	char *pTok = strtok(pszFrame, ",");
	if(pTok == NULL) return;
	int x = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int y = atoi(pTok);
	pTok = strtok(NULL, ",");
	if(pTok == NULL) return;
	int angle = atoi(pTok);


	clearLCD(LCD);
	sendStringToLCD(LCD, "Circle: {");
	sendIntToLCD(LCD, x);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, y);
	sendCharToLCD(LCD, '}');
	setLCDCursor(LCD, LCD->config.lineLength);
	sendStringToLCD(LCD, "Angle: ");
	sendIntToLCD(LCD, angle);
	if(abs(angle) > 2 ) {
		turnD(angle);
		botState.phoneLooking = false;
		return;
	}

	//botState.mode = MODE_LAUNCHING;

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
		botState.movement = MOVE_TURNRIGHT;
	} else {
		//turn left
		setDirR(1);
		setDirL(-1);
		botState.movement = MOVE_TURNLEFT;
	}
	sendStringToLCD(LCD, " M");
	sendIntToLCD(LCD, halfMag);
	//runTCF1(&sensorWatch, SONAR_TIMEOUT);
	setSpeed(halfMag);

	if(duration != 0) {
		//Duration hopefully is never zero, or else we're turning endlessly.
		_delay_ms(duration);
		setSpeed(0);
		//stopTCF1();
		botState.movement = MOVE_STILL;
	}

}

void advance(int l, int r, unsigned int duration) {
	//setSpeed(magnitude);
	//runTCF1(&sensorWatch, SONAR_TIMEOUT);
	setSpeedL(l);
	setSpeedR(r);
	if(duration != 0){
		_delay_ms(duration);
		setSpeed(0);
		//stopTCF1();
		botState.movement = MOVE_STILL;
	}

}



void moveTo( int x, int y){
	const unsigned int pwm = 5000;
	// lead biasing
	// inches per second = 6.5352431 ln( pwm speed ) - 40.6344299
	botState.movement = MOVE_FORWARD;
	double speed = 6.185 * log( (double) pwm ) - 41.635;
	clearLCD(LCD);
	sendStringToLCD(LCD, "di: ");
	double distance = 8.5595812*pow(1.0048532,((double) abs(480-y)));
	distance += INTENTIONAL_OVERSHOOT;
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
	setLCDCursor(LCD, LCD->config.lineLength);
	sendStringToLCD(LCD, "du: ");
	sendIntToLCD(LCD, (int) duration);
	//runTCF1(&sensorWatch, SONAR_TIMEOUT);

	advance( pwm+MOTOR_BIAS, pwm-MOTOR_BIAS, ((unsigned int) duration));
	//stopTCF1();
	botState.movement = MOVE_STILL;

}

void turnD(int degrees) {
	degrees = (int) (((double)degrees) * ((double)degrees)/(((double)degrees) - 1.0));
	const unsigned int pwm = 6000;
	// angular velocity calculation on hard floor
	// degrees per second = 35.710404 ln(x) - 192.689120
	double aVel = 35.710404*log((double) pwm) - 198.689120;
	unsigned int duration = 1100*((double) abs(degrees))/aVel;
	//at the high end of pwm speed angle is off with longer spins (about 20 degs at 360 rotation)
	sendStringToLCD(LCD, "Dur: ");
	sendIntToLCD(LCD, duration);
	sendStringToLCD(LCD, "AV: ");
	sendIntToLCD(LCD, (int) aVel);
	//setLCDCursor(LCD, LCD->config.lineLength);
	sendStringToLCD(LCD, "Deg: ");
	sendIntToLCD(LCD, degrees);
	//fudgefactor is inversely proportional to set speed.
	//unsigned int fudgeFactor = (9000-pwm)/9000;
	unsigned oldDL = getDirL();
	unsigned oldDR = getDirR();
	if(degrees < 0) {
		if( sonarVals.r <= 24 ) {
			clearLCD(LCD);
			sendStringToLCD(LCD, "Pre-evading-L");
			//Execute an evasive right turn
			evasiveTurn(1);
			return;
		}
		botState.movement = MOVE_TURNLEFT;
	} else {
		if( sonarVals.l <= 24 ) {
			clearLCD(LCD);
			sendStringToLCD(LCD, "Pre-evading-R");
			//Exceuting an evasive left turn
			evasiveTurn(-1);
			return;
		}
		botState.movement = MOVE_TURNRIGHT;
	}
	setDirL(degrees < 0?-1:1);
	setDirR(degrees < 0?1:-1);
	//runTCF1(&sensorWatch, SONAR_TIMEOUT);
	setSpeed(pwm);

	_delay_ms(duration);
	setSpeed(0);
	//stopTCF1();
	botState.movement = MOVE_STILL;
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



/*
 * The following *HIGH* priority ISRs get every sonar reading.
 */
ISR( TCD0_CCA_vect ) {
	sonarVals.f = adcSmooth(Sonar0, (unsigned int) (TCD0_CCA*2)/58);
	evade();
}

ISR( TCD0_CCB_vect ) {
	sonarVals.r = adcSmooth(Sonar1, (unsigned int) (TCD0_CCB*2)/58);
	evade();
}

ISR( TCD0_CCC_vect ) {
	sonarVals.l = adcSmooth(Sonar2, (unsigned int) (TCD0_CCC*2)/58);
	evade();
}

SRCALLBACK(csetHolder) {
	int pos = atoi(pszFrame);
	TCC0_CCC = pos;
}

void setHolder(unsigned int position) {
	TCC0_CCC = position;
}

ISR( ADCA_CH0_vect ) {
	setBotMode(MODE_LAUNCHING);
	disableTBDetect();
	ADCA_CH0_INTFLAGS = ADC_CH_CHIF_bm;
	return;

}

void evade() {

	switch(botState.movement) {
		case MOVE_TURNLEFT:
			if(sonarVals.r <= 24) {
				//abort the movement. reverse safely
				evasiveTurn(1);
				clearLCD(LCD);
				sendStringToLCD(LCD, "Obstacle right");

				_delay_s(3);
			}
			break;
		case MOVE_TURNRIGHT:
			if(sonarVals.l <= 24) {
				//abort the movement. reverse safely
				evasiveTurn(-1);
				clearLCD(LCD);
				sendStringToLCD(LCD, "Obstacle left");
				_delay_s(3);
			}
			break;
		case MOVE_FORWARD:
			if(sonarVals.f <= 20) {
				//abort the movement.
				setSpeed(0);
				clearLCD(LCD);
				sendStringToLCD(LCD, "Obstacle blocking fwd");
				_delay_s(3);
			}
			break;
		default:
			break;
	}
}

void setBotMode( unsigned int mode ) {
	botState.mode = mode;
	botState.modeNeedInit = true;
}
