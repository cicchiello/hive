#ifndef appchannel_h
#define appchannel_h

#include <str.h>
#include <strbuf.h>
#include <couchutils.h>

#include <TimeProvider.h>

class HiveConfig;
class Mutex;


class AppChannel : public TimeProvider {
 public:
    AppChannel(const HiveConfig &config, unsigned long now, Mutex *wifiMutex, Mutex *sdMutex);
    ~AppChannel();

    // returns true when it has the time; false to be called back later
    bool loop(unsigned long now);
    
    const char *getName() const {return "appchannel";}
    // since there can be only one

    bool isOnline() const {return mWasOnline;}
    
    bool haveMessage() const {return mHavePayload;}

    void consumePayload(StrBuf *payload, Mutex *alreadyOwnedSdMutex);

    unsigned long secondsAtMark() const;
    unsigned long markTimestamp() const {return mTimestamp;}

    // TimeProvider API
    void toString(unsigned long now, Str *str) const;
    bool haveTimestamp() const;
    
 private:
    bool loadPrevMsgId();
    bool getterLoop(unsigned long now, Mutex *wifiMutex, bool gettingHeader);
    bool processDoc(const CouchUtils::Doc &doc, bool gettingHeader, unsigned long now, unsigned long *callMeBackIn_ms);

    unsigned long mNextAttempt, mStartTime, mTimestamp, mSecondsAtMark;
    int mState, mRetryCnt;

    const HiveConfig &mConfig;
    bool mInitialMsg, mHavePayload, mWasOnline, mHaveTimestamp;
    StrBuf mPrevMsgId, mNewMsgId, mPayload, mPrevETag;
    Str mChannelUrl;
    
    class AppChannelGetter *mGetter;
    Mutex *mWifiMutex, *mSdMutex;
};

inline
bool AppChannel::haveTimestamp() const
{
    return mHaveTimestamp;
}

inline
unsigned long AppChannel::secondsAtMark() const
{
    return mSecondsAtMark;
}

#endif
