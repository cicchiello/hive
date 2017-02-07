#ifndef adc_buffifo_h
#define adc_buffifo_h

#include <Arduino.h>

class ADC_BufFifo {
public:
    ADC_BufFifo(int numBufs);
    ~ADC_BufFifo();
    
    void push(uint16_t *buf);
    uint16_t *pop();

    bool isEmpty() const;
    bool isFull() const;

    int depth() const;
    
private:
    ADC_BufFifo(const ADC_BufFifo &); // intentionally unimplemented
    ADC_BufFifo &operator=(const ADC_BufFifo &); // intentionally unimplemented
    
    uint16_t **m_bufs;
    uint8_t m_i, m_numBufs;
    int m_pushCnt, m_popCnt;
};

inline
bool ADC_BufFifo::isEmpty() const
{
    for (int i = 0; i < m_numBufs; i++)
        if (m_bufs[i] != NULL)
	    return false;
    return true;
}


inline
bool ADC_BufFifo::isFull() const
{
    for (int i = 0; i < m_numBufs; i++)
        if (m_bufs[i] == NULL)
	    return false;
    return true;
}

#endif
