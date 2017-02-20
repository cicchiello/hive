#include <Actuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <str.h>
#include <strutils.h>

#include <hive_platform.h>


/* STATIC */
Actuator *Actuator::sActiveActuators[Actuator::MAX_ACTUATORS];

/* STATIC */
int Actuator::sNumActiveActuators = 0;


/* STATIC */
void Actuator::activate(Actuator *actuator)
{
    assert(!actuator->mIsActive, "!actuator->mIsActive");
    actuator->mIsActive = true;
    sActiveActuators[sNumActiveActuators++] = actuator;
}

void Actuator::deactivate(int index)
{
    Actuator *oneToDeactiveate = sActiveActuators[index];
    for (int i = index; i < sNumActiveActuators-1; i++)
        sActiveActuators[i] = sActiveActuators[i+1];

    sActiveActuators[--sNumActiveActuators] = NULL;
}



Actuator::Actuator(const char *name, unsigned long now)
  : mName(new Str(name)), mIsActive(false)
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


Str Actuator::TAG(const char *memberfunc, const char *msg) const
{
    Str func = className();
    func.append("(");
    func.append(*mName);
    func.append(")::");
    func.append(memberfunc);
    return StringUtils::TAG(func.c_str(), msg);
}




