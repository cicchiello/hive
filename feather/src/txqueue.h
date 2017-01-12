#ifndef txqueue_h
#define txqueue_h

class Adafruit_BluefruitLE_SPI;

#include "Adafruit_BluefruitLE_SPI.h"

#include <str.h>


#define TIMEOUT_S 5

template <class E> class FreeList;

template <class E>
class TxQueue {
 private:
  E **queue;
  int len, sz;
  bool queueIsBusy;
  
 public:

  TxQueue() : queue(new E*[10]), len(0), sz(10), queueIsBusy(false)
  {
      //DL("TxQueue::TxQueue");
  }

  int getLen() const {return len;}
  
  void push(FreeList<E> *freeList, const char *sensorName, const char *value, const char *timestamp)
  {
      //DL("TxQueue::push(...)");
      E *e = freeList->pop();
      if (e == 0)
          e = new E();
    
      e->set(sensorName, value, timestamp);
      queue[len++] = e;

      if (len == sz) {
          E **newQueue = new E*[2*sz];
	  for (int i = 0; i < sz; i++) {
	      newQueue[i] = queue[i];
	  }
	  delete [] queue;
	  queue = newQueue;
	  sz *= 2;
      }
  }
    

  void push(E *e)
  {
      //DL("TxQueue::push(E*)");
      queue[len++] = e;

      if (len == sz) {
          E **newQueue = new E*[2*sz];
	  for (int i = 0; i < sz; i++) {
	      newQueue[i] = queue[i];
	  }
	  delete [] queue;
	  queue = newQueue;
	  sz *= 2;
      }
  }
    

  void attemptPost(Adafruit_BluefruitLE_SPI &ble)
  {
      static long timeoutExpiry = 0;
      //DL("TxQueue::attemptPost");
      if (queueIsBusy) {
          if (!ble.isConnected())
	      queueIsBusy = false;
	  else if (millis() > timeoutExpiry) {
	      PL("TxQueue::attemptPost; timeout expired, will re-post");
	      queueIsBusy = false;
	  }
      } else {
	  //D("TxQueue::attemptPost; len: "); DL(len);
	  //D("TxQueue::attemptPost; isConnected: "); DL(ble.isConnected());
          if ((len>0) && ble.isConnected()) {
	      //DL("TxQueue::attemptPost; posting to BLE");
              queueIsBusy = true;
	      timeoutExpiry = millis() + TIMEOUT_S*1000;
	      E *e = queue[0];
	      e->post(e->getName(), ble);
	  }
      }
  }

  void receivedFailureConfirmation()
  {
      // just need to free up the queue's busy flag, and the next attemp to post
      // will resend the top of the queue
      queueIsBusy = false;
  }


  void receivedSuccessConfirmation(FreeList<E> *freeList)
  {
      //DL("TxQueue::receivedSuccessConfirmation; successful post confirmed");
      
      // remove the 0th entry from the queue ('cause it's just been confirmed)
      E *e = queue[0];

      // bump the queue up one level
      for (int i = 1; i < len; i++) {
          queue[i-1] = queue[i];
      }
      len--;
    
      // put the struct on the freelist
      freeList->push(e);

      queueIsBusy = false;
  }
    
};

#endif
