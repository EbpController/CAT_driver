// this project is part of the 'EBP controller' solution
//
// the interface detects the hall sensors and sends information on the 'TFM daisy chain' bus
// a maximum of 6 detectors (= 12 hall sensors) can be controlled
// each detector exist of 2 hall sensors (sensor A and sensor B)
// the 6 A-sensors are connected to Port D bit 2 to bit 7 (for detector 0 to 5)
// the 6 B-sensors are connected to Port B bit 0 to bit 5 (for detector 0 to 5)
//
// written by: Jan van Hooydonk
//
// version: 1.0   date: 20/05/2020    comment: creation
//
// principle of working
// ====================
//
// _____(track left)_____<hall sensor A>_____<hall sensor B>_____(track right)_____
//
// normaly, the state of hall sensor A and hall sensor B are '1' (active low)
// if a train passes a detector, there are two possibilities:
//  1) first hall sensor A becomes '0', then hall sensor B becomes '0' (for a train movement form the track left to track right)
//  2) first hall sensor B becomes '0', then hall sensor A becomes '0' (for a train movement form the track right to track left)
// important condition: there must be an overlap ('0') from hall sensor A(B) to hall sensor B(A) when a train passes the detector
//
// loop:
//  read the state of hall sensor A and see if the state is changed (form '1' to '0' or from '0' to '1')
//  if the state is not changed, stay in this loop
//
// state of hall sensor is changed:
//  read the state of hall sensor B
//  if state of hall sensor A = '0' and state of hall sensor B = '1' then counter +1
//  if state of hall sensor A = '1' and state of hall sensor B = '0' then counter +1
//  if state of hall sensor A = '0' and state of hall sensor B = '0' then counter -1
//  if state of hall sensor A = '1' and state of hall sensor B = '1' then counter -1
//  if counter = +2 then send '1' (= track left -1, track right +1), set counter = 0
//  if counter = -2 then send '0' (= track left +1, track right -1), set counter = 0
//  back to loop
//
// included libraries
//
#include "EbpSerial.h"

#define ADDRESS_HIGH 0b0000

byte inputValuesPortB;
byte oldInputValuesPortB;
signed char catCounter[] = {0, 0, 0, 0, 0, 0};

// EbpSerial
int myAddress;
EbpSerial EbpSerial;

void setup()
{
  // set bits 0 to 5 of port B as inputs with pullup, without changing the state of bits 6 and 7
  DDRB &= 0b11000000;
  PORTB |= 0b00111111;

  // set bits 0 to 5 of port C as inputs with pullup, without changing the state of bits 6 and 7
  DDRC &= 0b11000000;
  PORTC |= 0b00111111;

  // set bits 2 to 7 of port D as inputs with pullup, without changing the state of bits 0 and 1
  DDRD &= 0b00000011;
  PORTD |= 0b11111100;

  // read input value from port B
  oldInputValuesPortB = PINB & 0b00111111;

  // setup serial port
  myAddress = (ADDRESS_HIGH << 6) + ((PINC ^ 0b11111111) & 0b00111111);
  EbpSerial.init(myAddress);
}

void loop()
{
  // poll TFM daisy chain bus
  EbpSerial.poll();

  // poll hall sensors
  inputValuesPortB = PINB & 0b00111111;
  // if sensor(s) state changed then analyse the sensor(s)
  if (inputValuesPortB != oldInputValuesPortB) {
    analyzeInputValues(inputValuesPortB, oldInputValuesPortB);
    oldInputValuesPortB = inputValuesPortB;
  }
}

void analyzeInputValues(byte inputValuesPortB, byte oldInputValuesPortB)
{
  byte inputValuesPortD = (PIND >> 2) & 0b00111111;
  byte changedInputValuesPortB = inputValuesPortB ^ oldInputValuesPortB;

  for (int i = 0; i < 6; i++) {
    // look only to the channel(s) of the changed hall sensor(s)
    if ((changedInputValuesPortB & (1 << i)) != 0) {
      // see comment above for principle of working
      if ((inputValuesPortB & (1 << i)) == (inputValuesPortD & (1 << i))) {
        catCounter[i]++;
        if (catCounter[i] == 2)
        {
          // bit 0 = '1'
          // bit 1 to 3 = device (subaddress)
          // refer to protocol information
          EbpSerial.sendData(((i << 1) & 0b00001110) | 1);
          catCounter[i] = 0;
        }
      }
      else {
        catCounter[i]--;
        if (catCounter[i] == -2)
        {
          // bit 0 = '0'
          // bit 1 to 3 = device (subaddress)
          // refer to protocol information
          EbpSerial.sendData((i << 1) & 0b00001110);
          catCounter[i] = 0;
        }
      }
    }
  }
}
