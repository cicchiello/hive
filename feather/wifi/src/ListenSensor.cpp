#include <ListenSensor.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <str.h>

#include <hive_platform.h>

#include <listener.h>
#include <Mutex.h>
#include <TimeProvider.h>


#define NOOP            0
#define RECORDING       1
#define UPLOADING       3

ListenSensor::ListenSensor(const HiveConfig &config,
			   const char *name, 
			   const RateProvider &rateProvider,
			   const TimeProvider &timeProvider,
			   unsigned long now,
			   int ADCPIN, int BIASPIN,
			   Mutex *wifiMutex, Mutex *sdMutex)
  : AudioUpload(config, name, "listen.wav", "audio/wav", "/LISTEN.WAV", rateProvider, timeProvider,
		now, wifiMutex, sdMutex),
    mState(NOOP), mStart(false),
    mListener(0), mMillisecondsToRecord(0), mADCPIN(ADCPIN), mBIASPIN(BIASPIN), mValue(new Str())
{
}


ListenSensor::~ListenSensor()
{
    TF("ListenSensor::~ListenSensor");
    assert(!mListener, "!mListener");
    delete mListener;

    delete mValue;
}


void ListenSensor::start(int millisecondsToRecord, const char *attName)
{
    TF("ListenSensor::start");
    TRACE("entry");
    mStart = true;
    setAttachmentName(attName);
    mMillisecondsToRecord = millisecondsToRecord;
}


bool ListenSensor::isItTimeYet(unsigned long now)
{
    switch (mState) {
    case NOOP: return mStart;
      
    case RECORDING: 
    case UPLOADING: return now > getNextPostTime();
      
    default: assert(false, "Invalid state");
    }
}


bool ListenSensor::loop(unsigned long now)
{
    TF("ListenSensor::loop");
    
    bool callMeBack = true;
    unsigned long callMeBackIn_ms = 50l;
    switch (mState) {
    case NOOP: if (mStart) {
	  bool muticesOwned = (getWifiMutex()->whoOwns() == this) && (getSdMutex()->whoOwns() == this);
	  if (!muticesOwned) {
	      bool muticesAvailable = getWifiMutex()->isAvailable() && getSdMutex()->isAvailable();
	      if (muticesAvailable) {
	          TRACE("Both Mutexes aquired");
		  bool ownWifi = getWifiMutex()->own(this);
		  bool ownSd = getSdMutex()->own(this);
		  assert(ownWifi && ownSd, "ownWifi && ownSd");
		  muticesOwned = true;
	      }
	  }
	  if (muticesOwned) {
	      TRACE("Acquired both mutexes");
	      mListener = new Listener(mADCPIN, mBIASPIN);
	      bool stat = mListener->record(mMillisecondsToRecord, "/LISTEN.WAV", true);
	      HivePlatform::nonConstSingleton()->registerPulseGenConsumer_22K(getPulseGenConsumer());
	      callMeBackIn_ms = 1l;

	      assert(stat, "Couldn't start recording");

	      *mValue = "";
	      mValue->append(mMillisecondsToRecord);

	      TRACE("Recording...");
	      mStartTimestamp = now;
	      mState = RECORDING;
	      mStart = false;
	  }
      }
      break;

    case RECORDING: {
        assert(!mStart, "!mStart");

	bool stat = mListener->loop(true);
	while (!mListener->isDone()) {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    delay(5);
	    stat = mListener->loop(true);
	}
	
	if (mListener->isDone()) {
	    HivePlatform::nonConstSingleton()->unregisterPulseGenConsumer_22K(getPulseGenConsumer());
	    if (mListener->hasError()) {
	        PH2("Audio capture Failed: ", mListener->getErrmsg());
		callMeBack = false;
		mState = NOOP;
	    } else {
	        TRACE2("Audio Capture complete; proceeding to upload it:", (millis()-mStartTimestamp));
		mState = UPLOADING;
	    }
	    getSdMutex()->release(this);
	    getWifiMutex()->release(this);
	    delete mListener;
	    mListener = NULL;
	}
    }
      break;

    case UPLOADING: {
        callMeBack = AudioUpload::loop(now);
	if (!callMeBack) {
	    Str timeStr;
	    getTimeProvider().toString(millis(), &timeStr);
	    PH2("Audio Capture Success!  Total time: ", (millis()-mStartTimestamp));
	    PH2("Time now: ", timeStr.c_str());
	    mState = NOOP;
	}
    }
      break;
      
    default:
      assert2(false, "mState is invalid: ", mState);
    }
    
    setNextPostTime(now + callMeBackIn_ms);
    return callMeBack;
}


bool ListenSensor::sensorSample(Str *value)
{
    *mValue = "";
    mValue->append(mMillisecondsToRecord);
    *value = *mValue;
    return true;
}


extern void ADCPulseCallback();

void ListenSensor::pulse(unsigned long now)
{
    ADCPulseCallback();
}


