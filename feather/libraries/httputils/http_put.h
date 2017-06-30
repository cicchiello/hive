#ifndef http_put_h
#define http_put_h

#include <http_op.h>

class HttpHeaderConsumer;

class HttpPut : public HttpOp {
 public:
    static const char *DefaultContentType;

    virtual ~HttpPut() {}

    virtual EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);

    virtual int getContentLength() const;
    
    virtual HttpResponseConsumer &getResponseConsumer();
    
    virtual HttpHeaderConsumer &getHeaderConsumer() = 0;
    
    void setHeaderConsumer(HttpHeaderConsumer *newConsumer);

    void setDoc(const Str &doc);
    
 protected:
    HttpPut(const char *ssid, const char *ssidPswd, 
	    const char *host, int port, const char *urlPieces[],
	    const char *dbUser, const char *dbPswd, 
	    const char *contentType = DefaultContentType,
	    bool isSSL = false);
    
    void sendPUT(class Stream &) const;
    void sendDoc(class Stream &, const char *docStr) const;

    Str m_doc;
    const Str m_contentType;
    const char **m_urlPieces;
    
    HttpPut(const HttpPut &); //intentionally unimplemented
};

inline
int HttpPut::getContentLength() const {return m_doc.len();}


#endif
