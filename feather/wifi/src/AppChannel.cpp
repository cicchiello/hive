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

#include <strbuf.h>

#include <sdutils.h>

#include <SdFat.h>


extern "C" {void GlobalYield();};


#define LOAD_PREV_MSG_ID 1
#define READ_HEADER      2
#define READ_MSG         3
#define HAVE_PAYLOAD     4
#define DONE             5

static const char *DateTag = "Date: ";
static const char *ETag = "ETag: \"";

#define RETRY_WAIT 2000l
#define FREQ       2500l

class MyTimeProvider : public TimeProvider {
private:
    unsigned long mSecondsAtMark, mTimestampAtMark;
  
public:
    MyTimeProvider(unsigned long secondsAtMark, unsigned long timestampAtMark)
      : mSecondsAtMark(secondsAtMark), mTimestampAtMark(timestampAtMark) {}

    unsigned long getSecondsAtMark() const {return mSecondsAtMark;}
    unsigned long getTimestampAtMark() const {return mTimestampAtMark;}
  
    // TimeProvider API
    void toString(unsigned long now, Str *str) const;
    unsigned long getSecondsSinceEpoch(unsigned long now) const;
};


AppChannel::AppChannel(const HiveConfig &config, unsigned long now, TimeProvider **timeProvider, Mutex *wifiMutex, Mutex *sdMutex)
  : mNextAttempt(0), mState(LOAD_PREV_MSG_ID), mConfig(config),
    mInitialMsg(false), mHavePayload(false), mWasOnline(false), 
    mPrevMsgId("undefined"), mPrevETag("undefined"), mNewMsgId(), mPayload(),
    mGetter(NULL), mWifiMutex(wifiMutex), mSdMutex(sdMutex), mTimeProvider(timeProvider)
{
    TF("AppChannel::AppChannel");
    StrBuf channelId(config.getHiveId().c_str()), encodedId, channelUrl;
    channelId.append("-app");
    CouchUtils::urlEncode(channelId.c_str(), &encodedId);
    CouchUtils::toURL(config.getChannelDbName().c_str(), encodedId.c_str(), &channelUrl);
    mChannelUrl = channelUrl.c_str();
}


AppChannel::~AppChannel()
{
    TF("AppChannel::~AppChannel");
    if (mGetter != NULL) {
        PH("Deleting mGetter");
	delete mGetter;
    }
}



#define NEW_FILENAME  "/NEWMSGID.DAT"
#define FILENAME      "/MSGID.DAT"

