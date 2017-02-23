#include <http_binput.t.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <http_binaryput.h>
#include <http_couchget.h>
#include <http_dataprovider.h>

#include <couchutils.h>

//
// Most code taken from Adafruit_WINC1500/examples/WifiWebClient.ino
//

static const char *docid = "feather-binaryget-test";
static const char *ATTACHMENT_NAME = "listen.wav";
static const char *CONTENT_TYPE = "audio/wav";


class MyDataProvider : public HttpDataProvider {
private:
  const int mNumChunksToSend, mBytesPerChunk, mSz;
  unsigned char *buf;
  
  int mNumChunksLeft;
  
public:
  void init()
  {
      mNumChunksLeft = mNumChunksToSend;
  }
  
  MyDataProvider(int numChunksToSend, int bytesPerChunk)
    : mNumChunksToSend(numChunksToSend), mBytesPerChunk(bytesPerChunk),
      mSz(numChunksToSend*bytesPerChunk)
  {
      init();

      int data = 0;
      buf = new unsigned char [mBytesPerChunk];
      for (int i = 0; i < mBytesPerChunk; i++) {
	  buf[i] = data++;
	  if (data == 0xff)
	      data = 0;
      }
  }
  ~MyDataProvider();

  virtual void start() {}

  virtual int takeIfAvailable(unsigned char **_buf) {
      int l = mNumChunksLeft > 0 ? mBytesPerChunk : 0;
      if (l > 0) {
	  mNumChunksLeft--;
	  *_buf = buf;
      }
      return l;
  }
  
  virtual bool isDone() const {return mNumChunksLeft==0;}
  
  virtual int getSize() const {return mSz;}
  
  virtual void close() {}

  virtual void reset() {init();}
};


MyDataProvider::~MyDataProvider()
{
  delete [] buf;
}

HttpBinaryPutTest::HttpBinaryPutTest()
  : HttpPutBaseTest(defaultDbUser, defaultDbPswd, defaultDbHost, defaultDbPort, false),
    m_putter(0), m_provider(0), mNow(0), mTransferStart(0), mTransferTime(0)
{
}


HttpBinaryPutTest::HttpBinaryPutTest(const char *dbUser, const char *dbPswd, 
				     const char *dbHost,
				     int dbPort,
				     bool isSSL)
  : HttpPutBaseTest(dbUser, dbPswd, dbHost, dbPort, isSSL),
    m_putter(0), m_provider(0), mNow(0), mTransferStart(0), mTransferTime(0)
{
}


HttpBinaryPutTest::~HttpBinaryPutTest()
{
    assert(m_putter == NULL, "m_putter hasn't been deleted");
    delete m_provider;
}


const char *HttpBinaryPutTest::getDocid() const
{
    return ::docid;
}


bool HttpBinaryPutTest::createPutter(const CouchUtils::Doc &originalDoc)
{
    TF("HttpBinaryPutTest::createPutter");
    TRACE(testName());
    TRACE("entry");

    int i = originalDoc.lookup("_rev");
    if (i >= 0) {
        Str revision = originalDoc[i].getValue().getStr();

	Str url;
	CouchUtils::toAttachmentPutURL(defaultDbName, getDocid(), ATTACHMENT_NAME, revision.c_str(), &url);

	int bytes = 44000*2;
	int bytesPerChunk = 512;
	int chunks = bytes/bytesPerChunk;
	if (chunks*bytesPerChunk < bytes) chunks++;
	m_provider = new MyDataProvider(chunks, bytesPerChunk);
	TRACE2("dbHost: ", getDbHost());
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


bool HttpBinaryPutTest::loop() {
    TF("HttpBinaryPutTest::loop");
    unsigned long now = millis();
    mNow = now;
    if (now > m_timeToAct && !m_didIt) {
        if (m_putter == NULL) {
	    return this->HttpPutBaseTest::loop();
	} else {
            unsigned long callMeBackIn_ms = 0;
            HttpBinaryPut::EventResult r = m_putter->event(now, &callMeBackIn_ms);
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
		        TRACE2("Found unexpected information in the header response:",
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


