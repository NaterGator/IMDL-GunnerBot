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

		TCF0_INTCTRLB = (TCF0_INTCTRLB & ~TC0_CCAINTLVL_gm) | TC_CCAINTLVL_OFF_gc;
		botState.mode = MODE_WAITING;
		return;
	}
	botState.lookingCount++;
	turnD(30);
	isrPtrs.TCF0_milliloops = TURN_DELAY;
	TCF0_CNT = 0;
}

void initTCF0() {
	TCF0_CTRLA = TC_CLKSEL_DIV1_gc;
	TCF0_CTRLB = TC0_CCAEN_bm | TC0_WGMODE0_bm;
	TCF0_INTCTRLB = TC_CCAINTLVL_OFF_gc;
}

void runTCF0( void *funcPtr, unsigned int countMs ) {
	isrPtrs.TCF0_milliloops = countMs;
	isrPtrs.TCF0_CCA_PTR = funcPtr;
	TCF0_CTRLA = TC_CLKSEL_DIV1_gc;
	TCF0_CNT = 0;
	TCF0_CCA = 32000;
	TCF0_INTCTRLB = TC_CCAINTLVL_LO_gc;
}


ISR( TCF0_CCA_vect ) {

	if(isrPtrs.TCF0_milliloops == 0) {
		(*(isrPtrs.TCF0_CCA_PTR))();
		return;
	} else
		isrPtrs.TCF0_milliloops--;
}

void pauseTCF0() {
	TCF0_CTRLA = TC_CLKSEL_OFF_gc;
}

void resumeTCF0(){
	TCF0_CTRLA = TC_CLKSEL_DIV1_gc;
}
