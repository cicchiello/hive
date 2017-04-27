#include <hiveconfig.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <platformutils.h>

#include <strutils.h>
#include <couchutils.h>

#include <strbuf.h>


/* STATIC */
HiveConfig::UpdateFunctor *HiveConfig::mUpdateFunctor = NULL;

static Str sEmpty;

HiveConfig::HiveConfig(const char *resetCause, const char *versionId)
  : mDoc(), mRCause(resetCause), mVersionId(versionId)
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
    TRACE("entry");
    mDoc.clear();
    mDoc.addNameValue(new CouchUtils::NameValuePair(HiveIdProperty, CouchUtils::Item(getHiveId())));
    mDoc.addNameValue(new CouchUtils::NameValuePair(HiveFirmwareProperty, CouchUtils::Item(mVersionId)));
    mDoc.addNameValue(new CouchUtils::NameValuePair(TimestampProperty, CouchUtils::Item("1400000000")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(SsidProperty, CouchUtils::Item("<MyWifi>")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(SsidPswdProperty, CouchUtils::Item("<MyPasscode>")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbHostProperty, CouchUtils::Item("hivewiz.cloudant.com")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbPortProperty, CouchUtils::Item("443")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(IsSslProperty, CouchUtils::Item("true")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbUserProperty, CouchUtils::Item("gromespecorgingeoughtnev")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbPswdProperty, CouchUtils::Item("075b14312a355c8563a77bd05c91fe519873fdf4")));
    
    TRACE("exit");
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
    T((strcmp(d[d.lookup(IsSslProperty)].getValue().getStr().c_str(),"true") == 0) ||
	  (strcmp(d[d.lookup(IsSslProperty)].getValue().getStr().c_str(),"false") == 0));
    
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
      !d[d.lookup(SsidProperty)].getValue().getStr().equals("<MyWifi>") &&
      d[d.lookup(SsidPswdProperty)].getValue().isStr() &&
      !d[d.lookup(SsidPswdProperty)].getValue().getStr().equals("<MyPasscode>") &&
      d[d.lookup(DbHostProperty)].getValue().isStr() &&
      d[d.lookup(DbPortProperty)].getValue().isStr() &&
      StringUtils::isNumber(d[d.lookup(DbPortProperty)].getValue().getStr().c_str()) &&
      d[d.lookup(DbUserProperty)].getValue().isStr() &&
      d[d.lookup(DbPswdProperty)].getValue().isStr() &&
      d[d.lookup(IsSslProperty)].getValue().isStr() &&
      ((strcmp(d[d.lookup(IsSslProperty)].getValue().getStr().c_str(),"true") == 0) ||
       (strcmp(d[d.lookup(IsSslProperty)].getValue().getStr().c_str(),"false") == 0));
}


bool HiveConfig::isValid() const
{
    TF("HiveConfig::isValid");
    T(mDoc[mDoc.lookup("hive-id")].getValue().getStr().equals(getHiveId()));

    return docIsValidForConfig(mDoc) && 
      mDoc[mDoc.lookup(HiveIdProperty)].getValue().getStr().equals(getHiveId());
}


//const char *HiveConfig::getSSID() const {return mDoc[mDoc.lookup(SsidProperty)].getValue().getStr().c_str();}
const Str &HiveConfig::getSSID() const {return mDoc[mDoc.lookup(SsidProperty)].getValue().getStr();}
//const char *HiveConfig::getPSWD() const {return mDoc[mDoc.lookup(SsidPswdProperty)].getValue().getStr().c_str();}
const Str &HiveConfig::getPSWD() const {return mDoc[mDoc.lookup(SsidPswdProperty)].getValue().getStr();}
//const char *HiveConfig::getDbHost() const {return mDoc[mDoc.lookup(DbHostProperty)].getValue().getStr().c_str();}
const Str &HiveConfig::getDbHost() const {return mDoc[mDoc.lookup(DbHostProperty)].getValue().getStr();}
//const char *HiveConfig::getDbUser() const {return mDoc[mDoc.lookup(DbUserProperty)].getValue().getStr().c_str();}
const Str &HiveConfig::getDbUser() const {return mDoc[mDoc.lookup(DbUserProperty)].getValue().getStr();}
//const char *HiveConfig::getDbPswd() const {return mDoc[mDoc.lookup(DbPswdProperty)].getValue().getStr().c_str();}
const Str &HiveConfig::getDbPswd() const {return mDoc[mDoc.lookup(DbPswdProperty)].getValue().getStr();}
const int HiveConfig::getDbPort() const {return atoi(mDoc[mDoc.lookup(DbPortProperty)].getValue().getStr().c_str());}
const int HiveConfig::isSSL() const {return strcmp(mDoc[mDoc.lookup(IsSslProperty)].getValue().getStr().c_str(),"true")==0;}

//const char *HiveConfig::getLogDbName() const {return "hive-sensor-log";}
const Str &HiveConfig::getLogDbName() const {static Str n("hive-sensor-log"); return n;}      // couchdb name
//const char *HiveConfig::getConfigDbName() const {return "hive-config";}
const Str &HiveConfig::getConfigDbName() const {static Str n("hive-config"); return n;}      // couchdb name
//const char *HiveConfig::getChannelDbName() const {return "hive-channel";}
const Str &HiveConfig::getChannelDbName() const {static Str c("hive-channel"); return c;}    // couchdb name
//const char *HiveConfig::getDesignDocId() const {return "_design/SensorLog";} // couchdb design document
const Str &HiveConfig::getDesignDocId() const {static Str d("_design/SensorLog"); return d;} // couchdb designt
//const char *HiveConfig::getSensorByHiveViewName() const {return "by-hive-sensor";}
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
        if (mDoc[index].getValue().equals(value))
	    return false;
	
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


