#ifndef AccessPointactuator_h
#define AccessPointactuator_h

#include <Actuator.h>

class Str;
class HiveConfig;

class AccessPointActuator : public Actuator {
 public:
    AccessPointActuator(HiveConfig *config, const char *name, unsigned long now);
    ~AccessPointActuator();

    bool isMyMsg(const char *msg) const;
    void processMsg(unsigned long now, const char *msg);
    
    bool loop(unsigned long now);
    
 protected:
    virtual const char *className() const {return "AccessPointActuator";}

 private:
    HiveConfig &mConfig;
};


#endif
