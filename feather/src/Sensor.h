#ifndef sensor_h
#define sensor_h


class Adafruit_BluefruitLE_SPI;
class Str;


class Sensor {
 public:
    Sensor(const char *sensorName, const class RateProvider &rateProvider, unsigned long now);
    ~Sensor();

    virtual bool isItTimeYet(unsigned long now);
    virtual void scheduleNextSample(unsigned long now);
    
    virtual bool sensorSample(Str *value) = 0;
    
    virtual void enqueueRequest(const char *value, const char *timestamp);

    void attemptPost(Adafruit_BluefruitLE_SPI &ble);
    
    bool isMyResponse(const Str &response) const;
    void processResponse(Str *response);

    virtual const char *getName() const;
    
 protected:
    void enqueueFullRequest(const char *sensorName, const char *value, const char *timestamp);

    virtual const char *className() const = 0;
    
    Str TAG(const char *memberfunc, const char *msg) const;
    
    unsigned long mNextSampleTime;

 private:
    Str *mName;
    const class RateProvider &mRateProvider;
};


#endif
