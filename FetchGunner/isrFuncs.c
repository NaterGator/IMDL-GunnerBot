/*
 * isrFuncs.c
 *
 *  Created on: Nov 28, 2010
 *      Author: nate
 */

#include "isrFuncs.h"

void idleCamera() {
	if(botState.lookingCount == 24 || botState.bluetoothState == false ) {
		//Tried too many times.
		sendStringToLCD(LCD, "Could not locate ball");
		botState.lookingCount = 0;
		TCF0_INTCTRLB = (TCF0_INTCTRLB & ~TC0_CCAINTLVL_gm) | TC_CCAINTLVL_OFF_gc;
		botState.mode = MODE_WAITING;
		return;
	}
	botState.lookingCount++;
	clearLCD(LCD);
	sendStringToLCD(LCD, "Looking right");
	//TODO: Switch back to 30 for demo day
	turnD(50);
}

void sensorWatch() {
	/* do something useful with sonar sensor data
	 *
	 * should run during turn(), advance()
	 */
	int fSon = adcSmooth(Sonar0, (unsigned int) (SONF*2)/58);
	int lSon = adcSmooth(Sonar1, (unsigned int) (SONL*2)/58);
	int rSon = adcSmooth(Sonar2, (unsigned int) (SONR*2)/58);
	switch(botState.movement) {
		case MOVE_TURNLEFT:
			if(rSon <= 24) {
				//abort the movement. reverse safely
				evasiveTurn(1);
				clearLCD(LCD);
				sendStringToLCD(LCD, "Obstacle right");

				_delay_s(3);
			}
			break;
		case MOVE_TURNRIGHT:
			if(lSon <= 24) {
				//abort the movement. reverse safely
				evasiveTurn(-1);
				clearLCD(LCD);
				sendStringToLCD(LCD, "Obstacle left");
				_delay_s(3);
			}
			break;
		case MOVE_FORWARD:
			if(fSon <= 20) {
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
	/*setLCDCursor(LCD, 0);
	sendStringToLCD(LCD, "F,L,R:");
	sendIntToLCD(LCD, fSon);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, lSon);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, rSon);
	sendCharToLCD(LCD, ':');
	sendIntToLCD(LCD, count++);
	sendStringToLCD(LCD, "     ");*/

}

void initTCF0() {
	TCF0_CTRLA = TC_CLKSEL_DIV1_gc;
	TCF0_CTRLB = TC0_CCAEN_bm | TC0_WGMODE0_bm;
	TCF0_INTCTRLB = TC_CCAINTLVL_OFF_gc;
}

void runTCF0( void *funcPtr, unsigned int countMs ) {
	isrPtrs.TCF0_milliloops = countMs;
	isrPtrs.TCF0_millis = countMs;
	isrPtrs.TCF0_CCA_PTR = funcPtr;
	TCF0_CTRLA = TC_CLKSEL_DIV1_gc;
	TCF0_CNT = 0;
	TCF0_CCA = 32000;
	TCF0_INTCTRLB = TC_CCAINTLVL_LO_gc;
}

void stopTCF0( void ) {
	isrPtrs.TCF0_milliloops = 0;
	isrPtrs.TCF0_millis = 0;
	isrPtrs.TCF0_CCA_PTR = &nothing;
	TCF0_CTRLA = TC_CLKSEL_OFF_gc;
}


void initTCF1() {
	TCF1_CTRLA = TC_CLKSEL_DIV1_gc;
	TCF1_CTRLB = TC1_CCAEN_bm | TC1_WGMODE0_bm;
	TCF0_INTCTRLB = TC_CCAINTLVL_OFF_gc;
	TCF1_CCA = 32000;
}

void runTCF1( void *funcPtr, unsigned int countMs ) {
	isrPtrs.TCF1_milliloops = countMs;
	isrPtrs.TCF1_millis = countMs;
	isrPtrs.TCF1_CCA_PTR = funcPtr;
	TCF1_CTRLA = TC_CLKSEL_DIV1_gc;
	TCF1_CNT = 0;
	TCF1_INTCTRLB = TC_CCAINTLVL_LO_gc;
}

void stopTCF1( void ) {
	isrPtrs.TCF1_milliloops = 0;
	isrPtrs.TCF1_millis = 0;
	isrPtrs.TCF1_CCA_PTR = &nothing;
	TCF1_CTRLA = TC_CLKSEL_OFF_gc;
}


ISR( TCF0_CCA_vect ) {

	if(isrPtrs.TCF0_milliloops == 0) {
		(*(isrPtrs.TCF0_CCA_PTR))();
		isrPtrs.TCF0_milliloops = isrPtrs.TCF0_millis;
		return;
	} else
		isrPtrs.TCF0_milliloops--;
}

ISR( TCF1_CCA_vect ) {

	if(isrPtrs.TCF1_milliloops == 0) {
		(*(isrPtrs.TCF1_CCA_PTR))();
		isrPtrs.TCF1_milliloops = isrPtrs.TCF1_millis;
		return;
	} else
		isrPtrs.TCF1_milliloops--;
}

void pauseTCF0() {
	TCF0_CTRLA = TC_CLKSEL_OFF_gc;
}

void resumeTCF0(){
	TCF0_CTRLA = TC_CLKSEL_DIV1_gc;
}

void pauseTCF1() {
	TCF1_CTRLA = TC_CLKSEL_OFF_gc;
}

void resumeTCF1(){
	TCF1_CTRLA = TC_CLKSEL_DIV1_gc;
}

void nothing() {
	return;

}


void tellSonars() {
	int fSon = adcSmooth(Sonar0, (unsigned int) (SONF*2)/58);
	int lSon = adcSmooth(Sonar1, (unsigned int) (SONL*2)/58);
	int rSon = adcSmooth(Sonar2, (unsigned int) (SONR*2)/58);
	setLCDCursor(LCD, 0);
	sendStringToLCD(LCD, "L,F,R: ");
	sendIntToLCD(LCD, lSon);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, fSon);
	sendCharToLCD(LCD, ',');
	sendIntToLCD(LCD, rSon);
	sendStringToLCD(LCD, "     ");
}
