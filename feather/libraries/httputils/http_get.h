#ifndef http_get_h
#define http_get_h

#include <http_op.h>

class HttpHeaderConsumer;

class HttpGet : public HttpOp {
 public:
   HttpGet(const char *ssid, const char *ssidPswd, 
	   const char *host, int port, const char *page,
	   const char *credentials, bool isSSL = false);
   HttpGet(const char *ssid, const char *ssidPswd, 
	   const IPAddress &hostip, int port, const char *page,
	   const char *credentials, bool isSSL = false);

   virtual ~HttpGet() {}

   virtual EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);

   virtual bool processEventResult(EventResult r);
   
   virtual HttpHeaderConsumer &getHeaderConsumer() = 0;
   virtual const HttpHeaderConsumer &getHeaderConsumer() const = 0;
   
   virtual HttpResponseConsumer &getResponseConsumer();
   
 protected:
   // returns true on success
   virtual bool testSuccess() const {return true;}

   virtual void sendGET(class Stream &) const;
   virtual void sendPage(class Stream &) const;

   HttpGet(const HttpGet &); //intentionally unimplemented

   const Str m_page;
};

#endif
