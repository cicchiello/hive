#include <SensorRateActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <RateProvider.h>
#include <hiveconfig.h>
#include <str.h>



class SensorRateGetter : public ActuatorBase::Getter {
private:
    bool mHasRate, mIsError, mIsParsed;
    int mRate;
  
public:
    SensorRateGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
		     const char *url, const char *dbuser, const char *dbpswd)
      : ActuatorBase::Getter(ssid, pswd, dbHost, dbPort, url, dbuser, dbpswd, isSSL),
	mHasRate(false), mIsError(false), mIsParsed(false)
    {
        TF("SensorRateGetter::SensorRateGetter");
	TRACE("entry");
    }
    ~SensorRateGetter() {}

    bool isError() const
    {
      TF("SensorRateGetter::isError");
      TRACE2("mIsError: ", mIsError);
      return mIsError || ActuatorBase::Getter::isError();
    }
  
    int getRate() const {
        TF("SensorRateGetter::getRate");
        assert(mHasRate, "mHasRate");
	return mRate;
    }

    bool hasResult() const {return hasRate();}
  
    bool hasRate() const {
        TF("SensorRateGetter::hasRate");
	TRACE2("mHasRate: ", mHasRate);
	
        if (mIsParsed)
	    return mHasRate;
	
	TRACE("parsing");
	SensorRateGetter *nonConstThis = (SensorRateGetter*)this;
	nonConstThis->mIsParsed = true;

	const Str *vstr = getSingleValue();
	if (vstr != NULL) {
	    nonConstThis->mRate = atoi(vstr->c_str());
	    nonConstThis->mHasRate = true;
	    nonConstThis->mIsError = false;
	    return true;
	}
	return false;
    }

    static const char *ClassName() {return "SensorRateGetter";}
  
    const char *className() const;
};

const char *SensorRateGetter::className() const
{
    return SensorRateGetter::ClassName();
}


ActuatorBase::Getter *SensorRateActuator::createGetter() const
{
    TF("StepperActuator::createGetter");
    TRACE("creating getter");

    // curl -X GET 'http://jfcenterprises.cloudant.com/hive-sensor-log/_design/SensorLog/_view/by-hive-sensor?endkey=%5B%22F0-17-66-FC-5E-A1%22,%22sample-rate%22,%2200000000%22%5D&startkey=%5B%22F0-17-66-FC-5E-A1%22,%22sensor-rate%22,%2299999999%22%5D&descending=true&limit=1'
	  
    Str encodedUrl;
    buildStandardSensorEncodedUrl(getName(), &encodedUrl);

    Str url;
    CouchUtils::toURL(getConfig().getLogDbName(), encodedUrl.c_str(), &url);
    TRACE2("URL: ", url.c_str());
	
    return new SensorRateGetter(getConfig().getSSID(), getConfig().getPSWD(),
				getConfig().getDbHost(), getConfig().getDbPort(), getConfig().isSSL(),
				url.c_str(), getConfig().getDbUser(), getConfig().getDbPswd());
}


void SensorRateActuator::processResult(ActuatorBase::Getter *baseGetter)
{
    TF("SensorRateActuator::processResult");
    
    if (baseGetter->className() == SensorRateGetter::ClassName()) {
        SensorRateGetter *getter = (SensorRateGetter*) baseGetter;
        PH2("hasRate(): ", getter->hasRate());
	mSeconds = getter->getRate();
    } else {
        ERR("class cast exception");
    }
}


SensorRateActuator::SensorRateActuator(const HiveConfig &config, const char *name, unsigned long now)
  : ActuatorBase(config, *this, name, now),
#ifndef NDEBUG    
    mSeconds(30l)
#else    
    mSeconds(5*60)
#endif  
{
}


bool SensorRateActuator::isMyMsg(const char *msg) const
{
    assert(false, "unimplemented");
    return strncmp(msg, getName(), strlen(getName())) == 0;
}

void SensorRateActuator::processMsg(unsigned long now, const char *msg)
{
  assert(false, "unimplemented");
}
