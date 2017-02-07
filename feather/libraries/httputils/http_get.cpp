#include <http_get.h>

//#define HEADLESS
#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <http_headerconsumer.h>

#include <MyWiFi.h>


HttpGet::HttpGet(const char *ssid, const char *ssidPswd, 
		 const char *host, int port, const char *page,
		 const char *credentials, bool isSSL)
  : HttpOp(ssid, ssidPswd, host, port, credentials, isSSL),
    m_page(page)
{
    TF("HttpGet::HttpGet (1)");
    TRACE2("host/port: ", Str(host).append(":").append(port).c_str());
}

HttpGet::HttpGet(const char *ssid, const char *ssidPswd, 
		 const IPAddress &hostip, int port, const char *page,
		 const char *credentials, bool isSSL)
  : HttpOp(ssid, ssidPswd, hostip, port, credentials, isSSL),
    m_page(page)
{
    TF("HttpGet::HttpGet (2)");
    DH(hostip); D(":"); DL(port);
}


HttpResponseConsumer &HttpGet::getResponseConsumer()
{
    return getHeaderConsumer();
}


bool HttpGet::processEventResult(HttpGet::EventResult r)
{
    TF("HttpGet::processEventResult");
    
    bool done = true;
    // since there's so few successful paths, assume the worst and be specific otherwise
    
    switch (r) {
    case HttpGet::CallMeBack: 
	done = false;
	break;
    case HttpGet::ConnectTimeout:
        TRACE("WiFi shield couldn't connect!");
	break;
    case HttpGet::ConnectFailed:
        TRACE("Unexpected WiFi Connect state: ");
	TRACE(getWifiConnectState());
	break;
    case HttpGet::IssueOpFailed:
        TRACE("failure while issuing HTTP request");
	break;
    case HttpGet::NoHTTPResponse:
        TRACE("Failure: No response received for the HTTP GET");
	break;
    case HttpGet::HTTPFailureResponse:
        TRACE("Received an HTTP failure response: ");
	TRACE(getHeaderConsumer().getResponse().c_str());
	break;
    case HttpGet::HTTPSuccessResponse:
        if (!testSuccess()) {
	    if (getRetryCnt() < MaxRetries) {
	        TRACE("retry #"); TRACE(getRetryCnt()+1);
		setRetryCnt(getRetryCnt()+1);
		setFinalResult(HTTPRetry);
		resetForRetry();
		done = false;
	    } else {
	        TRACE("giving up after "); P(MaxRetries); TRACE(" retries");
	    }
	} else {
	    TRACE("Success");
	}
	break;
    case HttpOp::DisconnectFailure:
        TRACE("Disconnect FAILURE");
	break;
    case HttpOp::UnknownFailure:
        TRACE("Unknown FAILURE");
	break;
    default: 
        TRACE2("Unexpected EventResult: ", r);
    }
    return !done;
}


void HttpGet::sendGET(Stream &client) const
{
    TF("HttpGet::sendGET");
    
    assert(getContext().getClient().connected(), "Client isn't connected !?!? (1)");
    client.print("GET "); sendPage(client); client.print(" "); client.println(TAGHTTP11);
    client.println("User-Agent: Hive/0.0.1");
    sendHost(client);
    client.println("Accept: */*");
    client.println("Connection: close");
    client.println();
    assert(getContext().getClient().connected(), "Client isn't connected !?!? (2)");
}


void HttpGet::sendPage(Stream &client) const
{
    P(m_page.c_str());
    client.print(m_page.c_str());
}


HttpGet::EventResult HttpGet::event(unsigned long now, unsigned long *callMeBackIn_ms)
{
    TF("HttpGet::event");
    
    OpState opState = getOpState();
    switch (opState) {
    case ISSUE_OP: {
        TRACE("ISSUE_OP");
        WiFiClient &client = getContext().getClient();
	sendGET(client);
	setOpState(CONSUME_RESPONSE);
	*callMeBackIn_ms = 10l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpOp::event(now, callMeBackIn_ms);
    }
}


