#include <http_op.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <strbuf.h>
#include <strutils.h>

#include <MyWiFi.h>

#include <wifiutils.h>
#include <base64.h>

#include <hive_platform.h>


#undef min
#undef max
#include <string>
#include <map>
#include <utility>

/* STATIC */
const char *HttpOp::TAGHTTP11 = "HTTP/1.1";
const int HttpOp::MaxRetries = 5;
const long HttpOp::RetryDelay_ms = 3000l;
Str HttpOp::sCachedEncodedAuth(Str::sEmpty), HttpOp::sCachedDbUser(Str::sEmpty), HttpOp::sCachedDbPswd(Str::sEmpty);


class MyMap {
private:
  Str keys[10];
  IPAddress values[10];
  int num;
  
public:
  MyMap() : num(0) {}
  bool isFull() {return (num == 10);}
  void add(const Str& key, const IPAddress &value) {
    keys[num] = key;
    values[num++] = value;
  }
  bool contains(const Str &key) const {
    for (int i = 0; i < num; i++)
      if (keys[i].equals(key))
	return true;
    return false;
  }
  const IPAddress &getValue(const Str &key) const {
    for (int i = 0; i < num; i++)
      if (keys[i].equals(key))
	return values[i];
    return IPAddress();
  }
};

typedef MyMap MapType;
static MapType sDnsResolutions;


HttpOp::~HttpOp()
{
    TF("HttpOp::~HttpOp");

    if (m_shutdown) {
        PH("calling WiFi.end()");
	getContext().getWifi().end();
	getContext().reset();
    }
}


void HttpOp::init()
{
    TF("HttpOp::init");
    TRACE("entry");

    // check for the presence of the shield:
    if (getContext().getWifi().status() == WL_NO_SHIELD) {
        TRACE("WiFi shield not present");
	FAIL();
    }

    // make sure that I'm not already connected
    if (getContext().getWifi().status() == WL_CONNECTED) {
        TRACE("WiFi shield is already connected!?!?");
	FAIL();
    }

    mDnsCnt = mHttpConnectCnt = mWifiConnectState = m_retries = m_disconnectCnt = 0;
    mWifiWaitStart = mDnsWaitStart = mHttpWaitStart = 0;
    m_finalResult = UnknownFailure;
    m_opState = WIFI_INIT;
}


void HttpOp::resetForRetry()
{
    TF("HttpOp::resetForRetry");
    init();
}


static StrBuf IPtoString(const IPAddress &ip) 
{
    StrBuf s;
    for (int i =0; i < 3; i++)
    {
        s.append(ip[i]);
	s.append(".");
    }
    s.append(ip[3]);
    return s;
}


HttpOp::ConnectStat HttpOp::httpConnectInit(const IPAddress &host, const char *hostname)
{
    TF("HttpOp::httpConnectInit");
    TRACE("entry");

    TRACE2("m_port: ", m_port);
    TRACE2("m_isSSL: ", (m_isSSL ? "true" : "false"));
    TRACE2("host: ", IPtoString(host).c_str());
    TRACE2("hostname: ", hostname);
    
    WiFiClient::ConnectStat s = m_ctxt.getClient().connectNoWait(host, m_port,
								 m_isSSL ? SOCKET_FLAGS_SSL : 0,
								 (const uint8_t*) hostname);
    switch (s) {
    case WiFiClient::FAILED: return HttpOp::FAILED;
    case WiFiClient::WORKING: yield(); return HttpOp::WORKING;
    case WiFiClient::CONNECTED: return HttpOp::CONNECTED;
    default:
      ERR(HivePlatform::singleton()->error("Unknown WiFiClient::ConnectStat value"));
    }
}


HttpOp::ConnectStat HttpOp::httpConnectCheck()
{
    TF("HttpOp::httpConnectCheck");
    //TRACE("entry");
    
    WiFiClient::ConnectStat s = m_ctxt.getClient().checkConnectNoWait();
    switch (s) {
    case WiFiClient::FAILED: return HttpOp::FAILED;
    case WiFiClient::WORKING: yield(); return HttpOp::WORKING;
    case WiFiClient::CONNECTED: return HttpOp::CONNECTED;
    default:
      ERR(HivePlatform::singleton()->error("Unknown WiFiClient::ConnectStat value"));
    }
}


