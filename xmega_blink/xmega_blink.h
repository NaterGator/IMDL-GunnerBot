/*
 * xmega_blink.h
 *
 *  Created on: Sep 29, 2010
 *      Author: nate
 */

#ifndef XMEGA_BLINK_H_
#define XMEGA_BLINK_H_




struct adcSmooth_struct {
	unsigned int *prevVals;
	unsigned int depth;
	unsigned int index;
	unsigned int avg;
};
#define NEW_SMOOTH(name, depth) struct adcSmooth_struct  name = { calloc(depth, sizeof(int)), depth, 0, 0};


void ADCAInit(void);
void setSpeedL( char *pszFrame );
void setSpeedR( char *pszFrame );
void setDirL( char *pszFrame );
void setDirR( char *pszFrame );
void initXmega( void );
unsigned int ADCA0(void);
unsigned int adcSmooth(struct adcSmooth_struct *smoother, unsigned int iNewVal);
void ServoCInit(void);

#endif /* XMEGA_BLINK_H_ */
