#include <mempool.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdio>
#include <string.h>
#endif

#define HEADLESS
#define NDEBUG

#ifdef ARDUINO
#   include <Trace.h>
#else
#   include <CygwinTrace.h>
#endif


bool sIsPoolConstructed = false;
const void *sInitialAllocPtr = 0;
int sPoolAllocCnt = 0;
int sPoolDeallocCnt = 0;

void PoolPrint(const char *msg)
{
  TF("::PoolPrint");
  PH(msg);
}

#ifdef foo
void PoolAllocCnt(const char *classname, int blksz)
{
  TF("::PoolAllocCnt");
  if (!sPoolError) {
    sPoolAllocCnt++;
    TRACE5("allocation from Pool<", classname, ",", blksz, ">");
  } else {
    PH(sPoolError);
  }
}

void PoolDeallocCnt(const char *classname, int blksz)
{
  TF("::PoolDeallocCnt");
  if (!sPoolError) {
    TRACE5("deallocation from Pool<", classname, ",", blksz, ">");
    sPoolDeallocCnt++;
  } else {
    PH(sPoolError);
  }
}

void PoolCTOR(const char *Tclassname, int blksz, const void *v)
{
  TF("::PoolCTOR");
  if (!sPoolError) {
    if (sIsPoolConstructed) {
      PoolErr(Tclassname, blksz, "Pool already constructed");
    } else {
      sIsPoolConstructed = true;
      sInitialAllocPtr = v;
    }
  }
}

#endif

void PoolErr(const char *classname, int blksz, const char *msg)
{
  TF("::PoolErr");
  PH6("Pool<", classname, ",", blksz, ">: ", msg);
  if (TraceScope::sSerialIsAvailable) {
    assert(false, msg);
  }
}


void PoolHighWater(const char *Tclassname, int blksz, int highWater)
{
  TF("::PoolHighWater");
  PH6("new high-water-mark for Pool<", Tclassname, ",", blksz, ">: ", highWater);
}



extern unsigned long __HeapLimit;
extern unsigned long __StackLimit;
static unsigned long sStackTop = 0;
static unsigned long sHeapBottom = 0;
bool memValidity()
{
    TF("::memValidity");
    const void *dummy = NULL;
    const void *stackPtr = &dummy;
    unsigned long stack = (unsigned long) stackPtr;
    if ((sStackTop == 0) || (sStackTop < stack))
        sStackTop = stack;
    const void *heapPtr = new char[2048];
    unsigned long heap = (unsigned long) heapPtr;
    if ((sHeapBottom == 0) || ((heap > 0) && (heap < sHeapBottom)))
        sHeapBottom = heap;
    bool valid = (heap != 0) && (stack > heap);
    delete [] heapPtr;
    if (!valid) {
        PH2("stack: ", stack);
	PH2("stack sz: ", (sStackTop - stack));
	PH2("heap: ", heap);
	PH2("heap sz: ", (heap - sHeapBottom));
	PH2("__HeapLimit: ", __HeapLimit);
	PH2("stack > heap ?  ", (stack > heap ? "true" : "false"));
    }
    return valid;
}
