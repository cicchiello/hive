#ifndef AudioActuator_h
#define AudioActuator_h


#include <Actuator.h>
#include <pulsegen_consumer.h>

class Str;
class BleStream;


class AudioActuator : public Actuator, private PulseGenConsumer {
 public:
    AudioActuator(const char *name, unsigned long now, BleStream *bleStream);
    ~AudioActuator();

    virtual void act(class Adafruit_BluefruitLE_SPI &ble);

    bool isItTimeYet(unsigned long now) const;
    void scheduleNextAction(unsigned long now);
    
    bool isMyCommand(const Str &response) const;
    void processCommand(Str *response);

    PulseGenConsumer *getPulseGenConsumer() {return this;}
    
 protected:
    void pulse(unsigned long now);

    void streamStart(Adafruit_BluefruitLE_SPI *ble, int bytesToFollow);
    void streamStop(Adafruit_BluefruitLE_SPI *ble);
    
    virtual const char *className() const {return "AudioActuator";}

    enum State {
        Idle=0, RecordingRequested, ReadyToRecord, Recording, RecordingDone, LastState
    };
    static const char *sStateStrings[];
    
    bool mInputEnabled;
    unsigned int mSampleCnt;
    unsigned long mDuration, mStopTime;
    State mState;
    class Sine *mSine;

    unsigned int mIndex;
    unsigned short *mBufs[2], *mCurrBuf;
    
    BleStream *mBle;
};


#endif
