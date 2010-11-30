/*
 * FetchGunner.h
 *
 *  Created on: Oct 19, 2010
 *      Author: nate
 */

#ifndef FETCHGUNNER_H_
#define FETCHGUNNER_H_

#ifndef SERIALSTREAM_H_
#include <serialstream.h>
#endif

#include <stdbool.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_x.h>
#include <lcdiface.h>
#include <usart.h>
#include <string.h>
#include <stdlib.h>
#include "isrFuncs.h"

#define MODE_SEEKING 0
#define MODE_WAITING 1
#define MODE_PICKUP 2
#define MODE_LAUNCHING 3

#define MOVE_STILL 0
#define MOVE_TURNLEFT 1
#define MOVE_TURNRIGHT 2
#define MOVE_FORWARD 3

#define SONF TCD0_CCA
#define SONR TCD0_CCB
#define SONL TCD0_CCC

#define TURN_DELAY 5000
#define SONAR_TIMEOUT 50
#define EVASIVE_SPEED 3000
#define EVASIVE_TURN_DURATION 700
#define EVASIVE_REV_DURATION 300


struct adcSmooth_struct {
	unsigned int *prevVals;
	unsigned int depth;
	unsigned int index;
	unsigned int avg;
};

struct isrFuncPtrs {
	void (*TCF0_CCA_PTR)( );
	void (*TCF1_CCA_PTR)( );
	unsigned int TCF0_milliloops;
	unsigned int TCF1_milliloops;
	unsigned int TCF0_millis;
	unsigned int TCF1_millis;
} isrPtrs;

struct motorData {
	unsigned int max;
	unsigned int min;
} motors;


struct FGoperator {
	unsigned int mode;
	unsigned int queuedOps;
	bool phoneLooking;
	unsigned int lookingCount;
	bool bluetoothState;
	bool launcherOn;
	unsigned movement;
} botState;


#define NEW_SMOOTH(name, depth) struct adcSmooth_struct  name = { calloc(depth, sizeof(int)), depth, 0, 0};
#define min(X,Y) (((X) < (Y)) ?  (X) : (Y))
#define max(X,Y) (((X) > (Y)) ?  (X) : (Y))

void ADCAInit(void);
void csetSpeedL( char *pszFrame );
void csetSpeedR( char *pszFrame );
void csetDirL( char *pszFrame );
void csetDirR( char *pszFrame );
void setSpeedL( unsigned int speed );
void setSpeedR( unsigned int speed );
void setSpeed(unsigned int speed);
void setDirL( int dir );
void setDirR( int dir );
void setDir( int dir );
unsigned getDirL();
unsigned getDirR();
SRCALLBACK(timeMotor);
SRCALLBACK(readCircle);
void initXmega( void );
void initPWMread(void);

void turn(int magnitude, unsigned int duration);
void advance(int l, int r, unsigned int duration);
unsigned int ADCA0(void);
unsigned int adcSmooth(struct adcSmooth_struct *smoother, unsigned int iNewVal);
void ServoCInit(void);
void stop();
void brush(unsigned onoff);
void moveTo(int x, int y);
SRCALLBACK(move);

void turnD(int degrees);
void evasiveTurn( int dir );

SRCALLBACK(cturnD);
void sendPendingChar(struct USARTconfig* conf);

struct LCDinfo* LCD;
struct USARTconfig* USARTE;
struct serialstream_struct *blueSmirfStream;


struct adcSmooth_struct *Sonar0;
struct adcSmooth_struct *Sonar1;
struct adcSmooth_struct *Sonar2;
unsigned int count;
//struct motorData motors;




#endif /* FETCHGUNNER_H_ */
