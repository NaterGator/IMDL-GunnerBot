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
void initTCF0(void);
void runTCF0( void *funcPtr, unsigned int countMs );


#endif /* ISRFUNCS_H_ */
