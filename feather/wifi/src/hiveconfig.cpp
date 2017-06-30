#include <hiveconfig.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <platformutils.h>

#include <TimeProvider.h>

#include <strutils.h>
#include <couchutils.h>

#include <strbuf.h>


/* STATIC */
HiveConfig::UpdateFunctor *HiveConfig::mUpdateFunctor = NULL;

static Str sEmpty;

HiveConfig::HiveConfig(const char *resetCause, const char *versionId, const TimeProvider **timeProvider)
  : mDoc(), mRCause(resetCause), mVersionId(versionId), mTimeProvider(timeProvider)
{
    TF("HiveConfig::HiveConfig");
    setDefault();
}


HiveConfig::~HiveConfig()
{
    delete mUpdateFunctor;
}


bool HiveConfig::setDoc(const CouchUtils::Doc &doc)
{
    TF("HiveConfig::setDoc");

    CouchUtils::Doc cleanedDoc; // make a new doc that uses the known property names
    for (int i = 0; i < doc.getSz(); i++) {
        bool found = false;
	for (int j = 0; !found && j < mDoc.getSz(); j++) {
	    if (mDoc[j].getName().equals(doc[i].getName())) {
	        found = true;
		if (mDoc[j].getName().equals(HiveFirmwareProperty)) {
		    cleanedDoc.addNameValue(new CouchUtils::NameValuePair(HiveFirmwareProperty, mVersionId));
		} else if (mDoc[j].getName().equals(HiveIdProperty)) {
		    cleanedDoc.addNameValue(new CouchUtils::NameValuePair(HiveIdProperty, getHiveId()));
		} else if (mDoc[j].getName().equals(TimestampProperty)) {
		    Str propStr;
		    if (mTimeProvider && *mTimeProvider) {
		        (*mTimeProvider)->toString(millis(), &propStr);
			cleanedDoc.addNameValue(new CouchUtils::NameValuePair(TimestampProperty, propStr));
		    } else {
		        cleanedDoc.addNameValue(new CouchUtils::NameValuePair(mDoc[j]));
		    }
		} else {
		    if (mDoc[j].getValue().isStr() && doc[i].getValue().isStr() &&
			mDoc[j].getValue().getStr().equals(doc[i].getValue().getStr())) {
		        cleanedDoc.addNameValue(new CouchUtils::NameValuePair(mDoc[j]));
		    } else {
		        cleanedDoc.addNameValue(new CouchUtils::NameValuePair(mDoc[j].getName(), doc[i].getValue()));
		    }
		}
	    }
	}
	if (!found) {
	    cleanedDoc.addNameValue(new CouchUtils::NameValuePair(doc[i].getName(), doc[i].getValue()));
	}
    }
    
    mDoc = cleanedDoc;
    
    StrBuf dump;
    TRACE2("cleaned config doc: ", CouchUtils::toString(mDoc, &dump));
 
    if (mUpdateFunctor != NULL) {
        TRACE("Calling HiveConfig updater");
        mUpdateFunctor->onUpdate(*this);
    }
    
    return isValid();
}


const Str True("true");
const Str False("false");

const Str DEFAULT_MyWifi("<MyWifi>");
const Str DEFAULT_Timestamp("1400000000");
const Str DEFAULT_Passcode("<MyPasscode>");
const Str DEFAULT_DbHost("jfcenterprises.cloudant.com");
const Str DEFAULT_Port("443");
const Str DEFAULT_IsSSL(True);
const Str DEFAULT_DbUser("afteptsecumbehisomorther");
const Str DEFAULT_DbPswd("e4f286be1eef534f1cddd6240ed0133b968b1c9a");


const Str HiveConfig::HiveIdProperty("hive-id");
const Str HiveConfig::HiveFirmwareProperty("hive-version");
const Str HiveConfig::TimestampProperty("timestamp");
const Str HiveConfig::SsidProperty("ssid");
const Str HiveConfig::SsidPswdProperty("pswd");
const Str HiveConfig::DbHostProperty("db-host");
const Str HiveConfig::DbPortProperty("db-port");
const Str HiveConfig::IsSslProperty("is-ssl");
const Str HiveConfig::DbUserProperty("db-user");
const Str HiveConfig::DbPswdProperty("db-pswd");

