#ifndef config_reader_h
#define config_reader_h

#include <couchutils.h>

class ConfigReader {
  public:
    ConfigReader(const char *filename);
    ~ConfigReader();

    // returns true on success
    bool setup() {}

    // on true, call back later; else done
    bool loop();

    // error message iff loop returns false
    const char *errMsg() const {return mErrMsg->c_str();}

    bool hasConfig() const {return mHasConfig;}

    const CouchUtils::Doc &getConfig() const {return mConfig;}
    
 private:
    CouchUtils::Doc mConfig;
    Str *mErrMsg, *mFilename;
    bool mHasConfig, mIsDone;
};

#endif
