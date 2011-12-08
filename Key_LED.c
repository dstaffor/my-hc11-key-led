/*	=================================================================
	Filename:			Key_LED.c
	Author:				Corey Davyduke
	Creation date:		2011-12-07
	Last modified date:	2011-12-08
	Description:		This file contains the source code to blink
	an LED, read the keypad, and write to the LCD.
	=================================================================
*/

#include <sys/ports.h>
#include "hc11.h"

#define BIT_0 (1<<0)
#define BIT_1 (1<<1)
#define BIT_2 (1<<2)
#define BIT_3 (1<<3)
#define BIT_4 (1<<4)
#define BIT_5 (1<<5)
#define BIT_6 (1<<6)
#define BIT_7 (1<<7)

/* Port D */
#define BOARD_RX     (BIT_0)
#define BOARD_TX     (BIT_1)
#define BOARD_LCD_0  (BIT_2)
#define BOARD_LCD_1  (BIT_3)
#define BOARD_LCD_2  (BIT_4)
#define BOARD_LCD_3  (BIT_5)

/* Port A */
#define BOARD_KEY_0   (BIT_0)
#define BOARD_KEY_1   (BIT_1)
#define BOARD_KEY_2   (BIT_2)
#define BOARD_ROM_DIS (BIT_3)
#define BOARD_LCD_4   (BIT_4)
#define BOARD_LCD_5   (BIT_5)
#define BOARD_LCD_6   (BIT_6)
#define BOARD_KEY_3   (BIT_7)

/* Port E */
#define BOARD_PEB_0   (BIT_0)
#define BOARD_PEB_1   (BIT_1)
#define BOARD_PEB_2   (BIT_2)
#define BOARD_PEB_3   (BIT_3)
#define BOARD_PEB_4   (BIT_4)
#define BOARD_PEB_5   (BIT_5)
#define BOARD_PEB_6   (BIT_6)
#define BOARD_PEB_7   (BIT_7)

/* Control of Fan, Light and mains detection on Port A.  */
#define EBCS_CMD_FAN     (BOARD_LCD_5)
#define EBCS_CMD_LIGHT   (BOARD_LCD_6)
#define EBCS_SENSE_LIGHT (BOARD_KEY_0)
#define EBCS_SENSE_MAINS (BOARD_PE2)

#define LINE_1      0x80                // beginning position of LCD line 1
#define LINE_2      0xC0                // beginning position of LCD line 2
#define LINE_3      0x94                // beginning position of LCD line 3
#define LINE_4      0xD4                // beginning position of LCD line 4

#define DELAY1MS	73		// number of loops = 1ms

// I/O Port Addresses
#define LCD_CMD  *(unsigned char *)(_IO_BASE + 0xA5F0)
#define LCD_DAT  *(unsigned char *)(_IO_BASE + 0xA5F1)

// Function Prototypes
void InitKey();
char kbhit();
char getkey();
void LCD_Command(unsigned char cval);
void LCD_busy(void);
void cprint(char dval);
void LCDprint(char *sptr);
void delay100ms(unsigned int secs);
void delay1ms(unsigned int msecs);
char dosomething();

// Global Variables
unsigned char	NewKey, LastKey;	// global variables used by keypad
unsigned char	ColCnt, colnum, rownum, keybuf;

/* The I/O registers are represented by a volatile array.
   Address if fixed at link time.  For this particular example,
   the _io_ports address is defined in the `memory.x' file.  */
//extern volatile unsigned char _io_ports[];

void delay_ms (unsigned ms);

#define TABLE_SIZE(X) ((sizeof X) / sizeof (X[0]))

/* Speed is defined as a delay and a repetition counter.

   SPEED(200,4) means 4 blinking at 200ms

   Delay and counter are stored using 4-bits each.
*/
#define SPEED(T,CNT) ((((T)/100) & 0x0F) | ((CNT) & 0x0F)<<4)

static const unsigned char speeds[] = {
  SPEED(200,4),
  SPEED(500,2),
  SPEED(300,1),
  SPEED(500,5),
  SPEED(800,3),
  SPEED(200,4),
  SPEED(100,6),
  SPEED(500,4),

  SPEED(200,3),
  SPEED(600,4),
  SPEED(100,2),
  SPEED(400,6),
  SPEED(800,2),
  SPEED(100,1),
  SPEED(900,1),
  SPEED(100,2),
  SPEED(900,1),
  SPEED(100,2),
  SPEED(900,1),

  SPEED(800,4),
  SPEED(100,1),
  SPEED(800,4),
  SPEED(100,2),
  SPEED(800,3),
  SPEED(100,4),
  SPEED(800,2),
  SPEED(100,8),
  SPEED(800,1),

  SPEED(1500,2)
};

