#include <http_stream.t.h>

#include <Arduino.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <http_binaryput.h>
#include <http_couchget.h>
#include <adcdataprovider.h>

#include <couchutils.h>

//
// Most code taken from Adafruit_WINC1500/examples/WifiWebClient.ino
//

static const char *docid = "feather-binaryget-test";
static const char *ATTACHMENT_NAME = "listen2.wav";
static const char *CONTENT_TYPE = "audio/wav";

static const int SAMPLING_DURATION_MS = 15000;

HttpStreamTest::HttpStreamTest()
  : HttpPutBaseTest(defaultDbUser, defaultDbPswd, defaultDbHost, defaultDbPort, false),
    m_putter(0), m_provider(0), mNow(0), mTransferStart(0), mTransferTime(0)
{
}


HttpStreamTest::HttpStreamTest(const char *dbUser, const char *dbPswd,
			       const char *dbHost,
			       int dbPort,
			       bool isSSL)
  : HttpPutBaseTest(dbUser, dbPswd, dbHost, dbPort, isSSL),
    m_putter(0), m_provider(0), mNow(0), mTransferStart(0), mTransferTime(0)
{
}


HttpStreamTest::~HttpStreamTest()
{
    assert(m_putter == NULL, "m_putter hasn't been deleted");
    delete m_provider;
}


const char *HttpStreamTest::getDocid() const
{
    return ::docid;
}


bool HttpStreamTest::createPutter(const CouchUtils::Doc &originalDoc)
{
    TF("HttpStreamTest::createPutter");
    TRACE(testName());
    TRACE("entry");

    int i = originalDoc.lookup("_rev");
    if (i >= 0) {
        Str revision = originalDoc[i].getValue().getStr();

	StrBuf url;
	CouchUtils::toAttachmentPutURL(defaultDbName, getDocid(), ATTACHMENT_NAME, revision.c_str(), &url);

	m_provider = new ADCDataProvider(SAMPLING_DURATION_MS);
	m_putter = new HttpBinaryPut(ssid, pass,
				     getDbHost(), getDbPort(),
				     url.c_str(), getDbUser(), getDbPswd(), getIsSSL(),
				     m_provider, CONTENT_TYPE);
	m_putter->setRetryCnt(HttpOp::MaxRetries); // so no retries
	mTransferStart = mNow;
	mTransferTime = 0;
	return m_putter != NULL;
    } else {
        TRACE("Found unexpected information in the doc:");
	CouchUtils::printDoc(m_getter->getDoc());
	return false;
    }
}


bool HttpStreamTest::loop() {
    TF("HttpStreamTest::loop");
    unsigned long now = millis();
    mNow = now;
    if (now > m_timeToAct && !m_didIt) {
        if (m_putter == NULL) {
	    return this->HttpPutBaseTest::loop();
	} else {
            unsigned long callMeBackIn_ms = 0;
            HttpBinaryPut::EventResult r = m_putter->event(now,
							   &callMeBackIn_ms);
	    if (m_provider->isDone() && (mTransferTime == 0)) {
	        mTransferTime = now - mTransferStart;
		TRACE2("Transfer completed after (ms): ", mTransferTime);
	    }
	    if (!m_putter->processEventResult(r)) {
	        m_success = false;
		if (m_putter->getFinalResult() == HttpOp::HTTPSuccessResponse) {
		    if (m_putter->haveDoc()) {
		        if (m_putter->getDoc().lookup("ok") >= 0) {
			    TRACE("doc successfully uploaded!");
			    m_success = true;
			} else {
			    TRACE("Found unexpected information in the doc:");
			    CouchUtils::printDoc(m_putter->getDoc());
			}
		    } else {
		        TRACE2("Found unexpected information in the header response: ",
			       m_putter->getHeaderConsumer().getResponse().c_str());
		    }
		}  else {
		    TRACE("unknown failure");
		}
		m_didIt = true;
		delete m_putter;
		delete m_provider;
		m_putter = NULL;
		m_provider = NULL;
	    }
	    m_timeToAct = now + callMeBackIn_ms;
	}
    }

    return m_success;
}


