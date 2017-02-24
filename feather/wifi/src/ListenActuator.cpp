#include <ListenActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <ListenSensor.h>

#include <hiveconfig.h>

#include <str.h>
#include <strutils.h>


#define SENSOR_RATE_PROPNAME "sensor-rate-seconds"


ListenActuator::ListenActuator(ListenSensor &listener, const char *name, unsigned long now)
  : Actuator(name, now), mListener(listener)
{
}


bool ListenActuator::loop(unsigned long now)
{
    return false;
}


void ListenActuator::processMsg(unsigned long now, const char *msg)
{
    static const char *InvalidMsg = "invalid msg; did isMyMsg return true?";
    
    TF("ListenActuator::processMsg");

    const char *name = getName();
    int namelen = strlen(name);
    assert2(strncmp(msg, name, namelen) == 0, InvalidMsg, msg);
    
    msg += namelen;
    const char *cmd = "|start|";
    assert2(strncmp(msg, cmd, strlen(cmd)) == 0, InvalidMsg, msg);
    msg += strlen(cmd);
    assert2(StringUtils::isNumber(msg), InvalidMsg, msg);
    int milliseconds = 1000*atoi(msg);
    while (*msg && (*msg >= '0') && (*msg <= '9'))
        msg++;
    assert2(*msg == '|', InvalidMsg, msg);
    Str attName(msg+1);
    PH4("starting recording for ", milliseconds, " ms to attachment ", attName.c_str());
    mListener.start(milliseconds, attName.c_str());
}


bool ListenActuator::isMyMsg(const char *msg) const
{
    TF("ListenActuator::isMyMsg");
    const char *name = getName();
    TRACE2("isMyMsg for motor: ", name);
    TRACE2("Considering msg: ", msg);
    
    if (strncmp(msg, name, strlen(name)) == 0) {
        msg += strlen(name);
	const char *cmd1 = "|start|";
	bool r = (strncmp(msg, cmd1, strlen(cmd1)) == 0) && StringUtils::isNumber(msg+strlen(cmd1));
	if (r) {
	    msg += strlen(cmd1);
	    while (*msg && (*msg >= '0') && (*msg <= '9'))
	        msg++;
	    if (*msg == '|') {
	        TRACE("it's mine!");
		return true;
	    }
	}
    }
    return false;
}