void HttpOp::httpConnectCancel()
{
    TF("HttpOp::connectFailed");
    TRACE("entry");
    m_ctxt.getClient().connectionFailed();
}


void HttpOp::sendHost(class Stream &s) const
{
    TF("HttpOp::sendHost");
    assert(getContext().getClient().connected(), "Client isn't connected !?!? (1)");

    P("Host: ");
    s.print("Host: ");
    
    if (mSpecifiedHostname.len() == 0) {
        P(mSpecifiedHostIP);
        s.print(mSpecifiedHostIP);
    } else {
        P(mSpecifiedHostname.c_str());
        s.print(mSpecifiedHostname.c_str());
    }
    assert(getContext().getClient().connected(), "Client isn't connected !?!? (2)");
    P(":");
    s.print(":");
    PL(m_port);
    s.println(m_port);
    if (m_dbuser.len() > 0) {
        if (!m_dbuser.equals(sCachedDbUser) || !m_dbpswd.equals(sCachedDbPswd)) {
	    StrBuf creds;
	    creds.append(m_dbuser.c_str()).append(":").append(m_dbpswd.c_str());
	
	    StrBuf encoded;
	    base64_encode(&encoded, creds.c_str(), creds.len());

	    sCachedEncodedAuth = encoded.c_str();
	    sCachedDbUser = m_dbuser;
	    sCachedDbPswd = m_dbpswd;
	}
        const char *authStr = sCachedEncodedAuth.c_str();

        P("Authorization: Basic ");
	PL(authStr);
	s.print("Authorization: Basic ");
	s.println(authStr);
    }
    assert(getContext().getClient().connected(), "Client isn't connected !?!? (3)");
}


