#include <AppChannel.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <Mutex.h>

#include <hive_platform.h>
#include <hiveconfig.h>

#include <MyWiFi.h>
#include <wifiutils.h>

#include <rtcconversions.h>
#include <http_couchget.h>

#include <sdutils.h>

#include <SdFat.h>


#define LOAD_PREV_MSG_ID 1
#define READ_HEADER      2
#define READ_MSG         3
#define DONE             4

static const char *DateTag = "Date: ";

#define FREQ 5000l


AppChannel::AppChannel(const HiveConfig &config, unsigned long now)
  : mNextAttempt(0), mState(LOAD_PREV_MSG_ID), mConfig(config),
    mInitialMsg(false), mHavePayload(false), mIsOnline(false), mHaveTimestamp(false),
    mPrevMsgId("undefined"), mNewMsgId(), mPayload(),
    mGetter(NULL), mOfflineTime(now)
{
}


AppChannel::~AppChannel()
{
    if (mGetter != NULL) {
        PH("Deleting mGetter");
	delete mGetter;
    }
}



#define NEW_FILENAME  "/NEWMSGID.DAT"
#define FILENAME      "/MSGID.DAT"

static bool readMsgIdFile(SdFat &sd, const char *filename, Str *msgId)
{
    TF("::readMsgIdFile");
        
    SdFile f;
    if (!f.open(filename, O_READ)) {
        return false;
    }

    const int bufsz = 40;
    char buf[bufsz];
    SDUtils::ReadlineStatus stat = SDUtils::readline(&f, buf, bufsz);
    if (stat == SDUtils::ReadBufOverflow)
        return false;

    f.close();
    
    *msgId = buf;
    TRACE2("msgId: ", msgId->c_str());
    return true;
}


static bool writeMsgIdFile(SdFat &sd, const char *filename, const char *msgId, int len)
{
    TF("::writeMsgIdFile");
    TRACE2("filename: ", filename);

    SdFile f;
    if (!f.open(filename, O_CREAT | O_WRITE)) {
        TRACE("create failed");
        return false;
    }

    f.write(msgId, len);
    f.write("\n", 1);
    f.close();
    
    return true;
}


bool AppChannel::loadPrevMsgId()
{
    TF("AppChannel::loadPrevMsgId");
    TRACE("entry");

    SdFat sd;
    SDUtils::initSd(sd);

    //sd.remove(FILENAME);
 
    if (sd.exists(NEW_FILENAME)) {
        TRACE("Found pre-existing NEW_FILENAME");
	
        // the file is there!  so try to read it, 'cause it's probably the right one
        // since NEW_FILENAME exists, it means the previous run crashed mid-stream
        if (readMsgIdFile(sd, NEW_FILENAME, &mPrevMsgId)) {
	    // was able to read from NEW_FILENAME; now cleanup the file system by removing NEW_FILENAME
	    // and ensuring that FILENAME is correct
	    sd.remove(FILENAME);

	    bool wrote = writeMsgIdFile(sd, FILENAME, mPrevMsgId.c_str(), mPrevMsgId.len());
	    assert(wrote, "Couldn't create or write to FILENAME");

	    bool removed = sd.remove(NEW_FILENAME);
	    assert(removed, "Couldn't remove file NEW_FILENAME");

	    TRACE2("mPrevMsgId loaded: ", mPrevMsgId.c_str());
	    mState = READ_HEADER;
	    mInitialMsg = false;
	} else {
	    // NEW_FILENAME can't be read; try deleting it to cleanup the file system, then
	    // try loading again
	    sd.remove(NEW_FILENAME);

	    return loadPrevMsgId();
	}
    } else if (sd.exists(FILENAME)) {
        // the normal file exists; looks like everything is in order
        // read it and continue
        if (readMsgIdFile(sd, FILENAME, &mPrevMsgId)) {
	    mState = READ_HEADER;
	    mInitialMsg = false;
	} else {
	    // fatal -- FILENAME exists but cannot be read, so no idea what msg id should be used
	    ERR(P("Couldn't read file: "); PL(FILENAME););
	    return false;
	}
    } else {
        // neither file exists -- this means it's the first time we're running
        TRACE("No previous msg id file found; assuming this is the first time we're running");
	
	mState = READ_HEADER;
        mInitialMsg = true;
    }
    return true;
}


