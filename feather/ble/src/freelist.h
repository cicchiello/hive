#ifndef freelist_h
#define freelist_h



template <class E> class FreeList {
private:
    int len, sz;
    E **freeList;
  
public:
    FreeList() : freeList(new E*[10]), len(0), sz(10)
    {
        //DL("FreeList::FreeList");
    }

    E *pop() {
        //DL("FreeList::pop");
        if (len > 0) {
	    E *e = freeList[len-1];
	    freeList[len-1] = 0;
	    len--;
	    return e;
	} else {
	    return 0;
	}
    }

    void push(E *e) {
        //DL("FreeList::push");
        freeList[len++] = e;
	if (len == sz) {
	    E **newFreeList = new E*[2*sz];
	    for (int i = 0; i < sz; i++) {
	        newFreeList[i] = freeList[i];
	    }
	    delete [] freeList;
	    freeList = newFreeList;
	    sz *= 2;
	}
    }
};

#endif
