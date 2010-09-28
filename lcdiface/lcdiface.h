/*
 * lcdiface.h
 *
 *  Created on: Sep 26, 2010
 *      Author: nate
 */

#ifndef LCDIFACE_H_
#define LCDIFACE_H_

#define LCD_CMD 0x0
#define	LCD_CHR 0x2

struct LCDconfig {
	unsigned char  cursorInc;	  // 0 = Decrement cursor, 1 = Increment cursor
	unsigned char  displayShift;  // 0 = No display shift, 1 = Display shift
	unsigned char  displayEnable; // 0 = Display Off, 1 = Display On
	unsigned char  cursorEnable;  // 0 = Cursor Off, 1 = Cursor On
	unsigned char  cursorBlink;   // 0 = No blink, 1 = Blinks!
	unsigned char  shiftDisplay;  // 0 = Move Cursor, 1 = shift display
	unsigned char  shiftRight;	  // 0 = Shift Left, 1 = Shift right
	unsigned char  interface8bit; // 0 = 4bit interface, 1 = 8 bit interface
	unsigned char  twoLines;	  // 0 = 1 line, 1 = 2 lines
	unsigned char  tallFont;	  // 0 = 5x7 font, 1 = 5x10 font
	unsigned int   startPos;	  // Starting cursor position
	unsigned int   lineLength;	  //Total characters displayed in 1 line
};

struct LCDinfo {
	volatile int * pLCDDataBus;
	unsigned int iLen;
	unsigned int started;
	unsigned int cursorPos;
	char *pszLine1;
	char *pszLine2;
	char *pszCurrentLine;
	struct LCDinfo * this;
	struct LCDconfig config;
};

extern void backspace( struct LCDinfo *pLCD );
extern void clearLCD( struct LCDinfo *pLCD );
extern void initLCD( struct LCDinfo *pLCD, int clear );
extern void LCDBusWrite( volatile int * pBus, unsigned int iData);
extern void resendBuffToLCD( struct LCDinfo *pLCD );
extern void sendCharToLCD( struct LCDinfo *pLCD, char iChar );
extern void sendCommandToLCD(struct LCDinfo *pLCD, unsigned int nextCmd[] );
extern void setLCDCursor( struct LCDinfo *pLCD, unsigned int iLoc );
extern void sendStringToLCD( struct LCDinfo *pLCD, char *pszInput );


#endif /* LCDIFACE_H_ */
