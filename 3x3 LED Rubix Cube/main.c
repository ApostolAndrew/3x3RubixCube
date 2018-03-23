#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile unsigned char TimerFlag = 0; //TimerISR() sets this to 1. C programmer should clear to 0.

//Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

//
unsigned short Purple[3][3] = {{ 100, 101, 102 }, //Orange
{ 110, 111, 112 },
{ 120, 121, 122 }};

unsigned short SeaGreen[3][3] = {{ 200, 201, 202 }, //White
{ 210, 211, 212 },
{ 220, 221, 222 }};

unsigned short Cyan[3][3] = {{ 300, 301, 302 }, //Yellow
{ 310, 311, 312 },
{ 320, 321, 322 }};

unsigned short Blue[3][3] = {{ 400, 401, 402 },
{ 410, 411, 412 },
{ 420, 421, 422}};

unsigned short Green[3][3] = {{ 500, 501, 502 },
{ 510, 511, 512 },
{ 520, 521, 522 }};

unsigned short Red[3][3] = {{ 600, 601, 602 },
{ 610, 611, 612 },
{ 620, 621, 622 }};

unsigned short currentColor = 111;

//rotate the passed in matrix clockwise
void rotateClock(unsigned short Face[3][3]) {
	
	unsigned short Dummy2D[3][3];
	
	for (int i = 0; i < 3; i++) {
		Dummy2D[i][2] = Face[0][i]; // dummy[0..2][2] = Face[0][0..2]
	}
	
	for (int i = 0; i < 3; i++) {
		Dummy2D[i][0] = Face[2][i]; // dummy[0..2][0] = Face[2][0..2]
	}
	
	Dummy2D[0][1] = Face[1][0];
	Dummy2D[1][1] = Face[1][1];
	Dummy2D[2][1] = Face[1][2];
	
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			Face[i][j] = Dummy2D[i][j];
		}
	}
}


unsigned char GetBit(unsigned short x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}


// x: 8-bit value.   k: bit position to set, range is 0-7.   b: set bit to this, either 1 or 0
unsigned char SetBit(unsigned short x, unsigned char k, unsigned char b) {
	return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
	//   Set bit to 1           Set bit to 0
}

unsigned short NES;
unsigned char A, B, L, R, X, Y, start, select, up, down, left, right;

unsigned short Controller_Read() {
	
	unsigned short buttonPress = 0x0000;
	PORTB = SetBit(PINB, 7, 1);
	_delay_us(12);
	PORTB = SetBit(PINB, 7, 0);
	_delay_us(6);
	
	for (unsigned char i = 0; i < 16; i++) {
		if(!GetBit(PINB,5)){
			buttonPress = SetBit(buttonPress, i, 1);
		}
		else{
			buttonPress = SetBit(buttonPress, i, 0);
		}
		PORTB = SetBit(PINB, 6, 1);
		_delay_ms(6);
		PORTB = SetBit(PINB, 6, 0);
		_delay_ms(6);
	}
	
	return buttonPress;
}

void button() {
	B = GetBit(NES,0);
	Y = GetBit(NES,1);
	select = GetBit(NES,2);
	start = GetBit(NES,3);
	up = GetBit(NES,4);
	down = GetBit(NES,5);
	left = GetBit(NES,6);
	right = GetBit(NES,7);
	A = GetBit(NES,11);
	X = GetBit(NES,12);
	L = GetBit(NES,14);
	R = GetBit(NES,15);
}
//rotate the passed in matrix counter clockwise
void rotateCounter(unsigned short Face[3][3]) {
	
	unsigned short Dummy2D[3][3];
	
	for (int i = 0; i < 3; i++) { //dummy[0..2][0] = Face[0][2..0]
		Dummy2D[i][0] = Face[0][2-i];
	}
	
	for (int i = 1; i < 3; i++) { //dummy[2][1..2] = Face[1..2][0]
		Dummy2D[2][i] = Face[i][0];
	}
	
	for (int i = 0; i < 3; i++) { //dummy[0..1][2] = Face[2][0..1]
		Dummy2D[i][2] = Face[2][2-i];
	}
	
	Dummy2D[0][1] = Face[1][2]; //set right column middle to top row middle
	Dummy2D[1][1] = Face[1][1]; //set the middle to be the middle

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			Face[i][j] = Dummy2D[i][j];
		}
	}
}

