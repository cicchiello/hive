#include <ServoConfigActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>

#include <hiveconfig.h>

#include <str.h>
#include <strbuf.h>
#include <strutils.h>



static Str TRIP_TEMP_PROPNAME("latch-trip-temp");
static Str LOWER_LIMIT_TICKS_PROPNAME("latch-llticks");
static Str UPPER_LIMIT_TICKS_PROPNAME("latch-ulticks");
static Str IS_CLOCKWISE_PROPNAME("latch-iscw");


ServoConfigActuator::ServoConfigActuator(HiveConfig *config, const char *name, unsigned long now)
  : mConfig(*config),
    Actuator(name, now+10*1000),
    mTripTempC(37.7), mLowerLimitTicks(40), mUpperLimitTicks(43), mIsClockwise(true)
{
    TF("ServoConfigActuator::ServoConfigActuator");

    if (mConfig.hasProperty(TRIP_TEMP_PROPNAME)) {
        const char *tripTempStr = mConfig.getProperty(TRIP_TEMP_PROPNAME).c_str();
	TRACE2("parsing: ", tripTempStr);
        mTripTempC = atoi(tripTempStr);
    }

    if (mConfig.hasProperty(LOWER_LIMIT_TICKS_PROPNAME)) {
        const char *llTicksStr = mConfig.getProperty(LOWER_LIMIT_TICKS_PROPNAME).c_str();
	mLowerLimitTicks = atoi(llTicksStr);
    }

    if (mConfig.hasProperty(UPPER_LIMIT_TICKS_PROPNAME)) {
        const char *ulTicksStr = mConfig.getProperty(UPPER_LIMIT_TICKS_PROPNAME).c_str();
        mUpperLimitTicks = atoi(ulTicksStr);
    }

    if (mConfig.hasProperty(IS_CLOCKWISE_PROPNAME)) {
        const char *isCWStr = mConfig.getProperty(IS_CLOCKWISE_PROPNAME).c_str();
	mIsClockwise = strcmp(isCWStr, "true") == 0;
    }

    TRACE2("mTripTempC: ", mTripTempC);
    TRACE2("mLowerLimitTicks: ", mLowerLimitTicks);
    TRACE2("mUpperLimitTicks: ", mUpperLimitTicks);
    TRACE2("mIsClockwise: ", (mIsClockwise ? "true" : "false"));
}


ServoConfigActuator::~ServoConfigActuator()
{
}


bool ServoConfigActuator::loop(unsigned long now)
{
    return false;
}


bool ServoConfigActuator::isMyMsg(const char *cmsg) const
{
    TF("ServoConfigActuator::isMyMsg");
    TRACE2("considering: ", cmsg);

    const char *name = getName();
    if (strncmp(cmsg, name, strlen(name)) == 0) {
        cmsg += strlen(name);
	const char *token = "|temp|dir|minTicks|maxTicks|";
	bool isMine = strncmp(cmsg, token, strlen(token)) == 0;
	TRACE("It's mine!");
	return isMine;
    }
    return false;
}


void ServoConfigActuator::processMsg(unsigned long now, const char *msg)
{
    TF("ServoConfigActuator::processMsg");

    StrBuf msgAtDir;

    // using otherwise-unnecessary scoping to make sure variables aren't unexpectedly used later
    {
	const char *token1 = getName();
	const char *token2 = "|temp|dir|minTicks|maxTicks|";
	const char *cmsgAtTemp = msg + strlen(token1) + strlen(token2);

	PH2("parsing: ", cmsgAtTemp);
	int newTripTempC = atoi(cmsgAtTemp);
	if (newTripTempC != mTripTempC) {
	    PH2("mTripTempC was ", mTripTempC);
	    PH2("new temp is ", newTripTempC);
	    mTripTempC = newTripTempC;
	    StrBuf tripTempStr; tripTempStr.append(newTripTempC);
	    mConfig.addProperty(TRIP_TEMP_PROPNAME, tripTempStr.c_str());
	}
	msgAtDir = cmsgAtTemp;
	StringUtils::consumeNumber(&msgAtDir);
    }

    Str msgAtMinTicks;
    bool haveDir = false;
    {
        const char *clockwiseToken = "|CW|";
	const char *counterclockwiseToken = "|CCW|";
	const char *cmsgAtDir = msgAtDir.c_str();
	
	if (strncmp(cmsgAtDir, clockwiseToken, strlen(clockwiseToken)) == 0) {
	    PH2("mIsClockwise was ", (mIsClockwise ? "CW" : "CCW"));
	    PH("new isClockwise is true");
	    mIsClockwise = true;
	    mConfig.addProperty(IS_CLOCKWISE_PROPNAME, "true");
	    msgAtMinTicks = cmsgAtDir + strlen(clockwiseToken);
	    haveDir = true;
	} else if (strncmp(cmsgAtDir, counterclockwiseToken, strlen(counterclockwiseToken)) == 0) {
	    PH2("mIsClockwise was ", (mIsClockwise ? "CW" : "CCW"));
	    PH("new isClockwise is false");
	    mIsClockwise = false;
	    mConfig.addProperty(IS_CLOCKWISE_PROPNAME, "false");
	    msgAtMinTicks = cmsgAtDir + strlen(counterclockwiseToken);
	    haveDir = true;
	}
    }

    if (haveDir) {
        const char *cmsgAtMinTicks = msgAtMinTicks.c_str();
	PH2("have temp and dir; parsing: ", cmsgAtMinTicks);
        int newLowerLimitTicks = atoi(cmsgAtMinTicks);
	if (newLowerLimitTicks != mLowerLimitTicks) {
	    PH2("mLowerLimitTicks was ", mLowerLimitTicks);
	    PH2("new lowerLimitTicks is ", newLowerLimitTicks);
	    mLowerLimitTicks = newLowerLimitTicks;
	    StrBuf llTicks; llTicks.append(mLowerLimitTicks);
	    mConfig.addProperty(LOWER_LIMIT_TICKS_PROPNAME, llTicks.c_str());
	} else {
	    PH2("mLowerLimitTicks matches ", mLowerLimitTicks);
	}
	StrBuf t(cmsgAtMinTicks);
	StringUtils::consumeNumber(&t);

	if (t.len() && (*t.c_str() == '|')) {
	    const char *cmsgAtMaxTicks = t.c_str() + 1;
	    int newUpperLimitTicks = atoi(cmsgAtMaxTicks);
	    if (newUpperLimitTicks != mUpperLimitTicks) {
	        PH2("mUpperLimitTicks was ", mUpperLimitTicks);
		PH2("new upperLimitTicks is ", newUpperLimitTicks);
		mUpperLimitTicks = newUpperLimitTicks;
		StrBuf ulTicks; ulTicks.append(mUpperLimitTicks);
		mConfig.addProperty(UPPER_LIMIT_TICKS_PROPNAME, ulTicks.c_str());
	    } else {
	        PH2("mUpperLimitTicks matchs ", mUpperLimitTicks);
	    }
	}
    }
}

