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

#define MAXDELAY 61 // 1 second


#define LEDPIN PINB1
#define BUZZERPIN PINB2
#define BUTTON1 PIND2
#define TRIGGER_PIN PIND5
#define ECHO_PIN PIND6
#define MAXDISTANCE 4 // m
#define SPEED 343 // m/s
#define TIMEOUT 23000//((MAXDISTANCE / SPEED) * 2) * 1000000 ///2 for round trip

void setPrescaler_tc0(char option);
void setMax_count_tc0(unsigned long num);
void setPrescaler_tc1(char option);
volatile unsigned long num0V = 0;
volatile int toggle = 0;
volatile int sound = 1;
volatile unsigned int delay = 61;
int distance = 400;
void pulseTrigger();

ISR(TIMER0_OVF_vect)
{
  num0V++;
  if (num0V >= delay){
    toggle = !toggle;
    num0V = 0;
  }
}

// trigger the ultrasonic sensor
void pulseTrigger()
{
  bitSet(PORTD, TRIGGER_PIN);
  _delay_us(10);
  bitClear(PORTD, TRIGGER_PIN);
}

// get time of ultrasonic sensor
float listen()
{
    int time=0;
    while(!(bitRead(PIND,ECHO_PIN)));
    while((bitRead(PIND,ECHO_PIN))){
        _delay_us(1);
        time++;
        if (time > TIMEOUT){
            return -1;
        }
    }
    float duration = (float)time * 0.0343 / 2;
    return duration;
}

void setup()
{
  usart_init(MYUBRR); // 103-9600 bps; 8-115200
  bitSet(TIMSK0, TOIE0); // enable timer0 overflow interrupt
  set_tc1_mode(3); //TOP 0x03FF
  setPrescaler_tc1(2);
  bitSet(TCCR1A, COM1A1);
  bitSet(TCCR1A, COM1B1);
  bitSet(DDRB, LEDPIN); // set pin 9 as output
  bitSet(DDRB, BUZZERPIN); // set pin 10 as output
  bitSet(DDRD, TRIGGER_PIN); 
  bitClear(DDRD, ECHO_PIN); 
  bitClear(DDRD, BUTTON1);
  bitSet(PORTD, BUTTON1);
  sei(); // enable global interrupts
  setPrescaler_tc0(5);
}

int main()
{
  setup();
  int state = toggle;
  int previous_button1 = 1;
  while (1)
  {
    
    int current_button1 = bitRead(PIND, BUTTON1);
    if (current_button1 != previous_button1 && current_button1 == 0) {
        _delay_ms(50);
        if (bitRead(PIND, BUTTON1) == current_button1) {
            sound = !sound;
        }
    }
    previous_button1 = current_button1;

    pulseTrigger();
    float buffer = listen();
    _delay_us(10);
    if (buffer != -1){
      usart_tx_string("Distance: ");
      usart_tx_float(buffer, 3, 2);
      usart_transmit('\n');
      _delay_us(10);
      distance = buffer;
      if (distance >= 100) {
      delay = MAXDELAY;
      }
      else if (distance <= 8) {
        delay = 5;
      }
      else{
        delay = MAXDELAY * ((float)distance / 100);
      }
    }
    else{
      usart_tx_string("Timeout\n");
      buffer = 0.0;
    }
    usart_tx_string("Delay: ");
    usart_tx_float((float)delay, 3, 2);
    usart_transmit('\n');
    
    if (sound){
      if (state != toggle){
        state = toggle;
        if (state){
          OCR1A = 20;
          OCR1B = 5;
          //bitSet(PORTB, PIN);
        }
        else{
          OCR1A = 0;
          OCR1B = 0;
          //bitClear(PORTB, PIN);
        }
      }
    }
    else{
      _delay_ms(50);
      OCR1A = 0;
      OCR1B = 0;
    }
  }
}