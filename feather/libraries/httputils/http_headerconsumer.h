#ifndef http_headerconsumer_h
#define http_headerconsumer_h

#include <http_respconsumer.h>

#include <strbuf.h>

class HttpHeaderConsumer : public HttpResponseConsumer {
private:
  bool m_gotCR, m_haveFirstCRLF, m_haveHeader, m_parsedDoc, m_firstConsume;
  bool m_timedout, m_err, m_hasOk, m_hasNotFound;
  bool mHasContentLength, mIsChunked;
  int m_contentLength;
  unsigned long m_firstConsumeTime;

protected:
  void setResponse(const char *newResponse);
  void appendToResponse(char c);
  void appendToResponse(const char *s);
  
private:
  StrBuf m_response;
  
public:
  static const char *TAGContentLength;
  static const char *TAGTransferEncoding;
  static const char *TAGChunked;
  static const char *TAG200;
  static const char *TAG201;
  static const char *TAG404;

  HttpHeaderConsumer(const WifiUtils::Context &ctxt);

  ~HttpHeaderConsumer();
  
  bool consume(unsigned long now);
  bool hasContentLength() const {return mHasContentLength;}
  bool isChunked() const {return mIsChunked;}
  int getContentLength() const {return m_contentLength;}

  const StrBuf &getResponse() const {return m_response;}
  
  bool isError() const {return m_err;}
  const StrBuf &getErrmsg() const {return m_response;}
  
  bool hasOk() const;
  bool hasNotFound() const;
  
  bool isTimeout() const {return m_timedout;}

  unsigned long consumeStart() const {return m_firstConsumeTime;}

  void reset();

  void setTimedOut(bool v) {m_timedout = v;}
  
 private:
  void init();

  void setError(bool v) {m_err = v;}
  
  HttpHeaderConsumer(const HttpHeaderConsumer &); // intentionally unimplemented
};

#endif
