#ifndef appchannel_h
#define appchannel_h

#include <str.h>

class HiveConfig;
class Mutex;


class AppChannel {
 public:
    AppChannel(const HiveConfig &config);
    ~AppChannel();

    // returns true when it has the time; false to be called back later
    bool loop(unsigned long now, Mutex *wifiMutex);
    
    const char *getName() const {return "appchannel";}
    // since there can be only one

    bool haveMessage() const {return mHavePayload;}
    
    void consumePayload(Str *payload);
    
 private:
    bool loadPrevMsgId();
    bool getterLoop(unsigned long now, Mutex *wifiMutex, bool gettingHeader);

    unsigned long mNextAttempt;
    int mState, mRetryCnt;

    const HiveConfig &mConfig;
    bool mInitialMsg, mHavePayload;
    Str mPrevMsgId, mNewMsgId, mPayload;
    
    class HttpCouchGet *mGetter;
};

#endif
