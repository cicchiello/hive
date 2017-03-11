#ifndef servoconfigactuator_h
#define servoconfigactuator_h

#include <Actuator.h>
#include <ServoConfig.h>

class Str;
class HiveConfig;

class ServoConfigActuator : public Actuator, public ServoConfig {
 public:
    ServoConfigActuator(HiveConfig *config, const char *name, unsigned long now);
    ~ServoConfigActuator();

    bool isMyMsg(const char *msg) const;
    void processMsg(unsigned long now, const char *msg);
    
    bool loop(unsigned long now);
    
    double getTripTemperatureC() const {return mTripTempC;}
    bool isClockwise() const {return mIsClockwise;}
    int getLowerLimitTicks() const {return mLowerLimitTicks;}
    int getUpperLimitTicks() const {return mUpperLimitTicks;}

 protected:
    virtual const char *className() const {return "ServoConfigActuator";}

 private:
    double mTripTempC;
    int mLowerLimitTicks, mUpperLimitTicks;
    bool mIsClockwise;
    HiveConfig &mConfig;
};


#endif
