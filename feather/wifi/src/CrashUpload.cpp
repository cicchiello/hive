#include <CrashUpload.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <sdutils.h>

#include <SdFat.h>

#include <hive_platform.h>

CrashUpload::CrashUpload(const HiveConfig &config,
			 const char *sensorName,
			 const char *attachmentName,
			 const char *contentType,
			 const class RateProvider &rateProvider,
			 unsigned long now,
			 Mutex *wifiMutex, Mutex *sdMutex)
  : AttachmentUpload(config, sensorName, contentType, rateProvider, now, wifiMutex, sdMutex),
    mFirstEntry(true), mDeleteFile(false), mHaveFileToUpload(false)
{
    TF("CrashUpload::CrashUpload");
    setAttachmentName(attachmentName);
}


CrashUpload::~CrashUpload()
{
}


const char *CrashUpload::logValue() const
{
    return "crash-report";
}


bool CrashUpload::loop(unsigned long now)
{
    TF("CrashUpload::loop");

    bool callMeBack = true;
    if (mFirstEntry) {
        if (getSdMutex()->own(this)) {
	    mFirstEntry = false;
	    
	    SdFat sd;
	    SDUtils::initSd(sd);
	    if (sd.exists(HivePlatform::STACKTRACE_FILENAME)) {
	        TRACE2("Found ", HivePlatform::STACKTRACE_FILENAME);
		setFilename(HivePlatform::STACKTRACE_FILENAME);
		mHaveFileToUpload = true;
		setNextPostTime(now + 500l);  // call me back in .5s to attempt the upload
	    } else {
	        TRACE("No stack trace file found; nothing to upload");
	        mHaveFileToUpload = false;
		setNextPostTime(now+1000000000); // never call me back
		callMeBack = false;
	    }
	    getSdMutex()->release(this);
	} else {
	    setNextPostTime(now + 600l);
	}
    } else if (mHaveFileToUpload) {
        callMeBack = AttachmentUpload::loop(now);
	
	if (isAttachmentUploadDone()) {
	    mHaveFileToUpload = false;
	    mDeleteFile = true;
	    callMeBack = true;
	}
    } else if (mDeleteFile) {
        if (getSdMutex()->own(this)) {
	    SdFat sd;
	    SDUtils::initSd(sd);
	    sd.remove(HivePlatform::STACKTRACE_FILENAME);
	    setNextPostTime(now+1000000000); // never call me back
	    callMeBack = false;
	    getSdMutex()->release(this);
	}
    }
    
    return callMeBack;
}

