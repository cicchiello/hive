#include <AudioUpload.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <RateProvider.h>

#include <http_binaryput.h>
#include <http_dataprovider.h>

#include <sdutils.h>

#include <SdFat.h>

#include <hiveconfig.h>

#include <couchutils.h>
#include <http_couchpost.h>


class MyDataProvider : public HttpDataProvider {
public:
    static const int BUFSZ = 1024;
  
private:
    unsigned char buf[BUFSZ];
    Str mFilename;
    SdFat mSd;
    SdFile mF;
    bool mIsDone;
    int mSz;
  
public:
    MyDataProvider(const char *filename)
      : mIsDone(false), mFilename(filename)
    {
        TF("MyDataProvider::MyDataProvider");
        init();
    }

    ~MyDataProvider();

    void init()
    {
        TF("MyDataProvider::init");
	TRACE("entry");
        SDUtils::initSd(mSd);
	bool stat = mF.open(mFilename.c_str(), O_READ);
	assert2(stat,"Couldn't open file",mFilename.c_str());
	mSz = mF.fileSize();
    }
  
    void start() {}

    int takeIfAvailable(unsigned char **_buf) {
        TF("MyDataProvider::takeIfAvailable");
	int avail = mSz - mF.curPosition();
	if (avail > 1024) avail = 1024;
	int n = mF.read(buf, avail);
	if (n > 0) {
	    TRACE(n == avail ? "providing full buffer" : "providing partially full buffer");
	    mIsDone = mF.curPosition() == mSz;
	} else {
	    PH("Error encounted");
	    mIsDone = true;
	    n = 0;
	}
	*_buf = buf;
	return n;
    }

    bool isDone() const {
        TF("MyDataProvider::isDone");
	return mIsDone;
    }
  
    int getSize() const {
        TF("MyDataProvider::getSize");
	return mSz;
    }
  
    void close() {
        TF("MyDataProvider::close");
	mF.close();
    }

    void reset() {
        TF("MyDataProvider::reset");
        init();
    }

    void giveBack(unsigned char *b) {
        TF("MyDataProvider::giveBack");
	assert(b == buf, "Unexpected value supplied");
    }
};


MyDataProvider::~MyDataProvider()
{
    TF("MyDataProvider::~MyDataProvider");
    mF.close();
}



AudioUpload::AudioUpload(const HiveConfig &config,
			 const char *sensorName,
			 const char *attachmentDescription,
			 const char *attachmentName,
			 const char *contentType,
			 const char *filename, 
			 const class RateProvider &rateProvider,
			 unsigned long now,
			 Mutex *wifiMutex, Mutex *sdMutex)
  : SensorBase(config, sensorName, rateProvider, now, wifiMutex),
    mSdMutex(sdMutex),
    mBinaryPutter(NULL), mDataProvider(NULL),
    mAttachmentName(new Str(attachmentName)), mDocId(new Str()), mRevision(new Str()),
    mContentType(new Str(contentType)), mFilename(new Str(filename)),
    mAttDesc(new Str(attachmentDescription)),
    mIsDone(false), mHaveDocId(false)
{
    TF("AudioUpload::AudioUpload");
    setNextPostTime(now+100l);
    setSample(*mAttDesc);
}


AudioUpload::~AudioUpload()
{
    delete mDocId;
    delete mRevision;
    delete mAttachmentName;
    delete mAttDesc;
    delete mContentType;
    delete mFilename;
    delete mBinaryPutter;
    delete mDataProvider;
}


bool AudioUpload::isItTimeYet(unsigned long now)
{
    return now >= getNextPostTime();
}


bool AudioUpload::sensorSample(Str *valueStr)
{
    *valueStr = *mAttDesc;
    return true;
}


