#ifndef Parse_h
#define Parse_h

class Parse {
 public:
  // assumes we're at the end of the line -- just terminators remain before
  // the end of the string or the beginning of the next line
  static const char *consumeEOL(const char *line);

  // assumes there're arbitrary characters before the line terminator
  static const char *consumeToEOL(const char *line);

  static const char *consumeNumber(const char *line);

  static bool hasEOL(const char *line);
};

#endif
