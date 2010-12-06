/*
 * motorControl.c
 *
 *  Created on: Nov 28, 2010
 *      Author: nate
 */

#include "FetchGunner.h"


void csetSpeedL( char *pszFrame ) {
	unsigned int iVal = atoi(pszFrame);
	clearLCD(LCD);
	sendStringToLCD(LCD, "Set L to: ");
	sendIntToLCD(LCD, iVal);
	setSpeedL(iVal);
}

void csetSpeedR( char *pszFrame ) {
	//setLCDCursor(LCD, LCD->config.lineLength);
	//sendStringToLCD(LCD, pszFrame);
	unsigned int iVal = atoi(pszFrame);
	clearLCD(LCD);
	sendStringToLCD(LCD, "Set R to: ");
	sendIntToLCD(LCD, iVal);
	setSpeedR(iVal);
}

void csetDirL( char *pszFrame ) {
	//setLCDCursor(LCD, LCD->config.lineLength);
	//sendStringToLCD(LCD, pszFrame);

	clearLCD(LCD);
	if(strcmp(pszFrame,"F") == 0) {
		setDirL(1);
		sendStringToLCD(LCD, "Set L to fwd");
	} else if(strcmp(pszFrame,"R") == 0) {
		setDirL(-1);
		sendStringToLCD(LCD, "Set L to rev");
	} else {
		sendStringToLCD(LCD, "Could not understand Ldir");
	}

}

void csetDirR( char *pszFrame ) {
	//setLCDCursor(LCD, LCD->config.lineLength);
	//sendStringToLCD(LCD, pszFrame);

	clearLCD(LCD);
	if(strcmp(pszFrame,"F") == 0) {
		setDirR(1);
		sendStringToLCD(LCD, "Set R to fwd");
	} else if(strcmp(pszFrame,"R") == 0) {
		setDirR(-1);
		sendStringToLCD(LCD, "Set R to rev");
	} else {
		sendStringToLCD(LCD, "Could not understand Rdir");
	}

}

void setSpeedL( unsigned int speed ){
	if(speed == 0)
		TCC0_CCA = 0;
	else
		TCC0_CCA = max(min(speed, motors.max), motors.min);
}
void setSpeedR( unsigned int speed ){
	if(speed == 0)
		TCC0_CCB = 0;
	else
		TCC0_CCB = max(min(speed, motors.max), motors.min);
}

unsigned getDirL() {
	return ((PORTB_OUT & PIN0_bm) == PIN0_bm) ? 1 : -1;
}
unsigned getDirR() {
	return ((PORTB_OUT & PIN2_bm) == PIN2_bm) ? 1 : -1;
}
void setDirL( int dir ){
	int prevSpeed = TCC0_CCA;
	if(prevSpeed != 0 ){
		TCC0_CCA = 0;
		_delay_ms(200);
	}

	if( dir > 0 )
		PORTB_OUT = (PORTB_OUT & 0xFC) | PIN0_bm;
	else if(dir < 0)
		PORTB_OUT = (PORTB_OUT & 0xFC) | PIN1_bm;

	if(prevSpeed != 0 && TCC0_CCA == 0)
			TCC0_CCA = prevSpeed;
}
void setDirR( int dir ){
	int prevSpeed = TCC0_CCB;
	if(prevSpeed != 0 ){
		TCC0_CCB = 0;
		_delay_ms(200);
	}
	if( dir > 0)
		PORTB_OUT = (PORTB_OUT & 0xF3) | PIN2_bm;
	else if(dir < 0)
		PORTB_OUT = (PORTB_OUT & 0xF3) | PIN3_bm;

	if(prevSpeed != 0 && TCC0_CCB == 0)
			TCC0_CCB = prevSpeed;

}

void setSpeed(unsigned int speed) {
	//setSpeedL(speed);
	//setSpeedR(speed);
	speed = min(speed, motors.max);
	if(speed < 800 ) speed = 0;
	TCC0.CCA = speed;
	TCC0.CCB = speed;
}

void setDir( int dir ) {
	setDirL(dir);
	setDirR(dir);
}


void brush(unsigned onoff){
	if(onoff == 1)
		PORTB_OUT |= PIN4_bm;
	else
		PORTB_OUT &= ~PIN4_bm;
}

void evasiveTurn( int dir ) {
	setSpeed(0);
	_delay_ms(10);
	unsigned dirL = getDirL();
	unsigned dirR = getDirR();
	setDirL((dir<0?-1:1));
	setDirR((dir<0?1:-1));
	setSpeed(EVASIVE_SPEED);
	_delay_ms(EVASIVE_TURN_DURATION);
	setSpeed(0);
	_delay_ms(150);
	setDir(-1);
	setSpeed(EVASIVE_SPEED);
	_delay_ms(EVASIVE_REV_DURATION);
	setSpeed(0);
	setDirL(dirL);
	setDirR(dirR);
}
