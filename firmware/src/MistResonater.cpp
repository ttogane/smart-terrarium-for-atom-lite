#include "Arduino.h"
#include "MistResonater.h"


MistResonater::MistResonater(int pin) 
{
  outPin = pin;  
  pinMode(outPin, OUTPUT);
  flg = true;
}

void MistResonater::init(int delaySec)
{
  delayMillis = delaySec * 1000;
  startMillis = millis();
  flg = false;
}

void MistResonater::task()
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


boolean MistResonater::isDone()
{
  return flg;
}


void MistResonater::setTaskState(boolean taskState) 
{
  flg = taskState;
}
