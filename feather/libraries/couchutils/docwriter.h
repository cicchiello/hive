#ifndef doc_writer_h
#define doc_writer_h

#include <couchutils.h>

class Mutex;
class StrBuf;

class DocWriter {
  public:
    DocWriter(const char *filename, const CouchUtils::Doc &doc, Mutex *sdMutex);
    ~DocWriter();

    // returns true on success
    bool setup() {}

    // on true, call back later; else done
    bool loop();

    // error message iff loop returns false
    const char *errMsg() const;

    bool isDone() const {return mIsDone;}
    bool hasError() const {return !mSuccess;}

    const CouchUtils::Doc &getDoc() {return mDoc;}
    
 private:
    DocWriter(const DocWriter &); // unimplemented
    DocWriter &operator=(const DocWriter &); // unimplemented
    
    const CouchUtils::Doc mDoc;
    StrBuf *mErrMsg;
    Str *mFilename;
    bool mIsDone, mSuccess;
    Mutex *mSdMutex;
};

#endif
