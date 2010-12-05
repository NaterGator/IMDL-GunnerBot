/*
 * isrFuncs.h
 *
 *  Created on: Nov 28, 2010
 *      Author: nate
 */

#ifndef ISRFUNCS_H_
#define ISRFUNCS_H_

#include "FetchGunner.h"

void idleCamera();
void lostBall();
void initTCF0(void);
void runTCF0( void *funcPtr, unsigned int countMs );
void stopTCF0( void );
void initTCF1(void);
void runTCF1( void *funcPtr, unsigned int countMs );
void stopTCF1( void );
void pauseTCF0(void);
void resumeTCF0(void);
void pauseTCF1(void);
void resumeTCF1(void);

void tellSonars();

void nothing(void);

#endif /* ISRFUNCS_H_ */
