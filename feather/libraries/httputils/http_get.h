#ifndef http_get_h
#define http_get_h

#include <http_op.h>

class HttpHeaderConsumer;

class HttpGet : public HttpOp {
 public:
   HttpGet(const Str &ssid, const Str &ssidPswd, 
	   const Str &host, int port, 
	   const Str &dbUser, const Str &dbPswd,
	   bool isSSL, const char *urlPieces[]);
  
   virtual ~HttpGet() {}

   virtual EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);

   virtual bool processEventResult(EventResult r);
   
   virtual HttpHeaderConsumer &getHeaderConsumer() = 0;
   virtual const HttpHeaderConsumer &getHeaderConsumer() const = 0;
   
   virtual HttpResponseConsumer &getResponseConsumer();
   
 protected:
   // returns true on success
   virtual bool testSuccess() const {return true;}

   virtual bool leaveOpen() const {return false;}
   
   virtual void sendGET(class Stream &) const;
   virtual void sendPage(class Stream &) const;

   HttpGet(const HttpGet &); //intentionally unimplemented

   const char **m_urlPieces;
};

#endif
