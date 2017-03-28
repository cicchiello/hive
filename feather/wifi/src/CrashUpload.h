#ifndef CrashUpload_h
#define CrashUpload_h


#include <AttachmentUpload.h>

class CrashUpload : public AttachmentUpload {
 public:
    CrashUpload(const HiveConfig &config,
		const char *sensorName,
		const char *attachmentName,
		const char *contentType,
		const class RateProvider &rateProvider,
		unsigned long now,
		Mutex *wifiMutex, Mutex *sdMutex);

    ~CrashUpload();

    const char *logValue() const;
    
    bool loop(unsigned long now);
    
    const char *className() const {return "CrashUpload";}
    
 private:
    bool mFirstEntry, mHaveFileToUpload, mDeleteFile;
};


#endif
