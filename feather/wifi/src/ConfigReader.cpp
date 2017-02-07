#include <ConfigReader.h>

#define NDEBUG
#include <strutils.h>

#include <str.h>

#include <Trace.h>

#include <sdutils.h>

#include <SdFat.h>



ConfigReader::ConfigReader(const char *filename)
  : mConfig(), mErrMsg(new Str("no error")), mFilename(new Str(filename)),
    mHasConfig(false), mIsDone(false)
{
    TF("ConfigReader::ConfigReader");
    TRACE("initializing ConfigReader");
}


ConfigReader::~ConfigReader()
{
    delete mErrMsg;
    delete mFilename;
}


static bool loadFile(const char *filename, Str *contents, Str *errMsg)
{
    TF("::loadFile");
    TRACE("entry");
    
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


bool ConfigReader::loop() {
    TF("ConfigReader::loop");

    if (!mIsDone) {
        mIsDone = true;
	mHasConfig = false;

TRACE("trace");	
	SdFat sd;
	SDUtils::initSd(sd);
TRACE("trace");	

	// file must exist to begin with
	bool exists = sd.exists(mFilename->c_str());
	if (!exists) {
	    *mErrMsg = *mFilename;
	    mErrMsg->append(" doesn't exist");
	    TRACE(mErrMsg->c_str());
	    return false;
	}
TRACE("trace");	

	Str rawContents;
	if (!loadFile(mFilename->c_str(), &rawContents, mErrMsg)) {
	    // errMsg set within loadFile
	    return false;
	}
	TRACE(rawContents.c_str());
	
	CouchUtils::Doc doc;
	const char *remainder = CouchUtils::parseDoc(rawContents.c_str(), &doc);
	if (remainder == NULL) {
	    *mErrMsg = "Parsing error: ";
	    mErrMsg->append(rawContents);
	    return false;
	}

	mConfig = doc;
	mHasConfig = true;
    }
    
    return false;
}


