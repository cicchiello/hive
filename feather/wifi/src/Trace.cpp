#include <Arduino.h>

#include <Trace.h>

TraceScope *TraceScope::sCurrScope = 0;
bool TraceScope::sSerialIsAvailable = false;
