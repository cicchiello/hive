#ifndef AudioActuator_h
#define AudioActuator_h


#include <Actuator.h>
#include <pulsegen_consumer.h>

class Str;


class AudioActuator : public Actuator, private PulseGenConsumer {
 public:
    AudioActuator(const char *name, unsigned long now);
    ~AudioActuator();

    bool isItTimeYet(unsigned long now) const;

    bool loop(unsigned long now, Mutex *wifi);
    
    PulseGenConsumer *getPulseGenConsumer() {return this;}
    
 protected:
    void pulse(unsigned long now);

    virtual const char *className() const {return "AudioActuator";}

    enum State {
        Idle=0, RecordingRequested, InitRecording, Recording, RecordingDone, LastState
    };
    static const char *sStateStrings[];
    
    bool mAcceptInput;
    unsigned int mSampleCnt;
    unsigned long mDuration, mStopTime, mNextVisitTime;
    State mState;
    class Sine *mSine;

    unsigned int mIndex;
    unsigned char *mBufs[2], *mCurrBuf, *mBufToStream;
};


#endif
