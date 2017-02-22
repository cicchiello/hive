#ifndef doc_reader_h
#define doc_reader_h

#include <couchutils.h>

class DocReader {
  public:
    DocReader(const char *filename);
    ~DocReader();

    // returns true on success
    bool setup() {}

    // on true, call back later; else done
    bool loop();

    // error message iff loop returns false
    const char *errMsg() const {return mErrMsg->c_str();}

    bool hasDoc() const {return mHasDoc;}

    const CouchUtils::Doc &getDoc() const {return mDoc;}
    
 private:
    CouchUtils::Doc mDoc;
    Str *mErrMsg, *mFilename;
    bool mHasDoc, mIsDone;
};

#endif
