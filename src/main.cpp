#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include "usart.h"
#include "bit.h"
#include "timer.h"

#define FOSC 16000000 // Frequency Oscillator 16Mhz for Uno R3
#define BAUD 9600 // 9600 Bits per second
#define MYUBRR FOSC / 16 / BAUD - 1 // My USART Baud Rate Register
#define PRESCALER 3 //250 KHz
#define DELAY 200 // 200 ms

#define PORT &PORTB
#define DDR &DDRB
#define PIN PINB1

int setPrescaler_tc0(char option);
int setMax_count_tc0(unsigned long num);
void my_delay_ms(unsigned long x);
volatile unsigned long num0V = 0;
volatile int toggle = 0;


// calculate overvlow max count
int setMax_count_tc0(unsigned long num)
{
  int prescalers[6] = {0, 1, 8, 64, 256, 1024};
  float overflow_count = ((1 / ((float)FOSC / prescalers[PRESCALER])) * 256);
  float time_sec = (float)num / 1000;
  float max = (time_sec / overflow_count);
  int max_count = (int)round(max);
  return max_count;
}

// custom delay with prescalers
void my_delay_ms(unsigned long x)
{
  unsigned long num0V_max = setMax_count_tc0(x);
  num0V = 0; // timer0 overflow counter, sets count to 0
  TCNT0 = 0; // timer0 counter register, sets count to 0
  setPrescaler_tc0(PRESCALER);
  while (num0V < num0V_max - 1); 
  
  setPrescaler_tc0(0);
}

ISR(TIMER0_OVF_vect)
{
  num0V++;
  if (num0V >= 61){
    toggle = !toggle;
    num0V = 0;
  }
}

void setup()
{
  usart_init(MYUBRR); // 103-9600 bps; 8-115200
  bitSet(TIMSK0, TOIE0); // enable timer0 overflow interrupt
  sei(); // enable global interrupts
  bitSet(*DDR, PIN); // set pin 8 as output
}

int main()
{
  setup();
  int state = toggle;
  setPrescaler_tc0(5);
  while (1)
  {
    if (state != toggle){
      state = toggle;
      if (state){
        bitSet(*PORT, PIN);
      }
      else{
        bitClear(*PORT, PIN);
      }
    }
  }
}