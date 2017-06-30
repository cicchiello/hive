#ifndef actuatorbase_h
#define actuatorbase_h

#include <Actuator.h>
#include <http_couchget.h>

class Str;
class HiveConfig;
class RateProvider;


class ActuatorBase : public Actuator {
 public:
    ActuatorBase(const HiveConfig &config, const RateProvider &rateProvider,
		 const char *actuatorName, unsigned long now);
    ~ActuatorBase() {}

    bool loop(unsigned long now, Mutex *wifi);

    const HiveConfig &getConfig() const;
    
    unsigned long getNextActionTime() const;
    
    class Getter : public HttpCouchGet {
    public:
        Getter(const char *ssid, const char *pswd,
	       const char *dbHost, int dbPort,
	       const char *dbUser, const char *dbPswd, bool isSSL, const char *urlPieces[]);
	~Getter() {}
        virtual bool hasResult() const = 0;
	virtual const char *className() const = 0;

	const CouchUtils::Doc *getSingleRecord() const;
	const Str *getSingleValue() const;

	virtual bool isError() const;
	
    private:
	virtual bool leaveOpen() const {return true;}
	
	bool mIsParsed, mIsValueParsed, mIsError;
	const CouchUtils::Doc *mRecord;
	const Str *mValue;
    };
    
 protected:
    virtual const char *className() const = 0;

    virtual Getter *createGetter() const = 0;
    
    virtual void processResult(Getter *getter) = 0;

    unsigned long setNextActionTime(unsigned long t);
    
    // helper function
    static void setNextTime(unsigned long now, unsigned long *t);
    
 private:
    unsigned long mNextActionTime;

    const RateProvider &mRateProvider;
    const HiveConfig &mConfig;
    Getter *mGetter;
};


inline
const HiveConfig &ActuatorBase::getConfig() const
{
    return mConfig;
}


inline
unsigned long ActuatorBase::getNextActionTime() const
{
    return mNextActionTime;
}

inline
unsigned long ActuatorBase::setNextActionTime(unsigned long t)
{
    mNextActionTime = t;
}


#endif
