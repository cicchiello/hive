#ifndef TempSensor_h
#define TempSensor_h


#include <Sensor.h>

class Str;


class TempSensor : public Sensor {
 public:

    TempSensor(unsigned long now);
    ~TempSensor();

    bool sensorSample(Str *value);
    
    void enqueueRequest(const char *value, const char *timestamp);

 private:
    Str *mPrev;

    friend class HumidSensor;
    static class DHT &getDht();
};


#endif
