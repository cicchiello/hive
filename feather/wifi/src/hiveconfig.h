#ifndef hive_config_h
#define hive_config_h

#include <couchutils.h>

class HiveConfig {
 public:
    // constucts a default config
    HiveConfig(const char *resetCause, const char *versionId);
    ~HiveConfig();
    
    void setDefault();

    // returns true if the resulting config is valid; otherwise it leaves this config unchanged
    bool setDoc(const CouchUtils::Doc &doc);

    const CouchUtils::Doc &getDoc() const {return mDoc;}

    void print() const;
    
    bool isValid() const;

    static const char *HiveIdProperty;
    static const char *HiveFirmwareProperty;
    static const char *TimestampProperty;
    static const char *SsidProperty;
    static const char *SsidPswdProperty;
    static const char *DbHostProperty;
    static const char *DbPortProperty;
    static const char *IsSslProperty;
    static const char *DbUserProperty;
    static const char *DbPswdProperty;
 
    const char *getHiveId() const;
    unsigned long getConfigTimestamp() const;
    
    const char *getSSID() const;
    const char *getPSWD() const;
    const char *getDbHost() const;
    const int getDbPort() const;
    const int isSSL() const;
    const char *getDbUser() const;
    const char *getDbPswd() const;

    const char *getLogDbName() const;
    const char *getConfigDbName() const;
    const char *getChannelDbName() const;
    const char *getDesignDocId() const;
    const char *getSensorByHiveViewName() const;

    const char *getResetCause() const {return mRCause.c_str();}
    const char *getVersionId() const {return mVersionId.c_str();}

    bool addProperty(const char *name, const char *value);
    const char *getProperty(const char *name) const;

    class UpdateFunctor {
    public:
      virtual ~UpdateFunctor() {}
      virtual void onUpdate(const HiveConfig &) = 0;
    };

    // takes ownership of supplied functor, and gives up ownership of the returned functor
    UpdateFunctor *onUpdate(UpdateFunctor *f);
    
    static bool docIsValidForConfig(const CouchUtils::Doc &d);
    
 private:
    CouchUtils::Doc mDoc;
    Str mRCause, mVersionId;

    static UpdateFunctor *mUpdateFunctor;
};

#endif
