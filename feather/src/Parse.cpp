#include <Parse.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG



#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#define P(args) 
#define PL(args) 
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif


#define PlatformAssert(c,msg) if (!(c)) {WDT_TRACE(msg); P("ASSERT: "); PL(msg); while(1);}

#include <platformutils.h>



class ParseTester {
public:
  ParseTester() {}
  void test();
};

static ParseTester *s_t = 0;


#define TEST() {if (s_t == 0) {s_t = new ParseTester(); s_t->test();}}


inline static bool isEOL(const char *c)
{
    return c && ((*c == 0x0d) || (*c == 0x0a) || ((*c == '\\') && (*(c+1) == 'n')));
}


/* STATIC */
const char *Parse::consumeEOL(const char *rsp)
{
    TEST();
    const char *c = rsp;
    if (*c == '\n' || *c == '\l')
        c++;
    if (*c == '\\' && *(c+1) == 'n')
        c += 2;
    return c;
}


/* STATIC */
const char *Parse::consumeNumber(const char *rsp)
{
    TEST();
    const char *c = rsp;
    while (*c && (((*c >= '0') && (*c <= '9')) || (*c == '-')))
        c++;
    return c;
}


/* STATIC */
const char *Parse::consumeToEOL(const char *rsp)
{
    TEST();
    const char *c = rsp;
    while (!isEOL(c))
        c++;
    return Parse::consumeEOL(c);
}



/* STATIC */
bool Parse::hasEOL(const char *line)
{
    TEST();
    while (line && *line) {
        if (isEOL(line++)) {
	    return true;
	}
    }
    return false;
}


void ParseTester::test()
{
    PlatformAssert(!Parse::hasEOL("foo"), "Parse::hasEOL(\"foo\")");
    const char *r = Parse::consumeToEOL("foo\na");
    PlatformAssert(*r == 'a', "Parse::consumeToEOL(\"foo\\na\") failed");
    const char *r2 = Parse::consumeToEOL("foo\n");
    PlatformAssert(r2 != NULL, "r2 != NULL");
    PlatformAssert(*r2 == 0, "Parse::consumeToEOL(\"foo\\n\") failed");
}


