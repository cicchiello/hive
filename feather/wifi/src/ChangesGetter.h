#ifndef changes_getter_h
#define changes_getter_h


class Mutex;
class HiveConfig;
class StrBuf;

class ChangesGetter {
 public:
    ChangesGetter(const HiveConfig &config, unsigned long now, Mutex *wifi);
    ~ChangesGetter();

    bool isItTimeYet(unsigned long now);
    bool loop(unsigned long now);

    bool hasLastSeqId() const {return mHasLastSeqId;}
    const StrBuf &getLastSeqId() const {return *mLastSeq;}
    
 protected:
    const char *className() const {return "ChangesGetter";}
    
 private:
    class ChangeGetter *createGetter(const HiveConfig &);

    bool processGetter(unsigned long now, unsigned long *callMeBackIn_ms);

    bool mGetNextSet, mHasLastSeqId;
    StrBuf *mLastSeq;
    unsigned long mNextActionTime;
    const HiveConfig &mConfig;
    class ChangeGetter *mGetter;
    Mutex *mWifiMutex;
};


#endif
