#include <SeqListener.h>


#include <Arduino.h>

//#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <TimeProvider.h>
#include <rtcconversions.h>

#include <hiveconfig.h>

#include <couchutils.h>

#include <http_jsonpost.h>
#include <http_couchget.h>
#include <SeqGetter.h>

#include <str.h>
#include <strbuf.h>
#include <strutils.h>


#define INIT                0
#define GET_INITIAL_PAYLOAD 1
#define GET_OLDER_PAYLOAD   2
#define CREATE_SEQID_GETTER 3
#define GET_INITIAL_SEQID   4
#define START_LISTEN        5
#define LISTEN              6
#define HAVE_PAYLOAD        7


/* STATIC */
Str SeqListener::APP_CHANNEL_MSGID_PROPNAME("app-channel-msgid");


class MyTimeProvider : public TimeProvider {
private:
    unsigned long mSecondsAtMark, mTimestampAtMark;
  
public:
    MyTimeProvider(unsigned long secondsAtMark, unsigned long timestampAtMark)
      : mSecondsAtMark(secondsAtMark), mTimestampAtMark(timestampAtMark) {}
    ~MyTimeProvider() {}

    unsigned long getSecondsAtMark() const {return mSecondsAtMark;}
    unsigned long getTimestampAtMark() const {return mTimestampAtMark;}
  
    // TimeProvider API
    void toString(unsigned long now, Str *str) const;
    unsigned long getSecondsSinceEpoch(unsigned long now) const;
};


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




SeqListener::SeqListener(HiveConfig *config, unsigned long now, const TimeProvider **timeProvider, const Str &hiveChannelId)
  : mConfig(*config), mHiveChannelId(hiveChannelId), mLastSeqId(), mLastRevision(), mRevision(),
    mNextActionTime(now + 100l), mIsOnline(false), mHasMsgObj(false),
    mHasLastSeqId(false), mListenerTimedOut(false), 
    mSequenceGetter(NULL), mState(INIT), mHasPayload(false), mTimeProvider(timeProvider)
{
    TF("SeqListener::SeqListener");
 
    CouchUtils::Arr docIdsArr;
    docIdsArr.append(CouchUtils::Item(mHiveChannelId));
    mSelectorDoc.addNameValue(new CouchUtils::NameValuePair("doc_ids", docIdsArr));
}


SeqListener::~SeqListener()
{
    assert(mSequenceGetter == NULL, "mSequenceGetter wasn't deleted");
    delete mSequenceGetter;
}


bool SeqListener::isItTimeYet(unsigned long now)
{
    switch (mState) {
    case GET_INITIAL_SEQID: return mSequenceGetter->isItTimeYet(now);
    default: return now > mNextActionTime;
    }
}


void SeqListener::restartListening()
{
    TF("SeqListener::restartListening");
    assert(mSequenceGetter == NULL, "mSequenceGetter wasn't deleted");

    PH2("Marking msgId as already processed: ", mMsgId.c_str());
    mConfig.addProperty(APP_CHANNEL_MSGID_PROPNAME, mMsgId.c_str());

    mState = mHasLastSeqId ? START_LISTEN : INIT;
    mHasPayload = false;
    mNextActionTime = millis();
}


HttpJSONPost *SeqListener::createChangeListener()
{
    TF("SeqListener::createChangeListener");

    static const char *sUrlPieces[7];

    // curl -X POST -H "Content-Type: application/json" "https://<host>/<db>/_changes?filter=_doc_ids&include_docs=true&feed=longpoll&since=<seqid>" -d '{"doc_ids":["<channelid>"]}'
  
#ifndef HEADLESS
    CouchUtils::println(mSelectorDoc, Serial, "POST doc: ");
#endif
    
    sUrlPieces[0] = "/";
    sUrlPieces[1] = mConfig.getChannelDbName().c_str();
    sUrlPieces[2] = sUrlPieces[0];
    sUrlPieces[3] = "_changes?filter=_doc_ids&include_docs=true&feed=longpoll&since=\"";
    sUrlPieces[4] = mLastSeqId.c_str();
    sUrlPieces[5] = "\"";
    sUrlPieces[6] = 0;
    
    mListenerTimedOut = false;
    mHasPayload = false;
    HttpJSONPost *l = new HttpJSONPost(mConfig.getSSID(), mConfig.getPSWD(),
				       mConfig.getDbHost(), mConfig.getDbPort(), mSelectorDoc,
				       mConfig.getDbUser(), mConfig.getDbPswd(), mConfig.isSSL(), sUrlPieces);
    l->getHeaderConsumer().setTimeout(60000); // wait for 1 minute before starting if nothing received
    return l;
}


