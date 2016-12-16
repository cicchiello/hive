#ifndef txqueue_h
#define txqueue_h

class Adafruit_BluefruitLE_SPI;


class TxQueue {
 public:
  TxQueue() {}

  void push(const char *sensorName, const char *value, const char *timestamp);

  void attemptPost(Adafruit_BluefruitLE_SPI &ble);
  void receivedFailureConfirmation();
  void receivedSuccessConfirmation();
};

#endif
