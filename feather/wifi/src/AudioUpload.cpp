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
#include <http_jsonconsumer.h>


class MyDataProvider : public HttpDataProvider {
public:
    static const int BUFSZ = 1024;
  
private:
    unsigned char buf[BUFSZ];
    SdFat mSd;
    SdFile mF;
    bool mIsDone;
    int mSz;
  
public:
    MyDataProvider(const char *filename)
      : mIsDone(false)
    {
        TF("MyDataProvider::MyDataProvider");
        init(filename);
    }

    ~MyDataProvider();

    void init(const char *filename)
    {
        TF("MyDataProvider::init");
	TRACE("entry");
        SDUtils::initSd(mSd);
	bool stat = mF.open(filename, O_READ);
	assert2(stat,"Couldn't open file",filename.c_str());
	mSz = mF.fileSize();
    }
  
    void start() {}

    int takeIfAvailable(unsigned char **_buf) {
        TF("MyDataProvider::takeIfAvailable");
	int avail = mSz - mF.curPosition();
	if (avail > BUFSZ) avail = BUFSZ;
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
	assert(0, "unimplemented");
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
			 const Str &attachmentDescription,
			 const Str &attachmentName,
			 const Str &contentType,
			 const Str &filename, 
			 const class RateProvider &rateProvider,
			 unsigned long now,
			 Mutex *wifiMutex, Mutex *sdMutex)
  : SensorBase(config, sensorName, rateProvider, now, wifiMutex),
    mSdMutex(sdMutex),
    mBinaryPutter(NULL), mDataProvider(NULL),
    mAttachmentName(attachmentName), mDocId(), mRevision(),
    mContentType(contentType), mFilename(filename),
    mAttDesc(attachmentDescription),
    mIsDone(false), mHaveDocId(false)
{
    TF("AudioUpload::AudioUpload");
    setNextPostTime(now+100l);
    setSample(mAttDesc);
}


AudioUpload::~AudioUpload()
{
    delete mBinaryPutter;
    delete mDataProvider;
}


bool AudioUpload::isItTimeYet(unsigned long now)
{
    return now >= getNextPostTime();
}


bool AudioUpload::sensorSample(Str *valueStr)
{
    *valueStr = mAttDesc;
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
	bool wifiMutexIsOwned = getWifiMutex()->whoOwns() == this;
	bool sdMutexIsOwned = mSdMutex->whoOwns() == this;
	bool muticesOwned = wifiMutexIsOwned && sdMutexIsOwned;
	if (!muticesOwned) {
	    bool wifiMutexIsOwnedOrAvailable = wifiMutexIsOwned || getWifiMutex()->isAvailable();
	    bool sdMutexIsOwnedOrAvailable = sdMutexIsOwned || mSdMutex->isAvailable();
	    if (wifiMutexIsOwnedOrAvailable && sdMutexIsOwnedOrAvailable) {
		bool ownWifi = getWifiMutex()->own(this);
		bool ownSd = mSdMutex->own(this);
		assert(ownWifi && ownSd, "ownWifi && ownSd");
	        TRACE("Both Mutexes aquired");
		muticesOwned = true;
	    }
	}
        if (muticesOwned) {
	    bool done = false;
	    if (mBinaryPutter == 0) {
	        TF("AudioUpload::loop; creating putter");
		PH2("Starting attachment upload; now: ", millis());
		
		static const char *urlPieces[9];
		urlPieces[0] = "/";
		urlPieces[1] = getConfig().getLogDbName().c_str();
		urlPieces[2] = urlPieces[0];
		urlPieces[3] = mDocId.c_str();
		urlPieces[4] = urlPieces[0];
		urlPieces[5] = mAttachmentName.c_str();
		urlPieces[6] = "?rev=";
		urlPieces[7] = mRevision.c_str();
		urlPieces[8] = 0;
    
		//TRACE2("creating binary attachment PUT with url: ", url.c_str());
		//TRACE2("thru wifi: ", getConfig().getSSID().c_str());
		//TRACE2("with pswd: ", getConfig().getPSWD().c_str());
		//TRACE2("to host: ", getConfig().getDbHost().c_str());
		//TRACE2("port: ", getConfig().getDbPort());
		//TRACE2("using ssl? ", (getConfig().isSSL() ? "yes" : "no"));
		//TRACE2("with dbuser: ", getConfig().getDbUser().c_str());
		//TRACE2("with dbpswd: ", getConfig().getDbPswd().c_str());

		mDataProvider = new MyDataProvider(mFilename.c_str());
		mBinaryPutter = new HttpBinaryPut(getConfig().getSSID(), getConfig().getPSWD(),
						  getConfig().getDbHost(), getConfig().getDbPort(),
						  getConfig().getDbUser(), getConfig().getDbPswd(),
						  getConfig().isSSL(),
						  mDataProvider, mContentType, urlPieces);
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
		        PH2("Unexpected failure; mBinaryPutter->getFinalResult(): ", mBinaryPutter->getFinalResult());
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


bool AudioUpload::processResult(const HttpJSONConsumer &consumer, unsigned long *callMeBackIn_ms,
				bool *keepMutex, bool *success)
{
    TF("AudioUpload::processResult");

    bool callMeBack = true;
    mIsDone = true; // only one path is *not* done, so handle other cases
    const CouchUtils::Doc &resultDoc = consumer.getDoc();
    int idIndex = resultDoc.lookup("id");
    int revIndex = resultDoc.lookup("rev");
    if (idIndex >= 0 && resultDoc[idIndex].getValue().isStr() &&
	revIndex >= 0 && resultDoc[revIndex].getValue().isStr()) {
        mDocId = resultDoc[idIndex].getValue().getStr();
	mRevision = resultDoc[revIndex].getValue().getStr();
	mHaveDocId = true;
	mIsDone = false;
	*keepMutex = true;
	*success = true;
	TRACE4("saved doc to id/rev: ", mDocId.c_str(), "/", mRevision.c_str());
    } else {
        PH2("Unexpected problem with the result: ", consumer.getResponse().c_str());
    }

    TRACE2("returning: ", (callMeBack?"true":"false"));
    return callMeBack; // indicate done (whether successful or not)
}

