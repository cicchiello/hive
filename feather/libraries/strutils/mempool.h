#ifndef mempool_h
#define mempool_h


extern void PoolPrint(const char *msg);
extern void PoolErr(const char *Tclassname, int blksz, const char *msg);
extern void PoolHighWater(const char *Tclassname, int blksz, int highWater);

template <class T, int BlkSz> class Pool {
 private:
  T tarr[BlkSz];
  T *free[BlkSz];
  int tail, mtail;
  int allocCnt, deallocCnt;

 public:
  Pool() : allocCnt(0), deallocCnt(0) {
    for (int i = 0; i < BlkSz; i++)
      free[i] = &tarr[i];
    mtail = tail = BlkSz-1;
  }

  void *alloc() {
    void *r = free[tail];
    free[tail--] = 0;
    if (tail < mtail) {
      if (tail < 0) 
	PoolErr(T::classname(), BlkSz, "Pool is exhausted");
      PoolHighWater(T::classname(), BlkSz, (BlkSz-1-tail));
      mtail = tail;
    }
    allocCnt++;
    return r;
  }
  
  void dealloc(void *t) {
    PoolPrint("dealloc called");
    deallocCnt++;
    if (tail >= BlkSz-1)
      PoolErr(T::classname(), BlkSz, "too many calls to dealloc");
    free[++tail] = (T*)t;
  }
  
};

#endif