bool AudioUpload::loop(unsigned long now)
{
    TF("AudioUpload::loop");
    
    if (mIsDone)
        return false;

    if (!mHaveDocId) {
        TF("AudioUpload::loop; deferring to SensorBase");
        return SensorBase::loop(now);
    } else {
        TF("AudioUpload::loop; my processing");
	unsigned long callMeBackIn_ms = 10l;
        const char *objName = getName();
	bool muticesOwned = (getWifiMutex()->whoOwns() == this) && (mSdMutex->whoOwns() == this);
	if (!muticesOwned) {
	    bool muticesAvailable = getWifiMutex()->isAvailable() && mSdMutex->isAvailable();
	    if (muticesAvailable) {
	        TRACE("Both Mutexes aquired");
		bool ownWifi = getWifiMutex()->own(this);
		bool ownSd = mSdMutex->own(this);
		assert(ownWifi && ownSd, "ownWifi && ownSd");
		muticesOwned = true;
	    }
	}
        if (muticesOwned) {
	    bool done = false;
	    if (mBinaryPutter == 0) {
		PH2("Starting attachment upload; now: ", millis());
	        const char *docId = mDocId->c_str();
		const char *attName = mAttachmentName->c_str();
		const char *rev = mRevision->c_str();
		StrBuf url;
		CouchUtils::toAttachmentPutURL(getConfig().getLogDbName().c_str(),
					       docId, attName, rev, 
					       &url);
	    
		//TRACE2("creating binary attachment PUT with url: ", url.c_str());
		//TRACE2("thru wifi: ", getConfig().getSSID());
		//TRACE2("with pswd: ", getConfig().getPSWD());
		//TRACE2("to host: ", getConfig().getDbHost());
		//TRACE2("port: ", getConfig().getDbPort());
		//TRACE2("using ssl? ", (getConfig().isSSL() ? "yes" : "no"));
		//TRACE2("with dbuser: ", getConfig().getDbUser());
		//TRACE2("with dbpswd: ", getConfig().getDbPswd());
		PH2("uploading attachment to doc: ", url.c_str());

		mDataProvider = new MyDataProvider(mFilename->c_str());
		mBinaryPutter = new HttpBinaryPut(getConfig().getSSID(), getConfig().getPSWD(),
						  getConfig().getDbHost(), getConfig().getDbPort(),
						  url.c_str(),
						  getConfig().getDbUser(), getConfig().getDbPswd(),
						  getConfig().isSSL(),
						  mDataProvider, mContentType->c_str());
	    } else {
	        TF("AudioUpload::loop; processing HttpBinaryPut event");
	        HttpBinaryPut::EventResult r = mBinaryPutter->event(now, &callMeBackIn_ms);
		if (!mBinaryPutter->processEventResult(r)) {
		    TF("AudioUpload::loop; processing HttpBinaryPut result");
		    mIsDone = true;
		    bool retry = true;
		    if (mBinaryPutter->getFinalResult() == HttpOp::HTTPSuccessResponse) {
		        if (mBinaryPutter->haveDoc()) {
			    if (mBinaryPutter->getDoc().lookup("ok") >= 0) {
			        TRACE("doc successfully uploaded!");
				assert(mDataProvider->isDone(),
				       "upload appeared to succeed, but not at EOF");
				retry = false;
				PH2("Upload done; now: ", millis());
			    } else {
			        StrBuf dump;
				PH2("Received unexpected couch response:",
				    CouchUtils::toString(mBinaryPutter->getDoc(), &dump));
			    }
			} else {
			    PH2("Found unexpected information in the header response:",
				mBinaryPutter->getHeaderConsumer().getResponse().c_str());
			}
		    }  else {
		        PH("unknown failure");
		    }
		    mBinaryPutter->shutdownWifiOnDestruction(retry);
		    delete mBinaryPutter;
		    delete mDataProvider;
		    mBinaryPutter = NULL;
		    mDataProvider = NULL;
		    getWifiMutex()->release(this);
		    mSdMutex->release(this);
		} else {
		    //TRACE2("mBinaryPutter scheduled a callback in (ms): ", callMeBackIn_ms);
		}
	    }
	} else {
	    //TRACE2("Mutices aren't available; now: ", now);
	}
	setNextPostTime(now + callMeBackIn_ms);
	return !mIsDone;
    }
}


bool AudioUpload::processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms,
				bool *keepMutex, bool *success)
{
    TF("AudioUpload::processResult");

    bool callMeBack = true;
    mIsDone = true; // only one path is *not* done, so handle other cases
    CouchUtils::Doc resultDoc;
    const char *remainder = consumer.parseDoc(&resultDoc);
    if (remainder != NULL) {
        int idIndex = resultDoc.lookup("id");
	int revIndex = resultDoc.lookup("rev");
	if (idIndex >= 0 && resultDoc[idIndex].getValue().isStr() &&
	    revIndex >= 0 && resultDoc[revIndex].getValue().isStr()) {
	    const Str &id = resultDoc[idIndex].getValue().getStr();
	    const Str &rev = resultDoc[revIndex].getValue().getStr();
	    *mDocId = id;
	    *mRevision = rev;
	    mHaveDocId = true;
	    mIsDone = false;
	    *keepMutex = true;
	    *success = true;
	    TRACE4("saved doc to id/rev: ", id.c_str(), "/", rev.c_str());
	} else {
	    PH("Unexpected problem with the result");
	}
    } else {
        PH("Unexpected problem with the result");
    }

    TRACE2("returning: ", (callMeBack?"true":"false"));
    return callMeBack; // indicate done (whether successful or not)
}

