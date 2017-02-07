#ifndef hive_config_h
#define hive_config_h

#include <couchutils.h>

class HiveConfig {
 public:
    // constucts a default config
    HiveConfig(const char *resetCause, const char *versionId);

    void setDefault();
    void setDoc(const CouchUtils::Doc &doc);

    const CouchUtils::Doc &getDoc() const {return mDoc;}

    void print() const;
    
    bool isValid() const;

    const char *getSSID() const;
    const char *getPSWD() const;
    const char *getDbHost() const;
    const int getDbPort() const;
    const int isSSL() const;
    const char *getDbCredentials() const;

    const char *getHiveId() const {return "F0-17-66-FC-5E-A1";} // for now -- eventually use mcu id or mac address

    const char *getLogDbName() const;
    const char *getDesignDocId() const;
    const char *getSensorByHiveViewName() const;

    const char *getResetCause() const {return mRCause.c_str();}
    const char *getVersionId() const {return mVersionId.c_str();}
    
 private:
    CouchUtils::Doc mDoc;
    Str mRCause, mVersionId;
};

#endif
