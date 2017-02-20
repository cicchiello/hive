#ifndef provision_h
#define provision_h

class HiveConfig;
class Mutex;

class Provision {
 public:
    Provision(const char *resetCause, const char *version, const char *configFile, unsigned long now);
    ~Provision();

    void start();
    void forcedStart(); // run AP, regardless of validity of config
    void stop();
    
    bool loop(unsigned long now, Mutex *wifi);

    bool hasConfig() const;
    bool isStarted() const;
    
    const HiveConfig &getConfig() const;
    
 private:
    const char *className() const {return "Provision";}

    class ProvisionImp *mImp;
};

#endif
