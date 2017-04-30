#include <jparse.h>

#define HEADLESS
#define NDEBUG

#ifdef ARDUINO
#   include <Arduino.h>
#   include <Trace.h>
#else
#   include <CygwinTrace.h>
#endif


#include <json_consumer.h>

#include <strutils.h>
#include <str.h>

#include <cstring>

#define ERR2(arg1,arg2) PH2((arg1),(arg2));
#define IS_SIGN(c) (((c) == '+') || ((c) == '-'))
#define IS_DIGIT(c) (((c) >= '0') && ((c) <= '9'))

JParse::JParse(JSONConsumer *consumer)
  : mState(0), mIsValid(true), mConsumer(consumer)
{
}


JParse::~JParse()
{
  while(mState)
    pop();
}


void JParse::push()
{
  ParseState *n = new ParseState();
  n->next = mState;
  mState = n;
}

void JParse::pop()
{
  ParseState *d = mState;
  mState = d->next;
  d->next = 0;
  delete d;
}

bool JParse::streamParseDoc(const char *chunk)
{
  TF("JParse::streamParseDoc");
  
  assert(streamIsValid(), "invalid stream");
  assert(chunk, "invalid stream");
    
  bool done = false;
  while (!done) {

    if (*chunk == 0)
      return false;

    done = true; // cases that need to repeat will change to false

    StrState s = mState ? mState->strState : OPEN;
    switch (s) {
    
    case OPEN: {
      const char *dmarker = strstr(chunk, "{");
      const char *amarker = strstr(chunk, "[");
      const char *marker = NULL;
      bool isDoc = false;
      if (dmarker && amarker) {
	isDoc = (dmarker < amarker);
	marker = isDoc ? dmarker : amarker;
      } else if (dmarker) {
	isDoc = true;
	marker = dmarker;
      } else if (amarker) {
	isDoc = false;
	marker = amarker;
      }
      if (marker != NULL) {
	push();
	if (isDoc) {
	  mConsumer->openDoc();
	  mState->strState = NV_START;
	} else {
	  mConsumer->openArr();
	  mState->strState = IN_ARR;
	}
	chunk = dmarker+1;
	done = false;
      }
    }; break;
    
    case NV_START: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker != 0) {
	const char *tmarker = StringUtils::eatPunctuation(marker, '}');
	if (tmarker) {
	  pop();
	  mConsumer->closeDoc();
	  chunk = marker+1;
	  done = false;
	} else if (*marker == '"') {
	  mState->strState = NV_NAME;
	  mState->buf = "";
	  chunk = marker + 1;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected a name for a name-value pair: ", chunk);
	}
      }
    }; break;

    case NV_NAME: {
      const char *marker = StringUtils::getToken(chunk, &mState->buf);
      if (*marker) {
	if (*marker == '"') {
	  mConsumer->nv_name(mState->buf.c_str());
	  TRACE2("parsed a name: ", buf.c_str());
	  mState->buf = "";
	  chunk = marker+1;
	  mState->strState = NV_COLON;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Improperly terminated identifier: ", mState->buf.c_str());
	}
      }
    }; break;

    case NV_COLON: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker) {
	if (*marker == ':') {
	  mState->strState = NV_VALUE;
	  chunk = marker+1;
	  done = false;
	} else {
	  mState->strState = ERROR;
	  ERR2("Expected ':'", chunk);
	}
      }
    }; break;

    case NV_VALUE: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker) {
	if (*marker == '{') {
	  // value is a document -- push state
	  mState->strState = NV_START;
	  mConsumer->openDoc();
	  chunk = marker+1;
	  done = false;
	} else if (*marker == '[') {
	  // value is an array -- push state
	  mState->strState = COMMA_OR_CLOSE_DOC;
	  mState->buf = "";
	  push();
	  mState->strState = IN_ARR;
	  mConsumer->openArr();
	  chunk = marker+1;
	  done = false;
	} else if (*marker == '"') {
	  // value is a token
	  done = false;
	  mState->strState = NV_VALUE_STRING;
	  mState->buf = "";
	  chunk = marker+1;
	} else if (IS_SIGN(*marker)) {
	  mState->sign = *marker == '-';
	  chunk = marker+1;
	  done = false;
	  mState->strState = NV_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else if (IS_DIGIT(*marker)) {
	  mState->sign = false;
	  chunk = marker;
	  done = false;
	  mState->strState = NV_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected a value: ", chunk);
	}
      }
    }; break;

    case IN_ARR: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker != 0) {
	const char *tmarker = StringUtils::eatPunctuation(marker, ']');
	if (tmarker) {
	  // pop state
	  pop();
	  mConsumer->closeArr();
	  chunk = marker+1;
	  done = false;
	} else {
	  chunk = marker;
	  mState->strState = ARR_VALUE;
	  mState->buf = "";
	  done = false;
	}
      }
    }; break;

    case ARR_VALUE: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker != 0) {
	if (*marker == '"') {
	  mState->strState = ARR_STRING;
	  mState->buf = "";
	  chunk = marker + 1;
	  done = false;
	} else if (IS_SIGN(*marker)) {
	  mState->sign = *marker == '-';
	  chunk = marker+1;
	  done = false;
	  mState->strState = ARR_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else if (IS_DIGIT(*marker)) {
	  mState->sign = false;
	  chunk = marker;
	  done = false;
	  mState->strState = ARR_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else if (*marker == '[') {
	  mState->strState = ERROR;
	} else if (*marker == '{') {
	  mState->strState = COMMA_OR_CLOSE_ARR;
	  mState->buf = "";
	  push();
	  mState->strState = NV_START;
	  mConsumer->openDoc();
	  chunk = marker+1;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected a name for a name-value pair: ", chunk);
	}
      }
    }; break;
      
    case ARR_STRING: {
      const char *marker = StringUtils::getToken(chunk, &mState->buf);
      if (*marker) {
	if (*marker == '"') {
	  mConsumer->arr_string(mState->buf.c_str());
	  TRACE2("parsed an array string element: ", buf.c_str());
	  mState->buf = "";
	  chunk = marker+1;
	  mState->strState = COMMA_OR_CLOSE_ARR;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Improperly terminated identifier: ", mState->buf.c_str());
	}
      }
    }; break;

    case COMMA_OR_CLOSE_ARR: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker) {
	if (*marker == ',') {
	  mState->strState = ARR_VALUE;
	  chunk = marker+1;
	  done = false;
	} else if (*marker == ']') {
	  pop();
	  mConsumer->closeArr();
	  chunk = marker+1;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected ',' or ']'", chunk);
	}
      }
    }; break;
      
    case COMMA_OR_CLOSE_DOC: {
      const char *marker = StringUtils::eatWhitespace(chunk);
      if (*marker) {
	if (*marker == ',') {
	  mState->strState = NV_START;
	  chunk = marker+1;
	  done = false;
	} else if (*marker == '}') {
	  pop();
	  mConsumer->closeDoc();
	  chunk = marker+1;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected ',' or '}', got: ", chunk);
	}
      }
    }; break;
      
    case NV_INTEGER: {
      // then we have a number -- but it may not all be in the chunk
      const char *marker = StringUtils::eatWhitespace(chunk);
      const char *p = marker;
      while (*p && IS_DIGIT(*p)) {
	mState->number *= 10;
	mState->number += (*p - '0');
	p++;
      }
      if (*p) {
	// then let's assume that we've hit the number terminator
	mConsumer->nv_valueInteger(mState->sign ? -mState->number : mState->number);
	chunk = p;
	mState->strState = COMMA_OR_CLOSE_DOC;
	done = false;
      }
    }; break;
      
    case NV_VALUE_STRING: {
      const char *marker = StringUtils::getToken(chunk, &mState->buf);
      if (*marker) {
	if (*marker == '"') {
	  mConsumer->nv_valueString(mState->buf.c_str());
	  TRACE2("parsed a value: ", mState->buf.c_str());
	  chunk = marker+1;
	  mState->strState = COMMA_OR_CLOSE_DOC;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Improperly terminated identifier: ", mState->buf.c_str());
	}
      }
    }; break;

    case ERROR: 
    default: ERR2("Invalid state: ", mState->strState);
    
    }
  }

  return false;
}

