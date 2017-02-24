#ifndef config_uploader_h
#define config_uploader_h


#include <Sensor.h>

class Str;
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
    HttpCouchPut *createPutter(const HiveConfig &, unsigned long now, const char *id, const char *rev);
    bool processGetter(const HiveConfig &, unsigned long now, unsigned long *callMeBackIn_ms);
    bool processPutter(unsigned long now, unsigned long *callMeBackIn_ms);
    
    unsigned long mNextActionTime;
    bool mDoUpload;
    const HiveConfig &mConfig;
    class ConfigGetter *mGetter;
    HttpCouchPut *mPutter;
    Mutex *mWifiMutex;
};


#endif
