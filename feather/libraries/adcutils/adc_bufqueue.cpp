#include <adc_bufqueue.h>

#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <platformutils.h>

#define failtest(t,msg) if(!(t)) {fail(msg);}


static void fail(const char *msg)
{
    TF("::fail");
    PH(msg);
    PlatformUtils::nonConstSingleton().resetToBootloader();
}


ADC_BufQueue::ADC_BufQueue(int queueSz)
  : m_i(0), m_sz(queueSz)
{
    m_bufs = new uint16_t*[m_sz];
    for (int i = 0; i < m_sz; i++)
        m_bufs[i] = NULL;
}

ADC_BufQueue::~ADC_BufQueue()
{
    delete [] m_bufs;
}

void ADC_BufQueue::push(uint16_t *buf)
{
    failtest(!isFull(), 
	     "being told to enqueue a buffer before there's space on the Queue");
    assert(buf, "attempted to add entry to the queue that is NULL");
    
    m_bufs[m_i] = buf;
    m_i++;
}


uint16_t *ADC_BufQueue::pop()
{
    failtest(!isEmpty(), "received a popQueue request when the Queue should be empty!?!?");
    m_i--;
    uint16_t *r = m_bufs[m_i];
    m_bufs[m_i] = NULL;
    return r;
}


void ADC_BufQueue::clear()
{
    m_i = 0;
    for (int i = 0; i < m_sz; i++)
        m_bufs[i] = NULL;
}

