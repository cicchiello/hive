#include <SeqGetter.h>


#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hiveconfig.h>
#include <couchutils.h>

#include <http_jsonpost.h>

#include <str.h>
#include <strbuf.h>
#include <strutils.h>


SeqGetter::SeqGetter(const HiveConfig &config, unsigned long now, const Str &hiveChannelDocId)
  : mConfig(config), mNextActionTime(now + 1000l), mGetter(0), 
    mLastSeq(), mGetNextSet(false), mRevision()
{
    TF("SeqGetter::SeqGetter");
    
    CouchUtils::Arr docIdsArr;
    docIdsArr.append(CouchUtils::Item(hiveChannelDocId));
    mSelectorDoc.addNameValue(new CouchUtils::NameValuePair("doc_ids", docIdsArr));
}


SeqGetter::~SeqGetter()
{
    assert(mGetter == NULL, "mGetter == NULL");
    delete mGetter;
}


bool SeqGetter::isItTimeYet(unsigned long now)
{
    return now > mNextActionTime;
}


#define CHANGESET_LIMIT "limit=3"

HttpJSONPost *SeqGetter::createGetter(const HiveConfig &config)
{
    TF("SeqGetter::createGetter");

    // curl -X POST -H "Content-Type: application/json" https://jfcenterprises.cloudant.com/hive-channel/_changes?filter=_doc_ids -d '{"doc_ids":["95efbfae334d4d512020204a311402ff-app"]}'
    
#ifndef HEADLESS
    CouchUtils::println(mSelectorDoc, Serial, "SeqGetter::createGetter; POST doc: ");
#endif
    
    static const char *urlPieces[5];
    urlPieces[0] = "/";
    urlPieces[1] = config.getChannelDbName().c_str();
    urlPieces[2] = urlPieces[0];
    urlPieces[3] = "_changes?filter=_doc_ids";
    urlPieces[4] = 0;
    
    mGetNextSet = false;
    HttpJSONPost *poster = new HttpJSONPost(config.getSSID(), config.getPSWD(),
					    config.getDbHost(), config.getDbPort(), mSelectorDoc,
					    config.getDbUser(), config.getDbPswd(), config.isSSL(),
					    urlPieces);
    return poster;
}


bool SeqGetter::processGetter(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("SeqGetter::processGetter");

    HttpJSONGet::EventResult er = mGetter->event(now, callMeBackIn_ms);
    
    if (mGetter->processEventResult(er))
        return true; // not done yet


    bool callMeBack = false;
    TRACE("getter is done"); // somehow, we're done getting -- see if and what more we can do
    bool retry = true; // more true cases than false
    if (mGetter->haveDoc()) {
        mError = false;
        const CouchUtils::Doc &changeDoc = mGetter->getDoc();
	// use getter's result just to get the last seq #

	bool parseError = true; // only one path can be successful
	int lastSeqIndex = changeDoc.lookup("last_seq");
	int pendingIndex = changeDoc.lookup("pending");
	if ((lastSeqIndex >= 0) && changeDoc[lastSeqIndex].getValue().isStr() &&
	    (pendingIndex >= 0) && changeDoc[pendingIndex].getValue().isStr() &&
	    StringUtils::isNumber(changeDoc[pendingIndex].getValue().getStr().c_str())) {
            const char *pendingCStr = changeDoc[pendingIndex].getValue().getStr().c_str();
            int pending = atoi(pendingCStr);

	    int resultsIndex = changeDoc.lookup("results");
	    if ((resultsIndex >= 0) &&
		changeDoc[resultsIndex].getValue().isArr() &&
		(changeDoc[resultsIndex].getValue().getArr().getSz() == 1)) {
	        const CouchUtils::Arr &resultsArr = changeDoc[resultsIndex].getValue().getArr();
		if (resultsArr[0].isDoc()) {
		    const CouchUtils::Doc &resultDoc = resultsArr[0].getDoc();
		    int changesIndex = resultDoc.lookup("changes");
		    if ((changesIndex >= 0) &&
			resultDoc[changesIndex].getValue().isArr() &&
			resultDoc[changesIndex].getValue().getArr().getSz() == 1) {
		        const CouchUtils::Arr &changesArr = resultDoc[changesIndex].getValue().getArr();
			if (changesArr[0].isDoc()) {
			    const CouchUtils::Doc &revDoc = changesArr[0].getDoc();
			    int revIndex = revDoc.lookup("rev");
			    if ((revIndex >= 0) && revDoc[revIndex].getValue().isStr()) {
			        mLastSeq = changeDoc[lastSeqIndex].getValue().getStr();
				mRevision = revDoc[revIndex].getValue().getStr();
				mHasLastSeqId = pending == 0;
				mGetNextSet = !mHasLastSeqId;
				parseError = false;
				retry = false;
			    }
			}
		    }
		}
	    }
	}
	if (parseError) {
	    StrBuf dump;
	    PH2("ChangeDoc parsing error: ", CouchUtils::toString(changeDoc, &dump));
	}
    } else if (mGetter->isTimeout()) {
        mError = true;
        PH2("http timeout; response: ", mGetter->getHeaderConsumer().getHeader().c_str());
	PH2("now: ", millis());
    } else if (mGetter->hasNotFound()) {
        PH2("http not found; response: ", mGetter->getHeaderConsumer().getHeader().c_str());
    } else if (mGetter->isError()) {
        PH2("http error; response: ", mGetter->getHeaderConsumer().getHeader().c_str());
    } else {
        PH2("Unknown failure; http response: ", mGetter->getHeaderConsumer().getHeader().c_str());
    }
    
    if (retry) {
        PH("retrying again in 5s");
	*callMeBackIn_ms = 5000l;
	callMeBack = true;
	mLastSeq = Str::sEmpty;
	mGetNextSet = false;
    } else {
	*callMeBackIn_ms = 10l;
    }
    
    mGetter->shutdownWifiOnDestruction(retry);
    delete mGetter;
    mGetter = NULL;

    return callMeBack;
}


bool SeqGetter::loop(unsigned long now)
{
    TF("SeqGetter::loop");

    bool callMeBack = true;
    unsigned long callMeBackIn_ms = 10l;
    if (mGetter == NULL) {
        //TRACE("creating getter");
        mGetter = createGetter(mConfig);
    } else {
        callMeBack = processGetter(now, &callMeBackIn_ms);
	if (!callMeBack && mGetNextSet) {
	    callMeBack = true;
	}
    }
    mNextActionTime = millis() + callMeBackIn_ms;

    return callMeBack;
}
