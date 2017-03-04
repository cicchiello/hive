#include <http_get.h>

#define HEADLESS
#define NDEBUG
#include <Trace.h>

#include <strutils.h>

#include <Trace.h>

#include <http_headerconsumer.h>

#include <MyWiFi.h>


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
	getHeaderConsumer().setTimedOut(true);
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
        TRACE2("Received an HTTP failure response: ", getHeaderConsumer().getResponse().c_str());
	break;
    case HttpGet::HTTPSuccessResponse:
        TRACE("Received an HTTP success response");
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
        TRACE2("Received an HTTP UNKNOWN failure response: ", getHeaderConsumer().getResponse().c_str());
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
    P("GET ");
    client.print("GET ");
    sendPage(client);
    P(" ");
    client.print(" ");
    PL(TAGHTTP11);
    client.println(TAGHTTP11);
    PL("User-Agent: Hive/0.0.1");
    client.println("User-Agent: Hive/0.0.1");
    sendHost(client);
    PL("Accept: */*");
    client.println("Accept: */*");
    if (!leaveOpen()) {
      PL("Connection: close");
      client.println("Connection: close");
    }
    PL();
    client.println();
    assert(getContext().getClient().connected(), "Client isn't connected !?!? (2)");
}


void HttpGet::sendPage(Stream &client) const
{
    TF("HttpGet::sendPage");
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
	sendGET(getContext().getClient());
	setOpState(ISSUE_OP_FLUSH);
	*callMeBackIn_ms = 10l;
	return CallMeBack;
    }
      break;
    default:
        return this->HttpOp::event(now, callMeBackIn_ms);
    }
}