bool SeqListener::processListener(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("SeqListener::processListener");

    mIsOnline = (mChangeListener->getOpState() == HttpOp::CONSUME_RESPONSE) &&
                ((mTimeProvider == NULL) || *mTimeProvider);
    HttpCouchGet::EventResult er = mChangeListener->event(now, callMeBackIn_ms);
    if ((er != HttpOp::HTTPRetry) || !mChangeListener->isTimeout())
        if (mChangeListener->processEventResult(er))
	    return true; // not done yet

    TRACE("getter is done"); // somehow, we're done getting -- see if and what more we can do
    bool error = false;
    if (mChangeListener->haveDoc()) {
        const CouchUtils::Doc &changeDoc = mChangeListener->getDoc();

	bool parseError = true; // only one path can be successful
	int resultsIndex = changeDoc.lookup("results");
	int lastSequenceIndex = changeDoc.lookup("last_seq");
	if ((resultsIndex >= 0) &&
	    changeDoc[resultsIndex].getValue().isArr()) {
	    const CouchUtils::Arr &resultsArr = changeDoc[resultsIndex].getValue().getArr();
	    if (resultsArr.getSz() == 0) {
	        // db occassionally just tells us that there's nothing new (without timing out);
	        // act the same as the timeout case
	        parseError = false;
	        mListenerTimedOut = true;
		PH2("db reported no new events; now: ", millis());
	    } else if ((resultsArr.getSz() >= 1) &&
		       (lastSequenceIndex >= 0) &&
		       changeDoc[lastSequenceIndex].getValue().isStr() &&
		       resultsArr[0].isDoc()) {
	        if (resultsArr.getSz() > 1) {
		    StrBuf dump;
		    PH("db reported more than one event; just processing the first for now");
		    PH2("Here's the db response: ", CouchUtils::toString(changeDoc, &dump));
		}
	        const CouchUtils::Doc &resultDoc = resultsArr[0].getDoc();
		int docIndex = resultDoc.lookup("doc");
		if ((docIndex >= 0) && resultDoc[docIndex].getValue().isDoc()) {
		    const CouchUtils::Doc &appChannelDoc = resultDoc[docIndex].getValue().getDoc();
		    int revIndex = appChannelDoc.lookup("_rev");
		    int msgIdIndex = appChannelDoc.lookup("msg-id");
		    int prevMsgIdIndex = appChannelDoc.lookup("prev-msg-id");
		    int payloadIndex = appChannelDoc.lookup("payload");
		    int timestampIndex = appChannelDoc.lookup("timestamp");
		    if ((revIndex >= 0) &&
			appChannelDoc[revIndex].getValue().isStr() && 
			(msgIdIndex >= 0) &&
			appChannelDoc[msgIdIndex].getValue().isStr() && 
			(prevMsgIdIndex >= 0) &&
			appChannelDoc[prevMsgIdIndex].getValue().isStr() &&
			(timestampIndex >- 0) &&
			appChannelDoc[timestampIndex].getValue().isStr() &&
			(payloadIndex >= 0) &&
			appChannelDoc[payloadIndex].getValue().isStr()) {
		      
		        mRevision = appChannelDoc[revIndex].getValue().getStr().c_str();
			mMsgId = appChannelDoc[msgIdIndex].getValue().getStr().c_str();
			mPrevMsgId = appChannelDoc[prevMsgIdIndex].getValue().getStr().c_str();
			mPayload = appChannelDoc[payloadIndex].getValue().getStr().c_str();
			mTimestamp = appChannelDoc[timestampIndex].getValue().getStr().c_str();
			mLastSeqId = changeDoc[lastSequenceIndex].getValue().getStr();
			
			parseError = false;
			mHasPayload = true;
			
			TRACE2("Loaded msgId: ", mMsgId.c_str());
			TRACE2("Loaded prevMsgId: ", mPrevMsgId.c_str());
			TRACE2("Loaded revision: ", mRevision.c_str());
			TRACE2("Loaded timestamp: ", mTimestamp.c_str());
			TRACE2("Loaded payload: ", mPayload.c_str());
			TRACE2("Loaded lastSeqId: ", mLastSeqId.c_str());
		    }
		}
	    }
	} 
	if (parseError) {
	    StrBuf dump;
	    PH2("ChangeDoc parsing error: ", CouchUtils::toString(changeDoc, &dump));
	}
    } else if (mChangeListener->isTimeout()) {
        mListenerTimedOut = true;
	PH2("timed out; now: ", millis());
    } else if (mChangeListener->hasNotFound()) {
        PH("http not found");
	error = true;
    } else if (mChangeListener->isError()) {
        PH("http error");
	error = true;
    } else {
        PH("unknown failure");
	error = true;
    }
    
    mChangeListener->shutdownWifiOnDestruction(error);
    delete mChangeListener;
    mChangeListener = NULL;
    mIsOnline = false;

    return false;
}