//Rotate the face to the left by:
//rotates face clockwise
//moves UFace bottom to LFace right
//LFace right to DFace top
//DFace top to RFace left
//RFace left to UFace bottom
void rotateLeft(unsigned short Face[3][3], unsigned short UFace[3][3],
unsigned short LFace[3][3], unsigned short DFace[3][3],
unsigned short RFace[3][3]) {
	
	//Rotate Face counter clockwise
	rotateCounter(Face);

	//Use a dummy [3] array to hold a set of values
	unsigned short Dummy1D[3] = {0,0,0};
	
	for (int i = 0; i < 3; i++) { //set dummy to LFace[0..2][2]
		Dummy1D[i] = LFace[i][2];
	}
	
	for (int i = 0; i < 3; i++) { //set LFace[2..0][2] to UFace[2][0..2]
		LFace[2-i][2] = UFace[2][i];
	}
	
	for (int i = 0; i < 3; i++) { //set UFace[2][0..2] to RFace[0..2][0]
		UFace[2][i] = RFace[i][0];
	}
	
	for (int i = 0; i < 3; i++) { //set RFace[0..2][0] to DFace[0][2..0]
		RFace[i][0] = DFace[0][2-i];
	}
	
	for (int i = 0; i < 3; i++) { //set DFace[0][0..2] to Dummy
		DFace[0][i] = Dummy1D[i];
	}
}

//Rotate the face to the right
//rotates face clockwise
//moves UFace bottom to RFace left
//RFace left to DFace top
//DFace top to LFace right
//LFace right to UFace bottom
void rotateRight(unsigned short Face[3][3], unsigned short UFace[3][3],
unsigned short LFace[3][3], unsigned short DFace[3][3],
unsigned short RFace[3][3]) {
	
	//Rotate Face clockwise
	rotateClock(Face);

	//Use a dummy [3] array to hold a set of values
	unsigned short Dummy1D[3] = {0,0,0};
	
	for (int i = 0; i < 3; i++) { //set dummy to LFace[0..2][2]
		Dummy1D[i] = LFace[i][2];
	}
	
	for (int i = 0; i < 3; i++) { //set LFace[0..2][2] to DFace[0][0..2]
		LFace[i][2] = DFace[0][i];
	}
	
	int j = 2;
	for (int i = 0; i < 3; i++) { //set DFace[0][0..2] to RFace[2..0][0]

		DFace[0][i] = RFace[j-i][0];
	}
	
	for (int i = 0; i < 3; i++) { //set RFace[0..2][0] to UFace[2][0..2]
		RFace[i][0] = UFace[2][i];
	}
	
	for (int i = 0; i < 3; i++) { //set DFace[0][0..2] to Dummy[2..0]
		UFace[2][i] = Dummy1D[j-i];
	}
}

