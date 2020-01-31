/*
 * D0003E_lab1
 *
 * By: Erik Serrander, Simon Malmström Berghem
 */

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

#define LCD_POSADDR 0xec
#define ODD_MASK 0x0f
#define EVEN_MASK 0xf0
#define SMALLEST_NIBBLE 0xf


// global variabels
bool prime_ok_g = true;
// button
bool button_seg_g = false; //witch segment
bool button_pressed_g = false; //is button already pressed

//blink
uint16_t blink_time_c_g = 31250; // 8000000 / 256 = 31250
bool blink_set_g = true; // is element on
bool blink_seg_on_g = false; //shadow lcd on

inline void lcd_init(void){
	// set lowpower waveform, no frame interupts, no blinking, LCD enable
	LCDCRA = (1 << LCDAB) | (1 << LCDEN);

	// set drivetime 300 micro s, contrast control v 3.35
	LCDCCR = (1 << LCDCC0) | (1 << LCDCC1) | (1 << LCDCC2) | (1 << LCDCC3);

	//ext async clk source, 1:3 bias, 1:4 duty cycle, 25 segmts
	LCDCRB = (1 << LCDCS) | (1 << LCDMUX0) | (1 << LCDMUX1) | (1 << LCDPM0) | (1 << LCDPM1) | (1 << LCDPM2);

	// prescaler N=16, clk div D = 8
	LCDFRR = (1 << LCDCD0) | (1 << LCDCD1) | (1 << LCDCD2);
}

inline void writeChar(char ch, int pos){

	//SCC table with num 0-9
	uint16_t scc_table[] = {0x1551, 0x0110, 0x1e11, 0x1b11, 0xb50, 0x1b41, 0x1f41,0x0111, 0x1f51, 0x0b51};

	//set set addr to first pos
	uint8_t *lcdaddr = (uint8_t *) LCD_POSADDR;

	//mask to get nibble
	uint8_t nibble_mask;

	//nibble to send to lcd
	uint8_t lcd_nibble = 0x0;

	// check if pos is unvalied
	if (pos < 0 || pos > 5){
		return;
	}

	uint16_t number = 0x0;
	// chec if char is valid
	if (ch >= '0' || ch <= '9'){
		//get number on table
		number = scc_table[ch - '0'];
	}


	// point to right pos
	lcdaddr += pos >> 1;


	// check if od or even
	if (pos % 2 == 0){
		nibble_mask = EVEN_MASK;
	} else {
		nibble_mask = ODD_MASK;
	}

	// place nibbel on right lcd adr
	for (int i = 0; i < 4; i++){
		lcd_nibble = number & SMALLEST_NIBBLE;
		number = number >> 4;

		//check pos even/odd
		if (pos % 2 != 0){
			lcd_nibble = lcd_nibble << 4;
		}

		*lcdaddr = (*lcdaddr & nibble_mask) | lcd_nibble;
		

		lcdaddr += 5;
	}
}


inline void writelong(long i){
	int pos = 5;

	for (int n = 0; n < 6; n++){
		if (i < 1){ //break no more positions
			break;
		}

		// write digit att pos
		writeChar(i % 10 + '0', pos);

		pos--; //next pos
		i = i/10; // decishift
	}
}

inline bool is_prime(long i){
	// try divifing with all the numbers
	for (int n = 2; n < i; n++){
		if (i % n == 0){
			return false;
		}
	}
	return true;
}
 
//prints all the primes
inline void primes(void){
	// (2 first prime)
	for (int n = 2; true; n++){
		if (is_prime(n)){
			writelong(n);
		}
	}
}


