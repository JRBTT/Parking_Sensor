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

#define PORT &PORTB
#define DDR &DDRB
#define PIN PINB1

void setPrescaler_tc0(char option);
void setMax_count_tc0(unsigned long num);
void setPrescaler_tc1(char option);
volatile unsigned long num0V = 0;
volatile int toggle = 0;

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

  set_tc1_mode(3); //TOP 0x03FF
  setPrescaler_tc1(2);
  bitSet(TCCR1A, COM1A1);
  bitSet(TCCR1A, COM1B1);
  bitSet(*DDR, PIN); // set pin 9 as output
  sei(); // enable global interrupts
  setPrescaler_tc0(5);
}

int main()
{
  setup();
  int state = toggle;
  while (1)
  {
    if (state != toggle){
      state = toggle;
      if (state){
        OCR1A = 20;
        //bitSet(*PORT, PIN);
      }
      else{
        OCR1A = 0;
        //bitClear(*PORT, PIN);
      }
    }
  }
}