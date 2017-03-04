#include <ListenSensor.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <str.h>
#include <strbuf.h>

#include <hive_platform.h>

#include <listener.h>
#include <Mutex.h>
#include <TimeProvider.h>


#define NOOP            0
#define RECORDING       1
#define UPLOADING       3


#define DEFAULT_ATTACHMENT_NAME  "listen.wav"
#define ATTACHMENT_CONTENT_TYPE  "audio/wav"
#define CONTENT_FILENAME         "/LISTEN.WAV"

ListenSensor::ListenSensor(const HiveConfig &config,
			   const char *name, 
			   const RateProvider &rateProvider,
			   const TimeProvider &timeProvider,
			   unsigned long now,
			   int ADCPIN, int BIASPIN,
			   Mutex *wifiMutex, Mutex *sdMutex)
  : SensorBase(config, name, rateProvider, timeProvider, now, wifiMutex),
    mState(NOOP), mStart(false), mUploader(NULL), mSdMutex(sdMutex),
    mListener(NULL), mMillisecondsToRecord(0), mADCPIN(ADCPIN), mBIASPIN(BIASPIN),
    mValue(new Str()), mAttName(new Str(DEFAULT_ATTACHMENT_NAME))
{
}


ListenSensor::~ListenSensor()
{
    TF("ListenSensor::~ListenSensor");
    assert(!mListener, "!mListener");
    delete mListener;

    delete mValue;
    delete mAttName;
}


void ListenSensor::start(int millisecondsToRecord, const char *attName)
{
    TF("ListenSensor::start");
    TRACE("entry");
    mStart = true;
    *mAttName = attName;
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

	      StrBuf m;
	      m.append(mMillisecondsToRecord);
	      *mValue = m.c_str();

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
		StrBuf attachmentDescription;
		attachmentDescription.append((int)(mMillisecondsToRecord/1000)).append("s-audio-clip");
		mState = UPLOADING;
		mUploader = new AudioUpload(getConfig(), getName(), attachmentDescription.c_str(),
					    mAttName->c_str(), ATTACHMENT_CONTENT_TYPE,
					    CONTENT_FILENAME, getRateProvider(), getTimeProvider(),
					    now, getWifiMutex(), getSdMutex());
	    }
	    getSdMutex()->release(this);
	    getWifiMutex()->release(this);
	    delete mListener;
	    mListener = NULL;
	}
    }
      break;

    case UPLOADING: {
        callMeBack = mUploader->loop(now);
	if (!callMeBack) {
	    Str timeStr;
	    getTimeProvider().toString(millis(), &timeStr);
	    PH2("Audio Capture Success!  Total time: ", (millis()-mStartTimestamp));
	    PH2("Time now: ", timeStr.c_str());
	    delete mUploader;
	    mUploader = NULL;
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
    StrBuf m;
    m.append(mMillisecondsToRecord);
    *mValue = m.c_str();
    return true;
}


extern void ADCPulseCallback();

void ListenSensor::pulse(unsigned long now)
{
    ADCPulseCallback();
}


