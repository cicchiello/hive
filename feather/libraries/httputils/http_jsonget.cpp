#include <http_jsonget.h>

#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>



HttpJSONGet::HttpJSONGet(const Str &ssid, const Str &ssidPswd, 
			 const Str &host, int port, 
			 const Str &dbUser, const Str &dbPswd, 
			 bool isSSL, const char *urlPieces[])
  : HttpGet(ssid, ssidPswd, host, port, dbUser, dbPswd, isSSL, urlPieces),
    mConsumer(getContext())
{
  TF("HttpJSONGet::HttpJSONGet");
  init();
}


void HttpJSONGet::init()
{
    TF("HttpJSONGet::init");
    mConsumer.reset();
}


void HttpJSONGet::resetForRetry()
{
    TF("HttpJSONGet::resetForRetry");
    init();

    HttpGet::resetForRetry();
}


bool HttpJSONGet::haveDoc() const
{
    TF("HttpJSONGet::haveDoc");
    return mConsumer.hasOk() && mConsumer.hasDoc();
}


bool HttpJSONGet::testSuccess() const
{
    TF("HttpJSONGet::testSuccess");
    bool r = getHeaderConsumer().hasOk() || getHeaderConsumer().hasNotFound();
    TRACE(r ? "looks good" : "failed");
    return r;
}


const StrBuf &HttpJSONGet::getETag() const {
    return mConsumer.getETag();
}


bool HttpJSONGet::getTimestamp(StrBuf *date) const {
    *date = mConsumer.getTimestamp();
    return true;
}


bool HttpJSONGet::hasTimestamp() const {
    TF("HttpJSONGet::hasTimestamp");
    return mConsumer.getTimestamp().len() > 0;
}
