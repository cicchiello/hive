#include <MotorSpeedActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <RateProvider.h>
#include <hiveconfig.h>

#include <strbuf.h>
#include <strutils.h>


static Str STEPS_PER_SECOND_PROPNAME("steps-per_second");


MotorSpeedActuator::MotorSpeedActuator(HiveConfig *config, const char *name, unsigned long now)
  : Actuator(name, now), mConfig(*config), mStepsPerSecond(200)
{
    if (mConfig.hasProperty(STEPS_PER_SECOND_PROPNAME)) {
        const char *value = mConfig.getProperty(STEPS_PER_SECOND_PROPNAME).c_str();
	mStepsPerSecond = StringUtils::isNumber(value) ? atoi(value) : mStepsPerSecond;
    }
}


bool MotorSpeedActuator::loop(unsigned long now)
{
    return false;
}


int MotorSpeedActuator::stepsPerSecond() const
{
    TF("MotorSpeedActuator::stepsPerSecond");

    PH2("Reporting stepsPerSecond of: ", mStepsPerSecond);
    return mStepsPerSecond;
}


void MotorSpeedActuator::processMsg(unsigned long now, const char *msg)
{
    static const char *InvalidMsg = "invalid msg; did isMyMsg return true?";
    
    TF("MotorSpeedActuator::processMsg");

    const char *name = getName();
    int namelen = strlen(name);
    assert2(strncmp(msg, name, namelen) == 0, InvalidMsg, msg);
    
    msg += namelen;
    const char *token = "|";
    int tokenlen = 1;
    assert2(strncmp(msg, token, tokenlen) == 0, InvalidMsg, msg);
    msg += tokenlen;
    assert2(StringUtils::isNumber(msg), InvalidMsg, msg);

    mStepsPerSecond = atoi(msg);
    
    StrBuf secondsStr;
    secondsStr.append(mStepsPerSecond);
    PH4("setting HiveConfig property ", STEPS_PER_SECOND_PROPNAME.c_str(), " to ", secondsStr.c_str());
    mConfig.addProperty(STEPS_PER_SECOND_PROPNAME, secondsStr.c_str());
}


bool MotorSpeedActuator::isMyMsg(const char *msg) const
{
    TF("StepperActuator::isMyMsg");
    const char *name = getName();
    TRACE2("isMyMsg for motor: ", name);
    TRACE2("Considering msg: ", msg);
    
    if (strncmp(msg, name, strlen(name)) == 0) {
        msg += strlen(name);
	const char *token = "|";
	bool r = (strncmp(msg, token, strlen(token)) == 0) && StringUtils::isNumber(msg+strlen(token));
	if (r) {
	    TRACE("it's mine!");
	}
	return r;
    } else {
        return false;
    }
}
