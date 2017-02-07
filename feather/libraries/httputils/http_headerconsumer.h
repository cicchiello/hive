#ifndef http_headerconsumer_h
#define http_headerconsumer_h

#include <http_respconsumer.h>

#include <str.h>

class HttpHeaderConsumer : public HttpResponseConsumer {
private:
  bool m_gotCR, m_haveFirstCRLF, m_haveHeader, m_parsedDoc, m_firstConsume, m_timedout, m_err;
  bool mHasContentLength, mIsChunked;
  int m_contentLength;
  unsigned long m_firstConsumeTime;

protected:
  Str m_response;
  
public:
  static const char *TAGContentLength;
  static const char *TAGTransferEncoding;
  static const char *TAGChunked;
  static const char *TAG200;
  static const char *TAG201;

  HttpHeaderConsumer(const WifiUtils::Context &ctxt);

  ~HttpHeaderConsumer() {}
  
  bool consume(unsigned long now);
  bool hasContentLength() const {return mHasContentLength;}
  bool isChunked() const {return mIsChunked;}
  int getContentLength() const {return m_contentLength;}

  const Str &getResponse() const {return m_response;}
  
  bool isError() const {return m_err;}
  const Str &getErrmsg() const {return m_response;}
  
  bool hasOk() const;
  bool isTimeout() const {return m_timedout;}

  unsigned long consumeStart() const {return m_firstConsumeTime;}

  void reset();

 protected:
  void setError(bool v) {m_err = v;}
  
 private:
  void init();

  HttpHeaderConsumer(const HttpHeaderConsumer &); // intentionally unimplemented
};

#endif
