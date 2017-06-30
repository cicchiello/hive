#ifndef jparse_h
#define jparse_h

#include <strbuf.h>

class JSONConsumer;

class JParse {
 private:
  enum StrState {
    UNDEFINED_STATE,
    CHUNKSZ,
    OPEN,
    NV_START,
    NV_NAME,
    NV_COLON,
    NV_VALUE,
    NV_VALUE_STRING,
    NV_INTEGER,
    COMMA_OR_CLOSE_DOC,
    COMMA_OR_CLOSE_ARR,
    IN_ARR,
    ARR_VALUE,
    ARR_STRING,
    ARR_ARR,
    ARR_INTEGER,
    ERROR,
    LAST_STATE
  };
  
  struct ParseState {
    ParseState()
      : strState(UNDEFINED_STATE), sign(false), number(0), buf(), next(0)  {}
    ParseState(const ParseState &o)
      : strState(o.strState), sign(o.sign), number(o.number), buf(o.buf), next(o.next) {}
    StrState strState;
    bool sign;
    int number;
    StrBuf buf;
    ParseState *next;
  } *mState;
  
  bool mIsValid, mIsChunked;
  int mCharCnt, mChunkSz;
  JSONConsumer *mConsumer;

  void inc(const char **ptr);
  void advance(int inc, const char **ptr);
  
 public:
  JParse(JSONConsumer *consumer);
  ~JParse();

  void push();
  void pop();

  void clear();

  void setIsChunked(bool v) {mIsChunked = v;}
  
  // streamParseDoc
  //    snippet : is a piece of the stream, and is assumed to be null-terminated of arbitrary length;
  //            each snippet should only be provided once;
  //            JParse will take a copy of any partial strings it might need
  //
  // should not be called unless streamIsValid() remains true
  //
  // returns true when *doc is complete
  bool streamParseDoc(const char *snippet);

  bool streamIsValid() const {return mIsValid;}
};


#endif
