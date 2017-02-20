#include <str.h>

#include <Arduino.h>

#include <Trace.h>

int Str::sBytesConsumed = 0;

static int sMaxReported = 0;

Str::Str(const char *s)
  : buf(0), cap(0), deleted(false)
{
    if (s != NULL) {
        int l = strlen(s);
	expand(l);
	strcpy(buf, s);
    }
}

Str::Str(const Str &str)
  : buf(0), cap(0), deleted(false)
{
    int l = str.len();
    expand(l);
    strcpy(buf, str.c_str());
}

Str::~Str()
{
    sBytesConsumed -= cap;
    delete [] buf;
    deleted = true;
}

int Str::len() const
{
    assert(!deleted, "!deleted");
    return cap > 0 ? strlen(buf) : 0;
}

void Str::add(char c)  
{
    assert(!deleted, "!deleted");
    
    // there is an incoming byte available from the host; make sure there's space, then consume it
    int l = len();
    expand(l+2);
    buf[l++] = c;
    buf[l] = 0;
}

Str &Str::append(const char *str)
{
    assert(!deleted, "!deleted");
    
    Str temp(str); // done this way just in case "str" is a point to within *this
    return append(temp);
}

Str &Str::append(const Str &str)
{
    assert(!deleted, "!deleted");
    
    expand(len()+str.len()+1);
    strcpy(&buf[len()], str.c_str());
    return *this;
}

Str &Str::append(int i)
{
    assert(!deleted, "!deleted");
    
    char buf[10];
    sprintf(buf, "%d", i);
    return append(buf);
}

Str &Str::append(long l)
{
    assert(!deleted, "!deleted");
    
    char buf[15];
    sprintf(buf, "%Ld", l);
    return append(buf);
}

Str &Str::append(unsigned long l)
{
    assert(!deleted, "!deleted");
    
    char buf[15];
    sprintf(buf, "%Lu", l);
    return append(buf);
}

void Str::clear()
{
    assert(!deleted, "!deleted");
    
    if (buf != NULL) 
        buf[0] = 0;
}

void Str::expand(int required)
{
    if (required < 16) 
        required = 16;
    else {
        int l = 16;
	while (l < required)
	    l *= 2;
	required = l;
    }
    
    if (required > cap) {
        char *newBuf = new char[required+1];
	sBytesConsumed += required+1;
	if (sBytesConsumed > 2*sMaxReported) {
	    P("Str malloc report: "); PL(sBytesConsumed);
	    sMaxReported = sBytesConsumed;
	}
        if (cap > 0) {
	    strcpy(newBuf, buf);
	    delete [] buf;
	    sBytesConsumed -= cap;
	} else {
	    newBuf[0] = 0;
	}
	buf = newBuf;
	cap = required+1;
    }
}

bool Str::equals(const Str &other) const
{
    if (this == &other)
        return true;

    return strcmp(c_str(), other.c_str()) == 0;
}


Str &Str::operator=(const Str &o)
{
    assert(!deleted, "!deleted");
    
    if (this == &o)
        return *this;

    expand(o.len());
    strcpy(buf, o.buf);

    return *this;
}
  
Str &Str::operator=(const char *o)
{
    assert(!deleted, "!deleted");
    
    if (buf == o) // trivial case
        return *this;
  
    Str temp(o); // done this way just in case "o" is a point within this->buf
    *this = temp;

    return *this;
}
  

bool Str::endsWith(const char *cmp) const
{
    return strcmp(c_str()+len()-strlen(cmp), cmp) == 0;
}


Str Str::tolower() const
{
    Str r(*this);

    for (int i = 0; i < r.len(); i++)
      if ((r.c_str()[i] >= 'A') && (r.c_str()[i] <= 'Z')) {
	  const char *ptr = r.c_str() + i;
	  char *nonConstPtr = (char*) ptr;
	  *nonConstPtr -= 'A' - 'a';
      }

    return r;
}


bool Str::lessThan(const Str &o) const
{
    return strcmp(c_str(), o.c_str()) < 0;
}
