#ifndef config_writer_h
#define config_writer_h

#include <couchutils.h>

class ConfigWriter {
  public:
    ConfigWriter(const char *filename, const CouchUtils::Doc &config);
    ~ConfigWriter();

    // returns true on success
    bool setup() {}

    // on true, call back later; else done
    bool loop();

    // error message iff loop returns false
    const char *errMsg() const {return mErrMsg->c_str();}

    bool isDone() const {return mIsDone;}
    bool hasError() const {return !mSuccess;}

    const CouchUtils::Doc &getConfig() {return mConfig;}
    
 private:
    ConfigWriter(const ConfigWriter &); // unimplemented
    
    const CouchUtils::Doc mConfig;
    Str *mErrMsg, *mFilename;
    bool mIsDone, mSuccess;
};

#endif
