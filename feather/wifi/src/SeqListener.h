#ifndef seq_listener_h
#define seq_listener_h


#include <strbuf.h>
#include <str.h>
#include <couchutils.h>

class HiveConfig;
class HttpJSONPost;
class HttpCouchGet;
class TimeProvider;


class SeqListener {
 public:
    SeqListener(HiveConfig *config, unsigned long now, const TimeProvider **timeProvider, const Str &hiveChannelId);
    ~SeqListener();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);


    void restartListening();

    bool isOnline() const {return mIsOnline;}
    
    bool hasLastSeqId() const {return mHasLastSeqId;}
    const Str &getLastSeqId() const {return mLastSeqId;}
    const Str &getLastRevision() const {return mLastRevision;}
    
    bool hasPayload() const {return mHasPayload;}
    const StrBuf &getMsgId() const {return mMsgId;}
    const StrBuf &getPrevMsgId() const {return mPrevMsgId;}
    const StrBuf &getRevision() const {return mRevision;}
    const StrBuf &getPayload() const {return mPayload;}
    const StrBuf &getTimestamp() const {return mTimestamp;}

    static Str APP_CHANNEL_MSGID_PROPNAME;
    
 protected:
    const char *className() const {return "SeqListener";}

    HttpJSONPost *createChangeListener();
    bool processListener(unsigned long now, unsigned long *callMeBackIn_ms);

    HttpCouchGet *createMsgObjGetter(const char *urlPieces[]);
    bool processMsgObjGetter(unsigned long now, unsigned long *callMeBackIn_ms, bool isHeader);
    
 private:
    HiveConfig &mConfig;
    Str mHiveChannelId, mLastSeqId, mLastRevision, mChannelHeaderUrl;
    unsigned long mNextActionTime;
    bool mHasLastSeqId, mListenerTimedOut, mHasPayload, mIsOnline, mHasMsgObj;
    int mState;
    CouchUtils::Doc mSelectorDoc;
    const TimeProvider **mTimeProvider;

    // instruction content
    StrBuf mRevision, mMsgId, mPrevMsgId, mPayload, mTimestamp;
    
    class SeqGetter *mSequenceGetter;
    class HttpJSONPost *mChangeListener;
    class HttpCouchGet *mMsgObjGetter;
};


#endif
