#include <str.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdio>
#include <string.h>
#endif

//#define HEADLESS
//#define NDEBUG

#ifdef ARDUINO
#   include <Trace.h>
#else
#   include <CygwinTrace.h>
#endif


int Str::sBytesConsumed = 0;

static int sMaxReported = 0;


/* STATIC */
Str Str::sEmpty((const char *)0);


Str::Item::Item(const char *_s)
  : buf(0), cap(0), refs(0)
{
    TF("Str::Item::Item (1)");
    assert(!TraceScope::sSerialIsAvailable || sizeof(Item) == 8, "sizeof(Item) changed");
    if (_s) {
        expand(strlen(_s) + 1);
	strcpy(buf, _s);
    }
}


Str::Item::Item(int sz)
  : buf(0), cap(0), refs(0)
{
    TF("Str::Item::Item (2)");
    assert(!TraceScope::sSerialIsAvailable || sizeof(Item) == 12, "sizeof(Item) changed");
    expand(sz+1);
    buf[0] = 0;
}


Str::Item *Str::Item::lookupOrCreate(const char *s)
{
    TF("Str::Item::lookupOrCreate");
    sBytesConsumed += sizeof(Item);
    return new Item(s);
}

void Str::Item::setCapacity(unsigned short c)
{
    TF("Str::Item::setCapacity");
    assert(c < 0xFFF, "Str::Item capcity exceeded");
    assert((c & 0xF) == 0, "Invalid capacity");
    cap = c >> 4;
}

int Str::Item::getLen() const
{
    return strlen(buf);
}


Str::Str(const char *s)
  : deleted(false)
{
    TF("Str::Str(const char*)");
    item = Item::lookupOrCreate(s);
    assert(!TraceScope::sSerialIsAvailable || sizeof(Str) == 8, "sizeof(Str) changed");
    item->inc();
}

Str::Str(Item *_item)
  : deleted(false)
{
    TF("Str::Str(Item*)");
    item = _item;
    item->inc();
}

Str::Str(const Str &str)
  : deleted(false)
{
    item = str.item;
    item->inc();
}

Str::~Str() {
    TF("Str::~Str");
    if (item->dec() == 0) {
        delete item;
	sBytesConsumed -= sizeof(Item);
    }
    item = 0;
    deleted = true;
}

const char *Str::c_str() const
{
    return item->buf;
}

bool Str::equals(const Str &other) const
{
    if (this->item == other.item)
        return true;

    return this->item->equals(*other.item);
}

Str &Str::operator=(const Str &o)
{
    TF("Str::operator=");
    assert(!deleted, "!deleted");
    
    if (this == &o)
        return *this;

    if (item->dec() == 0) {
        delete item;
	sBytesConsumed -= sizeof(Item);
	item = 0;
    }

    item = o.item;
    item->inc();
  
    return *this;
}
  
Str &Str::operator=(const char *o)
{
    TF("Str::operator=(const char*)");
    if (deleted) {PH("accessing a deleted Str");}
    assert(!deleted, "!deleted");
    
    if (c_str() == o) // trivial case
        return *this;

    if (strcmp(c_str(), o) == 0) // another trivial case
        return *this;

    Str temp(o); // done this way just in case "o" is a point within this->buf
    return this->operator=(temp);
}
  

bool Str::endsWith(const char *cmp) const
{
    return strcmp(c_str()+len()-strlen(cmp), cmp) == 0;
}


bool Str::lessThan(const Str &o) const
{
    return strcmp(c_str(), o.c_str()) < 0;
}


Str::Item::~Item()
{
    TF("Str::Item::~Item");
    assert(refs == 0, "refs == 0");
    TRACE4("destructing buf of capacity: ", getCapacity(), " containing: ", (buf ? buf : "<null>"));
    sBytesConsumed -= getCapacity();
    delete [] buf;
}

int Str::len() const
{
    TF("Str::len");
    assert(!deleted, "!deleted");
    return item->getLen();
}

void Str::Item::expand(int required)
{
    unsigned short c = getCapacity();
    Str::expand(required, &c, &buf);
    setCapacity(c);
}


int Str::Item::inc()
{
    TF("Str::Item::inc");
    assert(refs < 8193, "refcnt limit exceeded");
    return ++refs;
}


bool Str::Item::equals(const Str::Item &other) const
{
    if (this == &other)
        return true;
  
    return strcmp(buf, other.buf) == 0;
}




#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */
#define FIRSTH 37 /* also prime */

/* STATIC */
unsigned int Str::hash_str(const char* s)
{
   unsigned int h = FIRSTH;
   while (s && *s) {
     h = (h * A) ^ (s[0] * B);
     s++;
   }
   return h; // or return h % C;
}


// A binary search based function to find the position
// where item should be inserted in a[low..high]
//
// returns the index into a of item, or the index immediately
// preceding where item should be place
int binarySearch(int a[], int item, int low, int high)
{
    if (high <= low)
        return (item > a[low])?  (low + 1): low;
 
    int mid = (low + high)/2;
 
    if(item == a[mid])
        return mid+1;
 
    if(item > a[mid])
        return binarySearch(a, item, mid+1, high);
    return binarySearch(a, item, low, mid-1);
}


/* STATIC */
void Str::expand(int required, unsigned short *cap, char **buf)
{
    TF("Str::expand");
    if (required < 16) 
        required = 16;
    else {
        int l = 16;
	while (l < required)
	    l *= 2;
	required = l;
    }
    
    if (required > *cap) {
        TF("Str::expand; creating new buf");
	TRACE2("required: ", required);

	char *newBuf = new char[required];
	assert(newBuf, "allocation failed");
	
	sBytesConsumed += required;
	if (sBytesConsumed > 2*sMaxReported) {
	    if (TraceScope::sSerialIsAvailable) {P("Str malloc report: "); PL(sBytesConsumed);}
	    sMaxReported = sBytesConsumed;
	}
        if (*cap > 0) {
	    TF("Str::expand; copying old buf");
	    strcpy(newBuf, *buf);
	    TRACE4("deleting buffer of size: ", *cap, "; buf: ", *buf);
	    delete [] *buf;
	    sBytesConsumed -= *cap;
	} else {
	    newBuf[0] = 0;
	}
	*buf = newBuf;
	*cap = required;
    }
}
