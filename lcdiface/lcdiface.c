/*
 * lcdiface.c
 *
 *  Created on: Sep 26, 2010
 *      Author: nate
 */

#include <string.h>
#include <stdlib.h>
#include <delay.h>
#include "lcdiface.h"

void backspace( struct LCDinfo *pLCD );
void clearLCD( struct LCDinfo *pLCD );
void initLCD( struct LCDinfo *pLCD, int clear );
void LCDBusWrite( volatile int * pBus, unsigned int iData);
void resendBuffToLCD( struct LCDinfo *pLCD );
void sendCharToLCD( struct LCDinfo *pLCD, char iChar );
void sendCommandToLCD(struct LCDinfo *pLCD, unsigned int nextCmd[] );
void setLCDCursor( struct LCDinfo *pLCD, unsigned int iLoc );
void sendStringToLCD( struct LCDinfo *pLCD, char *pszInput );


void backspace( struct LCDinfo *pLCD ) {
	char *pszBuf1,*pszBuf2;
	if( pLCD->cursorPos == 0 ) return;

	pszBuf1 = malloc( sizeof( char ) * ( pLCD->config.lineLength*2 + 1 ) );
	pszBuf2 = malloc( sizeof( char ) * ( pLCD->config.lineLength*2 + 1 ) );
	memset( pszBuf2, '\0', ( pLCD->config.lineLength*2 + 1 ) );

	strcpy( pszBuf1, pLCD->pszLine1 );
	if( pLCD->pszLine2 != NULL) strcat( pszBuf1, pLCD->pszLine2 );

	strncpy( pszBuf2, pszBuf1, pLCD->cursorPos - 1 );
	strcat( pszBuf2, (pszBuf1+pLCD->cursorPos) );
	strcat( pszBuf2, " ");

	strncpy( pLCD->pszLine1, pszBuf2, pLCD->config.lineLength );
	pLCD->pszLine1[pLCD->config.lineLength]='\0';
	if( pLCD->pszLine2 != NULL ) strcpy( pLCD->pszLine2, (pszBuf2+pLCD->config.lineLength));
	pLCD->cursorPos--;
	free( pszBuf1 );
	free( pszBuf2 );
	resendBuffToLCD( pLCD );
}

void clearLCD( struct LCDinfo *pLCD ) {
	unsigned int data[8]={ 1, 0, 0, 0, 0, 0, 0, 0 };
	memset( pLCD->pszLine1, ' ', ( pLCD->config.lineLength ) );
	pLCD->pszLine1[ pLCD->config.lineLength ] = '\0';
	if( pLCD->config.twoLines == 1 ) {
		memset( pLCD->pszLine2, ' ', ( pLCD->config.lineLength ) );
		pLCD->pszLine2[ pLCD->config.lineLength ] = '\0';
	}
	pLCD->pszCurrentLine = pLCD->pszLine1;
	pLCD->cursorPos = 0;
	//Clear Screen
	sendCommandToLCD( pLCD, data ); //Clear Screen
	delay_ms(2);
}

void initLCD( struct LCDinfo *pLCD, int clear ) {
	//Setup command mode
	unsigned int data[8]={ 0, 0, 0, 0, 1, 1, 0, 0};

	if(pLCD->started == 0) {
		pLCD->started = 1;
		delay_ms(15);
		LCDBusWrite( pLCD->pLCDDataBus, 0x30);
		delay_ms(5);
		LCDBusWrite( pLCD->pLCDDataBus, 0x30);
		delay_ms(1);
		LCDBusWrite( pLCD->pLCDDataBus, 0x30);
		delay_ms(5);
		LCDBusWrite( pLCD->pLCDDataBus, 0x20 | pLCD->config.interface8bit << 4);
		delay_ms(1);
		//do startup
	}
	data[4] = pLCD->config.interface8bit;
	//sendCommandToLCD( pLCD, data );


	//Set font size, number of lines
	data[3] = pLCD->config.twoLines;
	data[2] = pLCD->config.tallFont;
	sendCommandToLCD( pLCD, data );
	delay_us(40);
	if( pLCD->pszLine1 == NULL ) {
		pLCD->pszLine1 = malloc( sizeof( char ) * ( pLCD->config.lineLength + 1 ) );
		memset( pLCD->pszLine1, ' ', ( pLCD->config.lineLength ) );
		pLCD->pszLine1[ pLCD->config.lineLength ] = '\0';
	}
	if( pLCD->config.twoLines == 1 && pLCD->pszLine2 == NULL ) {
		pLCD->pszLine2 = malloc( sizeof( char ) * ( pLCD->config.lineLength + 1 ) );
		memset( pLCD->pszLine2, ' ', ( pLCD->config.lineLength + 1 ) );
		pLCD->pszLine2[ pLCD->config.lineLength ] = '\0';
	}



	//Set display to off... turn off cursor, turn off blink
	data[5] = 0;
	data[4] = 0;
	data[3] = 1;
	data[2] = 0;
	data[1] = 0;
	data[0] = 0;
	sendCommandToLCD( pLCD, data ); //Display, Cursor, Blink Off
	delay_us(40);
	if(clear)
		clearLCD( pLCD );
	else
		resendBuffToLCD( pLCD );

	if( pLCD->config.startPos != 0 && clear ) {
		//set DDRAM position
		setLCDCursor( pLCD, pLCD->config.startPos );
	}


	//Set increment, shift
	data[3] = 0;
	data[2] = 1;
	data[1] = pLCD->config.cursorInc;
	data[0] = pLCD->config.displayShift;
	sendCommandToLCD( pLCD, data );
	delay_us(40);
	//Set display, cursor, blink
	data[3] = 1;
	data[2] = pLCD->config.displayEnable;
	data[1] = pLCD->config.cursorEnable;
	data[0] = pLCD->config.cursorBlink;
	sendCommandToLCD( pLCD, data );
	delay_us(40);
}

