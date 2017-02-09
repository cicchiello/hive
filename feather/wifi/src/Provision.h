#ifndef provision_h
#define provision_h

#include <Actuator.h>
#include <str.h>


class HiveConfig;
class ConfigReader;
class ConfigWriter;

class Provision : public Actuator {
 public:
    Provision(const char *resetCause, const char *version, const char *configFile, unsigned long now);
    ~Provision();
    
    void startConfiguration();
    
    bool isItTimeYet(unsigned long now) const;
    
    bool loop(unsigned long now, Mutex *wifi);

    const HiveConfig &getConfig() const {return *mConfig;}
    
 private:
    bool loadLoop(unsigned long now);
    bool writeLoop(unsigned long now);
    bool downloadLoop(unsigned long now, Mutex *wifi);
    bool uploadLoop(unsigned long now, Mutex *wifi);
    
    const char *className() const {return "Provision";}

    unsigned long mNextActionTime;
    int mMajorState, mMinorState, mPostWriteState, mRetryCnt;
    ConfigReader *mConfigReader;
    ConfigWriter *mConfigWriter;
    HiveConfig *mConfig;
    Str mResetCause, mVersionId, mConfigFilename;
    class ConfigGetter *mGetter;
    class HttpCouchPut *mPutter;
};

#endif
