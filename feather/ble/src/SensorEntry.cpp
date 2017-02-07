#include <SensorEntry.h>

#include <str.h>

#include <cloudpipe.h>


void SensorEntry::post(class Adafruit_BluefruitLE_SPI &ble)
{
    CloudPipe::singleton().uploadSensorReading(ble,
					       sensorName.c_str(),
					       value.c_str(),
					       timestamp.c_str());
}