static bool readMsgIdFile(SdFat &sd, const char *filename, StrBuf *msgId)
{
    TF("::readMsgIdFile");

    SdFile f;
    if (!f.open(filename, O_READ)) {
        return false;
    }

    const int bufsz = 40;
    char buf[bufsz];
    SDUtils::ReadlineStatus stat = SDUtils::readline(&f, buf, bufsz);
    if (stat == SDUtils::ReadBufOverflow) {
        f.close();
        return false;
    }

    f.close();
    
    if (strlen(buf) == 0) {
        TRACE("invalid prevMsgId found in " NEW_FILENAME);
	return false;
    } else {
        *msgId = buf;
        TRACE2("msgId: ", msgId->c_str());
	return true;
    }
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

    assert(mSdMutex->whoOwns() == this, "mutex error");

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
	    PH2("Couldn't read file: ",FILENAME);

	    // act like it's the first time we're running
	    mState = READ_HEADER;
	    mInitialMsg = true;
	
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
			    unsigned long now,
			    unsigned long *callMeBackIn_ms)
{
    TF("AppChannel::processDoc");

    bool isValid = true;
    if (gettingHeader) {
        int i = doc.lookup("msg-id");
	if (i < 0 || !doc[i].getValue().isStr()) {
	    isValid = false;
	} else {
	    TRACE2("mPrevMsgId: ", mPrevMsgId.c_str());
	    TRACE2("mInitialMsg: ", (mInitialMsg ? "true" : "false"));
	    const Str &msgId = doc[i].getValue().getStr();
	    if (!mInitialMsg && msgId.equals(mPrevMsgId.c_str())) {
	        PH4("Msg ", msgId.c_str(), " has already been processed; now: ", now);
		// nothing to do
	    } else {
	        int prevIndex = doc.lookup("prev-msg-id");
		if (prevIndex >= 0) {
		    // then I know the prev-msg-id; I should also have the payload for msgId
		    const Str &prevMsgId = doc[prevIndex].getValue().getStr();
		    bool isOriginalMsg = prevMsgId.equals("0");
		    if (isOriginalMsg || prevMsgId.equals(mPrevMsgId.c_str())) {
		        // good!  this means I have the next message that the system should process
		        // *and* it's in the header -- I saved a call to get the top of the linked list of commands
		        int payloadIndex = doc.lookup("payload");
			assert(payloadIndex >= 0, "payloadIndex >= 0");
			assert(doc[payloadIndex].getValue().isStr(), "doc[payloadIndex].getValue().isStr()");
			mPayload = doc[payloadIndex].getValue().getStr().c_str();
			PH4("Have the next message to process: ", mPayload.c_str(), " now: ", now);
			mHavePayload = true;
			mState = HAVE_PAYLOAD;
			mInitialMsg = false;
			mNewMsgId = msgId.c_str();
			mPrevMsgId = prevMsgId.c_str();
		    } else {
		        // then I should look further back in the history;
		        mNewMsgId = prevMsgId.c_str();
			PH2("There's an older message; getting: ", mNewMsgId.c_str());
			mState = READ_MSG;
			*callMeBackIn_ms = 10l;
		    }
		} else {
		    // then go get the msg -- don't release the mutex and continue very soon
		    PH4("Attempting to get message id: ", msgId.c_str(), " now: ", now);
		    mNewMsgId = msgId.c_str();
		    mState = READ_MSG;
		    *callMeBackIn_ms = 1l;
		}
		mPrevETag = "undefined";
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
	    if (isOriginalMsg || prevMsgId.equals(mPrevMsgId.c_str())) {
		// good!  this means I have the next message that the system should process
	        mPayload = doc[j].getValue().getStr().c_str();
		PH4("Have the next message to process: ", mPayload.c_str(), " now: ", now);
		mHavePayload = true;
		mState = HAVE_PAYLOAD;
		mInitialMsg = false;
	    } else {
		// then I should look further back in the history;
	        mNewMsgId = prevMsgId.c_str();
		PH2("There's an older message; getting: ", mNewMsgId.c_str());
		mState = READ_MSG;
		*callMeBackIn_ms = 10l;
	    }
	}
	mPrevETag = "undefined";
    }
    return isValid;
}


class AppChannelGetter : public HttpCouchGet {
public:
    AppChannelGetter(const Str &ssid, const Str &pswd, const Str &dbHost, int dbPort, bool isSSL,
		     const Str &url, const Str &dbUser, const Str& dbPswd)
      : HttpCouchGet(ssid, pswd, dbHost, dbPort, url, dbUser, dbPswd, isSSL)
    {
        TF("AppChannelGetter::AppChannelGetter");
    }
 
    void resetForRetry()
    {
        TF("AppChannelGetter::resetForRetry");
	HttpCouchGet::resetForRetry();
    }

    const StrBuf &getETag() const {
        static StrBuf buf;
        TF("AppChannelGetter::getETag");
        const char *ETagStr = strstr(m_consumer.getResponse().c_str(), ETag);
	if (ETagStr != NULL) {
	    buf = "";
	    ETagStr += strlen(ETag);
	    const char *start = ETagStr;
	    while (ETagStr && *ETagStr && (*ETagStr != '"'))
	        ETagStr++;
	    buf.add(start, ETagStr-start);
	    TRACE2("Received ETag: ", buf.c_str());
	} else {
	    buf = "unknown";
	}
	return buf;
    }
  
    StrBuf getTimestamp() const {
        TF("AppChannelGetter::getTimestamp");
        const char *dateStr = strstr(m_consumer.getResponse().c_str(), DateTag);
	if (dateStr != NULL) {
	    dateStr += strlen(DateTag);
	    StrBuf date;
	    while (*dateStr != 13) date.add(*dateStr++);
	    PH2("Received timestamp: ", date.c_str());
	    return date;
	} else {
	    return StrBuf("unknown");
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
        unsigned long callMeBackIn_ms = 5l;
	if (mGetter == NULL) {
	    mHavePayload = false;
	    mStartTime = now;

	    const Str *url = NULL;
	    Str msgUrl;
	    if (gettingHeader) {
	        url = &mChannelUrl;
	    } else {
	        StrBuf msgUrlBuf;
		CouchUtils::toURL(mConfig.getChannelDbName().c_str(), mNewMsgId.c_str(), &msgUrlBuf);
		msgUrl = msgUrlBuf.c_str();
		url = &msgUrl;
	    }
	    
	    TRACE2("creating getter with url: ", url->c_str());
	    TRACE2("thru wifi: ", mConfig.getSSID().c_str());
	    TRACE2("with pswd: ", mConfig.getPSWD().c_str());
	    TRACE2("to host: ", mConfig.getDbHost().c_str());
	    TRACE2("port: ", mConfig.getDbPort());
	    TRACE2("using ssl? ", (mConfig.isSSL() ? "yes" : "no"));
	    TRACE2("with db-user: ", mConfig.getDbUser().c_str());
	    TRACE2("with db-pswd: ", mConfig.getDbPswd().c_str());
	    mGetter = new AppChannelGetter(mConfig.getSSID(), mConfig.getPSWD(),
					   mConfig.getDbHost(), mConfig.getDbPort(), mConfig.isSSL(),
					   *url, mConfig.getDbUser(), mConfig.getDbPswd());
	} else {
	    HttpOp::EventResult er = mGetter->event(now, &callMeBackIn_ms);
	    if (!mGetter->processEventResult(er)) {
	        bool retry = false;
		callMeBackIn_ms = FREQ - ((now=millis())-mStartTime); // most cases should schedule a callback in FREQ
		if (callMeBackIn_ms < 100) callMeBackIn_ms = 1000l;
		bool isOnlineNow = false;
		if (!*mTimeProvider && mGetter->hasTimestamp()) {
		    TRACE("Starting the WDT");
		    HivePlatform::nonConstSingleton()->startWDT();
		    TF("hasTimestamp");
		    StrBuf timestampStr = mGetter->getTimestamp();
		    TRACE2("timestampStr: ", timestampStr.c_str());
		    unsigned long timestampAtMark;
		    bool stat = RTCConversions::cvtToTimestamp(timestampStr.c_str(), &timestampAtMark);
		    if (stat) {
		        PH2("Timestamp: ", timestampAtMark);
			unsigned long secondsAtMark = (millis()+500)/1000;
			*mTimeProvider = new MyTimeProvider(secondsAtMark, timestampAtMark);
		    } else {
		        PL("Conversion to unix timestamp failed; retrying on next AppChannel access");
		    }
		    isOnlineNow = true;
		} else {
		    if (mGetter->hasNotFound()) {
		        PH2("object not found, so nothing to do @ ", now);
			// nothing to do but report as much as possible
			if (gettingHeader) {
			    PH2("url: ", mChannelUrl.c_str());
			} else {
			    PH2("msgId: ", mNewMsgId.c_str());
			}
			isOnlineNow = true;
		    } else if (mGetter->isTimeout()) {
		        PH2("timed out; retrying again in 5s; now: ", now);
			retry = true;
			isOnlineNow = false;
		    } else if (mGetter->isError()) {
		        isOnlineNow = false;
			if (er == HttpOp::IssueOpFailed) {
			    PH("timed out while trying to open HTTP connection");
			} else {	    
			    PH("failed for unknown reason; retrying again in 5s");
			    PH2("errmsg: ", mGetter->getHeaderConsumer().getErrmsg().c_str());
			    PH2("er: ", er);
			}
			retry = true;
		    } else {
		        const StrBuf &etag = mGetter->getETag();
			if (strcmp(etag.c_str(), mPrevETag.c_str()) != 0) {
			    if (mGetter->haveDoc()) {
			        bool isValid = processDoc(mGetter->getDoc(), gettingHeader, now, 
							  &callMeBackIn_ms);
				if (gettingHeader) 
				    mPrevETag = etag;
				isOnlineNow = true;
				if (!isValid) {
				    PH("Improper HeaderMsg found; ignoring");
				    PH2("doc: ", mGetter->getHeaderConsumer().getResponse().c_str());
				}
			    } else {
			        isOnlineNow = false;
				PH("failed for unknown reason; retrying again in 5s");
				retry = true;
			    }
			} else {
			    isOnlineNow = true;
			    PH2("No new AppChannel msg available; now: ", now);
			}
		    }
		}
		if (!isOnlineNow && mWasOnline) {
		    mWasOnline = false;
		} else if (isOnlineNow && !mWasOnline) {
		    mWasOnline = true;
		}
		if (retry) {
		    TRACE("setting up for a retry");
		    callMeBackIn_ms = RETRY_WAIT;
		    mRetryCnt++;
		} else {
		    mRetryCnt = 0;
		}
		mGetter->shutdownWifiOnDestruction(retry);
		delete mGetter;
		mGetter = NULL;

		bool keepMutex = !retry && isOnlineNow && mWasOnline && (mState == READ_MSG);
		if (!keepMutex) {
		    wifiMutex->release(this);
		} else {
		    PH2("Keeping the mutex, and calling back in ", callMeBackIn_ms);
		}		  
	    } else {
	        //TRACE2("getter asked to be called again in (ms) ", callMeBackIn_ms);
	    }
	}
	mNextAttempt = now + callMeBackIn_ms;
    }
    return callMeBack;
}


bool AppChannel::loop(unsigned long now)
{
    TF("AppChannel::loop");
    switch (mState) {
    case LOAD_PREV_MSG_ID: {
        mNextAttempt = now + 50l;
	bool stat = true;
	if (mSdMutex->own(this)) {
	    stat = loadPrevMsgId();
	    mSdMutex->release(this);
	} 
	return stat;
    }
    case READ_HEADER:
        return getterLoop(now, mWifiMutex, true);
    case READ_MSG:
        return getterLoop(now, mWifiMutex, false);
    case HAVE_PAYLOAD:
        if (!mHavePayload) {
	    // done; can now move on to read the next message in the channel
	    mState = READ_HEADER;
	}
	return true; // return true, regardless
    default:
        FAIL("Unknown AppChannel state");
    }
}


void AppChannel::consumePayload(StrBuf *result, Mutex *alreadyOwnedSdMutex)
{
    TF("AppChannel::consumePayload");

    assert(alreadyOwnedSdMutex->whoOwns() == this, "mutex problem");
    
    // now it's safe to write the msg id out to file to complete the atomic operation of aquiring a message
    SdFat sd;
    SDUtils::initSd(sd);

    GlobalYield();
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
    
	GlobalYield();
	
	bool wrote = writeMsgIdFile(sd, NEW_FILENAME, mNewMsgId.c_str(), mNewMsgId.len());
	assert(wrote, "Couldn't create or write to NEW_FILENAME");

	GlobalYield();
	
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

	GlobalYield();
	
    } else {
        bool wrote = writeMsgIdFile(sd, FILENAME, mNewMsgId.c_str(), mNewMsgId.len());
	assert(wrote, "Couldn't write to FILENAME");
    }

    GlobalYield();
	
    *result = mPayload;
    mPrevMsgId = mNewMsgId;
    mHavePayload = false;
    mPayload = "";
}


unsigned long MyTimeProvider::getSecondsSinceEpoch(unsigned long now) const
{
    unsigned long secondsSinceBoot = (now+500)/1000;
    unsigned long secondsSinceMark = secondsSinceBoot - getSecondsAtMark();
    unsigned long secondsSinceEpoch = secondsSinceMark + mTimestampAtMark;
    return secondsSinceEpoch;
}


void MyTimeProvider::toString(unsigned long now, Str *str) const
{
    char timestampStr[16];
    sprintf(timestampStr, "%lu", getSecondsSinceEpoch(now));

    *str = timestampStr;
}

