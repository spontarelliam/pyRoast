// AC Control V1.1
//
// This arduino sketch is for use with the heater 
// control circuit board which includes a zero 
// crossing detect fucntion and an opto-isolated triac.
//
// AC Phase control is accomplished using the internal 
// hardware timer1 in the arduino
//
// Timing Sequence
// * timer is set up but disabled
// * zero crossing detected on pin 2
// * timer starts counting from zero
// * comparator set to "delay to on" value
// * counter reaches comparator value
// * comparator ISR turns on triac gate
// * counter set to overflow - pulse width
// * counter reaches overflow
// * overflow ISR truns off triac gate
// * triac stops conducting at next zero cross


// The hardware timer runs at 16MHz. Using a
// divide by 256 on the counter each count is 
// 16 microseconds.  1/2 wave of a 60Hz AC signal
// is about 520 counts (8,333 microseconds).


#include <avr/io.h>
#include <avr/interrupt.h>
#include "Adafruit_MAX31855.h"

#define DETECT 2  //zero cross detect
#define GATE 9    //triac gate
#define PULSE 4   //trigger pulse width (counts)

int thermoDO = 3;
int thermoCS = 4;
int thermoCLK = 5;
Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

int power=40;
float t3=1.0/120.0; //0.00833 sec per half wave
float resolution=5*pow(10,-7); //timer resolution with divide by 8

void setup(){
  Serial.begin(9600);
  
  // set up pins
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, HIGH); //enable pull-up resistor
  pinMode(GATE, OUTPUT);      //triac gate control
  
  // initialize Timer1
  cli();			// disable global interrupts
  TCCR1A = 0;		// set entire TCCR1A register to 0
  TCCR1B = 0;		// same for TCCR1B

  // set up Timer1 
  //(see ATMEGA 328 data sheet pg 134 for more details)
  OCR1A = 16660 / 2;      //initialize the comparator
  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  TIMSK1 |= (1 << OCIE1A); // enable compare interrupt
  TIMSK1 |= (1 << TOIE1); // enable overflow interrupt
  TCCR1B |= (1 << CS11); // divide by 8
  
  sei();			// enable global interrupts

  // set up zero crossing interrupt
  attachInterrupt(0,zeroCrossingInterrupt, CHANGE);    
    //IRQ0 is pin 2. Call zeroCrossingInterrupt 
    //on rising signal
}  

//Interrupt Service Routines

void zeroCrossingInterrupt(){ //zero cross detect 
//  Serial.print("zerocross, ");
//  Serial.println(millis());
  //TCCR1B=0x04; //start timer with divide by 256 input
  
  TCNT1 = 0;   //reset timer - count from zero
}

ISR(TIMER1_COMPA_vect){ //comparator match
//  Serial.print("match, ");
//  Serial.println(millis());
  digitalWrite(GATE,HIGH);  //set triac gate to high
  // Set the timer to 4 less than the overflow value so that it overflows in the next 4 counts and turns off
  TCNT1 = 65535-PULSE;      //trigger pulse width
}

ISR(TIMER1_OVF_vect){ //timer1 overflow
 // Serial.print("overflow, ");
//  Serial.println(millis());
  digitalWrite(GATE,LOW); //turn off triac gate
//  TCCR1B = 0x00;          //disable timer stopd unintended triggers
}

void loop(){ // sample code to exercise the circuit 
    Serial.print(power);
    Serial.print(" ");
    Serial.println(OCR1A);
  if (power >= 100)
  {
    OCR1A = 1;
  }
  else
  {
    OCR1A = floor(((1.0-(power/100.0)) * t3) / resolution) - 1; 
  }
  
  if (Serial.available())
  {
    //if (Serial.peek() == 'power')
    if (Serial.peek() == 'n')
    {
      Serial.read();
      power = Serial.parseInt();
    }
    while (Serial.available() > 0)
    {
      Serial.read();
    }
  }

  // Thermocouple Code
  // basic readout test, just print the current temp
   //Serial.print("Internal Temp = ");
   //Serial.println(thermocouple.readInternal());

/*   double c = thermocouple.readCelsius();
   if (isnan(c)) {
     Serial.println("Something wrong with thermocouple!");
   } else {
     Serial.print("C = "); 
     Serial.println(c);
   }*/
   Serial.print("F = ");
   Serial.println(thermocouple.readFarenheit());
 
   delay(1000);
}