bool AppChannel::processDoc(const CouchUtils::Doc &doc,
			    bool gettingHeader,
			    unsigned long *callMeBackIn_ms)
{
    bool isValid = true;
    if (gettingHeader) {
        int i = doc.lookup("msg-id");
	if (i < 0 || !doc[i].getValue().isStr()) {
	    isValid = false;
	} else {
	    TRACE2("mPrevMsgId: ", mPrevMsgId.c_str());
	    TRACE2("mInitialMsg: ", (mInitialMsg ? "true" : "false"));
	    const Str &msgId = doc[i].getValue().getStr();
	    if (!mInitialMsg && msgId.equals(mPrevMsgId)) {
	        TRACE3("Msg ", msgId.c_str(), " has already been processed");
		// nothing to do
	    } else {
	        // then go get the msg -- don't release the mutex and continue very soon
	        TRACE2("Attempting to get message id: ", msgId.c_str());
		mNewMsgId = msgId;
		mState = READ_MSG;
		*callMeBackIn_ms = 10l;
	    }
	} 
    } else {
        int i = doc.lookup("prev-msg-id");
	int j = doc.lookup("payload");
	if (i < 0 || !doc[i].getValue().isStr() ||
	    j < 0 || !doc[j].getValue().isStr()) {
	    isValid = false;
	} else {
	    const Str &prevMsgId = doc[i].getValue().getStr();
	    bool isOriginalMsg = prevMsgId.equals("0");
	    if (isOriginalMsg || prevMsgId.equals(mPrevMsgId)) {
	        mPayload = doc[j].getValue().getStr();
		// good!  this means I have the next message that the system should process
		PH2("Have the next message to process: ", mPayload.c_str());
		mHavePayload = true;
		mState = READ_HEADER;
		mInitialMsg = false;
	    } else {
	        mNewMsgId = prevMsgId;
		// then I should look further back in the history;
		PH2("There's an older message; getting: ", mNewMsgId.c_str());
		mState = READ_MSG;
		*callMeBackIn_ms = 10l;
	    }
	}
    }
    return isValid;
}


class AppChannelGetter : public HttpCouchGet {
public:
    AppChannelGetter(const char *ssid, const char *pswd, const char *dbHost, int dbPort, bool isSSL,
		     const char *url, const char *dbUser, const char *dbPswd)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, dbUser, dbPswd, isSSL)
    {
        TF("AppChannelGetter::AppChannelGetter");
    }
 
    void resetForRetry()
    {
        TF("AppChannelGetter::resetForRetry");
	HttpCouchGet::resetForRetry();
    }
  
    Str getTimestamp() const {
        TF("AppChannelGetter::getTimestamp");
        const char *dateStr = strstr(m_consumer.getResponse().c_str(), DateTag);
	if (dateStr != NULL) {
	    dateStr += strlen(DateTag);
	    Str date;
	    while (*dateStr != 13) date.add(*dateStr++);
	    PH2("Received timestamp: ", date.c_str());
	    return date;
	} else {
	    return Str("unknown");
	}
    }

    bool hasTimestamp() const {
        TF("AppChannelGetter::hasTimestamp");
	TRACE2("consumer response: ", m_consumer.getResponse().c_str());
        const char *dateStr = strstr(m_consumer.getResponse().c_str(), DateTag);
	return dateStr != NULL;
    }
};