void lookUp(unsigned short Face[3][3], unsigned short ClockFace[3][3],
unsigned short CounterFace[3][3]) {
	
	rotateClock(ClockFace);
	rotateCounter(CounterFace);
	currentColor = Face[1][1];
}
unsigned short cnt = 0;
void display(unsigned short Face[3][3]) {
	unsigned char dummy = 0xFF;
	unsigned char dummy2 = 0xFF;
	unsigned char PHold = 0xED;
	unsigned char SGHoldR = 0xEF;
	unsigned char SGHoldG = 0xFE;
	unsigned char CHoldG = 0xFE;
	unsigned char CHoldB = 0xFD;
	unsigned char RHold = 0xEF;
	unsigned char BHold = 0xFD;
	unsigned char GHold = 0xFE;
	

	for(int j = 0; j < 3; j++) {
		if(Face[j][cnt] >= 100 && Face[j][cnt] < 200) { //Purple
			dummy = dummy & PHold;
		}
		else if(Face[j][cnt] >= 200 && Face[j][cnt] < 300) { //SeaGreen
			dummy = dummy & SGHoldR;
			dummy2 = dummy2 & SGHoldG;
		}
		else if(Face[j][cnt] >= 300 && Face[j][cnt] < 400) { //Cyan
			dummy = dummy & CHoldB;
			dummy2 = dummy2 & CHoldG;
		}
		else if(Face[j][cnt] >= 400 && Face[j][cnt] < 500) { //Blue
			dummy = dummy & BHold;
		}
		else if(Face[j][cnt] >= 500 && Face[j][cnt] < 600) { //Green
			dummy2 = dummy2 & GHold;
		}
		else if(Face[j][cnt] >= 600 && Face[j][cnt] < 700) { //Red
			dummy = dummy & RHold;
		}
		else;

		SGHoldG = (SGHoldG << 1) | 0x01;
		SGHoldR = (SGHoldR << 1) | 0x01;
		PHold = (PHold << 1) | 0x01;
		CHoldB = (CHoldB << 1) | 0x01;
		CHoldG = (CHoldG << 1) | 0x01;
		BHold = (BHold << 1) | 0x01;
		GHold = (GHold<< 1) | 0x01;
		RHold = (RHold << 1) | 0x01;
	}
	PORTC = dummy2; //0-2 is green
	PORTD = dummy; //1-3 is blue 4-6 is red
}

typedef struct task {
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
} task;

//SM1: DEMO LED matrix
enum SM1_States {sm1_display};
int SM1_Tick(int state) {
	
	//Local Variables
	static unsigned char column_val = 0x01;
	//Transitions
	switch(state) {
		case sm1_display: break;
		default:          state=sm1_display;
		break;
	}
	
	//Actions
	switch(state) {
		case sm1_display:
		if(column_val == 0x10) { //if 3rd column display 1st column
			column_val = 0x20;
		}
		else if(column_val == 0x08) { //if 2nd column display 3rd column
			column_val = 0x10;
		}
		else { //display 2nd column
			column_val = 0x08;
		}
		if(cnt == 3) {
			cnt = 0;
		}
		break;
		default:           break;
	}
	
	PORTA = column_val; // PORTA displays column pattern
	switch(currentColor) {
		case 111:    display(Purple);
		break;
		case 211:    display(SeaGreen);
		break;
		case 311:    display(Cyan);
		break;
		case 411:    display(Blue);
		break;
		case 511:    display(Green);
		break;
		case 611:    display(Red);
		break;
		default:     break;
	}
	cnt++;
	return state;
};

