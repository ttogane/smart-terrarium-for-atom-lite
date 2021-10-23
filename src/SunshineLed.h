#ifndef SunshineLed_h
#define SunshineLed_h

class SunshineLed {

  private:
  int outPin;
  long delayMillis;

  boolean flg;
  int state = LOW;
  
  unsigned long startMillis = 0L;

  public:
  SunshineLed(int pin);
  void init(int delaySec);
  
  void task();
  boolean getTaskState();
  void setTaskState(boolean taskState);
};
#endif