int main ()
{
  unsigned short i;
  unsigned short dt;
  short j;
  
	DDRD = 0x3E;	// set keypad I/O data direction register

	NewKey = 0; 	// reset key variables
	LastKey = 0xFF;

	// Initialize the LCD
    LCD_Command(0x3C);                 // initialize command
    LCD_Command(0x0C);                 // display on, cursor off
    LCD_Command(0x06);
    LCD_Command(0x01);

    // Display Header Information on LCD
    LCD_Command(LINE_1);               // goto lcd line 1
    LCDprint("Keypad Test");
    LCD_Command(LINE_2);               // goto lcd line 2
    LCDprint("Press Keys");
    LCD_Command(LINE_3);               // goto lcd line 3
	
  i = 0;
  while (1)
    {
      dt = speeds[i];
      j = (short) (speeds[i] >> 4);
      dt = (unsigned short) ((speeds[i] & (char) 0x0F) * (char) 100);

      do {
        /* Turn on the light.  */
        _io_ports[M6811_PORTA] |= EBCS_CMD_FAN | EBCS_CMD_LIGHT;

        /* Pause the delay specified in the speeds table.  */
        delay_ms (dt);

        /* Turn off the light.  */
        _io_ports[M6811_PORTA] &= ~(EBCS_CMD_FAN | EBCS_CMD_LIGHT);

        /* Pause again and repeat according to speeds table.  */
        delay_ms (dt);
      } while (--j >= 0);

      /* Pick next blinking rate (wrap to beginning if we reach the end).  */
      i++;
      if (i > TABLE_SIZE (speeds))
        i = 0;

	  if(kbhit())
	  {				// if new key pressed
			cprint(NewKey);			// display it
		}

    }
  return 0;
}

/* Wait 'ms' milliseconds (not accurate (:- (:-), hand adjusted
   and based on human time accuracy (understand, SCz feeling).  */
void
delay_ms (unsigned ms)
{
  unsigned short tcnt;

  while (ms > 0)
    {
      unsigned i;
      
      for (i = 100; --i != 0;)
        tcnt = get_timer_counter ();

      ms--;
    }
}

// Wait for the LCD busy pin to clear
void LCD_busy(){
    while ((LCD_CMD & 0x80)) ;
}

void LCD_Command(unsigned char cval){
//unsigned char cval;
    LCD_busy();                         // wait for busy to clear
    LCD_CMD = cval;                     // ouptut command
}

//* LCD Display Character
void cprint(char dval){
    LCD_busy();                         // wait for busy to clear
    LCD_DAT = dval;                     // ouptut data
}

// LCD Display String
void LCDprint(char *sptr){
	while( *sptr ){
		cprint(*sptr);
		++sptr;
	}
}

//  Busy wait loop to generate a delay multiple of 1ms
void delay1ms(unsigned int msecs) {
  unsigned int i,j;

  if (msecs > 0) {
     for (j=0; j<=msecs; j++)
        for (i=0; i<DELAY1MS; i++);
  }
  return;
}

//  Busy wait loop to generate a delay multiple of 100ms.
void delay100ms(unsigned int secs){
  unsigned int i,j;

  if (secs > 0) {
     for (j=0; j<=secs; j++)
        for (i=0; i<100; i++)	delay1ms(1);
  }
}


// returns 1 if a key is pressed.
// the key value/index is stored in the global variable NewKey.
char kbhit(){
	unsigned char KeyDATA[] =
	 {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};

	for(ColCnt = 0, colnum = 0x04; colnum < 0x40; ColCnt = ColCnt +4){
		PORTD = colnum;	// set column value
		dosomething();	// delay
		rownum = PORTE;	// get row value
		rownum = (rownum & 0x0f);	// mask all but rows
		if(rownum){	// if a row in this column is high
			if(rownum == 0x04) rownum = 3;
			if(rownum == 0x08) rownum = 4;	/* figure row number */
			rownum = rownum - 1;
			keybuf = ColCnt + rownum;	/* calculate key index */
			if(keybuf == LastKey)	return 0; /* ignore if same key pressed */
			LastKey = keybuf;	/* save this key for repeat */
			NewKey = KeyDATA[keybuf];
			return 1;			/* return YES  */
		}
		colnum = colnum << 1;	// shift column left
	}
	LastKey = 0xff;	/* initialize key repeat compare value*/
	return 0;	/* no key pressed, return 0 */
}

char dosomething(){
	return 0;
}
// waits for a key press and returns it
char getkey(){
	while( !kbhit()){		// wait for key press
		dosomething();
	}
	return(NewKey);
}
