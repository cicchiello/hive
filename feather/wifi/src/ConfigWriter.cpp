#include <ConfigWriter.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <str.h>

#include <Trace.h>

#include <sdutils.h>

#include <SdFat.h>



ConfigWriter::ConfigWriter(const char *filename, const CouchUtils::Doc &config)
  : mConfig(config), mIsDone(false)
    //  ,mErrMsg(new Str("no error")), mFilename(new Str(filename))
{
    TF("ConfigWriter::ConfigWriter");
    TRACE("trace");
    mErrMsg = new Str("no error");
    TRACE("trace");
    mFilename = new Str(filename);
    TRACE("initializing ConfigWriter; doc: ");
    //CouchUtils::printDoc(mConfig);
}


ConfigWriter::~ConfigWriter()
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


static bool writeFile(const char *filename, const CouchUtils::Doc &contents, Str *errMsg)
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


bool ConfigWriter::loop() {
    TF("ConfigWriter::loop");

    if (!mIsDone) {
TRACE("doc: ");	
CouchUtils::printDoc(mConfig);
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
	    *mErrMsg = *mFilename;
	    mErrMsg->append(" exists and couldn't be deleted");
	    TRACE(mErrMsg->c_str());
	    return false;
	}
	
	Str rawContents;
	if (!writeFile(mFilename->c_str(), mConfig, mErrMsg)) {
	    // errMsg set within writeFile
	    return false;
	}

TRACE("doc: ");	
CouchUtils::printDoc(mConfig);
	mSuccess = true;
    }
    
    return !mIsDone;
}


