#ifndef AudioUpload_h
#define AudioUpload_h


#include <SensorBase.h>

class AudioUpload : public SensorBase {
 public:

    AudioUpload(const HiveConfig &config,
		const char *sensorName,
		const char *attachmentDescription,
		const char *attachmentName,
		const char *contentType,
		const char *filename,
		const class RateProvider &rateProvider,
		unsigned long now,
		Mutex *wifiMutex, Mutex *sdMutex);
    ~AudioUpload();

    bool isItTimeYet(unsigned long now);

    bool loop(unsigned long now);

    bool sensorSample(Str *value);

    Mutex *getSdMutex() const {return mSdMutex;}
    
    virtual bool processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms,
			       bool *keepMutex, bool *success);

 private:
    AudioUpload(const AudioUpload &); // intentionally unimplemented
    const AudioUpload &operator=(const AudioUpload &o); // intentionallly unimplemented
    
    const char *className() const {return "AudioUpload";}

    unsigned long mTransferStart;
    class MyDataProvider *mDataProvider;
    class HttpBinaryPut *mBinaryPutter;
    bool mHaveDocId, mIsDone;
    Str *mDocId, *mRevision, *mAttachmentName, *mContentType, *mFilename, *mAttDesc;
    Mutex *mSdMutex;
};


#endif
