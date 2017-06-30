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
#define IS_LOWER_A_THRU_F(c) (((c) >= 'a') && ((c) <= 'f'))
#define IS_UPPER_A_THRU_F(c) (((c) >= 'A') && ((c) <= 'F'))


JParse::JParse(JSONConsumer *consumer)
  : mState(0), mIsValid(true), mConsumer(consumer),
    mCharCnt(0), mChunkSz(0), mIsChunked(false)
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

void JParse::clear()
{
  while (mState)
    pop();
  mIsValid = true;
}


void JParse::inc(const char **ptr)
{
  (*ptr)++;
  if (mIsChunked) mCharCnt--;
}

void JParse::advance(int inc, const char **ptr)
{
  (*ptr) += inc;
  if (mIsChunked) mCharCnt -= inc;
}


bool JParse::streamParseDoc(const char *snippet)
{
  TF("JParse::streamParseDoc");
  
  assert(streamIsValid(), "invalid stream");
  assert(snippet, "invalid stream");
    
  bool done = false;
  while (!done) {

    if (*snippet == 0)
      return false;

    done = true; // cases that need to repeat will change to false
    StrState s = mState ? mState->strState : OPEN;

    if (mIsChunked && (mCharCnt<=0)) {
      if (mCharCnt == 0) mChunkSz = 0;
      bool case0 = IS_DIGIT(*snippet);
      bool case1 = IS_LOWER_A_THRU_F(*snippet);
      bool case2 = IS_UPPER_A_THRU_F(*snippet);
      bool case3 = *snippet == 0x0d;
      bool case4 = *snippet == 0x0a;
      if (case0 || case1 || case2) {
	mChunkSz *= 16;
	if (case0) mChunkSz += *snippet - '0';
	if (case1) mChunkSz += (*snippet - 'a') + 10;
	if (case2) mChunkSz += (*snippet - 'A') + 10;
      } else if (case3) {
      } else if (case4) {
	mCharCnt = mChunkSz+3;
      }
      s = CHUNKSZ;
    } 

    TRACE2("mCharCnt: ", mCharCnt);
    TRACE2("mIsChunked: ", (mIsChunked ? "true":"false"));
    TRACE2("snippet: ", snippet);
    TRACE2("*snippet: ", *snippet);
    TRACE2("int(*snippet): ", int(*snippet));
    TRACE2("mChunkSz: ", mChunkSz);
 
    switch (s) {

    case CHUNKSZ: {
      inc(&snippet); 
      done = false;
    }; break;
      
    case OPEN: {
      if (*snippet == '{') {
	push();
	mConsumer->openDoc();
	mState->strState = NV_START;
      } else if (*snippet == '[') {
	push();
	mConsumer->openArr();
        mState->strState = IN_ARR;
      }
      done = false;
      inc(&snippet); 
    }; break;
    
    case NV_START: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker != 0) {
	const char *tmarker = StringUtils::eatPunctuation(marker, '}');
	if (tmarker) {
	  pop();
	  mConsumer->closeDoc();
	  advance(tmarker-snippet+1, &snippet);
	  done = false;
	} else if (*marker == '"') {
	  advance(marker-snippet+1, &snippet);
	  mState->strState = NV_NAME;
	  mState->buf = "";
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected a name for a name-value pair: ", snippet);
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case NV_NAME: {
      const char *marker = StringUtils::getToken(snippet, &mState->buf);
      if (*marker) {
	if (*marker == '"') {
	  advance(marker-snippet+1, &snippet);
	  mConsumer->nv_name(mState->buf.c_str());
	  mState->buf = "";
	  mState->strState = NV_COLON;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Improperly terminated identifier: ", mState->buf.c_str());
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case NV_COLON: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker) {
	if (*marker == ':') {
	  advance(marker-snippet+1, &snippet);
	  mState->strState = NV_VALUE;
	  done = false;
	} else {
	  mState->strState = ERROR;
	  ERR2("Expected ':'", snippet);
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case NV_VALUE: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker) {
	if (*marker == '{') {
	  // value is a document -- push state
	  advance(marker-snippet+1, &snippet);
	  mState->strState = NV_START;
	  mConsumer->openDoc();
	  done = false;
	} else if (*marker == '[') {
	  // value is an array -- push state
	  advance(marker-snippet+1, &snippet);
	  mState->strState = COMMA_OR_CLOSE_DOC;
	  mState->buf = "";
	  push();
	  mState->strState = IN_ARR;
	  mConsumer->openArr();
	  done = false;
	} else if (*marker == '"') {
	  // value is a token
	  advance(marker-snippet+1, &snippet);
	  done = false;
	  mState->strState = NV_VALUE_STRING;
	  mState->buf = "";
	} else if (IS_SIGN(*marker)) {
	  advance(marker-snippet+1, &snippet);
	  mState->sign = *marker == '-';
	  done = false;
	  mState->strState = NV_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else if (IS_DIGIT(*marker)) {
	  advance(marker-snippet, &snippet);
	  mState->sign = false;
	  done = false;
	  mState->strState = NV_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected a value: ", snippet);
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case IN_ARR: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker != 0) {
	const char *tmarker = StringUtils::eatPunctuation(marker, ']');
	if (tmarker) {
	  // pop state
	  advance(tmarker-snippet, &snippet);
	  pop();
	  mConsumer->closeArr();
	  done = false;
	} else {
	  advance(marker-snippet, &snippet);
	  mState->strState = ARR_VALUE;
	  mState->buf = "";
	  done = false;
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case ARR_VALUE: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker != 0) {
	if (*marker == '"') {
	  advance(marker-snippet+1, &snippet);
	  mState->strState = ARR_STRING;
	  mState->buf = "";
	  done = false;
	} else if (IS_SIGN(*marker)) {
	  advance(marker-snippet+1, &snippet);
	  mState->sign = *marker == '-';
	  done = false;
	  mState->strState = ARR_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else if (IS_DIGIT(*marker)) {
	  advance(marker-snippet, &snippet);
	  mState->sign = false;
	  done = false;
	  mState->strState = ARR_INTEGER;
	  mState->number = 0;
	  mState->buf = "";
	} else if (*marker == '[') {
	  mState->strState = ERROR;
	} else if (*marker == '{') {
	  advance(marker-snippet+1, &snippet);
	  mState->strState = COMMA_OR_CLOSE_ARR;
	  mState->buf = "";
	  push();
	  mState->strState = NV_START;
	  mConsumer->openDoc();
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected a name for a name-value pair: ", snippet);
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;
      
    case ARR_STRING: {
      const char *marker = StringUtils::getToken(snippet, &mState->buf);
      if (*marker) {
	if (*marker == '"') {
	  advance(marker-snippet+1, &snippet);
	  mConsumer->arr_string(mState->buf.c_str());
	  mState->buf = "";
	  mState->strState = COMMA_OR_CLOSE_ARR;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Improperly terminated identifier: ", mState->buf.c_str());
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case COMMA_OR_CLOSE_ARR: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker) {
	if (*marker == ',') {
	  advance(marker-snippet+1, &snippet);
	  mState->strState = ARR_VALUE;
	  done = false;
	} else if (*marker == ']') {
	  advance(marker-snippet+1, &snippet);
	  pop();
	  mConsumer->closeArr();
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected ',' or ']'", snippet);
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;
      
    case COMMA_OR_CLOSE_DOC: {
      const char *marker = StringUtils::eatWhitespace(snippet);
      if (*marker) {
	if (*marker == ',') {
	  advance(marker-snippet+1, &snippet);
	  mState->strState = NV_START;
	  done = false;
	} else if (*marker == '}') {
	  advance(marker-snippet+1, &snippet);
	  pop();
	  mConsumer->closeDoc();
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Expected ',' or '}', got: ", snippet);
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;
      
    case NV_INTEGER: {
      // then we have a number -- but it may not all be in the snippet
      const char *marker = StringUtils::eatWhitespace(snippet);
      const char *p = marker;
      while (*p && IS_DIGIT(*p)) {
	mState->number *= 10;
	mState->number += (*p - '0');
	p++;
      }
      advance(p-snippet, &snippet);
      if (*p) {
	// then let's assume that we've hit the number terminator
	mConsumer->nv_valueInteger(mState->sign ? -mState->number : mState->number);
	mState->strState = COMMA_OR_CLOSE_DOC;
	done = false;
      }
    }; break;
      
    case NV_VALUE_STRING: {
      const char *marker = StringUtils::getToken(snippet, &mState->buf);
      if (*marker) {
	if (*marker == '"') {
	  advance(marker-snippet+1, &snippet);
	  mConsumer->nv_valueString(mState->buf.c_str());
	  mState->strState = COMMA_OR_CLOSE_DOC;
	  done = false;
	} else {
	  mIsValid = false;
	  mState->strState = ERROR;
	  ERR2("Improperly terminated identifier: ", mState->buf.c_str());
	}
      } else {
	advance(marker-snippet, &snippet);
      }
    }; break;

    case ERROR: 
    default: ERR2("Invalid state: ", mState->strState);
    
    }
  }

  return false;
}

