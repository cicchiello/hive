#ifndef seq_getter_h
#define seq_getter_h


#include <str.h>
#include <couchutils.h>

class HiveConfig;
class HttpJSONPost;


class SeqGetter {
 public:
    SeqGetter(const HiveConfig &config, unsigned long now, const Str &hiveChannelDocId);
    ~SeqGetter();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);

    bool hasLastSeqId() const {return mHasLastSeqId;}
    const Str &getLastSeqId() const {return mLastSeq;}
    const Str &getRevision() const {return mRevision;}
    
 protected:
    const char *className() const {return "SeqGetter";}
    
 private:
    HttpJSONPost *createGetter(const HiveConfig &);

    bool processGetter(unsigned long now, unsigned long *callMeBackIn_ms);

    bool mGetNextSet, mHasLastSeqId, mError;
    Str mLastSeq, mRevision;
    CouchUtils::Doc mSelectorDoc;
    unsigned long mNextActionTime;
    const HiveConfig &mConfig;
    HttpJSONPost *mGetter;
};


#endif
