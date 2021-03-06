#ifndef timestamp_h
#define timestamp_h

class Str;

class Mutex;


#include <TimeProvider.h>


class Timestamp : public TimeProvider {
 public:
    Timestamp(const char *ssid, const char *pswd, const char *dbHost, const int dbPort, bool isSSL,
	      const char *dbUser, const char *dbPswd);
    ~Timestamp();

    // returns true when it has the time; false to be called back later
    bool loop(unsigned long now, Mutex *wifi);
    
    const char *getName() const {return "timestamp";}
    // since there can be only one

    void toString(unsigned long now, Str *str) const;

    bool haveTimestamp() const;

    unsigned long secondsAtMark() const;
    unsigned long markTimestamp() const {return mTimestamp;}

 private:
    bool mHaveTimestamp;
    unsigned long mNextAttempt;
    unsigned long mTimestamp, mSecondsAtMark;

    const Str *mSsid, *mPswd, *mDbHost, *mDbUser, *mDbPswd;
    const int mDbPort;
    const bool mIsSSL;
    
    class RTCGetter *mGetter;
};


inline
bool Timestamp::haveTimestamp() const
{
    return mHaveTimestamp;
}

inline
unsigned long Timestamp::secondsAtMark() const
{
    return mSecondsAtMark;
}


#endif
