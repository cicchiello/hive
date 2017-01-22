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

    void postToApp(const char *msg, Adafruit_BluefruitLE_SPI &ble);

    PulseGenConsumer *getPulseGenConsumer() {return this;}
    
 protected:
    void pulse(unsigned long now);

    void prepareAppMsg(Str *msg, int bytesToFollow);
    
    virtual const char *className() const {return "AudioActuator";}

    enum State {
        Idle=0, RecordingRequested, InitRecording, Recording, RecordingDone, LastState
    };
    static const char *sStateStrings[];
    
    bool mBleInputEnabled, mAcceptInput;
    unsigned int mSampleCnt;
    unsigned long mDuration, mStopTime;
    State mState;
    class Sine *mSine;

    unsigned int mIndex;
    unsigned short *mBufs[2], *mCurrBuf;
    
    BleStream *mBle;
};


#endif
