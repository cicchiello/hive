#include <couchdoc_consumer.h>

#define HEADLESS
#define NDEBUG

#ifdef ARDUINO
#   include <Arduino.h>
#   include <Trace.h>
#else
#   include <CygwinTrace.h>
#endif


#include <strbuf.h>
#include <str.h>

#include <stdio.h>


/* STATIC */
bool CouchDocConsumer::sVerbose = false;


void CouchDocConsumer::push(State *n) {
  n->next = state;
  state = n;
}

void CouchDocConsumer::pop() {
  State *d = state;
  state = d->next;
  delete d;
}

void CouchDocConsumer::openDoc() {
  TF("CouchDocConsumer::openDoc");
  push(new DocState());
  if (sVerbose) {PH("entry");}
}

void CouchDocConsumer::closeDoc() {
  TF("CouchDocConsumer::closeDoc");
  if (sVerbose) {PH("entry");}
  DocState *ds = (DocState*)state;
  if (ds->next) {
    if (ds->next->isDoc) {
      DocState *nds = (DocState*)ds->next;
      nds->doc.addNameValue(new CouchUtils::NameValuePair(nds->name.c_str(), ds->doc));
    } else {
      ArrState *nas = (ArrState*)ds->next;
      nas->arr.append(ds->doc);
    }
  } else {
    mIsDoc = true;
    mResultDoc = ds->doc;
  }
  pop();
}

void CouchDocConsumer::openArr() {
  TF("CouchDocConsumer::openArr");
  push(new ArrState());
  if (sVerbose) {PH("entry");}
}

void CouchDocConsumer::closeArr() {
  TF("MyParse::closeArr");
  if (sVerbose) {PH("entry");}
  ArrState *as = (ArrState*)state;
  if (as->next) {
    if (as->next->isDoc) {
      DocState *nds = (DocState*)as->next;
      nds->doc.addNameValue(new CouchUtils::NameValuePair(nds->name.c_str(), as->arr));
    } else {
      ArrState *nas = (ArrState*)as->next;
      nas->arr.append(as->arr);
    }
  } else {
    PH("trace2");      
    mIsDoc = false;
    mResultArr = as->arr;
  }
  pop();
}

void CouchDocConsumer::nv_name(const char *ident) {
  TF("CouchDocConsumer::nv_name");
  if (sVerbose) {PH(ident);}
  DocState *ds = (DocState*) state;
  ds->name = ident;
}

void CouchDocConsumer::nv_valueString(const char *value) {
  TF("CouchDocConsumer::nv_valueString");
  if (sVerbose) {PH(value);}
  DocState *ds = (DocState*) state;
  ds->doc.addNameValue(new CouchUtils::NameValuePair(ds->name.c_str(), value));
}

void CouchDocConsumer::nv_valueInteger(int value) {
  TF("CouchDocConsumer::nv_valueInteger");
  if (sVerbose) {PH(value);}
  DocState *ds = (DocState*) state;
  char buf[10];
  sprintf(buf, "%d", value);
  ds->doc.addNameValue(new CouchUtils::NameValuePair(ds->name.c_str(), buf));
}

void CouchDocConsumer::arr_string(const char *element) {
  TF("CouchDocConsumer::arr_string");
  if (sVerbose) {PH(element);}
  ArrState *as = (ArrState*) state;
  as->arr.append(element);
}

void CouchDocConsumer::arr_integer(int element) {
  TF("CouchDocConsumer::arr_integer");
  if (sVerbose) {PH(element);}
  ArrState *as = (ArrState*) state;
  char buf[10];
  sprintf(buf, "%d", element);
  as->arr.append(buf);
}

