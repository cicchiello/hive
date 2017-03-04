#ifndef config_uploader_h
#define config_uploader_h


#include <Sensor.h>
#include <couchutils.h>

class Mutex;
class HiveConfig;
class HttpCouchPut;

class ConfigUploader : public Sensor {
 public:
    ConfigUploader(const HiveConfig &config,
		   const class RateProvider &rateProvider,
		   const class TimeProvider &timeProvider,
		   unsigned long now, Mutex *wifi);
    ~ConfigUploader();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);

    void upload();
    
 protected:
    const char *className() const {return "ConfigUploader";}
    
 private:
    class ConfigGetter *createGetter(const HiveConfig &);
    HttpCouchPut *createPutter(const HiveConfig &, CouchUtils::Doc *docToUpload, const char *id);
    bool processGetter(const HiveConfig &, unsigned long now, unsigned long *callMeBackIn_ms);
    bool processPutter(unsigned long now, unsigned long *callMeBackIn_ms);
    void prepareDocToUpload(const CouchUtils::Doc &existingDoc, CouchUtils::Doc *newDoc,
			    unsigned long now, const char *rev);
    
    unsigned long mNextActionTime;
    bool mDoUpload;
    const HiveConfig &mConfig;
    class ConfigGetter *mGetter;
    CouchUtils::Doc *mDocToUpload;
    HttpCouchPut *mPutter;
    Mutex *mWifiMutex;
};


#endif
