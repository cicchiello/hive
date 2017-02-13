#include <strutils.h>

#include <str.h>

#include <Arduino.h>

/* STATIC */
void StringUtils::consumeChar(char c, Str *buf)
{
    buf->add(c);
}


/* STATIC */
const char *StringUtils::eatWhitespace(const char *ptr) 
{
    while ((*ptr != 0) && ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n') || (*ptr == 0x0d)))
        ptr++;
    return ptr;
}

/* STATIC */
const char *StringUtils::eatPunctuation(const char *ptr, char p) 
{
    ptr = eatWhitespace(ptr);
    if (*ptr == p) {
        ptr++;
	return eatWhitespace(ptr);
    } else return NULL;
}

/* STATIC */
const char *StringUtils::getToken(const char *ptr, Str *buf)
{
    // let's take all chars starting at ptr that are in the following set as the token:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+_."

    while ( ('a' <= *ptr && *ptr <= 'z') ||
	    ('A' <= *ptr && *ptr <= 'Z') ||
	    ('0' <= *ptr && *ptr <= '9') ||
	    ('_' == *ptr) || ('.' == *ptr) || 
	    ('-' == *ptr) || ('+' == *ptr)) {
	buf->add(*ptr++);
    }
    buf->add(0);
    return ptr;
}

/* STATIC */
const char *StringUtils::unquote(const char *ptr, Str *ident)
{
    if (*ptr != '"') 
        return getToken(ptr, ident);
    
    ptr = eatWhitespace(ptr+1);
    while ((*ptr != 0) && (*ptr != '"')) {
	ident->add(*ptr++);
    }
    ident->add(0);
    if (*ptr == '"')
        ptr++;
    
    return ptr;
}



/* STATIC */
void StringUtils::urlEncodePrint(Stream &stream, const char *msg)
{
    static const char *unreserved = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~" ",=:;@[]";
    static const char *hex = "0123456789abcdef";
    
    while (*msg) {
        size_t okspan = strspn(msg, unreserved);
	if (okspan > 0) {
	    stream.write((const unsigned char *) msg, okspan);
	    msg += okspan;
	}
	if (*msg) {
	    char enc[3] = {'%'};
	    enc[1] = hex[*msg >> 4];
	    enc[2] = hex[*msg & 0x0f];
	    stream.write((const unsigned char *) enc, 3);
	    msg++;
	}
    }
}


#define nibHigh(b) (((b)&0xf0)>>4)
#define nibLow(b) ((b)&0x0f)

inline static char hex2asc(unsigned char n) 
{
    return n>9 ? n-10+'a' : n+'0';
}


/* STATIC */
void StringUtils::itoahex(char buf[2], char i)
{
  buf[0] = hex2asc(nibHigh(i));
  buf[1] = hex2asc(nibLow(i));
}


/* STATIC */
void StringUtils::consumeEOL(Str *line)
{
    const char *c = line->c_str();
    if (*c == 0x0d || *c == 0x0a) {
        c++;
    } else if ((*c == '\\') && (*(c+1) == 'n')) {
        c += 2;
    }

    *line = c;
}


inline static bool isEOL(const char *c)
{
    return c && ((*c == 0x0d) || (*c == 0x0a) || ((*c == '\\') && (*(c+1) == 'n')));
}


/* STATIC */
void StringUtils::consumeToEOL(Str *rsp)
{
    const char *c = rsp->c_str();
    while (!isEOL(c))
        c++;

    *rsp = c;
    consumeEOL(rsp);
}


/* STATIC */
bool StringUtils::hasEOL(const Str &line)
{
  const char *cline = line.c_str();
    while (cline && *cline) {
        if (isEOL(cline++)) {
	    return true;
	}
    }
    return false;
}


/* STATIC */
bool StringUtils::isAtEOL(const Str &line)
{
    return isEOL(line.c_str());
}


/* STATIC */
void StringUtils::consumeNumber(Str *rsp)
{
    const char *c = rsp->c_str();
    while (*c && (((*c >= '0') && (*c <= '9')) || (*c == '-')))
        c++;

    *rsp = c;
}


/* STATIC */
Str StringUtils::TAG(const char *func, const char *msg) 
{
    Str tag = func;
    tag.append("; ");
    tag.append(msg);
    return tag;
}


int StringUtils::ahextoi(const char *hexascii, int len)
{
    int r = 0;
    for (int i = 0; i < len; i++) {
        unsigned char b = hexascii[i]-'0';
	if (b > 9 && hexascii[i]>='a' && hexascii[i]<='f')
	    b = hexascii[i]-'a'+10;
	if (b > 9 && hexascii[i]>='A' && hexascii[i]<='F')
	    b = hexascii[i]-'A'+10;
	r *= 16;
	r += b;
    }
    return r;
}
