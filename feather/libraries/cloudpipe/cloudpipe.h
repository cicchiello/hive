#ifndef cloud_pipe_h
#define cloud_pipe_h

class Str;
class Adafruit_BluefruitLE_SPI;

class CloudPipe {
  public:
    static const char *SensorLogDb;
  
    static const CloudPipe &singleton();
    static CloudPipe &nonConstSingleton();

    void getMacAddress(Adafruit_BluefruitLE_SPI &ble, Str *mac) const;
    
    void uploadSensorReading(Adafruit_BluefruitLE_SPI &ble,
			     const char *sensorName, const char *value, const char *timestamp) const;
    
 private:
    static CloudPipe s_singleton;
    CloudPipe();
};


inline
const CloudPipe &CloudPipe::singleton()
{
    return s_singleton;
}

inline
CloudPipe &CloudPipe::nonConstSingleton()
{
    return s_singleton;
}

#endif