void LCDBusWrite( volatile int * pBus, unsigned int iData) {
	*pBus = iData & 0xF3;
	delay_us(100);
	*pBus = iData | 0x08;
	delay_us(100);
	*pBus = iData & 0xF3;
	delay_us(500);
}

void resendBuffToLCD( struct LCDinfo *pLCD ) {
	unsigned int i = 0;
	unsigned int clrCmd[8] = { 1, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int cursorPos;

	cursorPos = pLCD->cursorPos;
	sendCommandToLCD( pLCD, clrCmd );
	delay_ms(2);
	setLCDCursor( pLCD, 0 );

	for(; i < pLCD->config.lineLength; i++)
		sendCharToLCD( pLCD, pLCD->pszLine1[ i ] );

	if( pLCD->config.twoLines == 1 )
		for(i = 0; i < pLCD->config.lineLength; i++)
			sendCharToLCD( pLCD, pLCD->pszLine2[ i ] );

	setLCDCursor( pLCD, cursorPos );
}

void sendCharToLCD( struct LCDinfo *pLCD, char iChar ) {
	unsigned int hBits = (iChar & 0xF0) ;
	unsigned int lBits = (iChar & 0xF) << 4;

	if( pLCD->config.displayShift == 0 ) {
		if( pLCD->config.twoLines == 1 && pLCD->cursorPos == pLCD->config.lineLength )
			setLCDCursor( pLCD, pLCD->cursorPos );
		if( pLCD->cursorPos >= ( pLCD->config.lineLength * ( pLCD->config.twoLines + 1 ) ) )
			setLCDCursor( pLCD, 0 );
	}
	pLCD->pszCurrentLine[ pLCD->cursorPos % pLCD->config.lineLength ] = iChar;

	LCDBusWrite( pLCD->pLCDDataBus, hBits | LCD_CHR);
	LCDBusWrite( pLCD->pLCDDataBus, lBits | LCD_CHR);

	pLCD->cursorPos++;
}

void sendCommandToLCD(struct LCDinfo *pLCD, unsigned int nextCmd[] )  {
	if( pLCD->config.interface8bit == 0 ) {
		//4 bit mode!
		LCDBusWrite( pLCD->pLCDDataBus,
					(nextCmd[7] << 7) |
					(nextCmd[6] << 6) |
					(nextCmd[5] << 5) |
					(nextCmd[4] << 4)
					);
		LCDBusWrite( pLCD->pLCDDataBus,
					(nextCmd[3] << 7) |
					(nextCmd[2] << 6) |
					(nextCmd[1] << 5) |
					(nextCmd[0] << 4)
					);
	} else
		return; //Listen, we don't really do 8 bit. Maybe later.
	//delay_ms(2);
}

void setLCDCursor( struct LCDinfo *pLCD, unsigned int iLoc ) {
	unsigned int data[8];
	unsigned int iMemPos=0;
	if( iLoc > pLCD->config.lineLength*2) iLoc = 0;
	if( iLoc >= pLCD->config.lineLength ) {
		pLCD->pszCurrentLine = pLCD->pszLine2;
		iMemPos = ( iLoc - pLCD->config.lineLength ) + 0x40;
	} else {
		pLCD->pszCurrentLine = pLCD->pszLine1;
		iMemPos = iLoc;
	}
	pLCD->cursorPos = iLoc;
	data[7] = 1;
	data[6] = (iMemPos & 0x40) >> 6;
	data[5] = (iMemPos & 0x20) >> 5;
	data[4] = (iMemPos & 0x10) >> 4;
	data[3] = (iMemPos & 0x8) >> 3;
	data[2] = (iMemPos & 0x4) >> 2;
	data[1] = (iMemPos & 0x2) >> 1;
	data[0] = iMemPos & 0x1;
	sendCommandToLCD( pLCD, data );
	delay_us(40);
}

void sendStringToLCD( struct LCDinfo *pLCD, char *pszInput ) {
	while(*pszInput)
		sendCharToLCD(pLCD, *(pszInput++) );
}

