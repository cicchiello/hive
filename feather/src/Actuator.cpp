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


Actuator::Actuator(unsigned long now)
{
    // schedule first sample time
    mNextActionTime = now + 5*1000;
}


Actuator::~Actuator()
{
}


bool Actuator::isItTimeYet(unsigned long now)
{
    return now >= mNextActionTime;
}


void Actuator::scheduleNextAction(unsigned long now)
{
    mNextActionTime = now + 60*1000;
}


const char *Actuator::processCommand(const char *msg)
{
    PL("Actuator::processCommand (not overridden by derived class)");
    const char *prefix = "action|";
    Str command(msg + strlen(prefix));
    D("Received action cmd: ");
    DL(command.c_str());
    return msg;
}



