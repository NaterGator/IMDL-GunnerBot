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

#define MODE_WAITING 1
#define MODE_PICKUP 2
#define MODE_LAUNCHING 3



struct adcSmooth_struct {
	unsigned int *prevVals;
	unsigned int depth;
	unsigned int index;
	unsigned int avg;
};

struct ballData {
	int coordX, coordY, confidence;
};

struct FGoperator {
	unsigned int mode;
	unsigned int queuedOps;
	bool phoneLooking;
	struct ballData* ballState;
};


#define NEW_SMOOTH(name, depth) struct adcSmooth_struct  name = { calloc(depth, sizeof(int)), depth, 0, 0};


void ADCAInit(void);
void setSpeedL( char *pszFrame );
void setSpeedR( char *pszFrame );
void setDirL( char *pszFrame );
void setDirR( char *pszFrame );
SRCALLBACK(readCircle);
void initXmega( void );
void initPWMread(void);
unsigned int ADCA0(void);
unsigned int adcSmooth(struct adcSmooth_struct *smoother, unsigned int iNewVal);
void ServoCInit(void);

struct LCDinfo* LCD;
struct serialstream_struct *blueSmirfStream;


#endif /* FETCHGUNNER_H_ */
