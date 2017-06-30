#ifndef appchannel_h
#define appchannel_h

#include <str.h>
#include <strbuf.h>
#include <couchutils.h>

class HiveConfig;
class Mutex;
class TimeProvider;

class AppChannel {
 public:
    AppChannel(HiveConfig *config, unsigned long now, TimeProvider **timeProvider, Mutex *wifiMutex, Mutex *sdMutex);
    ~AppChannel();

    // returns true when it has the time; false to be called back later
    bool loop(unsigned long now);
    
    const char *getName() const {return "appchannel";}
    // since there can be only one

    bool isOnline() const {return mWasOnline;}
    
    bool haveMessage() const {return mHavePayload;}

    void getPayload(StrBuf *payload);
    void consumePayload(StrBuf *payload);

 private:
    bool loadPrevMsgId();
    bool getterLoop(unsigned long now, Mutex *wifiMutex, bool gettingHeader);
    bool processDoc(const CouchUtils::Doc &doc, bool gettingHeader, unsigned long now, unsigned long *callMeBackIn_ms);

    void setPrevMsgId(const char *prevMsgId);
    
    unsigned long mNextAttempt, mStartTime;
    int mState, mRetryCnt;

    HiveConfig *mConfig;
    bool mInitialMsg, mHavePayload, mWasOnline;
    StrBuf mPrevMsgId, mNewMsgId, mPayload, mPrevETag;
    Str mChannelUrl;

    TimeProvider **mTimeProvider;
    
    class AppChannelGetter *mGetter;
    Mutex *mWifiMutex, *mSdMutex;
};


#endif
