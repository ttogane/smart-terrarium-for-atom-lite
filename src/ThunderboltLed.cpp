#include "Arduino.h"
#include "ThunderboltLed.h"

ThunderboltLed::ThunderboltLed(int pin) {
  outPin = pin;
  pinMode(outPin, OUTPUT);
  flg = true;
}

void ThunderboltLed::init(int delaySec)
{
  delayMillis = delaySec * 1000;
  startMills = millis();
  previousMillis = millis();
  preState = 0;
  flg = false;
}

void ThunderboltLed::task()
{
  unsigned long currentMillis = millis();

  if(preState == 0) {
    if((state == LOW)  && (currentMillis - previousMillis >= offTime)) {
      state = HIGH;
      previousMillis = millis();
      digitalWrite(outPin, state);
    } else if ((state == HIGH)  && (currentMillis - previousMillis >= onTime)) {
      state = LOW;
      previousMillis = millis();
      offTime = random(3, 11) * 100;
      preState = 1;
      digitalWrite(outPin, state);
    }
  } else {
    if((state == LOW)  && (currentMillis - previousMillis >= offTime))
    {
      state = HIGH;
      previousMillis = millis();
      digitalWrite(outPin, state);
    } 
    else if ((state == HIGH)  && (currentMillis - previousMillis >= onTime))
    {
      state = LOW;
      previousMillis = millis();
      offTime = random(1, 4) * 1000;
      preState = 0;
      digitalWrite(outPin, state);
    }
  }

  if((state == LOW)  && (currentMillis - startMills > delayMillis)) {
    digitalWrite(outPin, state);
    setTaskState(true);
  }
}


boolean ThunderboltLed::isDone()
{
  return flg;
}


void ThunderboltLed::setTaskState(boolean taskState) 
{
  flg = taskState;
}