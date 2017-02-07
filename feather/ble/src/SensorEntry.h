#ifndef sensorentry_h
#define sensorentry_h

#ifndef str_h
#   include <str.h>
#endif

class SensorEntry {
public:
    Str sensorName, value, timestamp;
    
    SensorEntry() {}
    
    SensorEntry(const char *_sensorName, const char *_value, const char *_timestamp)
      : sensorName(_sensorName), value(_value), timestamp(_timestamp)
    {
    }

  
    void set(const char *_sensorName, const char *_value, const char *_timestamp)
    {
        sensorName = _sensorName;
	value = _value;
	timestamp = _timestamp;
    }

    void post(class Adafruit_BluefruitLE_SPI &ble);
    
};


#endif
