#include "Arduino.h"
#include "SunshineLed.h"


SunshineLed::SunshineLed(int pin) 
{
  outPin = pin;  
  pinMode(outPin, OUTPUT);  
}

void SunshineLed::init(int delaySec)
{
  delayMillis = delaySec * 1000;
  startMillis = millis();
  flg = false;
}

void SunshineLed::task()
{
  unsigned long currentMillis = millis();
  if((state == LOW)  && (currentMillis - startMillis < delayMillis))
  {
    state = HIGH;
    digitalWrite(outPin, state);
  } 
  else if ((state == HIGH)  && (currentMillis - startMillis > delayMillis))
  {
    state = LOW;
    digitalWrite(outPin, state);
    setTaskState(true);
  }
}


boolean SunshineLed::isDone()
{
  return flg;
}


void SunshineLed::setTaskState(boolean taskState) 
{
  flg = taskState;
}

