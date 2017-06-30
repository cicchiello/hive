#ifndef AttachmentUpload_h
#define AttachmentUpload_h


#include <SensorBase.h>
#include <str.h>

class AttachmentUpload : public SensorBase {
 protected:
    AttachmentUpload(const HiveConfig &config,
		     const char *sensorName,
		     const char *contentType,
		     const class RateProvider &rateProvider,
		     unsigned long now,
		     Mutex *wifiMutex, Mutex *sdMutex);

 public:
    ~AttachmentUpload();

    bool isItTimeYet(unsigned long now);
    
    bool loop(unsigned long now);

    bool sensorSample(Str *value);

    Mutex *getSdMutex() const {return mSdMutex;}

    virtual const char *logValue() const = 0;
    
    virtual bool processResult(const HttpJSONConsumer &consumer, unsigned long *callMeBackIn_ms,
			       bool *keepMutex, bool *success);

    bool isAttachmentUploadDone() const {return mIsDone;}
    
 protected:
    void setAttachmentName(const Str &attachmentName) {mAttachmentName = attachmentName;}
    void setFilename(const Str &filename) {mFilename = filename;}
    
 private:
    AttachmentUpload(const AttachmentUpload &); // intentionally unimplemented
    const AttachmentUpload &operator=(const AttachmentUpload &o); // intentionallly unimplemented
    
    unsigned long mTransferStart;
    class AttachmentDataProvider *mDataProvider;
    class HttpBinaryPut *mBinaryPutter;
    bool mHaveDocId, mIsDone;
    Str mDocId, mRevision, mAttachmentName, mContentType, mFilename;
    Mutex *mSdMutex;
};


#endif
