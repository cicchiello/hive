#include <AttachmentUpload.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <Mutex.h>

#include <RateProvider.h>
#include <TimeProvider.h>

#include <http_binaryput.h>
#include <http_dataprovider.h>

#include <sdutils.h>

#include <SdFat.h>

#include <hiveconfig.h>

#include <couchutils.h>
#include <http_couchpost.h>

#include <strbuf.h>


class AttachmentDataProvider : public HttpDataProvider {
public:
    static const int BUFSZ = 512;
  
private:
    unsigned char buf[BUFSZ];
    Str mFilename;
    SdFat mSd;
    SdFile mF;
    bool mIsDone;
    int mSz;
  
public:
    AttachmentDataProvider(const char *filename)
      : mIsDone(false), mFilename(filename)
    {
        TF("AttachmentDataProvider::AttachmentDataProvider");
        init();
    }

    ~AttachmentDataProvider();

    void init()
    {
        TF("AttachmentDataProvider::init");
	TRACE("entry");
        SDUtils::initSd(mSd);
	bool stat = mF.open(mFilename.c_str(), O_READ);
	assert2(stat,"Couldn't open file",mFilename.c_str());
	mSz = mF.fileSize();
    }
  
    void start() {}

    int takeIfAvailable(unsigned char **_buf) {
        TF("AttachmentDataProvider::takeIfAvailable");
        int b = 0, i = 0;
	while (mF.curPosition() < mSz) {
	    b = mF.read();
            if (b == -1) {
		TRACE("EOF encountered; returning buffer");
	        mIsDone = true;
		*_buf = buf;
	        return i;
	    } else {
	        char c = (char) (b & 0xff);
		buf[i++] = c;
		if (i >= BUFSZ) {
		    //TRACE2("returning a buffer of size ", i);
		    *_buf = buf;
		    return i;
		}
	    }
	}
	TRACE("returning empty or partial buffer");
	*_buf = buf;
	mIsDone = true;
	return i;
    }

    bool isDone() const {
        TF("AttachmentDataProvider::isDone");
	return mIsDone;
    }
  
    int getSize() const {
        TF("AttachmentDataProvider::getSize");
	return mSz;
    }
  
    void close() {
        TF("AttachmentDataProvider::close");
	mF.close();
    }

    void reset() {
        TF("AttachmentDataProvider::reset");
        init();
    }

    void giveBack(unsigned char *b) {
        TF("AttachmentDataProvider::giveBack");
	assert(b == buf, "Unexpected value supplied");
    }
};


AttachmentDataProvider::~AttachmentDataProvider()
{
    TF("AttachmentDataProvider::~AttachmentDataProvider");
    mF.close();
}



AttachmentUpload::AttachmentUpload(const HiveConfig &config,
			 const char *sensorName,
			 const char *contentType,
			 const class RateProvider &rateProvider,
			 const class TimeProvider &timeProvider,
			 unsigned long now,
			 Mutex *wifiMutex, Mutex *sdMutex)
  : SensorBase(config, sensorName, rateProvider, timeProvider, now, wifiMutex),
    mTransferStart(0), mSdMutex(sdMutex),
    mBinaryPutter(NULL), mDataProvider(NULL),
    mContentType(contentType),
    mIsDone(false), mHaveDocId(false)
{
    TF("AttachmentUpload::AttachmentUpload");
}


AttachmentUpload::~AttachmentUpload()
{
    delete mBinaryPutter;
    delete mDataProvider;
}


bool AttachmentUpload::isItTimeYet(unsigned long now)
{
    return now >= getNextPostTime();
}


bool AttachmentUpload::sensorSample(Str *valueStr)
{
    *valueStr = logValue();
    return true;
}


bool AttachmentUpload::loop(unsigned long now)
{
    TF("AttachmentUpload::loop");
    
    if (mIsDone)
        return false;

    if (!mHaveDocId) {
        return SensorBase::loop(now);
    } else {
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
        if ((now >= getNextPostTime()) && muticesOwned) {
	    unsigned long callMeBackIn_ms = 10l;
	    bool done = false;
	    if (mBinaryPutter == 0) {
	        TF("AttachmentUpload::loop; creating HttpBinaryPut");
	        const char *docId = mDocId.c_str();
		const char *attName = mAttachmentName.c_str();
		const char *rev = mRevision.c_str();
		StrBuf url;
		CouchUtils::toAttachmentPutURL(getConfig().getLogDbName().c_str(),
					       docId, attName, rev, 
					       &url);
	    
		TRACE2("creating binary attachment PUT with url: ", url.c_str());
		TRACE2("thru wifi: ", getConfig().getSSID().c_str());
		TRACE2("with pswd: ", getConfig().getPSWD().c_str());
		TRACE2("to host: ", getConfig().getDbHost().c_str());
		TRACE2("port: ", getConfig().getDbPort());
		TRACE2("using ssl? ", (getConfig().isSSL() ? "yes" : "no"));
		TRACE2("with dbuser: ", getConfig().getDbUser().c_str());
		TRACE2("with dbpswd: ", getConfig().getDbPswd().c_str());

		mDataProvider = new AttachmentDataProvider(mFilename.c_str());
		mBinaryPutter = new HttpBinaryPut(getConfig().getSSID(), getConfig().getPSWD(),
						  getConfig().getDbHost(), getConfig().getDbPort(),
						  url.c_str(),
						  getConfig().getDbUser(), getConfig().getDbPswd(),
						  getConfig().isSSL(),
						  mDataProvider, mContentType.c_str());
		mTransferStart = now;
	    } else {
	        TF("AttachmentUpload::loop; processing HttpBinaryPut event");
	        HttpBinaryPut::EventResult r = mBinaryPutter->event(now, &callMeBackIn_ms);
		if (!mBinaryPutter->processEventResult(r)) {
		    TF("AttachmentUpload::loop; processing HttpBinaryPut result");
		    mIsDone = true;
		    if (mBinaryPutter->getFinalResult() == HttpOp::HTTPSuccessResponse) {
		        if (mBinaryPutter->haveDoc()) {
			    if (mBinaryPutter->getDoc().lookup("ok") >= 0) {
			        TRACE("doc successfully uploaded!");
				assert(mDataProvider->isDone(),
				       "upload appeared to succeed, but not at EOF");
				unsigned long transferTime = now - mTransferStart;
				PH3("Upload completed; upload took ", transferTime, " ms");
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
	    setNextPostTime(now + callMeBackIn_ms);
	}
	return !mIsDone;
    }
}


bool AttachmentUpload::processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms)
{
    TF("AttachmentUpload::processResult");

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
	    mDocId = id;
	    mRevision = rev;
	    mHaveDocId = true;
	    mIsDone = false;
	    PH2("saved doc to id/rev: ", StrBuf(id.c_str()).append("/").append(rev.c_str()).c_str());
	} else {
	    PH("Unexpected problem with the result");
	}
    } else {
        PH("Unexpected problem with the result");
    }

    TRACE2("returning: ", (callMeBack?"true":"false"));
    return callMeBack; // indicate done (whether successful or not)
}

