#ifndef ListenSensor_h
#define ListenSensor_h

#include <AudioUpload.h>

#include <pulsegen_consumer.h>

class HiveConfig;
class RateProvider;
class TimeProvider;
class ListenerWavCreator;

class ListenSensor : public AudioUpload, PulseGenConsumer {
public:
    ListenSensor(const HiveConfig &config,
		 const char *name, 
		 const RateProvider &rateProvider,
		 const TimeProvider &timeProvider,
		 unsigned long now,
		 int ADCPIN, int BIASPIN,
		 Mutex *wifiMutex, Mutex *sdMutex);
    virtual ~ListenSensor();
    
    PulseGenConsumer *getPulseGenConsumer() {return this;}
    
    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);

    void start(int millisecondsToRecord, const char *attName);
    
    const char *className() const {return "ListenSensor";}
    
    bool sensorSample(Str *value);
    
 private:
    void pulse(unsigned long now);

    Str *mValue;
    bool mStart;
    int mState;
    int mMillisecondsToRecord;
    int mADCPIN, mBIASPIN;
    unsigned long mStartTimestamp;
    class Listener *mListener;
};

#endif
