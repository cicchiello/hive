#ifndef config_persister_h
#define config_persister_h


#include <Sensor.h>
#include <couchutils.h>

class Mutex;
class HiveConfig;
class DocWriter;

class ConfigPersister : public Sensor {
 public:
    ConfigPersister(const HiveConfig &config,
		    const class RateProvider &rateProvider,
		    const char *configFilename,
		    unsigned long now, Mutex *sdMutex);
    ~ConfigPersister();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);

    void persist();
    
 protected:
    const char *className() const {return "ConfigPersister";}
    
 private:
    unsigned long mNextActionTime;
    bool mDoPersist;
    Mutex *mSdMutex;
    DocWriter *mWriter;
    const char *mConfigFilename;
    const HiveConfig &mConfig;
};


#endif
