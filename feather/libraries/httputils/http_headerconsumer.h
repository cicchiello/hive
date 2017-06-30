#ifndef http_headerconsumer_h
#define http_headerconsumer_h

#include <http_respconsumer.h>

#include <strbuf.h>

class HttpHeaderConsumer : public HttpResponseConsumer {
private:
  bool m_gotCR, m_haveFirstCRLF, m_haveHeader, m_firstConsume;
  unsigned long m_firstConsumeTime;
  
  bool m_timedOut, m_err, m_hasOk, m_hasNotFound, m_isChunked, m_hasContentLength;
  int m_contentLength;

private:
  StrBuf m_line;
  StrBuf m_errMsg;
  
public:
  static const char *TAGContentLength;
  static const char *TAGTransferEncoding;
  static const char *TAGChunked;
  static const char *TAG200;
  static const char *TAG201;
  static const char *TAG404;

  HttpHeaderConsumer(const WifiUtils::Context &ctxt);

  ~HttpHeaderConsumer() {}
  
  bool consume(unsigned long now);
  
  bool hasContentLength() const {return m_hasContentLength;}
  bool isChunked() const {return m_isChunked;}
  int getContentLength() const {return m_contentLength;}

  bool isError() const {return m_err;}
  
  const StrBuf &getErrmsg() const {return m_errMsg;}
  
  bool hasOk() const {return m_hasOk;}
  bool hasNotFound() const {return m_hasNotFound;}
  bool isTimeout() const {return m_timedOut;}

  unsigned long consumeStart() const {return m_firstConsumeTime;}

  void reset();

  void setTimedOut(bool v) {m_timedOut = v;}

 protected:
  virtual void parseHeaderLine(const StrBuf &line);
  
 private:
  void init();

  void setError(bool v) {m_err = v;}
  
  HttpHeaderConsumer(const HttpHeaderConsumer &); // intentionally unimplemented
};

#endif