HttpCouchGet *SeqListener::createMsgObjGetter(const char *urlPieces[])
{
    TF("SeqListener::createMsgObjGetter");
    
    mHasMsgObj = false;
    
    //TRACE2("thru wifi: ", mConfig.getSSID().c_str());
    //TRACE2("with pswd: ", mConfig.getPSWD().c_str());
    //TRACE2("to host: ", mConfig.getDbHost().c_str());
    //TRACE2("port: ", mConfig.getDbPort());
    //TRACE2("using ssl? ", (mConfig.isSSL() ? "yes" : "no"));
    //TRACE2("with db-user: ", mConfig.getDbUser().c_str());
    //TRACE2("with db-pswd: ", mConfig.getDbPswd().c_str());
    return new HttpCouchGet(mConfig.getSSID(), mConfig.getPSWD(),
			    mConfig.getDbHost(), mConfig.getDbPort(), 
			    mConfig.getDbUser(), mConfig.getDbPswd(),
			    mConfig.isSSL(), urlPieces);
}


bool SeqListener::processMsgObjGetter(unsigned long now, unsigned long *callMeBackIn_ms, bool isHeader)
{
    TF("SeqListener::processMsgObjGetter");

    mIsOnline = (mMsgObjGetter->getOpState() == HttpOp::CONSUME_RESPONSE) && ((mTimeProvider == NULL) || *mTimeProvider);
    HttpCouchGet::EventResult er = mMsgObjGetter->event(now, callMeBackIn_ms);
    if (mMsgObjGetter->processEventResult(er))
        return true; // not done yet

    if (mTimeProvider && !*mTimeProvider && mMsgObjGetter->hasTimestamp()) {
        StrBuf timestampStr;
	mMsgObjGetter->getTimestamp(&timestampStr);
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
    }

    TRACE("getter is done"); // somehow, we're done getting -- see if and what more we can do
    bool error = false;
    if (mMsgObjGetter->haveDoc()) {
        const CouchUtils::Doc &headerDoc = mMsgObjGetter->getDoc();

	bool parseError = true; // only one path can be successful
	int msgIdIndex = isHeader ? headerDoc.lookup("msg-id") : headerDoc.lookup("_id");
	int prevMsgIdIndex = headerDoc.lookup("prev-msg-id");
	int payloadIndex = headerDoc.lookup("payload");
	int revisionIndex = headerDoc.lookup("_rev");
	int timestampIndex = headerDoc.lookup("timestamp");
	if ((msgIdIndex >= 0) &&
	    headerDoc[msgIdIndex].getValue().isStr() &&
	    (prevMsgIdIndex >= 0) &&
	    headerDoc[prevMsgIdIndex].getValue().isStr() &&
	    (payloadIndex >= 0) &&
	    headerDoc[payloadIndex].getValue().isStr() &&
	    (revisionIndex >= 0) &&
	    headerDoc[revisionIndex].getValue().isStr() &&
	    (timestampIndex >= 0) &&
	    headerDoc[timestampIndex].getValue().isStr()) {
	    mMsgId = headerDoc[msgIdIndex].getValue().getStr().c_str();
	    mPrevMsgId = headerDoc[prevMsgIdIndex].getValue().getStr().c_str();
	    mPayload = headerDoc[payloadIndex].getValue().getStr().c_str();
	    mRevision = headerDoc[revisionIndex].getValue().getStr().c_str();
	    mTimestamp = headerDoc[timestampIndex].getValue().getStr().c_str();

	    TRACE2("Loaded msgId: ", mMsgId.c_str());
	    TRACE2("Loaded prevMsgId: ", mPrevMsgId.c_str());
	    TRACE2("Loaded revision: ", mRevision.c_str());
	    TRACE2("Loaded timestamp: ", mTimestamp.c_str());
	    TRACE2("Loaded payload: ", mPayload.c_str());

	    parseError = false;
	    mHasMsgObj = true;
	}

	if (parseError) {
	    StrBuf dump;
	    PH2("ChangeDoc parsing error: ", CouchUtils::toString(headerDoc, &dump));
	}
    } else if (mMsgObjGetter->isTimeout()) {
	PH2("timed out; now: ", millis());
    } else if (mMsgObjGetter->hasNotFound()) {
        PH("http not found;");
	error = true;
    } else if (mMsgObjGetter->isError()) {
        PH("http error;");
	error = true;
    } else {
        PH("unknown failure");
	error = true;
    }

    mMsgObjGetter->shutdownWifiOnDestruction(error);
    delete mMsgObjGetter;
    mMsgObjGetter = NULL;

    return false;
}



