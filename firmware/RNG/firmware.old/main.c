/* Name: main.c
 * Author: <insert your name here>
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#include <avr/io.h>
#include <util/delay.h>

unsigned char ent1 = 0;
unsigned char ent2 = 0;
unsigned char count = 0;
unsigned char buffer = 0;

int main(void){
  /* insert your hardware initialization here */
  // Change the clock prescaler to 0
  CLKPR = 0b10000000;
  CLKPR = 0b00000000;

  // Set the LED pins on Port D as output, and default the LED pins to HIGH (off)
  DDRD = 0b00110011;
  PORTD = 0b00110011;

  // Set Entropy pin to input
  DDRC = DDRC & 0b11111011;

  for(;;){
    ent1 = (PINC & 0b00000100) >> 2;
    ent2 = (PINC & 0b00000100) >> 2;

    if(ent1 != ent2){
      if(ent1 > 0){
        buffer = buffer | (0b00000001 << count)
      }
      if(count >= 7){
        //print buffer, for now we'll just clear it.
        buffer = 0;
      }else{
        count++;
      }
    }

    PORTD = ((count & 0b00001100) << 2) | (count & 0b00000011);

    /* insert your main loop code here
    PORTD = 0b00110011;
    _delay_ms(250);
    PORTD = 0b00110010;
    _delay_ms(250); */
  }

  return 0;   /* never reached */
}
