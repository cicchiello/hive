#ifndef http_respconsumer_h
#define http_respconsumer_h

#include <wifiutils.h>

class Str;

class HttpResponseConsumer {
 public:
  HttpResponseConsumer(const WifiUtils::Context &ctxt);
  virtual ~HttpResponseConsumer() {}

  virtual void reset();
  
  // return true to indicate it must be called again;
  // false indicates done or error (and shouldn't be called again)
  virtual bool consume(unsigned long now) = 0;

  virtual bool isError() const = 0;
  virtual const Str &getErrmsg() const = 0;
  
  virtual bool isTimeout() const = 0;

  unsigned long getTimeout() const {return m_timeout;}
  void setTimeout(unsigned long ms) {m_timeout = ms;}
  
 protected:
  const WifiUtils::Context &m_ctxt;

 private:
  unsigned long m_timeout;

  HttpResponseConsumer(const HttpResponseConsumer &); // intentionally unimplemented
};

#endif