enum SM7_States {SM7_read};//checks to see what buttons are pressed
int SMTick7(int state){//transitions
	switch(state){
		case SM7_read:
		state = SM7_read;
		break;
		default:
		state = SM7_read;
		break;
	}
	switch(state){
		case SM7_read:
		NES = Controller_Read();
		button();
		break;
		default:
		break;
	}
	return state;
};
// 
enum SM8_States {SM8_init, SM8_checkbuttons, SM8_second, SM8_output, SM8_wait};
int SMTick8(int state) {//transitions
	switch(state) {
		
		case SM8_init:
		state = SM8_checkbuttons;
		break;
		
		case SM8_checkbuttons:
		if(!(NES)) {
			state=SM8_checkbuttons;
		}
		else if (NES==0x0002 || NES == 0x0001) {
			state=SM8_second;
		}
		else if (NES) {
			state=SM8_output;
		}
		else {
			state=SM8_checkbuttons;
		}
		break;
		
		case SM8_second: 
		if(!NES || !(NES==0x0001 || NES==0x0002)) {
			state=SM8_checkbuttons;
		}
		else if(NES==0x0001 || NES == 0x0002) {
			state=SM8_second;
		}
		else if(NES==0x0081 || NES == 0x0082 || NES == 0x0041 || NES == 0x0042 
		    ||  NES==0x0021 || NES == 0x0022 || NES == 0x0011 || NES == 0x0012) {
			state=SM8_output;
		}
		else {
			state=SM8_checkbuttons;
		}
		break;
		
		case SM8_output:
		state = SM8_wait;
		break;
		
		case SM8_wait:
		if(NES) {
			state = SM8_wait;
		}
		else if(!NES) {
			state = SM8_checkbuttons;
		}
		else {
			state = SM8_wait;
			
		}
		break;
		default:
		break;
	}
	switch(state) { //actions
		case SM8_init:
		break;
		case SM8_checkbuttons:
		break;
		case SM8_second:
		break;
		case SM8_output:
		
		switch(currentColor) {
			case 111: //Purple
			    if(NES==0x0012) { //LEFTUP
				rotateCounter(SeaGreen);
				rotateClock(Cyan);
				rotateLeft(Blue,SeaGreen,Red,Cyan,Purple);
				rotateClock(SeaGreen);
				rotateCounter(Cyan);
				}
				else if(NES==0x0042) { //YLEFT
				rotateCounter(Blue);
				rotateClock(Green);
				rotateLeft(Cyan, Purple, Blue, Red, Green);
				rotateClock(Blue);
				rotateCounter(Green);
				}
				else if(NES==0x0022) {//LEFTDOWN
				rotateCounter(SeaGreen);
				rotateClock(Cyan);
				rotateRight(Blue,SeaGreen,Red,Cyan,Purple);
				rotateClock(SeaGreen);
				rotateCounter(Cyan);
			    }	
			    else if(NES==0x0082) {//YRIGHT
				    rotateCounter(Green);
					rotateClock(Blue);
					rotateLeft(SeaGreen, Red,Blue,Purple,Green);
					rotateClock(Green);
					rotateCounter(Blue);
				}
				else if(NES==0x0011) {//RIGHTUP
					rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateRight(Green, SeaGreen,Purple,Cyan,Red);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
				else if(NES==0X0041) {//B LEFT
				    rotateCounter(Green);
				    rotateClock(Blue);
				    rotateRight(SeaGreen, Red,Blue,Purple,Green);
				    rotateClock(Green);
				    rotateCounter(Blue);
				}
				else if(NES==0x0021) {//RIGHTDOWN
				    rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateLeft(Green, SeaGreen,Purple,Cyan,Red);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
				else if(NES==0x0081) { //BRIGHT
					rotateCounter(Blue);
					rotateClock(Green);
					rotateRight(Cyan, Purple, Blue, Red, Green);
					rotateClock(Blue);
					rotateCounter(Green);
				}
			    else if(NES==0x0004) { //SEL
				rotateLeft(Purple,SeaGreen,Blue,Cyan,Green);
				}
				else if(NES==0x0008) {//START
				rotateRight(Purple, SeaGreen,Blue,Cyan,Green);
				}
			    else if(NES == 0x0010) { //UP
				lookUp(SeaGreen, Blue, Green);
				rotateClock(Cyan);
				rotateClock(Cyan);
				rotateClock(Red);
				rotateClock(Red);
				}
				else if(NES==0x0020) { //DOWN
				lookUp(Cyan,Green,Blue);
				rotateClock(SeaGreen);
				rotateClock(SeaGreen);
				rotateClock(Red);
				rotateClock(Red);
				}
				else if(NES == 0x0040) { //LEFT
				lookUp(Blue,Cyan,SeaGreen);
				}
				else if(NES==0x0080) { //RIGHT
				lookUp(Green,SeaGreen,Cyan);
				}
				break;
			case 211://SeaGreen
			    if(NES==0x0012) { //LEFTUP
				    rotateCounter(Red);
				    rotateClock(Purple);
				    rotateLeft(Blue,Red,Cyan,Purple,SeaGreen);
				    rotateClock(Red);
				    rotateCounter(Purple);
			    }
				else if(NES==0x042) { //YLEFT
					rotateCounter(Blue);
                    rotateClock(Green);
                    rotateLeft(Purple, SeaGreen,Blue,Cyan,Green);
                    rotateClock(Blue);
                    rotateCounter(Green);
				}
			    else if(NES==0x0022) { //LEFTDOWN
				    rotateCounter(Red);
				    rotateClock(Purple);
				    rotateRight(Blue,Red,Cyan,Purple,SeaGreen);
				    rotateClock(Red);
				    rotateCounter(Purple);
			    }
				else if(NES==0x0082) {//YRIGHT
				    rotateCounter(Green);
				    rotateClock(Blue);
				    rotateLeft(Red, Cyan, Blue, SeaGreen, Green);
				    rotateClock(Green);
				    rotateCounter(Blue);
				}
				else if(NES==0x0011) {//RIGHTUP
				    rotateCounter(Purple);
					rotateClock(Red);
					rotateRight(Green,Red,SeaGreen,Purple, Cyan);
					rotateClock(Purple);
					rotateCounter(Red);
				}
				else if(NES==0x0041) {//BLEFT
				    rotateCounter(Green);
					rotateClock(Blue);
					rotateRight(Red, Cyan, Blue, SeaGreen, Green);
					rotateClock(Green);
					rotateCounter(Blue);
				}
				else if(NES==0x0021) {//RIGHTDOWN
					rotateCounter(Purple);
					rotateClock(Red);
					rotateLeft(Green,Red,SeaGreen,Purple, Cyan);
					rotateClock(Purple);
					rotateCounter(Red);
				}
				else if(NES==0x0081) { //BRIGHT
				    rotateCounter(Blue);
				    rotateClock(Green);
				    rotateRight(Purple, SeaGreen, Blue, Cyan, Green);
				    rotateClock(Blue);
				    rotateCounter(Green);
				}
			else if(NES==0x0004) {//SEL
			    rotateLeft(SeaGreen,Red,Blue,Purple,Green);
			}
			else if(NES==0x0008){//START
				rotateRight(SeaGreen,Red, Blue, Purple, Green);
			}
			else if(NES == 0x0010) { //UP
				lookUp(Red, Green, Blue);
				rotateClock(SeaGreen);
				rotateClock(SeaGreen);
				rotateClock(Red);
				rotateClock(Red);
			}
			else if(NES==0x0020) { //DOWN
				lookUp(Purple, Green,Blue);
				rotateClock(Red);
				rotateClock(Red);
				rotateClock(Cyan);
				rotateClock(Cyan);
			}
			else if(NES==0x040) {//LEFT
				lookUp(Blue,Red ,SeaGreen);
				rotateClock(Red);
				rotateClock(Green);
				rotateCounter(Cyan);
			}
			else if(NES==0x0080) { //RIGHT
				lookUp(Green,SeaGreen,Blue);
				rotateClock(Cyan);
				rotateClock(Green);
				rotateClock(Red);
				rotateClock(Red);
			}
			break;
			case 311://Cyan
			    if(NES==0x0012) { //LEFTUP
				    rotateCounter(Purple);
				    rotateClock(Red);
				    rotateLeft(Blue,Purple,SeaGreen,Red,Cyan);
				    rotateClock(Purple);
				    rotateCounter(Red);
			    }
				else if(NES==0x0042) {//YLEFT
				    rotateCounter(Blue);
					rotateClock(Green);
					rotateLeft(Red, Cyan, Blue, SeaGreen, Green);
					rotateClock(Blue);
					rotateCounter(Green);
				}
				else if(NES==0x0022) { //LEFTDOWN
					rotateCounter(Purple);
					rotateClock(Red);
					rotateRight(Blue,Purple,SeaGreen,Red,Cyan);
					rotateClock(Purple);
					rotateCounter(Red);
				}
				else if(NES==0x0082) {//YRIGHT
				    rotateCounter(Green);
					rotateClock(Blue);
					rotateRight(Purple, SeaGreen, Blue, Cyan, Green);
					rotateClock(Blue);
					rotateCounter(Green);
				}
				else if(NES==0x0011) {//RIGHTUP
				    rotateCounter(Red);
					rotateClock(Purple);
					rotateRight(Green, Purple, Cyan, Red, SeaGreen);
					rotateClock(Red);
					rotateCounter(Purple);
				}
				else if(NES==0x0041) {//BLEFT
				    rotateCounter(Green);
					rotateClock(Blue);
					rotateLeft(Purple,SeaGreen,Blue,Cyan,Green);
					rotateClock(Green);
					rotateCounter(Blue);
				}
				else if(NES==0x0021) {//RIGHTDOWN
					rotateCounter(Red);
					rotateClock(Purple);
					rotateLeft(Green, Purple, Cyan, Red, SeaGreen);
					rotateClock(Red);
					rotateCounter(Purple);
				}
				else if(NES==0x0081) {//BRIGHT
					rotateCounter(Blue);
					rotateClock(Green);
					rotateRight(Red, Cyan, Blue,SeaGreen,Green);
					rotateClock(Blue);
					rotateCounter(Green);
				}
			else if(NES==0x0004){//SEL
			    rotateLeft(Cyan,Purple, Blue,Red,Green);
			}
			else if(NES==0x0008) {//START
				rotateRight(Cyan,Purple, Blue,Red,Green);
			}
			else if(NES == 0x0010) { //UP
				lookUp(Purple, Blue, Green);
				rotateClock(Red);
				rotateClock(Red);
				rotateClock(SeaGreen);
				rotateClock(SeaGreen);
			}
			else if(NES==0x0020) { //DOWN
			    lookUp(Red,Blue,Green);
				rotateClock(Cyan);
				rotateClock(Cyan);
				rotateClock(Red);
				rotateClock(Red);
			}
			else if(NES==0x0040) {
				lookUp(Blue, Cyan, Green);
				rotateClock(Blue);
				rotateClock(Red);
				rotateClock(Red);
				rotateClock(SeaGreen);
			}
			else if(NES==0x0080) { //RIGHT
				lookUp(Green,Blue,Cyan);
				rotateCounter(Green);
				rotateCounter(SeaGreen);
				rotateClock(Red);
				rotateClock(Red);
			}
			break;
			case 411://Blue
			    if(NES==0x0012) { //LEFTUP
				    rotateCounter(SeaGreen);
				    rotateClock(Cyan);
				    rotateLeft(Red,SeaGreen,Green,Cyan,Blue);
				    rotateClock(SeaGreen);
				    rotateCounter(Cyan);
			    }
				else if(NES==0x0042) {//YLEFT
				    rotateCounter(Red);
					rotateClock(Purple);
					rotateLeft(Cyan ,Blue, Red, Green, Purple);
					rotateClock(Red);
					rotateCounter(Purple);
				}
			    else if(NES==0x0022) { //LEFTDOWN
				    rotateCounter(SeaGreen);
				    rotateClock(Cyan);
				    rotateRight(Red,SeaGreen,Green,Cyan,Blue);
				    rotateClock(SeaGreen);
				    rotateCounter(Cyan);
			    }
				else if(NES==0x0082){ //YRIGHT
				    rotateCounter(Purple);
					rotateClock(Red);
					rotateRight(SeaGreen, Green, Red, Blue, Purple);
					rotateClock(Purple);
					rotateCounter(Red);
				}
				else if(NES==0x0011) {//RIGHTUP
				    rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateRight(Purple, SeaGreen, Blue, Cyan, Green);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
				else if(NES==0x0041) {//BLEFT
				    rotateCounter(Purple);
					rotateClock(Red);
					rotateLeft(SeaGreen,Green,Red,Blue,Purple);
					rotateClock(Purple);
					rotateCounter(Red);
				}
				else if(NES==0x0021) {//RIGHTDOWN
					rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateLeft(Purple, SeaGreen, Blue, Cyan, Green);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
				else if(NES==0x0081) {//BRIGHT
					rotateCounter(Red);
					rotateClock(Purple);
					rotateRight(Cyan,Blue,Red,Green,Purple);
					rotateClock(Red);
					rotateCounter(Purple);
				}
			else if(NES==0x0004) {//SEL
			    rotateLeft(Blue,SeaGreen,Red,Cyan,Purple);
			}
			else if(NES==0x0008) {//start
				rotateRight(Blue,SeaGreen,Red,Cyan,Purple);
			}
			else if(NES == 0x0010) { //UP
				lookUp(SeaGreen, Red, Green);
				rotateClock(Blue);
				rotateClock(SeaGreen);
				rotateClock(Red);
				rotateClock(Cyan);
			}
			else if(NES==0x0020) { //DOWN
			    lookUp(Cyan,Red,Blue);
				rotateCounter(Cyan);
				rotateClock(Red);
				rotateCounter(SeaGreen);
				rotateClock(Green);
			}
			else if(NES==0x0040) {//LEFT
				lookUp(Red,Cyan,SeaGreen);
			}
			else if(NES==0x0080) { //RIGHT
				lookUp(Purple,SeaGreen,Cyan);
			}
			break;
			case 511://Green
			    if(NES==0x0012) { //LEFTUP
				    rotateCounter(SeaGreen);
				    rotateClock(Cyan);
				    rotateLeft(Purple,SeaGreen,Blue,Cyan,Green);
				    rotateClock(SeaGreen);
				    rotateCounter(Cyan);
			    }
				else if(NES==0x0042) { //YLEFT
				    rotateCounter(Red);
					rotateClock(Purple);
					rotateLeft(Cyan, Green, Purple, Blue, Red);
					rotateClock(Red);
					rotateCounter(Purple);
				}
			    else if(NES==0x0022) { //LEFTDOWN
				    rotateCounter(SeaGreen);
				    rotateClock(Cyan);
				    rotateRight(Purple,SeaGreen,Blue,Cyan,Green);
				    rotateClock(SeaGreen);
				    rotateCounter(Cyan);
			    }
				else if(NES==0X0082) {//YRIGHT
				    rotateCounter(Purple);
					rotateClock(Red);
					rotateRight(SeaGreen, Blue, Purple, Green, Red);
					rotateClock(Purple);
					rotateCounter(Red);
				}
				else if(NES==0x0011) { //RIGHT UP
				    rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateRight(Red, SeaGreen, Green, Cyan, Blue);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
				else if(NES==0x0021) { //RIGHT DOWN
					rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateLeft(Red, SeaGreen, Green, Cyan, Blue);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
			else if(NES==0x0004) {//SEL
			    rotateLeft(Green,SeaGreen,Purple,Cyan,Red);
			}
			else if(NES==0x0008) {//Start
				rotateRight(Green,SeaGreen,Purple,Cyan,Red);
			}
			else if(NES == 0x0010) { //UP
				lookUp(SeaGreen, Blue, Cyan);
				rotateCounter(Green);
				rotateCounter(SeaGreen);
				rotateClock(Red);
				rotateClock(Red);
			}
			else if(NES==0x0020) {//DOWN
				lookUp(Cyan, Green, Blue);
				rotateClock(Red);
				rotateClock(Red);
				rotateClock(Cyan);
				rotateClock(SeaGreen);
			}
			else if(NES==0x0040) {//LEFT
				lookUp(Purple, Cyan, SeaGreen);
			}
			else if(NES==0x0080) { //RIGHT
				lookUp(Red,SeaGreen,Cyan);
			}
			break;
			case 611://Red
			    if(NES==0x0012) { //LEFTUP
				    rotateCounter(SeaGreen);
				    rotateClock(Cyan);
				    rotateLeft(Green,SeaGreen,Purple,Cyan,Red);
				    rotateClock(SeaGreen);
				    rotateCounter(Cyan);
			    }
				else if(NES==0x0042) {//LEFTLEFT
				    rotateCounter(Green);
					rotateClock(Blue);
					rotateLeft(Cyan, Red, Green,Purple, Blue);
					rotateClock(Green);
					rotateCounter(Blue);
				}
				else if(NES==0x0022) { //LEFTDOWN
					rotateCounter(SeaGreen);
					rotateClock(Cyan);
					rotateRight(Green,SeaGreen,Purple,Cyan,Red);
					rotateClock(SeaGreen);
					rotateCounter(Cyan);
				}
				else if(NES==0x0082) { //LEFTRIGHT
					rotateCounter(Blue);
					rotateClock(Green);
					rotateRight(SeaGreen, Purple, Green, Red, Blue);
					rotateClock(Blue);
					rotateCounter(Green);
				}
				else if(NES==0x0011) { //RIGHTUP
					rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateRight(Blue, SeaGreen, Red, Cyan, Purple);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
				else if(NES==0x0021) { //RIGHTDOWN
					rotateCounter(Cyan);
					rotateClock(SeaGreen);
					rotateLeft(Blue, SeaGreen, Red, Cyan, Purple);
					rotateClock(Cyan);
					rotateCounter(SeaGreen);
				}
			else if(NES==0x0004) {//SEL
				rotateLeft(Red,SeaGreen, Green, Cyan, Blue);
			}
			else if(NES==0x0008) {//START
				rotateRight(Red,SeaGreen, Green,Cyan,Blue);
			}
			else if(NES == 0x0010) { //UP
				lookUp(SeaGreen, Blue, Green);
				rotateClock(SeaGreen);
				rotateClock(SeaGreen);
				rotateClock(Red);
				rotateClock(Red);
			}
			else if(NES==0x0020) { //DOWN
				lookUp(Cyan,Red, Blue);
				rotateClock(Red);
				rotateClock(Green);
				rotateClock(Cyan);
				rotateClock(Cyan);
			}
			else if(NES==0x0040) {//LEFT
				lookUp(Green, Cyan, SeaGreen);
			}
			else if(NES==0x0080) { //RIGHT
				lookUp(Blue,SeaGreen,Cyan);
			}
			break;
		}
		break;
		case SM8_wait:
		break;
		default:
		break;
	}
	return state;
};

void TimerOn() {
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; // bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scalar /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	// AVR output compare register OCR1A.
	OCR1A = 125; // Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt
	
	//Initialize avr counter
	TCNT1=0;
	
	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

task tasks[3];
const unsigned short tasksNum = 3;
const unsigned short tasksPeriodGCD = 1;

void TimerISR() {
	unsigned char i;
	for (i = 0; i < tasksNum; i++) {
		if( tasks[i].elapsedTime >= tasks[i].period) {
			tasks[i].state = tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime = 0;
		}
		tasks[i].elapsedTime += tasksPeriodGCD;
	}
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xCF; PORTB = 0x30;
	DDRC = 0x0F; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	TimerSet(tasksPeriodGCD);
	TimerOn();
	
	unsigned char i=0;
	tasks[i].state = sm1_display;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SM1_Tick;
	
	i++;
	tasks[i].state = SM7_read;
	tasks[i].period = 50;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SMTick7;
	
	i++;
	tasks[i].state = SM8_init;
	tasks[i].period = 50;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &SMTick8;
	
	
	
	while(1)
	{
		while(!TimerFlag);
		TimerFlag = 0;
	}
}


