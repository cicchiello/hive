#include <docreader.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <str.h>
#include <strbuf.h>

#include <Trace.h>

#include <sdutils.h>

#include <SdFat.h>

#include <couchdoc_consumer.h>



DocReader::DocReader(const char *filename)
  : mErrMsg(new StrBuf("no error")), mFilename(new Str(filename)),
    mHasDoc(false), mIsDone(false)
{
    TF("DocReader::DocReader");
    TRACE("initializing DocReader");
}


DocReader::~DocReader()
{
    delete mErrMsg;
    delete mFilename;
}


const char *DocReader::errMsg() const {return mErrMsg->c_str();}


static bool loadFile(const char *filename, JParse *parser, StrBuf *contents, StrBuf *errMsg)
{
    TF("::loadFile");
    
    SdFile f;
    if (!f.open(filename, O_READ)) {
        *errMsg = "Couldn't open file ";
	errMsg->append(filename);
	TRACE(errMsg->c_str());
	return false;
    }
 
    int bufsz = 40;
    char *buf = new char[bufsz];
    SDUtils::ReadlineStatus stat;
    while (parser->streamIsValid() && ((stat = SDUtils::readline(&f, buf, bufsz)) != SDUtils::ReadEOF)) {
        while (stat == SDUtils::ReadBufOverflow) {
	    char *newBuf = new char[bufsz*2];
	    memcpy(newBuf, buf, bufsz);
	    delete [] buf;
	    buf = newBuf;
	    newBuf = buf + bufsz;
	    stat = SDUtils::readline(&f, newBuf, bufsz);
	    bufsz *= 2;
	}
	TRACE2("adding to the stream: ", buf);

	parser->streamParseDoc(buf);
    }
    if (!parser->streamIsValid()) {
        *errMsg = "Error during stream-based parsing";
    }

    delete [] buf;
    return parser->streamIsValid();
}


bool DocReader::loop() {
    TF("DocReader::loop");
 
    if (!mIsDone) {
        mIsDone = true;
	mHasDoc = false;

	SdFat sd;
	SDUtils::initSd(sd);

	// file must exist to begin with
	bool exists = sd.exists(mFilename->c_str());
	if (!exists) {
	    *mErrMsg = mFilename->c_str();
	    mErrMsg->append(" doesn't exist");
	    TRACE(mErrMsg->c_str());
	    return false;
	}

	CouchDocConsumer consumer(&mDoc);
	JParse parser(&consumer);
	StrBuf rawContents;
	if (!loadFile(mFilename->c_str(), &parser, &rawContents, mErrMsg)) {
	    // errMsg set within loadFile
	    return false;
	}
	
	mHasDoc = consumer.haveDoc();
	assert(mHasDoc, "error reading config from file");
    }
    
    return false;
}


