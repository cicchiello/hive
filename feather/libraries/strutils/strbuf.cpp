#include <strbuf.h>

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

#include <str.h>


StrBuf::StrBuf(const char *s)
  : buf(0), cap(0), deleted(false)
{
    if (s != NULL) {
        int l = strlen(s);
	expand(l);
	strcpy(buf, s);
    }
}

StrBuf::StrBuf(const StrBuf &str)
  : buf(0), cap(0), deleted(false)
{
    int l = str.len();
    expand(l);
    strcpy(buf, str.c_str());
}

StrBuf::~StrBuf()
{
    Str::sBytesConsumed -= cap;
    delete [] buf;
    deleted = true;
}

int StrBuf::len() const
{
    assert(!deleted, "!deleted");
    return cap > 0 ? strlen(buf) : 0;
}

void StrBuf::add(char c)  
{
    assert(!deleted, "!deleted");

    // there is an incoming byte available from the host; make sure there's space, then consume it
    int l = len();
    expand(l+2);
    buf[l++] = c;
    buf[l] = 0;
}

StrBuf &StrBuf::append(const char *str)
{
    assert(!deleted, "!deleted");
    expand(len()+strlen(str)+1);
    strcpy(&buf[len()], str);
    return *this;
}

StrBuf &StrBuf::append(int i)
{
    assert(!deleted, "!deleted");
    char buf[10];
    sprintf(buf, "%d", i);
    return append(buf);
}

StrBuf &StrBuf::append(long l)
{
    assert(!deleted, "!deleted");
    char buf[15];
    sprintf(buf, "%Ld", l);
    return append(buf);
}

StrBuf &StrBuf::append(unsigned long l)
{
    assert(!deleted, "!deleted");
    char buf[15];
    sprintf(buf, "%Lu", l);
    return append(buf);
}

void StrBuf::expand(int required)
{
    assert(!deleted, "!deleted");
    Str::expand(required, &cap, &buf);
}


StrBuf &StrBuf::operator=(const char *o)
{
    assert(!deleted, "!deleted");
    if (buf == o) // trivial case
        return *this;
  
    StrBuf temp(o); // done this way just in case "o" is a point within this->buf
    return this->operator=(temp);
}
  

StrBuf &StrBuf::operator=(const StrBuf &o)
{
    assert(!deleted, "!deleted");
    if (this == &o) // trivial case
        return *this;
  
    expand(o.len()+1);
    strcpy(buf, o.c_str());

    return *this;
}
  

StrBuf StrBuf::tolower() const
{
    assert(!deleted, "!deleted");
    StrBuf r(*this);

    for (int i = 0; i < r.len(); i++)
      if ((r.c_str()[i] >= 'A') && (r.c_str()[i] <= 'Z')) {
	  const char *ptr = r.c_str() + i;
	  char *nonConstPtr = (char*) ptr;
	  *nonConstPtr -= 'A' - 'a';
      }

    return r;
}



