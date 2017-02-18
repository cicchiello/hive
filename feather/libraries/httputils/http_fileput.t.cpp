#include <http_fileput.t.h>

//#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <http_fileput.h>
#include <http_couchget.h>
#include <http_dataprovider.h>

#include <couchutils.h>

//
// Most code taken from Adafruit_WINC1500/examples/WifiWebClient.ino
//

static const char *docid = "feather-binaryget-test";
static const char *LOCAL_FILENAME = "/LISTEN.WAV";
static const char *ATTACHMENT_NAME = "listen.wav";
static const char *CONTENT_TYPE = "audio/wav";


HttpFilePutTest::HttpFilePutTest()
  : HttpPutBaseTest(defaultDbUser, defaultDbPswd, defaultDbHost, defaultDbPort, false),
    m_putter(0), mNow(0), mTransferStart(0), mTransferTime(0)
{
}


HttpFilePutTest::HttpFilePutTest(const char *dbUser, const char *dbPswd, 
				 const char *dbHost,
				 int dbPort,
				 bool isSSL)
  : HttpPutBaseTest(dbUser, dbPswd, dbHost, dbPort, isSSL),
    m_putter(0), mNow(0), mTransferStart(0), mTransferTime(0)
{
}


HttpFilePutTest::~HttpFilePutTest()
{
    assert(m_putter == NULL, "m_putter hasn't been deleted");
}


const char *HttpFilePutTest::getDocid() const
{
    return ::docid;
}


bool HttpFilePutTest::createPutter(const CouchUtils::Doc &originalDoc)
{
    TF("HttpFilePutTest::createPutter");
    TRACE(testName());
    TRACE("entry");

    int i = originalDoc.lookup("_rev");
    if (i >= 0) {
        Str revision = originalDoc[i].getValue().getStr();

	Str url;
	CouchUtils::toAttachmentPutURL(defaultDbName, getDocid(), ATTACHMENT_NAME, revision.c_str(), &url);

	m_putter = new HttpFilePut(ssid, pass,
				     getDbHost(), getDbPort(),
				     url.c_str(), getDbUser(), getDbPswd(), getIsSSL(),
				     LOCAL_FILENAME, CONTENT_TYPE);
	mTransferStart = mNow;
	mTransferTime = 0;
	return m_putter != NULL;
    } else {
        TRACE("Found unexpected information in the doc:");
	CouchUtils::printDoc(m_getter->getDoc());
	return false;
    }
}


bool HttpFilePutTest::loop() {
    TF("HttpFilePutTest::loop");
    unsigned long now = millis();
    mNow = now;
    if (now > m_timeToAct && !m_didIt) {
        if (m_putter == NULL) {
	    return this->HttpPutBaseTest::loop();
	} else {
            unsigned long callMeBackIn_ms = 0;
            HttpFilePut::EventResult r = m_putter->event(now, &callMeBackIn_ms);
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
		        TRACE2("Found unexpected information in the header response:",
			       m_putter->getHeaderConsumer().getResponse().c_str());
		    }
		}  else {
		    TRACE("unknown failure");
		}
		m_didIt = true;
		delete m_putter;
		m_putter = NULL;
	    }
	    m_timeToAct = now + callMeBackIn_ms;
	}
    }

    return m_success;
}


