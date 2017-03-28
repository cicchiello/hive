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


#define NOOP            0
#define RECORDING       1
#define UPLOADING       3


#define DEFAULT_ATTACHMENT_NAME  "listen.wav"
#define ATTACHMENT_CONTENT_TYPE  "audio/wav"
#define CONTENT_FILENAME         "/LISTEN.WAV"

ListenSensor::ListenSensor(const HiveConfig &config,
			   const char *name, 
			   const RateProvider &rateProvider,
			   unsigned long now,
			   int ADCPIN, int BIASPIN,
			   Mutex *wifiMutex, Mutex *sdMutex)
  : SensorBase(config, name, rateProvider, now, wifiMutex),
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
    case NOOP:
      if (mStart) {
	  bool muticesOwned = (getWifiMutex()->whoOwns() == this) && (getSdMutex()->whoOwns() == this);
	  if (!muticesOwned) {
	      bool muticesAvailable = getWifiMutex()->isAvailable() && getSdMutex()->isAvailable();
	      if (muticesAvailable) {
		  bool ownWifi = getWifiMutex()->own(this);
		  bool ownSd = getSdMutex()->own(this);
		  assert(ownWifi && ownSd, "ownWifi && ownSd");
		  muticesOwned = true;
	      }
	  }
	  if (muticesOwned) {
	      TRACE("Acquired both mutices");
	      mListener = new Listener(mADCPIN, mBIASPIN);
	      bool stat = mListener->record(mMillisecondsToRecord, "/LISTEN.WAV", true);
	      callMeBackIn_ms = 1l;

	      assert(stat, "Couldn't start recording");

	      StrBuf m;
	      m.append(mMillisecondsToRecord);
	      *mValue = m.c_str();

	      PH2("Recording; now: ", now);
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
	    GlobalYield();
	    stat = mListener->loop(true);
	}
	
	if (mListener->isDone()) {
	    if (mListener->hasError()) {
	        PH2("Audio capture Failed: ", mListener->getErrmsg());
		callMeBack = false;
		mState = NOOP;
		getSdMutex()->release(this);
		getWifiMutex()->release(this);
	    } else {
	        PH2("Audio Capture complete; proceeding to upload it; now:", millis());
		StrBuf attachmentDescription;
		attachmentDescription.append((int)(mMillisecondsToRecord/1000)).append("s-audio-clip");
		mState = UPLOADING;
		mUploader = new AudioUpload(getConfig(), getName(), attachmentDescription.c_str(),
					    mAttName->c_str(), ATTACHMENT_CONTENT_TYPE,
					    CONTENT_FILENAME, getRateProvider(), 
					    now, getWifiMutex(), getSdMutex());
		
		// pass ownership of the mutices to the uploader
		getSdMutex()->release(this);
		getWifiMutex()->release(this);
		bool uploaderOwnsSdMutex = getSdMutex()->own(mUploader);
		bool uploaderOwnsWifiMutex = getWifiMutex()->own(mUploader);
		assert(uploaderOwnsSdMutex && uploaderOwnsWifiMutex, "Mutices *not* passed to AudioUpload object");
	    }

	    delete mListener;
	    mListener = NULL;
	    GlobalYield();
	}
    }
      break;

    case UPLOADING: {
        callMeBack = mUploader->loop(now);
	if (!callMeBack) {
	    PH2("Audio Capture Success!  Total time: ", (millis()-mStartTimestamp));
	    PH2("Time now: ", millis());
	    delete mUploader;
	    mUploader = NULL;
	    mState = NOOP;
	    assert(getSdMutex()->whoOwns() == NULL, "SD Mutex hasn't been released");
	    assert(getWifiMutex()->whoOwns() == NULL, "Wifi Mutex hasn't been released");
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