bool AppChannel::getterLoop(unsigned long now, Mutex *wifiMutex, bool gettingHeader)
{
    TF("AppChannel::getterLoop");
    bool callMeBack = true;
    if (now > mNextAttempt && wifiMutex->own(this)) {
        unsigned long callMeBackIn_ms = 10l;
        if (mGetter == NULL) {
	    mHavePayload = false;
	    mStartTime = now;
	  
	    Str objid, encodedUrl;
	    if (gettingHeader) {
	        objid.append(mConfig.getHiveId());
		objid.append("-app");
	    } else {
	        objid.append(mNewMsgId.c_str());
	    }
	    CouchUtils::urlEncode(objid.c_str(), &encodedUrl);
	    
	    Str url;
	    CouchUtils::toURL(mConfig.getChannelDbName(), encodedUrl.c_str(), &url);
	    TRACE2("creating getter with url: ", url.c_str());
	    TRACE2("thru wifi: ", mConfig.getSSID());
	    TRACE2("with pswd: ", mConfig.getPSWD());
	    TRACE2("to host: ", mConfig.getDbHost());
	    TRACE2("port: ", mConfig.getDbPort());
	    TRACE2("using ssl? ", (mConfig.isSSL() ? "yes" : "no"));
	    TRACE2("with db-user: ", mConfig.getDbUser());
	    TRACE2("with db-pswd: ", mConfig.getDbPswd());
	    mGetter = new AppChannelGetter(mConfig.getSSID(), mConfig.getPSWD(),
					   mConfig.getDbHost(), mConfig.getDbPort(), mConfig.isSSL(),
					   url.c_str(), mConfig.getDbUser(), mConfig.getDbPswd());
	} else {
	    HivePlatform::nonConstSingleton()->clearWDT();
	    HivePlatform::singleton()->markWDT("AppChannel::loop; calling getter::event");
	    HttpOp::EventResult er = mGetter->event(now, &callMeBackIn_ms);
	    HivePlatform::singleton()->markWDT("AppChannel::loop; processing event result");
	    if (!mGetter->processEventResult(er)) {
	        HivePlatform::singleton()->markWDT("AppChannel::loop; done processing event result");
		bool retry = false;
		if (!mHaveTimestamp && mGetter->hasTimestamp()) {
		    HivePlatform::singleton()->markWDT("have Timestamp");
		    TF("hasTimestamp");
		    Str timestampStr = mGetter->getTimestamp();
		    TRACE2("timestampStr: ", timestampStr.c_str());
		    bool stat = RTCConversions::cvtToTimestamp(timestampStr.c_str(), &mTimestamp);
		    if (stat) {
		        PH2("Timestamp: ", mTimestamp);
			mSecondsAtMark = (millis()+500)/1000;
			mHaveTimestamp = true;
		    } else {
		        PL("Conversion to unix timestamp failed; retrying on next AppChannel access");
		    }
		}
		callMeBackIn_ms = FREQ - ((now=millis())-mStartTime); // most cases should schedule a callback in FREQ
		if (mGetter->hasNotFound()) {
		    PH2("object not found, so nothing to do @ ", now);
		    // nothing to do
		    mIsOnline = true;
		} else if (mGetter->haveDoc()) {
		    mIsOnline = true;
		    bool isValid = processDoc(mGetter->getDoc(), gettingHeader, &callMeBackIn_ms);
		    if (!isValid) {
		        PH("Improper HeaderMsg found; ignoring");
			PH2("doc: ", mGetter->getHeaderConsumer().getResponse().c_str());
		    }
		} else if (mGetter->isTimeout()) {
		    PH("AppChannel::getterLoop timed out; retrying again in 5s");
		    retry = true;
		    mIsOnline = false;
		    mOfflineTime = now;
		} else if (mGetter->isError()) {
		    mIsOnline = false;
		    mOfflineTime = now;
		    if (er == HttpOp::IssueOpFailed) {
		        PH("AppChannel::getterLoop timed out while trying to open HTTP connection");
		    } else {	    
		        PH("AppChannel::getterLoop failed for unknown reason; retrying again in 5s");
			PH2("errmsg: ", mGetter->getHeaderConsumer().getErrmsg().c_str());
			PH2("er: ", er);
		    }
		    retry = true;
		} else {
		    mIsOnline = false;
		    mOfflineTime = now;
		    PH("AppChannel::getterLoop failed for unknown reason; retrying again in 5s");
		    retry = true;
		}
		if (retry) {
		    TRACE("setting up for a retry");
		    callMeBackIn_ms = 5000l;
		    mRetryCnt++;
		} else {
		    mRetryCnt = 0;
		}
		delete mGetter;
		mGetter = NULL;
		wifiMutex->release(this);
	    } else {
	        //TRACE2("getter asked to be called again in (ms) ", callMeBackIn_ms);
	    }
	}
	mNextAttempt = now + callMeBackIn_ms;
    }
    return callMeBack;
}


