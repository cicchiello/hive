#include <str.h>

#include <Arduino.h>

Str::Str(const char *s)
  : buf(0), cap(0)
{
    if (s != NULL) {
        int l = strlen(s);
	expand(l);
	strcpy(buf, s);
    }
}

Str::Str(const Str &str)
  : buf(0), cap(0)
{
    int l = str.len();
    expand(l);
    strcpy(buf, str.c_str());
}

Str::~Str()
{
    delete [] buf;
}

int Str::len() const
{
    return cap > 0 ? strlen(buf) : 0;
}

void Str::add(char c)  
{
    // there is an incoming byte available from the host; make sure there's space, then consume it
    int l = len();
    expand(l+2);
    buf[l++] = c;
    buf[l] = 0;
}

void Str::append(const char *str)
{
    expand(len()+strlen(str)+1);
    strcpy(&buf[len()], str);
}

void Str::append(const Str &str)
{
    expand(len()+str.len()+1);
    strcpy(&buf[len()], str.c_str());
}

void Str::clear()
{
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
        char *newBuf = new char[required];
        if (cap > 0) {
	    strcpy(newBuf, buf);
	    delete [] buf;
	} else {
	    newBuf[0] = 0;
	}
	buf = newBuf;
	cap = required;
    }
}

Str &Str::operator=(const Str &o)
{
    if (this == &o)
        return *this;

    expand(o.len());
    strcpy(buf, o.buf);

    return *this;
}
  
Str &Str::operator=(const char *o)
{
    expand(strlen(o));
    strcpy(buf, o);

    return *this;
}
  
