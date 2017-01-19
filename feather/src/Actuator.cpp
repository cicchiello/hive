#include <Actuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG


#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif


#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#include <str.h>
#include <strutils.h>

#include <hive_platform.h>


Actuator::Actuator(const char *name, unsigned long now)
  : mName(new Str(name))
{
    // schedule first sample time
    mNextActionTime = now + 5*1000;
}


Actuator::~Actuator()
{
    delete mName;
}

const char *Actuator::getName() const
{
    return mName->c_str();
}

bool Actuator::isItTimeYet(unsigned long now)
{
    return now >= mNextActionTime;
}


void Actuator::scheduleNextAction(unsigned long now)
{
    mNextActionTime = now + 60*1000;
}


void Actuator::processCommand(Str *msg)
{
    HivePlatform::singleton()->error("Actuator::processCommand; "
				     " not overridden by derived class");
    
    const char *prefix = "action|";
    const char *cmsg = msg->c_str();

    *msg = cmsg + strlen(prefix);
    
    D("Received action cmd: ");
    DL(msg->c_str());
}

Str Actuator::TAG(const char *memberfunc, const char *msg) const
{
    Str func = className();
    func.append("(");
    func.append(*mName);
    func.append(")::");
    func.append(memberfunc);
    return StringUtils::TAG(func.c_str(), msg);
}




