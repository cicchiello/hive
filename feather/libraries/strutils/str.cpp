#include <str.h>

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

int Str::sBytesConsumed = 0;

static int sMaxReported = 0;


/* STATIC */
Str Str::sEmpty((const char *)0);


Str::Item::Item(const char *_s)
  : buf(0), cap(0), refs(0), len(0)
{
    TF("Str::Item::Item (1)");
    if (_s) {
        expand((len = strlen(_s)) + 1);
	strcpy(buf, _s);
    }
}


Str::Item::Item(int sz)
  : buf(0), cap(0), len(sz)
{
    TF("Str::Item::Item (2)");
    expand(sz+1);
    buf[0] = 0;
    refs = 0;
}


Str::Item *Str::Item::lookupOrCreate(const char *s)
{
    TF("Str::Item::lookupOrCreate");
    sBytesConsumed += sizeof(Item);
    return new Item(s);
}

Str::Str(const char *s)
  : item(Item::lookupOrCreate(s)), deleted(false)
{
  item->refs++;
}

Str::Str(Item *_item)
  : item(_item), deleted(false)
{
  item->refs++;
}

Str::Item::~Item()
{
  assert(refs == 0, "refs == 0");
  sBytesConsumed -= cap;
  delete [] buf;
}

int Str::len() const
{
    assert(!deleted, "!deleted");
    if (item->len == -1)
      return item->len = (item->cap > 0 ? strlen(item->buf) : 0);
    else
      return item->len;
}

void Str::Item::expand(int required)
{
    Str::expand(required, &cap, &buf);
}

bool Str::Item::equals(const Str::Item &other) const
{
    if (this == &other)
        return true;
  
    return strcmp(buf, other.buf) == 0;
}

Str &Str::operator=(const Str &o)
{
    assert(!deleted, "!deleted");
    
    if (this == &o)
        return *this;

    if (--item->refs == 0) {
        delete item;
	sBytesConsumed -= sizeof(Item);
    }
    
    item = o.item;
    item->refs++;
  
    return *this;
}
  
Str &Str::operator=(const char *o)
{
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


/* STATIC */
int Str::cacheSz()
{
  return 0;
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
	assert(newBuf, "newBuf");
	sBytesConsumed += required;
	if (sBytesConsumed > 2*sMaxReported) {
	    P("Str malloc report: "); PL(sBytesConsumed);
	    sMaxReported = sBytesConsumed;
	}
        if (*cap > 0) {
	    TF("Str::expand; copying old buf");
	    strcpy(newBuf, *buf);
	    delete [] *buf;
	    sBytesConsumed -= *cap;
	} else {
	    newBuf[0] = 0;
	}
	*buf = newBuf;
	*cap = required;
    }
}
