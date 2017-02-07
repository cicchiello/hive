#ifndef adc_bufqueue_h
#define adc_bufqueue_h

#include <Arduino.h>

class ADC_BufQueue {
public:
    ADC_BufQueue(int queueSz);
    ~ADC_BufQueue();
    
    void push(uint16_t *buf);
    uint16_t *pop();

    void clear();

    bool isEmpty() const;
    bool isFull() const;

private:
    uint16_t **m_bufs;
    uint8_t m_i, m_sz;
};

inline
bool ADC_BufQueue::isEmpty() const
{
    return m_i == 0;
}


inline
bool ADC_BufQueue::isFull() const
{
  return m_i == m_sz;
}

#endif
