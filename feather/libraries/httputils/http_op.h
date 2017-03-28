#ifndef http_op_h
#define http_op_h

#include <http_respconsumer.h>

#include <str.h>

class IPAddress;

class HttpOp {
 public:
   static const char *TAGHTTP11;

   static const int MaxRetries;
   static const long RetryDelay_ms;
   
   enum OpState {
       WIFI_INIT,
       WIFI_WAITING,
       DNS_INIT,
       DNS_WAITING,
       HTTP_INIT, 
       HTTP_WAITING, 
       ISSUE_OP,
       ISSUE_OP_FLUSH,
       CHUNKING,
       CONSUME_RESPONSE,
       DISCONNECTING,
       DISCONNECTED
   }; 

   virtual ~HttpOp();

   enum EventResult {
       CallMeBack,
       ConnectTimeout,
       ConnectFailed,
       IssueOpFailed,
       NoHTTPResponse,
       HTTPFailureResponse,
       HTTPSuccessResponse,
       DisconnectFailure,
       HTTPRetry,
       UnknownFailure
   };

   virtual EventResult event(unsigned long now, unsigned long *callMeBackIn_ms);

   virtual HttpResponseConsumer &getResponseConsumer() = 0;
   
   const WifiUtils::Context &getContext() const;

   OpState getOpState() const;

   int getWifiConnectState() const;

   int getRetryCnt() const;

   EventResult getFinalResult() const;
  
   void setRetryCnt(int r);

   typedef void (*YieldHandler)();
   static YieldHandler registerYieldHandler(YieldHandler yieldHander);
   
   void yield() {sYieldHandler();}
   
   void shutdownWifiOnDestruction(bool shutdown) {m_shutdown = shutdown;}

 protected:
   static YieldHandler sYieldHandler;
   
   HttpOp(const char *ssid, const char *ssidPswd, const char *hostname, int port,
	  const char *dbUser, const char *dbPswd,
	  bool isSSL = false)
     : m_ssid(ssid), m_pswd(ssidPswd), mSpecifiedHostname(hostname), m_port(port),
       m_dbuser(dbUser), m_dbpswd(dbPswd), mSpecifiedHostIP(), m_isSSL(isSSL), m_shutdown(true)
   {
     init();
   }
   
   HttpOp(const Str &ssid, const Str &ssidPswd, const Str &hostname, int port,
	  const Str &dbUser, const Str &dbPswd,
	  bool isSSL = false);
   
   HttpOp(const char *ssid, const char *ssidPswd, const IPAddress &hostip, int port,
	  const char *dbUser, const char *dbPswd,
	  bool isSSL = false)
     : m_ssid(ssid), m_pswd(ssidPswd), mSpecifiedHostname((char*)0), m_port(port),
       m_dbuser(dbUser), m_dbpswd(dbPswd), mSpecifiedHostIP(hostip), m_isSSL(isSSL), m_shutdown(true)
   {
     init();
   }

   HttpOp(const HttpOp &); //intentionally unimplemented

   enum ConnectStat {FAILED, WORKING, CONNECTED};
   ConnectStat httpConnectInit(const IPAddress &host, const char *hostname);
   ConnectStat httpConnectCheck();
   void httpConnectCancel();
   
   void setOpState(OpState s);
   
   void init();
   
   void setFinalResult(EventResult r);
   
   void sendHost(class Stream &) const;
   
   virtual void resetForRetry();

 private:
   const WifiUtils::Context m_ctxt;

   const IPAddress mSpecifiedHostIP;
   const Str m_ssid, m_pswd, mSpecifiedHostname, m_dbuser, m_dbpswd;
   const int m_port;
   const bool m_isSSL;
   bool m_shutdown;
   
   IPAddress mResolvedHostIP;
   OpState m_opState;
   
   int mWifiConnectState, m_disconnectCnt, m_retries, mHttpConnectCnt, mDnsCnt;
   unsigned long mHttpWaitStart, mDnsWaitStart, mWifiWaitStart;
   EventResult m_finalResult;

   static Str sCachedEncodedAuth, sCachedDbUser, sCachedDbPswd;
};

inline
const WifiUtils::Context &HttpOp::getContext() const {return m_ctxt;}

inline 
int HttpOp::getWifiConnectState() const {return mWifiConnectState;}

inline
int HttpOp::getRetryCnt() const {return m_retries;}

inline
void HttpOp::setRetryCnt(int r) {m_retries = r;}

inline
HttpOp::OpState HttpOp::getOpState() const {return m_opState;}

inline
void HttpOp::setOpState(HttpOp::OpState s) {m_opState = s;}

inline
void HttpOp::setFinalResult(EventResult r) {m_finalResult = r;}

inline
HttpOp::EventResult HttpOp::getFinalResult() const {return m_finalResult;}

#endif
