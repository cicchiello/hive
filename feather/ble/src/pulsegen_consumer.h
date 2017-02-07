#ifndef pulsegen_consumer_h
#define pulsegen_consumer_h

class PulseGenConsumer {
 public:
  virtual void pulse(unsigned long now) = 0;
};
    

#endif
