#include <Actuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>


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

bool Actuator::isItTimeYet(unsigned long now) const
{
    return now >= mNextActionTime;
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




