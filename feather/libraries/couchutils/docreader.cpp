#include <docreader.h>

#include <Arduino.h>

#define NDEBUG
#include <strutils.h>

#include <str.h>
#include <strbuf.h>

#include <Trace.h>

#include <sdutils.h>

#include <SdFat.h>



DocReader::DocReader(const char *filename)
  : mDoc(), mErrMsg(new StrBuf("no error")), mFilename(new Str(filename)),
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

static bool loadFile(const char *filename, StrBuf *contents, StrBuf *errMsg)
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
    while ((stat = SDUtils::readline(&f, buf, bufsz)) != SDUtils::ReadEOF) {
        while (stat == SDUtils::ReadBufOverflow) {
	    char *newBuf = new char[bufsz*2];
	    memcpy(newBuf, buf, bufsz);
	    delete [] buf;
	    buf = newBuf;
	    newBuf = buf + bufsz;
	    stat = SDUtils::readline(&f, newBuf, bufsz);
	    bufsz *= 2;
	}
	TRACE(buf);
	contents->append(buf);
    }

    delete [] buf;
    return true;
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

	StrBuf rawContents;
	if (!loadFile(mFilename->c_str(), &rawContents, mErrMsg)) {
	    // errMsg set within loadFile
	    return false;
	}
	TRACE(rawContents.c_str());
	
	CouchUtils::Doc doc;
	const char *remainder = CouchUtils::parseDoc(rawContents.c_str(), &doc);
	if (remainder == NULL) {
	    *mErrMsg = "Parsing error: ";
	    mErrMsg->append(rawContents.c_str());
	    return false;
	}

	mDoc = doc;
	mHasDoc = true;
    }
    
    return false;
}