HttpOp::EventResult HttpOp::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpOp::event");
    
    switch (m_opState) {
    case WIFI_INIT:
    case WIFI_WAITING: {
        TF("HttpOp::event; WIFI_WAITING");
        if (m_opState == WIFI_INIT) {
	    TRACE("WIFI_INIT");
	    mWifiConnectState = 0;
	    mWifiWaitStart = now;
	} else {
	    yield();
	    TRACE("WIFI_WAITING");
	}
        WifiUtils::ConnectorStatus cstat = WifiUtils::connector(m_ctxt, m_ssid.c_str(),
								m_pswd.c_str(), &mWifiConnectState,
								callMeBackIn_ms);
	switch (cstat) {
	case WifiUtils::ConnectSucceed: {
	    TF("HttpOp::event; WIFI_WAITING; ConnectSucceed");
	    TRACE2("Wifi connected after (ms) ", (now-mWifiWaitStart));
	    TRACE2("SSID: ",m_ctxt.getWifi().SSID());
	    TRACE2("IP: ", IPtoString(m_ctxt.getWifi().localIP()).c_str());
	    TRACE2("GATEWAY: ", IPtoString(m_ctxt.getWifi().gatewayIP()).c_str());
	    TRACE2("SUBNET: ", IPtoString(m_ctxt.getWifi().subnetMask()).c_str());
	    TRACE2("DNS: ", IPtoString(m_ctxt.getWifi().dnsIP()).c_str());
	    TRACE2("URL: ", mSpecifiedHostname.c_str());
	    if (mSpecifiedHostname.len() == 0) {
	        TRACE("Skipping DNS lookup because an IP address was supplied");
		setOpState(HTTP_INIT);
	    } else {
	        if (sDnsResolutions.contains(mSpecifiedHostname)) {
		    const IPAddress &ip = sDnsResolutions.getValue(mSpecifiedHostname);
		    TRACE2("reusing previously resolved IP: ", IPtoString(ip).c_str());
		    mResolvedHostIP = ip;
		    setOpState(HTTP_INIT);
		} else {
		    TRACE("Performing DNS lookup");
		    setOpState(DNS_INIT);
		}
	    }
	    *callMeBackIn_ms = 10l;
	    return CallMeBack;
	}
	case WifiUtils::ConnectRetry: {
	    TF("HttpOp::event; WIFI_WAITING; ConnectRetry");
	    TRACE("ConnectRetry");
	    setOpState(WIFI_WAITING);
	    return CallMeBack;
	}
	case WifiUtils::ConnectTimeout: {
	    TF("HttpOp::event; WIFI_WAITING; ConnectTimeout");
	    TRACE("Wifi connect timeout");
	    return ConnectTimeout;
	}
	default: {
	    TF("HttpOp::event; WIFI_WAITING; ConnectFailed");
	    TRACE("Connect failed");
	    return ConnectFailed;
	}
	}
    }
	break;
    case DNS_INIT:
    case DNS_WAITING: {
        TF("HttpOp::event; DNS_INIT || DNS_WAITING");
        TRACE(m_opState == DNS_INIT ? "DNS_INIT" : "DNS_WAITING");
	MyWiFi &w = m_ctxt.getWifi();
	MyWiFi::DNS_State dnsState; 
	if (m_opState == DNS_INIT) {
	    TF("HttpOp::event; DNS_INIT");
	    TRACE2("resolving mSpecifiedHostname: ", mSpecifiedHostname.c_str());
	    mDnsCnt = 41; // 40x500ms == 20s
	    dnsState = w.dnsNoWait(mSpecifiedHostname.c_str(), mResolvedHostIP);
	    mDnsWaitStart = now;
	    setOpState(DNS_WAITING);
	} else {
	    TF("HttpOp::event; DNS_WAITING");
	    yield();
	    dnsState = w.dnsCheck(mResolvedHostIP);
	}
	
	*callMeBackIn_ms = 10l;
	switch (dnsState) {
	case MyWiFi::DNS_SUCCESS: {
	    TF("HttpOp::event; DNS_INIT; DNS_SUCCESS");
	    // have ipAddress
	    Str ip;
	    TRACE2("Have IP Address after waiting (ms) ", (now-mDnsWaitStart));
	    if (!sDnsResolutions.isFull())
	        sDnsResolutions.add(mSpecifiedHostname, mResolvedHostIP);
	    TRACE2("IP: ", IPtoString(mResolvedHostIP).c_str());
	    setOpState(HTTP_INIT);
	}
	  break;
	case MyWiFi::DNS_RETRY: {
	    TF("HttpOp::event; DNS_INIT; DNS_RETRY");
	    yield();
	    if (--mDnsCnt == 0) {
	        TRACE2("DNS_WAITING timeout after (ms) ", (now-mDnsWaitStart));
		w.dnsFailed();
		setFinalResult(IssueOpFailed);
		setOpState(DISCONNECTING);
	    } else *callMeBackIn_ms = 500l;
	}
	  break;
	case MyWiFi::DNS_FAILED: {
	    TF("HttpOp::event; DNS_INIT; DNS_FAILED");
	    TRACE2("DNS_FAILED after (ms) ", (now-mDnsWaitStart));
	    w.dnsFailed();
	    setFinalResult(IssueOpFailed);
	    setOpState(DISCONNECTING);
	}
	}
	
	return CallMeBack;
    }
        break;
    case HTTP_INIT:
    case HTTP_WAITING: {
        TF("HttpOp::event; HTTP_INIT");
        ConnectStat stat;
        if (m_opState == HTTP_INIT) {
	    TRACE("HTTP_INIT");
	    mHttpConnectCnt = 100; //100*100ms == 10s
	    stat = httpConnectInit(mResolvedHostIP, mSpecifiedHostname.c_str());
	    mHttpWaitStart = now;
	    setOpState(HTTP_WAITING);
	} else {
	    TRACE("HTTP_WAITING");
	    yield();
	    stat = httpConnectCheck();
	}
	switch (stat) {
	case WORKING: {
	    TF("HttpOp::event; HTTP_INIT; WORKING");
	    TRACE("WORKING");
	    yield();
	    *callMeBackIn_ms = 100l;
	    if (--mHttpConnectCnt == 0) {
	        TRACE2("HTTP_WAITING timeout after (ms) ", (now-mHttpWaitStart));
	        TRACE("disconnecting");
		httpConnectCancel();
		setFinalResult(IssueOpFailed);
		setOpState(DISCONNECTING);
		*callMeBackIn_ms = 10l;
	    }
	}
	    break;
	case CONNECTED: {
	    TF("HttpOp::event; HTTP_INIT; CONNECTED");
	    TRACE("CONNECTED");
	    setOpState(ISSUE_OP);
	    *callMeBackIn_ms = 10l;
	}
	    break;
	case FAILED: 
	default: {
	    TF("HttpOp::event; HTTP_INIT; default");
	    TRACE("connect failed");
	    httpConnectCancel();
	    setFinalResult(IssueOpFailed);
	    setOpState(DISCONNECTING);
	    *callMeBackIn_ms = 10l;
	}
	}
	return CallMeBack;
    }
      break;
    case ISSUE_OP:
    case CHUNKING:
        yield();
        TRACE("ERROR: derived class should handle the ISSUE_OP and CHUNKING states");
	return UnknownFailure;
    case ISSUE_OP_FLUSH: {
        int remaining;
	yield();
        if (getContext().getClient().flushOut(&remaining) && (remaining == 0)) {
	    setOpState(CONSUME_RESPONSE);
	}
	return CallMeBack;
    }
      break;
    case CONSUME_RESPONSE: {
        TF("HttpOp::event; CONSUME_RESPONSE");
	unsigned long mark = micros();
	yield();
	TRACE2("CONSUME_RESPONSE; yield took (us): ", (micros()-mark));
        if (!getResponseConsumer().consume(now)) {
	    m_finalResult = HTTPSuccessResponse; // start optimistically
	    if (!getResponseConsumer().isError()) {
	        TRACE("http operation done: ");
	    } else {
	        if (getResponseConsumer().isTimeout()) {
		    TRACE("TIMEOUT from response consumer");
		} else {
		    TRACE2("Errmsg from response consumer: ", getResponseConsumer().getErrmsg().c_str());
		}
		m_finalResult = HTTPFailureResponse;
	        if (getRetryCnt() < MaxRetries) {
		    TRACE2("retry #",(getRetryCnt()+1));
		    setRetryCnt(getRetryCnt()+1);
		    m_finalResult = HTTPRetry;
		} else {
		    TRACE("MaxRetries exceeded; giving up");
		}
	    }
	    setOpState(DISCONNECTING);
	}
	*callMeBackIn_ms = 50l;
	return CallMeBack;
    }
    case DISCONNECTING: {
        TF("HttpOp::event; DISCONNECTING");
        TRACE("DISCONNECTING");
        switch (m_ctxt.getWifi().status()) {
	case WL_CONNECTION_LOST:
	case WL_IDLE_STATUS:
	case WL_DISCONNECTED:
	    TRACE("Disconnected!");
	    if (m_finalResult == HTTPRetry) {
	        TRACE("setting up to retry");
	        resetForRetry();
		*callMeBackIn_ms = RetryDelay_ms;
		return CallMeBack;
	    } else {
	        setOpState(DISCONNECTED);
		return m_finalResult;
	    }
	default:
	    if (m_disconnectCnt == 20) { // 10s
	        TRACE("WARNING: Disconnection failure!");
		setOpState(DISCONNECTED);
	        return DisconnectFailure;
	    } else {
	        yield();
	        if (m_disconnectCnt == 0) {
		    TRACE("calling disconnect");
		    m_ctxt.getWifi().disconnect();
		}
		*callMeBackIn_ms = 500l;
		++m_disconnectCnt;
		return CallMeBack;
	    }
	}
    }
	break;
    case DISCONNECTED: {
        TF("HttpOp::event; CONSUME_RESPONSE");
        // no-op
        *callMeBackIn_ms = 10000l;
	return CallMeBack;
    }
    default: {
        TF("HttpOp::event; default");
        TRACE2("unknown m_opState==", m_opState);
	return UnknownFailure;
    }
    }
}


/* STATIC */
HttpOp::YieldHandler HttpOp::sYieldHandler = NULL;

/* STATIC */
HttpOp::YieldHandler HttpOp::registerYieldHandler(HttpOp::YieldHandler yieldHandler)
{
    YieldHandler prev = sYieldHandler;
    sYieldHandler = yieldHandler;
    return prev;
}