// make segment blink with 1HZ
void blink(void){
	// set prescale factor 256
	TCCR1B = (0 << CS10) | (0 << CS11) | (1 << CS12);

	// time counter 16 bit
	// sys clk 8 MHz (prescale factor 256)
	// 8000000 / 256 = 31250
	
	uint16_t time_c = 31250;

	// is element on
	bool set = true;

	//shadow lcd on
	bool seg_on = false;

	//blink
	while(true){
		if (TCNT1 < time_c){// make it setable if clk is lower then blink_time_c_g
			set = true;
			continue;
		}
		// put on if time is right and seg is on
		if (TCNT1 >= time_c && set){
			set = false;
			//check for wrap around
			if (time_c + 31250 > 0xffff){
				time_c = time_c + 31250 - 0xffff;
			} else {
				time_c = time_c + 31250;
			}

			// check if seg is on or of and flip it
			if (blink_seg_on_g){
				LCDDR8 = 0x00; //turn off
				blink_seg_on_g = false;
				} else {
				LCDDR8 = 0x01; //turn on
				blink_seg_on_g = true;
			}
		}
	}
}

inline void blink_init(void){
	// set prescale factor 256
	TCCR1B = (0 << CS10) | (0 << CS11) | (1 << CS12);
}

inline void blink_check(void){
	if (TCNT1 < blink_time_c_g){// make it setable if clk is lower then blink_time_c_g
		blink_set_g = true;
	}
}

// make segment blink with 1HZ
inline void blink_2(void){

	// time counter 16 bit
	// sys clk 8 MHz (prescale factor 256)
	// 8000000 / 256 = 31250

	// put on if time is right and seg is on
	if (TCNT1 >= blink_time_c_g && blink_set_g){
		blink_set_g = false;
		prime_ok_g = true;
		//check for wrap around
		if (blink_time_c_g + 31250 > 0xffff){
			blink_time_c_g = blink_time_c_g + 31250 - 0xffff;
		} else {
			blink_time_c_g = blink_time_c_g + 31250;
		}

		// check if seg is on or of and flip it
		if (blink_seg_on_g){
			LCDDR8 = 0x00; //turn off
			blink_seg_on_g = false;
		} else {
			LCDDR8 = 0x01; //turn on
			blink_seg_on_g = true;
		}
	}
}

void button(void){
	PORTB = 0x80; // set joystic down to read

	bool seg = false; //witch segment
	bool pressed = false; //is button already pressed

	LCDDR13 = 0x01; // light the first seg

	//waiting...
	while(true){
		if (PINB >> 7 == 1){ // check if button has been relesed
			pressed = false;
		}
		if (PINB >> 7 == 0 && !pressed){// check if button pressed
			pressed = true;
			switch(seg){
				case false:
					LCDDR13 = 0x00;
					LCDDR18 = 0x01;
					seg = true;
					break;
				case true:
					LCDDR13 = 0x01;
					LCDDR18 = 0x00;
					seg = false;
					break;
			}
		}

	}
	
}

inline void button_init(void){
	PORTB = 0x80; // set joystic down to read
	LCDDR13 = 0x01; // light the first seg
}

inline void button_2(void){
	if (PINB >> 7 == 1){ // check if button has been relesed
		button_pressed_g = false;
	}
	//waiting...
	if (PINB >> 7 == 0 && !button_pressed_g){// check if button pressed
		button_pressed_g = true;
		switch(button_seg_g){
			case false:
				LCDDR13 = 0x00;
				LCDDR18 = 0x01;
				button_seg_g = true;
				break;
			case true:
				LCDDR13 = 0x01;
				LCDDR18 = 0x00;
				button_seg_g = false;
				break;
		}
	}

	
}

inline void pifprime(long pn){
	if(is_prime(pn)){
		writelong(pn);
	}
}


int main(void){
	// disable clk prescaler
	CLKPR = 0x80;
	CLKPR = 0x00;

	lcd_init();
	button_init();
	blink_init();
	
	//writeChar('3', 5);
	//while (true)
	//{
	//}
	
	//blink();
	//button();
	//while (true);

	long pn = 25000;

	while(true){
		button_2();
		blink_check();
		blink_2();
		pifprime(pn);
		pn++;

		//if(prime_ok_g){
			//for (int n = 0;n < 15 - (n /1024); n++){
			//for (int n = 0;n < 10; n++){
				//button_2();
				//blink_check();
				//pifprime(pn);
				//pn++;
				//button_2();
			//}
			//prime_ok_g = false;
		//}
	}
	//primes();
	//blink();
	//button();
}


