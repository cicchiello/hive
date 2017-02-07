#include <adc_buffifo.h>

#define NDEBUG
#include <strutils.h>

#include <platformutils.h>




ADC_BufFifo::ADC_BufFifo(int numBufs)
  : m_bufs(new uint16_t*[numBufs]), m_i(0), m_numBufs(numBufs), m_pushCnt(0), m_popCnt(0)
{
    for (int i = 0; i < numBufs; i++)
        m_bufs[i] = NULL;
    assert(isEmpty(), "isEmpty test may be broken");
}

ADC_BufFifo::~ADC_BufFifo()
{
    delete [] m_bufs;

#ifdef TRACK_ALLOCATIONS    
    P("ADC_BufFifo:~ADC_BufFifo: m_pushCnt = "); PL(m_pushCnt);
    P("ADC_BufFifo:~ADC_BufFifo: m_popCnt = "); PL(m_popCnt);
#endif
}


void ADC_BufFifo::push(uint16_t *buf)
{
    m_pushCnt++;
    assert(buf, "buf is NULL");
    assert(m_bufs[m_i] == NULL,
	     "being told to persist a buffer before there's space on the FIFO");
    assert(!isFull(), "Fifo is full");
    
    m_bufs[m_i] = buf;
    m_i++;
    if (m_i == m_numBufs) 
        m_i = 0;

    assert(!isEmpty(), "Fifo is full");
}


uint16_t *ADC_BufFifo::pop()
{
    m_popCnt++;
    assert(!isEmpty(), "received a popFifo request when the FIFO should be empty!?!?");
    int i = m_i;
    for (int checkCnt = 0; checkCnt < m_numBufs; checkCnt++) {
        if (m_bufs[i] != NULL) {
	    // found the one to pop
	    uint16_t *r = m_bufs[i];
	    m_bufs[i] = NULL;
	    return r;
	} else {
	    i++;
	    if (i == m_numBufs)
	        i = 0;
	}
    }
    assert(false, "Fifo is empty");
    return NULL;
}


int ADC_BufFifo::depth() const
{
    int d = 0;
    for (int i = 0; i < m_numBufs; i++)
        if (m_bufs[i] != NULL)
	    d++;
    return d;
}

