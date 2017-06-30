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

  bool mHaveSomething, mVerbose;
  CouchUtils::Doc *mResultDoc;
  CouchUtils::Arr *mResultArr;
  
  void push(State *n);
  void pop();

public:
  CouchDocConsumer(CouchUtils::Doc *resultDoc)
    : state(0), mResultDoc(resultDoc), mResultArr(0), mHaveSomething(false), mVerbose(false) {}
  CouchDocConsumer(CouchUtils::Arr *resultArr)
    : state(0), mResultArr(resultArr), mResultDoc(0), mHaveSomething(false), mVerbose(false) {}
  ~CouchDocConsumer();

  void clear();

  void setVerbose(bool v) {mVerbose = v;}
  
  bool isValid() const {return state;}

  bool haveDoc() const {return mHaveSomething && mResultDoc;}
  bool haveArr() const {return mHaveSomething && mResultArr;}
  bool isDoc() const {return mResultDoc;}
  bool isArr() const {return mResultArr;}
  
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