void HiveConfig::setDefault()
{
    TF("HiveConfig::setDefault");
    mDoc.clear();
    mDoc.addNameValue(new CouchUtils::NameValuePair(HiveIdProperty, CouchUtils::Item(getHiveId())));
    mDoc.addNameValue(new CouchUtils::NameValuePair(HiveFirmwareProperty, CouchUtils::Item(mVersionId)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(TimestampProperty, CouchUtils::Item(DEFAULT_Timestamp)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(SsidProperty, CouchUtils::Item(DEFAULT_MyWifi)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(SsidPswdProperty, CouchUtils::Item(DEFAULT_Passcode)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbHostProperty, CouchUtils::Item(DEFAULT_DbHost)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbPortProperty, CouchUtils::Item(DEFAULT_Port)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(IsSslProperty, CouchUtils::Item(DEFAULT_IsSSL)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbUserProperty, CouchUtils::Item(DEFAULT_DbUser)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbPswdProperty, CouchUtils::Item(DEFAULT_DbPswd)));
}


void HiveConfig::print() const
{
    CouchUtils::printDoc(getDoc());
}


#ifndef HEADLESS
#define T(exp) if(!(exp)) {PH2("config is invalid due to: ", #exp );}
#else
#define T(exp) TRACE(exp)
#endif

bool HiveConfig::docIsValidForConfig(const CouchUtils::Doc &d)
{
    TF("HiveConfig::docIsValidForConfig");
    T(d.lookup(HiveIdProperty) >= 0);
    T(d.lookup(HiveFirmwareProperty) >= 0);
    T(d.lookup(TimestampProperty) >= 0);
    T(d.lookup(SsidProperty) >= 0);
    T(d.lookup(SsidPswdProperty) >= 0);
    T(d.lookup(DbHostProperty) >= 0);
    T(d.lookup(DbPortProperty) >= 0);
    T(d.lookup(DbUserProperty) >= 0);
    T(d.lookup(DbPswdProperty) >= 0);
    T(d.lookup(IsSslProperty) >= 0);
    T(d[d.lookup(HiveIdProperty)].getValue().isStr());
    T(d[d.lookup(HiveFirmwareProperty)].getValue().isStr());
    T(d[d.lookup(TimestampProperty)].getValue().isStr());
    T(d[d.lookup(SsidProperty)].getValue().isStr());
    T(d[d.lookup(SsidPswdProperty)].getValue().isStr());
    T(d[d.lookup(DbHostProperty)].getValue().isStr());
    T(d[d.lookup(DbUserProperty)].getValue().isStr());
    T(d[d.lookup(DbPswdProperty)].getValue().isStr());
    T(d[d.lookup(DbPortProperty)].getValue().isStr());
    T(StringUtils::isNumber(d[d.lookup(DbPortProperty)].getValue().getStr().c_str()));
    T(d[d.lookup(IsSslProperty)].getValue().isStr());
    T(d[d.lookup(IsSslProperty)].getValue().getStr().equals(True) ||
      d[d.lookup(IsSslProperty)].getValue().getStr().equals(False));
    
    return d.lookup(HiveIdProperty) >= 0 &&
      d.lookup(HiveFirmwareProperty) >= 0 && 
      d.lookup(TimestampProperty) >= 0 && 
      d.lookup(SsidProperty) >= 0 &&
      d.lookup(SsidPswdProperty) >= 0 &&
      d.lookup(DbHostProperty) >= 0 &&
      d.lookup(DbPortProperty) >= 0 &&
      d.lookup(DbUserProperty) >= 0 &&
      d.lookup(DbPswdProperty) >= 0 &&
      d.lookup(IsSslProperty) >= 0 &&
      d[d.lookup(HiveIdProperty)].getValue().isStr() &&
      d[d.lookup(HiveFirmwareProperty)].getValue().isStr() &&
      d[d.lookup(TimestampProperty)].getValue().isStr() &&
      d[d.lookup(SsidProperty)].getValue().isStr() &&
      !d[d.lookup(SsidProperty)].getValue().getStr().equals(DEFAULT_MyWifi) &&
      d[d.lookup(SsidPswdProperty)].getValue().isStr() &&
      !d[d.lookup(SsidPswdProperty)].getValue().getStr().equals(DEFAULT_Passcode) &&
      d[d.lookup(DbHostProperty)].getValue().isStr() &&
      d[d.lookup(DbPortProperty)].getValue().isStr() &&
      StringUtils::isNumber(d[d.lookup(DbPortProperty)].getValue().getStr().c_str()) &&
      d[d.lookup(DbUserProperty)].getValue().isStr() &&
      d[d.lookup(DbPswdProperty)].getValue().isStr() &&
      d[d.lookup(IsSslProperty)].getValue().isStr() &&
      (d[d.lookup(IsSslProperty)].getValue().getStr().equals(True) ||
       d[d.lookup(IsSslProperty)].getValue().getStr().equals(False));
}


bool HiveConfig::isValid() const
{
    TF("HiveConfig::isValid");
    T(mDoc[mDoc.lookup(HiveIdProperty)].getValue().getStr().equals(getHiveId()));

    return docIsValidForConfig(mDoc) && 
      mDoc[mDoc.lookup(HiveIdProperty)].getValue().getStr().equals(getHiveId());
}


const Str &HiveConfig::getSSID() const {return mDoc[mDoc.lookup(SsidProperty)].getValue().getStr();}
const Str &HiveConfig::getPSWD() const {return mDoc[mDoc.lookup(SsidPswdProperty)].getValue().getStr();}
const Str &HiveConfig::getDbHost() const {return mDoc[mDoc.lookup(DbHostProperty)].getValue().getStr();}
const Str &HiveConfig::getDbUser() const {return mDoc[mDoc.lookup(DbUserProperty)].getValue().getStr();}
const Str &HiveConfig::getDbPswd() const {return mDoc[mDoc.lookup(DbPswdProperty)].getValue().getStr();}
const int HiveConfig::getDbPort() const {return atoi(mDoc[mDoc.lookup(DbPortProperty)].getValue().getStr().c_str());}
const int HiveConfig::isSSL() const {return mDoc[mDoc.lookup(IsSslProperty)].getValue().getStr().equals(True);}

const Str &HiveConfig::getLogDbName() const {static Str n("hive-sensor-log"); return n;}      // couchdb name
const Str &HiveConfig::getConfigDbName() const {static Str n("hive-config"); return n;}      // couchdb name
const Str &HiveConfig::getChannelDbName() const {static Str c("hive-channel"); return c;}    // couchdb name
const Str &HiveConfig::getDesignDocId() const {static Str d("_design/SensorLog"); return d;} // couchdb designt
const Str &HiveConfig::getSensorByHiveViewName() const {static Str v("by-hive-sensor"); return v;}


const Str &HiveConfig::getHiveId() const
{
    static Str id(PlatformUtils::singleton().serialNumber());
    return id;
}


bool HiveConfig::addProperty(const Str &name, const char *value)
{
    TF("HiveConfig::addProperty");
    int index = mDoc.lookup(name);
    if (index >= 0) {
        if (mDoc[index].getValue().equals(value)) {
	    return false;
	}
	
	CouchUtils::Doc newDoc;
	for (int i = 0; i < mDoc.getSz(); i++) {
	    newDoc.addNameValue(i==index ?
				new CouchUtils::NameValuePair(name,Str(value)) :
				new CouchUtils::NameValuePair(mDoc[i]));
	}
	setDoc(newDoc);
    } else {
        CouchUtils::Doc newDoc = mDoc;
	newDoc.addNameValue(new CouchUtils::NameValuePair(name, Str(value)));
	setDoc(newDoc);
    }

    StrBuf dump;
    TRACE2("Just updated config: ", CouchUtils::toString(mDoc, &dump));
    
    return true;
}


bool HiveConfig::hasProperty(const Str &name) const
{
    int index = mDoc.lookup(name);
    return (index >= 0 && mDoc[index].getValue().isStr());
}


const Str &HiveConfig::getProperty(const Str &name) const
{
    int index = mDoc.lookup(name);
    if (index >= 0 && mDoc[index].getValue().isStr())
        return mDoc[index].getValue().getStr();

    return sEmpty;
}


HiveConfig::UpdateFunctor *HiveConfig::onUpdate(HiveConfig::UpdateFunctor *functor)
{
    UpdateFunctor *prev = mUpdateFunctor;
    mUpdateFunctor = functor;
    return prev;
}


