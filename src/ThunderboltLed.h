#ifndef ThunderboltLed_h
#define ThunderboltLed_h

class ThunderboltLed {

  private:
  int outPin;
  long delayMillis;

  long randNumber;
  int preState;
  
  long onTime = 50; //millis
  long offTime;

  boolean flg;
  int state = LOW;
  
  unsigned long previousMillis = 0L;
  unsigned long startMills = 0L;

  public:
  ThunderboltLed(int pin);
  void init(int delaySec);
  
  void task();
  boolean isDone();
  void setTaskState(boolean taskState);
};
#endif
