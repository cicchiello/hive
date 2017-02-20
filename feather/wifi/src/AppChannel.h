#ifndef appchannel_h
#define appchannel_h

#include <str.h>
#include <couchutils.h>

class HiveConfig;
class Mutex;


class AppChannel {
 public:
    AppChannel(const HiveConfig &config, unsigned long now);
    ~AppChannel();

    // returns true when it has the time; false to be called back later
    bool loop(unsigned long now, Mutex *wifiMutex);
    
    const char *getName() const {return "appchannel";}
    // since there can be only one

    bool isOnline() const {return mIsOnline;}
    
    bool haveMessage() const {return mHavePayload;}

    unsigned long getOfflineTime() const {return mOfflineTime;}
    
    void consumePayload(Str *payload);
    
 private:
    bool loadPrevMsgId();
    bool getterLoop(unsigned long now, Mutex *wifiMutex, bool gettingHeader);
    bool processDoc(const CouchUtils::Doc &doc, bool gettingHeader, unsigned long *callMeBackIn_ms);

    unsigned long mNextAttempt, mOfflineTime, mStartTime;
    int mState, mRetryCnt;

    const HiveConfig &mConfig;
    bool mInitialMsg, mHavePayload, mIsOnline;
    Str mPrevMsgId, mNewMsgId, mPayload;
    
    class HttpCouchGet *mGetter;
};

#endif
