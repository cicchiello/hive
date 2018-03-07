#ifndef hive_config_h
#define hive_config_h

#include <couchutils.h>

class HiveConfig {
 public:
    // constucts a default config
    HiveConfig(const char *resetCause, const char *versionId);
    ~HiveConfig();

    
    // Hardware versioning: 
    //   versions correspond to Fritzing version numbers
    //    i.e.  *_2_4 corresponds to hive_v2.4.fzz
    static const char *HDWR_VERSION_2_2;
    static const char *HDWR_VERSION_2_4;
    
    // must be called before GPIO lines are being used for anything else
    static void determineHdwrVersion();
    
    // can be called anytime after determineHdwrVersion has been called
    static const Str &getHdwrVersion();
    

    void setDefault();

    // returns true if the resulting config is valid; otherwise it leaves this config unchanged
    bool setDoc(const CouchUtils::Doc &doc);

    const CouchUtils::Doc &getDoc() const {return mDoc;}

    void print() const;
    
    bool isValid() const;

    static const Str HiveIdProperty;
    static const Str HiveFirmwareProperty;
    static const Str HiveHdwrVerProperty;
    static const Str TimestampProperty;
    static const Str SsidProperty;
    static const Str SsidPswdProperty;
    static const Str DbHostProperty;
    static const Str DbPortProperty;
    static const Str IsSslProperty;
    static const Str DbUserProperty;
    static const Str DbPswdProperty;
 
    const Str &getHiveId() const;
    unsigned long getConfigTimestamp() const;

    const Str &getSSID() const;
    const Str &getPSWD() const;
    const Str &getDbHost() const;
    const int getDbPort() const;
    const int isSSL() const;
    const Str &getDbUser() const;
    const Str &getDbPswd() const;

    const Str &getLogDbName() const;
    const Str &getConfigDbName() const;
    const Str &getChannelDbName() const;
    const Str &getDesignDocId() const;
    const Str &getSensorByHiveViewName() const;

    const Str &getResetCause() const {return mRCause;}
    const Str &getVersionId() const {return mVersionId;}

    bool addProperty(const Str &name, const char *value);
    bool hasProperty(const Str &name) const;
    const Str &getProperty(const Str &name) const;

    class UpdateFunctor {
    public:
      virtual ~UpdateFunctor() {}
      virtual void onUpdate(const HiveConfig &) = 0;
    };

    // takes ownership of supplied functor, and gives up ownership of the returned functor
    UpdateFunctor *onUpdate(UpdateFunctor *f);
    
    static bool docIsValidForConfig(const CouchUtils::Doc &d);
    
 private:
    HiveConfig(const HiveConfig &); // intentionally unimplemented
    HiveConfig &operator=(const HiveConfig &); // intentionally unimplemented
    
    CouchUtils::Doc mDoc;
    Str mRCause, mVersionId;

    static UpdateFunctor *mUpdateFunctor;
};

#endif
