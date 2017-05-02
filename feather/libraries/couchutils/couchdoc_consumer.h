#ifndef couchdoc_consumer_h
#define couchdoc_consumer_h

#include <couchutils.h>
#include <json_consumer.h>


class CouchDocConsumer : public JSONConsumer {
private:
  struct State {
    State(bool _isDoc) : next(0),isDoc(_isDoc) {}
    virtual ~State() {}
    State *next;
    bool isDoc;
  };
  struct DocState : public State {
    DocState() : State(true) {}
    CouchUtils::Doc doc;
    StrBuf name;
  };
  struct ArrState : public State {
    ArrState() : State(false) {}
    CouchUtils::Arr arr;
  };

  State *state;

  bool mIsDoc, mHaveSomething, mIsChunked;
  CouchUtils::Doc mResultDoc;
  CouchUtils::Arr mResultArr;
  
  void push(State *n);
  void pop();

public:
  static bool sVerbose;
  
  CouchDocConsumer()
    : state(0),mIsDoc(false), mHaveSomething(false), mIsChunked(false) {}
  ~CouchDocConsumer() {}

  void clear();

  void setIsChunked(bool v) {mIsChunked = v;}
  
  bool isValid() const {return state;}

  bool haveDoc() const {return mHaveSomething && mIsDoc;}
  bool haveArr() const {return mHaveSomething && !mIsDoc;}
  bool isDoc() const {return mIsDoc;}
  bool isArr() const {return !mIsDoc;}
  
  const CouchUtils::Doc &getDoc() const {return mResultDoc;}
  const CouchUtils::Arr &getArr() const {return mResultArr;}
  
  void openDoc();
  void closeDoc();
  void openArr();
  void closeArr();
  void nv_name(const char *ident);
  void nv_valueString(const char *value);
  void nv_valueInteger(int value);
  void arr_string(const char *element);
  void arr_integer(int element);
};

#endif
