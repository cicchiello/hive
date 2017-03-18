#ifndef provision_h
#define provision_h

class HiveConfig;
class Mutex;

class Provision {
 public:
    Provision(const char *resetCause, const char *version, const char *configFile,
	      unsigned long now, Mutex *wifiMutex);
    ~Provision();

    void start(unsigned long now);
    
    void forcedStart(unsigned long now); // run AP, regardless of validity of config
    void stop();
    
    bool loop(unsigned long now);

    bool hasConfig() const;
    bool isStarted() const;

    unsigned long getStartTime() const;
    
    const HiveConfig &getConfig() const;
    HiveConfig &getConfig();
    
    const char *className() const {return "Provision";}

 private:
    class ProvisionImp *mImp;
    Mutex *mWifiMutex;
};

#endif
