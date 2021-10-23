#include "Arduino.h"
#include "WaterSupplyPomp.h"


WaterSupplyPomp::WaterSupplyPomp(int pin) {
  outPin = pin;
    
  pinMode(outPin, OUTPUT);
    
}
  
void WaterSupplyPomp::init(int delaySec)
{
  delayMillis = delaySec * 1000;
  previousMillis = millis();
  flg = false;
}

void WaterSupplyPomp::task()
{
  unsigned long currentMillis = millis();
  if((state == LOW)  && (currentMillis - previousMillis < delayMillis))
  {
    state = HIGH;
    digitalWrite(outPin, state);
  } 
  else if ((state == HIGH)  && (currentMillis - previousMillis > delayMillis))
  {
    state = LOW;
    digitalWrite(outPin, state);
    setTaskState(true);
  }
}


boolean WaterSupplyPomp::getTaskState()
{
  return flg;
}


void WaterSupplyPomp::setTaskState(boolean taskState) 
{
  flg = taskState;
}
