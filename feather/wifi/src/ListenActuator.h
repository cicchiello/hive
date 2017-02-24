#ifndef listen_actuator_h
#define listen_actuator_h

#include <Actuator.h>

class HiveConfig;
class ListenSensor;

class ListenActuator : public Actuator {
 public:
    ListenActuator(ListenSensor &listener, const char *name,
		   unsigned long now);
    ~ListenActuator() {}

    bool isMyMsg(const char *msg) const;
    void processMsg(unsigned long now, const char *msg);
    
    bool loop(unsigned long now);
    
 protected:
    virtual const char *className() const {return "ListenActuator";}

 private:
    ListenSensor &mListener;
};


#endif
