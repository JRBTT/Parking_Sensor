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
#define MAXPWM 1023
#define MAXDELAY 61 // 1 second

#define LEDPIN PINB1
#define BUZZERPIN PINB2
#define BUTTON1 PIND2
#define BUTTON2 PIND3
#define TRIGGER_PIN PIND5
#define ECHO_PIN PIND6
#define MAXDISTANCE 4 // m
#define SPEED 343 // m/s
#define TIMEOUT 23000//((MAXDISTANCE / SPEED) * 2) * 1000000 ///2 for round trip
// Test line
void setPrescaler_tc0(char option);
void setMax_count_tc0(unsigned long num);
void setPrescaler_tc1(char option);
volatile unsigned long num0V = 0;
volatile int toggle = 0;
volatile int sound = 1;
volatile unsigned int delay = 61;
volatile bool adcReady = false; 
volatile int adcResult = 0;
int distance = 400;
void pulseTrigger();

ISR(TIMER0_OVF_vect)
{
  num0V++;
  if (num0V >= delay){
    toggle = !toggle; // reverses current value if 0 to 1 or 1 to 0 after defined delay.
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

void setAdcbit(){
  // ADC initialization for reading the photoresistor
  ADMUX = (1 << REFS0) | (1 << MUX0) | (1 << MUX2);
  ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
  DIDR0 = (1 << ADC5D);
}

int updateADC(){
  bitSet(ADCSRA, ADSC); // start conversion
}

ISR(ADC_vect) {
    // Handle the ADC conversion result
    adcResult = ADC;
    adcReady = true;
   
    // Do something with result
}

void setup()
{
  usart_init(MYUBRR); // 103-9600 bps; 8-115200
  bitSet(TIMSK0, TOIE0); // enable timer0 overflow interrupt
  set_tc1_mode(3); // pwm mode TOP 0x03FF 1023
  setPrescaler_tc1(2); // timer1 for pwm
  bitSet(TCCR1A, COM1A1); // clear on compare match
  bitSet(TCCR1A, COM1B1); // clear on compare match
  bitSet(DDRB, LEDPIN); // set pin 9 as output
  bitSet(DDRB, BUZZERPIN); // set pin 10 as output
  bitSet(DDRD, TRIGGER_PIN); 
  bitClear(DDRD, ECHO_PIN); 
  bitClear(DDRD, BUTTON1);
  bitSet(PORTD, BUTTON1);
  bitClear(DDRD, BUTTON2);
  bitSet(PORTD, BUTTON2);
  setAdcbit(); // set ADC5 as input
  bitSet(ADCSRA, ADEN); // enable ADC
  bitSet(ADCSRA, ADIE); // enable ADC interrupt
  sei(); // enable global interrupts
  setPrescaler_tc0(5); // timer0 for just keeping time 1024 prescaler controlling on and off.
}

int main()
{
  setup();
  int state = toggle;   // 0 = off, 1 = on
  int previous_button1 = 1;
  int previous_button2 = 1;
  int volume[3] = {5, 400, 800}; //changed by button 2
  int brightness[3] = {20, 400, 1023}; //changed by ADC photoresistor
  int i = 0;
  int newOCR1A = MAXPWM;

  while (1)
  {
    if (adcReady){
      adcReady = false;
      // threshold for brightness up to 1023.
      if (adcResult > 750) {
        newOCR1A = brightness[0]; //torchlight will make it dark
      }
      else if (adcResult >= 400 && adcResult <= 750) { // room light will make it medium
        newOCR1A = brightness[1];
      }
      else{
        newOCR1A = brightness[2]; // finger covering the photoresistor will make it bright
      }
      // usart_tx_string("ADC: ");
      // usart_tx_float((float)adcResult,3,2);
      // usart_transmit('\n');
  }
    // switch device on or off
    int current_button1 = bitRead(PIND, BUTTON1);
    if (current_button1 != previous_button1 && current_button1 == 0) {
        _delay_ms(50);
        if (bitRead(PIND, BUTTON1) == current_button1) {
            sound = !sound; // switch device on or off
        }
    }
    previous_button1 = current_button1;


    // cycle through the volume levels
    int current_button2 = bitRead(PIND, BUTTON2); 
    if (current_button2 != previous_button2 && current_button2 == 0) {
        _delay_ms(50);
        if (bitRead(PIND, BUTTON1) == current_button1) {
            i++;
            if (i > 2){
              i = 0;
            }
        }
    }
    previous_button2 = current_button2;

    
    // send ultrasound signal and listen for echo
    pulseTrigger();
    float buffer = listen();
    _delay_us(10);
    // checks if there is a valid result from ultrasonic sensor
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
    
    // if the device switched on/off via button
    if (sound){
      if (state != toggle){
        state = toggle;
        if (state){
          OCR1A = newOCR1A; // led pwm timer1 register
          OCR1B = volume[i]; // buzzer pwm timer1 register
          updateADC();
          //bitSet(PORTB, PIN);
        }
        else{
          OCR1A = 0;
          OCR1B = 0;
          //bitClear(PORTB, PIN);
        }
      }
    }
    // switched off
    else{
      _delay_ms(50);
      OCR1A = 0;
      OCR1B = 0;
    }
  }
}