#include <docwriter.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <str.h>
#include <strbuf.h>

#include <Trace.h>

#include <sdutils.h>

#include <SdFat.h>

#include <Mutex.h>


DocWriter::DocWriter(const char *filename, const CouchUtils::Doc &config, Mutex *sdMutex)
  : mDoc(config), mIsDone(false), mSdMutex(sdMutex)
    //  ,mErrMsg(new Str("no error")), mFilename(new Str(filename))
{
    TF("DocWriter::DocWriter");
    TRACE("trace");
    mErrMsg = new StrBuf("no error");
    TRACE("trace");
    mFilename = new Str(filename);
    TRACE("initializing DocWriter; doc: ");
    //CouchUtils::printDoc(mDoc);
}


DocWriter::~DocWriter()
{
    delete mErrMsg;
    delete mFilename;
}


static bool writeDoc(SdFile &f, const CouchUtils::Doc &doc, int indent);

static bool writeNameValuePair(SdFile &f, const CouchUtils::NameValuePair &nv, int indent)
{
    f.write("\"",1);
    f.write(nv.getName().c_str(), nv.getName().len());
    f.write("\" : ", 4);
    if (nv.getValue().isDoc()) {
        f.write("{\n",2);
	writeDoc(f, nv.getValue().getDoc(), indent+4);
	for (int j = 0; j < indent; j++) {
	    f.write(" ", 1);
	}
	f.write("}",1);
    } else {
        f.write("\"", 1);
	f.write(nv.getValue().getStr().c_str(), nv.getValue().getStr().len());
	f.write("\"", 1);
    }
}


/* STATIC */
bool writeDoc(SdFile &f, const CouchUtils::Doc &doc, int indent)
{
    for (int i = 0; i < doc.getSz(); i++) {
        for (int j = 0; j < indent; j++) {
	    f.write(" ",1);
	}
	writeNameValuePair(f, doc[i], indent);
	if (i+1 < doc.getSz()) {
	    f.write(",\n", 2);
	} else {
	    f.write("\n", 1);
	}
    }
}


static bool writeFile(const char *filename, const CouchUtils::Doc &contents, StrBuf *errMsg)
{
    TF("::writeFile");
    TRACE("entry");
    
    SdFile f;
    if (!f.open(filename, O_CREAT | O_WRITE)) {
        *errMsg = "Couldn't create file ";
	errMsg->append(filename);
	TRACE(errMsg->c_str());
	return false;
    }

    f.write("{\n", 2);
    writeDoc(f, contents, 4);
    f.write("}\n", 2);
    f.close();
 
    return true;
}


bool DocWriter::loop() {
    TF("DocWriter::loop");
    TRACE("entry");

    if (!mIsDone && mSdMutex->own(this)) {
        mIsDone = true;
	mSuccess = false;
    
	SdFat sd;
	SDUtils::initSd(sd);

	// file must not exist to begin with -- if it does, delete it
	bool exists = sd.exists(mFilename->c_str());
	if (exists) {
	    sd.remove(mFilename->c_str());
	    exists = sd.exists(mFilename->c_str());
	}
	
	if (exists) {
	    *mErrMsg = mFilename->c_str();
	    mErrMsg->append(" exists and couldn't be deleted");
	    TRACE(mErrMsg->c_str());
	    mSdMutex->release(this);
	    return false;
	}
	
	Str rawContents;
	if (!writeFile(mFilename->c_str(), mDoc, mErrMsg)) {
	    // errMsg set within writeFile
	    mSdMutex->release(this);
	    return false;
	}

	PH2("INFO: wrote doc to file: ", mFilename->c_str());
	mSdMutex->release(this);
	mSuccess = true;
    }
    
    return !mIsDone;
}


const char *DocWriter::errMsg() const {return mErrMsg->c_str();}
