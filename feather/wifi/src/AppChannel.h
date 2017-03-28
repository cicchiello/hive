#ifndef appchannel_h
#define appchannel_h

#include <str.h>
#include <strbuf.h>
#include <couchutils.h>

#include <TimeProvider.h>

class HiveConfig;
class Mutex;


class AppChannel {
 public:
    AppChannel(const HiveConfig &config, unsigned long now, TimeProvider **timeProvider, Mutex *wifiMutex, Mutex *sdMutex);
    ~AppChannel();

    // returns true when it has the time; false to be called back later
    bool loop(unsigned long now);
    
    const char *getName() const {return "appchannel";}
    // since there can be only one

    bool isOnline() const {return mWasOnline;}
    
    bool haveMessage() const {return mHavePayload;}

    void consumePayload(StrBuf *payload, Mutex *alreadyOwnedSdMutex);

 private:
    bool loadPrevMsgId();
    bool getterLoop(unsigned long now, Mutex *wifiMutex, bool gettingHeader);
    bool processDoc(const CouchUtils::Doc &doc, bool gettingHeader, unsigned long now, unsigned long *callMeBackIn_ms);

    unsigned long mNextAttempt, mStartTime;
    int mState, mRetryCnt;

    const HiveConfig &mConfig;
    bool mInitialMsg, mHavePayload, mWasOnline;
    StrBuf mPrevMsgId, mNewMsgId, mPayload, mPrevETag;
    Str mChannelUrl;

    TimeProvider **mTimeProvider;
    
    class AppChannelGetter *mGetter;
    Mutex *mWifiMutex, *mSdMutex;
};


#endif
