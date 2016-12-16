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
    while ((*ptr != 0) && ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n')))
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


