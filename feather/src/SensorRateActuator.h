#ifndef sensorrate_h
#define sensorrate_h

#include <Actuator.h>
#include <RateProvider.h>


class Str;

class SensorRateActuator : public Actuator, public RateProvider {
 public:
    class QueueEntry {
    public:
        QueueEntry() {}
      
	const char *getName() const {return "sample-rate";}
    
	void post(const char *sensorName, class Adafruit_BluefruitLE_SPI &ble);
    };
  
    SensorRateActuator(const char *name, unsigned long now);
    ~SensorRateActuator();

    virtual void act(class Adafruit_BluefruitLE_SPI &ble);
    
    void enqueueRequest();
    void attemptPost(class Adafruit_BluefruitLE_SPI &ble);

    bool isItTimeYet(unsigned long now);
    void scheduleNextAction(unsigned long now);
    
    virtual bool isMyCommand(const char *response) const;
    virtual const char * processCommand(const char *response);

    int secondsBetweenSamples() const {return mSeconds;}

 protected:
    virtual const char *className() const {return "SensorRateActuator";}

 private:
    int mSeconds;
};


#endif