bool SeqListener::loop(unsigned long now)
{
    TF("SeqListener::loop");
    static const char *sUrlPieces[5];

    bool callMeBack = true; // always call back unless we've encountered an error
    switch (mState) {
    case INIT: {
        TRACE2("INIT; now: ", now);

	sUrlPieces[0] = "/";
	sUrlPieces[1] = mConfig.getChannelDbName().c_str();
	sUrlPieces[2] = sUrlPieces[0];
	sUrlPieces[3] = mHiveChannelId.c_str();
	sUrlPieces[4] = 0;
	
	mMsgObjGetter = createMsgObjGetter(sUrlPieces);
	
	mState = GET_INITIAL_PAYLOAD;
    }; break;
    case GET_OLDER_PAYLOAD: 
    case GET_INITIAL_PAYLOAD: {
        //TRACE2("GET_INITIAL_PAYLOAD; now: ", now);
        unsigned long callMeBackIn_ms;
        bool callMsgObjGetterBack = processMsgObjGetter(now, &callMeBackIn_ms, mState == GET_INITIAL_PAYLOAD);
	mNextActionTime = millis() + callMeBackIn_ms;
	if (!callMsgObjGetterBack) {
	    TRACE2("processMsgObjGetter is done; now: ", now);
	    if (mHasMsgObj) {
	        assert(mMsgObjGetter == NULL, "mMsgObjGetter == NULL");

		// get the last app channel msg id that has been processed (defaults to "0")
		const char *lastProcessedMsgId = "0";
		if (mConfig.hasProperty(APP_CHANNEL_MSGID_PROPNAME))
		    lastProcessedMsgId = mConfig.getProperty(APP_CHANNEL_MSGID_PROPNAME).c_str();

		// if the one just loaded matches or is "0", then we're in sync -- else have to load an older one
		if (strcmp(mMsgId.c_str(), lastProcessedMsgId) == 0) {
		    PH("We're up to date; starting listener");
		    mHasPayload = false;
		    mState = CREATE_SEQID_GETTER;
		} else if ((strcmp(mPrevMsgId.c_str(), "0") == 0) &&
			   (strcmp(mPrevMsgId.c_str(), lastProcessedMsgId) != 0)) {
		    // we're up to date 'cause the app channel queue has been reset
		    PH("We're on the most recent msg that needs to be processed");
		    mConfig.addProperty(APP_CHANNEL_MSGID_PROPNAME, "0");
		    mHasPayload = true;
		    mState = HAVE_PAYLOAD;
		} else if (strcmp(mPrevMsgId.c_str(), lastProcessedMsgId) == 0) {
		    PH("We're on the most recent msg that needs to be processed");
		    mHasPayload = true;
		    mState = HAVE_PAYLOAD;
		} else {
		    PH("we're out of sync with app channel queue; looking further back");
		    PH3("(until I find a message with prev-msg-id of ", lastProcessedMsgId, ")");

		    if (mState == GET_INITIAL_PAYLOAD) {
		        PH2("Skipping load of mirror msg objId: ", mMsgId.c_str());
		    }

		    sUrlPieces[0] = "/";
		    sUrlPieces[1] = mConfig.getChannelDbName().c_str();
		    sUrlPieces[2] = sUrlPieces[0];
		    sUrlPieces[3] = mPrevMsgId.c_str();
		    sUrlPieces[4] = 0;
	
		    mMsgObjGetter = createMsgObjGetter(sUrlPieces);
		    mHasPayload = false;
		    mState = GET_OLDER_PAYLOAD;
		}
	    } else {
	        // not sure what else I can do...
	        delay(1000);
		mState = INIT;
	    }
	}
    }; break;
    case CREATE_SEQID_GETTER: {
        mSequenceGetter = new SeqGetter(mConfig, now, mHiveChannelId);
	mState = GET_INITIAL_SEQID;
    }; break;
    case GET_INITIAL_SEQID: {
        TRACE2("GET_INITIAL_SEQID; now: ", now);
        bool callBack = mSequenceGetter->loop(now);
	if (!callBack) {
	    if (mSequenceGetter->hasLastSeqId()) {
	        PH2("most recent sequence id: ", mSequenceGetter->getLastSeqId().c_str());
		PH2("revision that corresponds to that seq id: ", mSequenceGetter->getRevision().c_str());
		mState = START_LISTEN;
		mLastSeqId = mSequenceGetter->getLastSeqId();
PH2("mLastSeqId: ", mLastSeqId.c_str());
		mLastRevision = mSequenceGetter->getRevision();
		mHasLastSeqId = true;
		delete mSequenceGetter;
		mSequenceGetter = NULL;
	    } else {
	        PH("Something went wrong with SequenceId lookup!");
		callMeBack = false;
	    }
	}
    }; break;
    case START_LISTEN: {
        TRACE2("START_LISTEN; now: ", now);
        mChangeListener = createChangeListener();
        mState = LISTEN;
    }; break;
    case LISTEN: {
        //TRACE2("LISTEN; now: ", now);
        unsigned long callMeBackIn_ms;
        bool callProcessListenerBack = processListener(now, &callMeBackIn_ms);
	
	mNextActionTime = millis() + callMeBackIn_ms;
	if (!callProcessListenerBack) {
	    if (mListenerTimedOut) {
	        TRACE2("timed out; restarting; now: ", now);
                mState = START_LISTEN;
	    } else if (mHasPayload) {
	        // just received a valid change notification on the app channel header doc
	        //
	        // Unfortunately, there was significant time that passed before the change notification
	        // was enabled, so there's a race condition to handle -- there might have been more than
	        // one app channel command registered.  If so, then have to go old-school and walk
	        // back thru the history to get and process them in the right order.
	      
		// get the last app channel msg id that has been processed (defaults to "0")
	        const char *lastProcessedMsgId = "0";
		if (mConfig.hasProperty(APP_CHANNEL_MSGID_PROPNAME))
		    lastProcessedMsgId = mConfig.getProperty(APP_CHANNEL_MSGID_PROPNAME).c_str();

		// if the one just loaded matches or is "0", then we're in sync -- else have to load an older one
		if ((strcmp(mPrevMsgId.c_str(), "0") == 0) &&
		    (strcmp(mPrevMsgId.c_str(), lastProcessedMsgId) != 0)) {
		    PH("We're up to date'cause the app channel queue has been reset; ok to act on current payload");
		    mConfig.addProperty(APP_CHANNEL_MSGID_PROPNAME, "0");
		    mState = HAVE_PAYLOAD;
		} else if (strcmp(mPrevMsgId.c_str(), lastProcessedMsgId) == 0) {
		    PH("We're up to date; ok to act on current payload");
		    mState = HAVE_PAYLOAD;
		} else {
		    PH("we're out of sync with app channel queue; going back in time");
 
		    sUrlPieces[0] = "/";
		    sUrlPieces[1] = mConfig.getChannelDbName().c_str();
		    sUrlPieces[2] = sUrlPieces[0];
		    sUrlPieces[3] = mPrevMsgId.c_str();
		    sUrlPieces[4] = 0;
		    
		    mMsgObjGetter = createMsgObjGetter(sUrlPieces);

		    mHasPayload = false;
		    mState = GET_OLDER_PAYLOAD;
		}

	    } else {
	        delay(1000);
		mState = INIT;
	    }
	}
    }; break;
    case HAVE_PAYLOAD: {
        // no-op
	mNextActionTime = millis() + 100l;
    }; break;
    default:
        PH2("Invalid state: ", mState);
        assert(0, "invalid state");
    }

    return callMeBack;
}
