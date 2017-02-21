#include <Actuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>


#include <str.h>
#include <strutils.h>



/* STATIC */
Actuator *Actuator::sActiveActuators[Actuator::MAX_ACTUATORS];

/* STATIC */
int Actuator::sNumActiveActuators = 0;


/* STATIC */
void Actuator::activate(Actuator *actuator)
{
    TF("Actuator::activate");
    assert(!actuator->mIsActive, "!actuator->mIsActive");
    actuator->mIsActive = true;
    sActiveActuators[sNumActiveActuators++] = actuator;
    TRACE2("Activated: ", actuator->getName());
    TRACE3("there are now ", sNumActiveActuators, " active actuators");
}

void Actuator::deactivate(int index)
{
    TF("Actuator::deactivate");
    assert(index >= 0, "invalid index supplied to deactivate");
    assert(index < sNumActiveActuators, "deactiving an actuators that isn't on the actived list");
    assert(sActiveActuators[index]->mIsActive, "deactivating an actuator that isn't active");
    
    Actuator *oneToDeactivate = sActiveActuators[index];
    assert(oneToDeactivate, "oneToDeactivate is NULL");
    for (int i = index; i < sNumActiveActuators-1; i++)
        sActiveActuators[i] = sActiveActuators[i+1];

    sActiveActuators[--sNumActiveActuators] = NULL;
    oneToDeactivate->mIsActive = false;
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




