#ifndef MistResonater_h
#define MistResonater_h

class MistResonater {

  private:
  int outPin;
  long delayMillis;

  boolean flg;
  int state = LOW;
  
  unsigned long startMillis = 0L;

  public:
  MistResonater(int pin);
  void init(int delaySec);

  
  void task();
  boolean getTaskState();
  void setTaskState(boolean taskState);
  
};

#endif
