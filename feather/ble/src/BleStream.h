#ifndef ble_stream_h
#define ble_stream_h


class Adafruit_BluefruitLE_SPI;
class Timestamp;
class Sensor;
class Actuator;
class Adafruit_BluefruitLE_SPI;

class BleStream {
 public:
    // only make one of these!
    BleStream(Adafruit_BluefruitLE_SPI *bleImplementation,
	      Timestamp *th, Sensor **sensors, Actuator **actuators);
    
    void processInput();

    bool setEnableInput(bool v);

    Adafruit_BluefruitLE_SPI *getBleImplementation() {return mBle;}
    
 private:
    Timestamp *mTimestamp;
    Sensor **mSensors;
    Actuator **mActuators;

    Adafruit_BluefruitLE_SPI *mBle;
    bool mInputEnabled;
};

#endif
