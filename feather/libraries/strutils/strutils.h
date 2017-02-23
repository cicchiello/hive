#ifndef string_utils_h
#define string_utils_h


#include <Arduino.h>


class Str;

class StringUtils {
public:
  static void consumeChar(char c, Str *buf);

  static const char *eatWhitespace(const char *ptr);
  static const char *eatPunctuation(const char *ptr, char p);
  static const char *getToken(const char *ptr, Str *buf);
  static const char *unquote(const char *ptr, Str *ident);

  static bool isAtEOL(const Str &line);
  static bool hasEOL(const Str &line);
  
  static void consumeEOL(Str *line);
  static void consumeToEOL(Str *line);
  static void consumeNumber(Str *line);
  
  static void itoahex(char buf[2], char i);
  static int ahextoi(const char *hexascii, int len);

  static bool isNumber(const char *);

  static void urlEncodePrint(Stream &stream, const char *msg);

  static const char *replace(Str *result, const char *orig, const char *match, const char *repl);
  
  static Str TAG(const char *func, const char *msg);
};

#endif
