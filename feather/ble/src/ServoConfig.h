#ifndef servoconfig_h
#define servoconfig_h

class ServoConfig {
 public:
    virtual double getTripTemperatureC() const = 0;
    virtual bool isClockwise() const = 0;
    virtual int getLowerLimitTicks() const = 0;
    virtual int getUpperLimitTicks() const = 0;
};


#endif
