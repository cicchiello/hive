#include <strutils.h>

#include <Arduino.h>

#include <Trace.h>

#include <str.h>
#include <strbuf.h>

/* STATIC */
void StringUtils::consumeChar(char c, StrBuf *buf)
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
const char *StringUtils::getToken(const char *ptr, StrBuf *buf)
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
    return ptr;
}

/* STATIC */
const char *StringUtils::unquote(const char *ptr, StrBuf *ident)
{
    TF("StringUtils::unquote");
    if (*ptr != '"') {
        return getToken(ptr, ident);
    }
    
    ptr = eatWhitespace(ptr+1);
    while ((*ptr != 0) && (*ptr != '"')) {
	ident->add(*ptr++);
    }
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
void StringUtils::consumeNumber(StrBuf *rsp)
{
    const char *c = rsp->c_str();
    while (*c && (((*c >= '0') && (*c <= '9')) || (*c == '-')))
        c++;

    *rsp = c;
}


/* STATIC */
StrBuf StringUtils::TAG(const char *func, const char *msg) 
{
    StrBuf tag(func);
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


/* STATIC */
const char *StringUtils::replace(StrBuf *result, const char *orig, const char *match, const char *repl)
{
    const char *src=orig; // points to current loc withini orig
    
    while (true) {
        const char *ins = strstr(src, match);
	if (ins == NULL) {
	    // no more matches to replace -- just need to copy the remainder of the unaltered src to dst
	    result->append(src);
	    return result->c_str();
	} else {
	    // copy everything before the match str
	    for (const char *ptr = src; ptr < ins; ptr++)
	        result->add(*ptr);
	    src += (ins-src) + strlen(match);     // advance src to loc just beyond match str
	    result->append(repl);
	}
    }

    return NULL;
}


bool StringUtils::isNumber(const char *s) {
  bool foundAtLeastOne = false;
  if (s && *s && ((*s == '+') || (*s == '-') || ((*s >= '0') && (*s <= '9')))) {
      s++;
      while (s && *s && (*s >= '0') && (*s <= '9')) {
	  foundAtLeastOne = true;
	  s++;
      }
  }
  return foundAtLeastOne;
}
