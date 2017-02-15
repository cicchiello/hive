#ifndef AudioUpload_h
#define AudioUpload_h


#include <SensorBase.h>

class AudioUpload : public SensorBase {
 public:

    AudioUpload(const HiveConfig &config,
		const char *sensorName,
		const char *attachmentName,
		const char *contentType,
		const char *filename,
		const class RateProvider &rateProvider,
		const class TimeProvider &timeProvider,
		unsigned long now);
    ~AudioUpload();

    bool isItTimeYet(unsigned long now);

    bool loop(unsigned long now, Mutex *wifi);
    
    bool sensorSample(Str *value);

    virtual bool processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms);
    
 private:
    AudioUpload(const AudioUpload &); // intentionally unimplemented
    const AudioUpload &operator=(const AudioUpload &o); // intentionallly unimplemented
    
    const char *className() const {return "AudioUpload";}

    unsigned long mTransferStart;
    class MyDataProvider *mDataProvider;
    class HttpBinaryPut *mBinaryPutter;
    bool mHaveDocId, mIsDone;
    Str *mDocId, *mRevision, *mAttachmentName, *mContentType, *mFilename;
};


#endif
