#ifndef WaterSupplyPomp_h
#define WaterSupplyPomp_h

class WaterSupplyPomp {

  private:
  int outPin;
  long delayMillis;

  boolean flg;
  int state = LOW;
  
  unsigned long previousMillis = 0L;

  public:
  WaterSupplyPomp(int pin);
  void init(int delaySec);
  
  void task();
  boolean isDone();
  void setTaskState(boolean taskState);
};
#endif
