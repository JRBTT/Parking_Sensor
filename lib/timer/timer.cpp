// prescaler.cpp
#include "timer.h"
#include <avr/io.h>
#include "bit.h"

int setPrescaler_tc0(char option)
{
  switch (option)
  {
  case 0:
    // no clock source 
    bitClear(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 1:
    // full clock: 16 Mhz = 62.5 ns
    bitSet(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 2:
    // 1/8 clock: 2 Mhz = 500 ns
    bitClear(TCCR0B, CS00);
    bitSet(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 3:
    // 1/64 clock: 250 kHz = 4 us
    bitSet(TCCR0B, CS00);
    bitSet(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 4:
    // 1/256 clock: 62.5 kHz = 16 us
    bitClear(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitSet(TCCR0B, CS02);
    break;
  case 5:
    // 1/1024 clock: 15.625 kHz = 64 us
    bitSet(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitSet(TCCR0B, CS02);
    break;
  default:
    return -1;
  }
  return option;
}