bool AppChannel::loop(unsigned long now, Mutex *wifiMutex)
{
    TF("AppChannel::loop");
    switch (mState) {
    case LOAD_PREV_MSG_ID:
        mNextAttempt = now + 10l;
        return loadPrevMsgId();
    case READ_HEADER:
        return getterLoop(now, wifiMutex, true);
    case READ_MSG:
        return getterLoop(now, wifiMutex, false);
    default:
        FAIL("Unknown AppChannel state");
    }
}


void AppChannel::consumePayload(Str *result)
{
    TF("AppChannel::consumePayload");
    
    // now it's safe to write the msg id out to file to complete the atomic operation of aquiring a message
    SdFat sd;
    SDUtils::initSd(sd);

    if (sd.exists(FILENAME)) {
        // the following ugliness exists to ensure that the operation is transactional.
        // Specifically, I want to leave the old file there until I'm certain that I've been able
        // to write a temporary file with the new id.  After successfully putting the temp file there,
        // I'll go back and attempt to delete FILENAME, write to it, then delete the temp file.
        //
        // If there is any kind of failure after writing the temp file but before deleting it, a subsequent
        // run can figure out the correct state
      
        if (sd.exists(NEW_FILENAME)) {
	    PL("Unexpectedly found NEW_FILENAME present; deleting it");
	    if (sd.remove(NEW_FILENAME)) {
	        PL("deleted.");
	    } else {
	        PL("Couldn't delete NEW_FILENAME");
	    }
	}
    
	bool wrote = writeMsgIdFile(sd, NEW_FILENAME, mNewMsgId.c_str(), mNewMsgId.len());
	assert(wrote, "Couldn't create or write to NEW_FILENAME");

	if (sd.remove(FILENAME)) {
	    if (!writeMsgIdFile(sd, FILENAME, mNewMsgId.c_str(), mNewMsgId.len())) {
	        // not quite fatal... but things probably won't proceed well!
	        TRACE2("Couldn't write to: ", FILENAME);
	    } else {
	        // if we get here, it means we've just written the latest id to both files, so no need for the
	        // transitional file any longer
	        sd.remove(NEW_FILENAME);
	    }
	} else {
	    // not quite fatal, but pretty bad... the id has been written to the NEW_FILENAME, but I cannot cleanup
	    TRACE("The id has been written to NEW_FILENAME, but cannot cleanup FILENAME");
	}
    } else {
        bool wrote = writeMsgIdFile(sd, FILENAME, mNewMsgId.c_str(), mNewMsgId.len());
	assert(wrote, "Couldn't write to FILENAME");
    }

    *result = mPayload;
    mPrevMsgId = mNewMsgId;
    mHavePayload = false;
    mPayload = "";
}


void AppChannel::toString(unsigned long now, Str *str) const
{
    unsigned long secondsSinceBoot = (now+500)/1000;
    unsigned long secondsSinceMark = secondsSinceBoot - secondsAtMark();
    unsigned long secondsSinceEpoch = secondsSinceMark + mTimestamp;
    char timestampStr[16];
    sprintf(timestampStr, "%lu", secondsSinceEpoch);

    *str = timestampStr;
}

