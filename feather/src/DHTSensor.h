#ifndef DHTsensor_h
#define DHTsensor_h


#include <Sensor.h>

class Adafruit_BluefruitLE_SPI;
class Str;


class DHTSensor : public Sensor {
 public:

    DHTSensor(unsigned long now) : Sensor(now) {}
    ~DHTSensor() {}

    void sensorSample(Str *value);
    
    void enqueueRequest(const char *value, const char *timestamp);
};


#endif
