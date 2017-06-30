#include <http_sslbinput.t.h>

#include <Arduino.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <http_dataprovider.h>

#include <http_sslbinaryput.h>


//
// Most code taken from Adafruit_WINC1500/examples/WifiWebClient.ino
//

static const char *docid = "feather-binaryget-test";
static const char *ATTACHMENT_NAME = "listen.wav";
static const char *CONTENT_TYPE = "audio/wav";

class MyOtherDataProvider : public HttpDataProvider {
private:
  const int mNumChunksToSend, mBytesPerChunk, mSz;
  
  unsigned char *buf;
  int mNumChunksLeft;
  
public:
  void init()
  {
      mNumChunksLeft = mNumChunksToSend;
  }
  
  MyOtherDataProvider(int numChunksToSend, int bytesPerChunk)
    : mNumChunksToSend(numChunksToSend), mBytesPerChunk(bytesPerChunk),
      mSz(numChunksToSend*bytesPerChunk)
  {
      int data = 0;
      buf = new unsigned char [mBytesPerChunk];
      for (int i = 0; i < mBytesPerChunk; i++) {
	  buf[i] = data++;
	  if (data == 0xff)
	      data = 0;
      }
      init();
  }
  ~MyOtherDataProvider();

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


MyOtherDataProvider::~MyOtherDataProvider()
{
  delete [] buf;
}


HttpSSLBinaryPutTest::HttpSSLBinaryPutTest()
  : HttpBinaryPutTest(sslDbUser, sslDbPswd, sslDbHost, sslDbPort, true)
{
}

HttpSSLBinaryPutTest::~HttpSSLBinaryPutTest() {}

bool HttpSSLBinaryPutTest::createPutter(const CouchUtils::Doc &originalDoc)
{
    TF("HttpSSLBinaryPutTest::createPutter");
    TRACE(testName());
    TRACE("entry");

    int i = originalDoc.lookup("_rev");
    if (i >= 0) {
	static const char *urlPieces[9];
	urlPieces[0] = "/";
	urlPieces[1] = defaultDbName;
	urlPieces[2] = urlPieces[0];
	urlPieces[3] = getDocid();
	urlPieces[4] = urlPieces[0];
	urlPieces[5] = ATTACHMENT_NAME;
	urlPieces[6] = "?rev=";
	urlPieces[7] = originalDoc[i].getValue().getStr().c_str();
	urlPieces[8] = 0;
	
	int bytes = 44000*2;
	int bytesPerChunk = 128;
	int chunks = bytes/bytesPerChunk;
	if (chunks*bytesPerChunk < bytes) chunks++;
	m_provider = new MyOtherDataProvider(chunks, bytesPerChunk);
	m_putter = new HttpSSLBinaryPut(ssid, pass,
					getDbHost(), getDbPort(),
					getDbUser(), getDbPswd(), 
					m_provider, CONTENT_TYPE, urlPieces);
	mTransferStart = mNow;
	mTransferTime = 0;
	return m_putter != NULL;
    } else {
        TRACE("Found unexpected information in the doc:");
	CouchUtils::printDoc(m_getter->getDoc());
	return false;
    }
}



