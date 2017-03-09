#include <AccessPointActuator.h>

#include <Arduino.h>


#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <hive_platform.h>

#include <hiveconfig.h>

#include <str.h>
#include <strbuf.h>
#include <strutils.h>



AccessPointActuator::AccessPointActuator(HiveConfig *config, const char *name, unsigned long now)
  : mConfig(*config),
    Actuator(name, now+10*1000)
{
    TF("AccessPointActuator::AccessPointActuator");
}


AccessPointActuator::~AccessPointActuator()
{
}


bool AccessPointActuator::loop(unsigned long now)
{
    return false;
}


bool AccessPointActuator::isMyMsg(const char *cmsg) const
{
    TF("AccessPointActuator::isMyMsg");
    TRACE2("considering: ", cmsg);

    const char *name = getName();
    if (strncmp(cmsg, name, strlen(name)) == 0) {
        cmsg += strlen(name);
	const char *token = "|";

	if (strncmp(cmsg, token, strlen(token)) == 0) {
	    // eat any chars up to next '|'
	    cmsg++;
	    const char *ssid = cmsg;
	    const char *ptr = ssid;
	    while (ptr && *ptr && (*ptr != '|'))
	        ptr++;
	    if (ptr && *ptr && (*ptr == '|')) {
	        // get the final identifier until end of string
	        ptr++;
		const char *pswd = ptr;
		ptr = pswd;
		while (ptr && *ptr)
		    ptr++;
		
	        TRACE("It's mine!");
		return true;
	    }
	}
    }
    return false;
}


void AccessPointActuator::processMsg(unsigned long now, const char *msg)
{
    TF("AccessPointActuator::processMsg");

    const char *token1 = getName();
    const char *token2 = "|";
    const char *cmsg = msg + strlen(token1) + strlen(token2);

    TRACE2("parsing: ", cmsg);
    
    // take all chars up to next '|' as ssid

    StrBuf ssid;
    const char *ptr = cmsg;
    while (ptr && *ptr && (*ptr != '|')) {
        ssid.append(*ptr);
	ptr++;
    }
    assert(ptr && *ptr && (*ptr == '|'), "parsing problem");
    
    // get the final identifier until end of string
    ptr++;
    StrBuf pswd;
    while (ptr && *ptr) {
        pswd.append(*ptr);
	ptr++;
    }

    PH3("Have ssid/pswd: ", ssid.c_str(), pswd.c_str());
    CouchUtils::Doc doc = mConfig.getDoc();
    int issid = doc.lookup(HiveConfig::SsidProperty);
    int ipswd = doc.lookup(HiveConfig::SsidPswdProperty);
    doc.setValue(HiveConfig::SsidProperty, ssid.c_str());
    doc.setValue(HiveConfig::SsidPswdProperty, pswd.c_str());

    StrBuf dump;
    PH2("Modified config doc: ", CouchUtils::toString(doc, &dump));
    
    mConfig.setDoc(doc);
}

