#include <AudioUpload.h>

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



class MyDataProvider : public HttpDataProvider {
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
			 const char *attachmentName,
			 const char *contentType,
			 const char *filename, 
			 const class RateProvider &rateProvider,
			 const class TimeProvider &timeProvider,
			 unsigned long now,
			 Mutex *wifiMutex, Mutex *sdMutex)
  : SensorBase(config, sensorName, rateProvider, timeProvider, now, wifiMutex),
    mTransferStart(0), mSdMutex(sdMutex),
    mBinaryPutter(NULL), mDataProvider(NULL),
    mAttachmentName(new Str(attachmentName)), mDocId(new Str()), mRevision(new Str()),
    mContentType(new Str(contentType)), mFilename(new Str(filename)),
    mIsDone(false), mHaveDocId(false)
{
    TF("AudioUpload::AudioUpload");
    TRACE2("now: ", now);
}


AudioUpload::~AudioUpload()
{
    delete mDocId;
    delete mRevision;
    delete mAttachmentName;
    delete mContentType;
    delete mFilename;
    delete mBinaryPutter;
    delete mDataProvider;
}


void AudioUpload::setAttachmentName(const char *attName)
{
    *mAttachmentName = attName;
}


bool AudioUpload::isItTimeYet(unsigned long now)
{
    return now >= getNextPostTime();
}


bool AudioUpload::sensorSample(Str *valueStr)
{
    *valueStr = "10s-audio-clip";
    return true;
}


bool AudioUpload::loop(unsigned long now)
{
    TF("AudioUpload::loop");
    
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
	        TF("AudioUpload::loop; creating HttpBinaryPut");
	        const char *docId = mDocId->c_str();
		const char *attName = mAttachmentName->c_str();
		const char *rev = mRevision->c_str();
		Str url;
		CouchUtils::toAttachmentPutURL(getConfig().getLogDbName(),
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
		    if (mBinaryPutter->getFinalResult() == HttpOp::HTTPSuccessResponse) {
		        if (mBinaryPutter->haveDoc()) {
			    if (mBinaryPutter->getDoc().lookup("ok") >= 0) {
			        TRACE("doc successfully uploaded!");
				assert(mDataProvider->isDone(),
				       "upload appeared to succeed, but not at EOF");
				unsigned long transferTime = now - mTransferStart;
				TRACE2("Transfer completed after (ms): ", transferTime);
			    } else {
			        Str dump;
				TRACE2("Received unexpected couch response:",
				       CouchUtils::toString(mBinaryPutter->getDoc(), &dump));
			    }
			} else {
			    TRACE2("Found unexpected information in the header response:",
				   mBinaryPutter->getHeaderConsumer().getResponse().c_str());
			}
		    }  else {
		        TRACE("unknown failure");
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


bool AudioUpload::processResult(const HttpCouchConsumer &consumer, unsigned long *callMeBackIn_ms)
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
	    PH2("saved doc to id/rev: ", Str(id).append("/").append(rev).c_str());
	}
    } else {
        PH("Unexpected problem with the result");
    }

    TRACE2("returning: ", (callMeBack?"true":"false"));
    return callMeBack; // indicate done (whether successful or not)
}

