#ifndef servoconfigactuator_h
#define servoconfigactuator_h

#include <Actuator.h>
#include <ServoConfig.h>

class Str;

class ServoConfigActuator : public Actuator, public ServoConfig {
 public:
    class QueueEntry {
    public:
        QueueEntry() {}
      
	const char *getName() const {return "latch-config";}
    
	void post(const char *sensorName, class Adafruit_BluefruitLE_SPI &ble);
    };
  
    ServoConfigActuator(const char *name, unsigned long now);
    ~ServoConfigActuator();

    virtual void act(class Adafruit_BluefruitLE_SPI &ble);
    
    void enqueueRequest();
    void attemptPost(class Adafruit_BluefruitLE_SPI &ble);

    bool isItTimeYet(unsigned long now);
    void scheduleNextAction(unsigned long now);
    
    virtual bool isMyCommand(const Str &response) const;
    virtual void processCommand(Str *response);

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
};


#endif
