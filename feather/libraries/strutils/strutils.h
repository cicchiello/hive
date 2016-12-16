#ifndef string_utils_h
#define string_utils_h


#include <Arduino.h>


#define LOGGER Serial
#define P(msg) Serial.print(msg)
#define PL(msg) Serial.println(msg)
#define PC(v,c) Serial.print(v,c)
#define PLC(v,c) Serial.println(v,c)

static const char *s_func = "<undefined>";
#define PF(func) s_func = func;
#define PH(msg) {P(s_func); P(msg);}
#define PHL(msg) {P(s_func); PL(msg);}


#ifdef NDEBUG
#define D(msg)
#define DL(msg)
#define DH(msg)
#define DHL(msg)
#ifndef FAIL
#define FAIL() 
#endif
#define assert(t,msg) 
#else
#ifndef FAIL
#define FAIL() {for (;1==1;);}
#endif
#define D(msg) P(msg)
#define DH(msg) PH(msg)
#define DHL(msg) PHL(msg)
#define DL(msg) PL(msg)
#define assert(t,msg) if (!(t)) {D(__FILE__); D("; line "); D(__LINE__); D("; "); DL(msg); FAIL();}
#endif



class Str;

class StringUtils {
public:
  static void consumeChar(char c, Str *buf);

  static const char *eatWhitespace(const char *ptr);
  static const char *eatPunctuation(const char *ptr, char p);
  static const char *getToken(const char *ptr, Str *buf);
  static const char *unquote(const char *ptr, Str *ident);

  static void urlEncodePrint(Stream &stream, const char *msg);
};

#endif
