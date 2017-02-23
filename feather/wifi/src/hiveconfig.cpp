#include <hiveconfig.h>

#include <Arduino.h>

#define NDEBUG

#include <Trace.h>

#include <platformutils.h>

#include <strutils.h>
#include <couchutils.h>


/* STATIC */
HiveConfig::UpdateFunctor *HiveConfig::mUpdateFunctor = NULL;


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
    mDoc = doc;

    if (mUpdateFunctor != NULL) {
        mUpdateFunctor->onUpdate(*this);
    }
    
    return isValid();
}


const char *HiveConfig::HiveIdProperty = "hive-id";
const char *HiveConfig::HiveFirmwareProperty = "hive-version";
const char *HiveConfig::TimestampProperty = "timestamp";
const char *HiveConfig::SsidProperty = "ssid";
const char *HiveConfig::SsidPswdProperty = "pswd";
const char *HiveConfig::DbHostProperty = "db-host";
const char *HiveConfig::DbPortProperty = "db-port";
const char *HiveConfig::IsSslProperty = "is-ssl";
const char *HiveConfig::DbUserProperty = "db-user";
const char *HiveConfig::DbPswdProperty = "db-pswd";


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
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbHostProperty, CouchUtils::Item("jfcenterprises.cloudant.com")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbPortProperty, CouchUtils::Item("443")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(IsSslProperty, CouchUtils::Item("true")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbUserProperty, CouchUtils::Item("afteptsecumbehisomorther")));
    mDoc.addNameValue(new CouchUtils::NameValuePair(DbPswdProperty, CouchUtils::Item("e4f286be1eef534f1cddd6240ed0133b968b1c9a")));
    TRACE("exit");
}


void HiveConfig::print() const
{
    CouchUtils::printDoc(getDoc());
}


bool HiveConfig::docIsValidForConfig(const CouchUtils::Doc &d)
{
    TF("HiveConfig::docIsValidForConfig");
    TRACE(d.lookup(HiveIdProperty) >= 0);
    TRACE(d.lookup(HiveFirmwareProperty) >= 0);
    TRACE(d.lookup(TimestampProperty) >= 0);
    TRACE(d.lookup(SsidProperty) >= 0);
    TRACE(d.lookup(SsidPswdProperty) >= 0);
    TRACE(d.lookup(DbHostProperty) >= 0);
    TRACE(d.lookup(DbPortProperty) >= 0);
    TRACE(d.lookup(DbUserProperty) >= 0);
    TRACE(d.lookup(DbPswdProperty) >= 0);
    TRACE(d.lookup(IsSslProperty) >= 0);
    TRACE(d[d.lookup(HiveIdProperty)].getValue().isStr());
    TRACE(d[d.lookup(HiveFirmwareProperty)].getValue().isStr());
    TRACE(d[d.lookup(TimestampProperty)].getValue().isStr());
    TRACE(d[d.lookup(SsidProperty)].getValue().isStr());
    TRACE(d[d.lookup(SsidPswdProperty)].getValue().isStr());
    TRACE(d[d.lookup(DbHostProperty)].getValue().isStr());
    TRACE(d[d.lookup(DbUserProperty)].getValue().isStr());
    TRACE(d[d.lookup(DbPswdProperty)].getValue().isStr());
    TRACE(d[d.lookup(DbPortProperty)].getValue().isStr());
    TRACE(StringUtils::isNumber(d[d.lookup(DbPortProperty)].getValue().getStr().c_str()));
    TRACE(d[d.lookup(IsSslProperty)].getValue().isStr());
    TRACE((strcmp(d[d.lookup(IsSslProperty)].getValue().getStr().c_str(),"true") == 0) ||
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
    TRACE(mDoc[mDoc.lookup("hive-id")].getValue().getStr().equals(getHiveId()));

    return docIsValidForConfig(mDoc) && 
      mDoc[mDoc.lookup(HiveIdProperty)].getValue().getStr().equals(getHiveId());
}


const char *HiveConfig::getSSID() const {return mDoc[mDoc.lookup(SsidProperty)].getValue().getStr().c_str();}
const char *HiveConfig::getPSWD() const {return mDoc[mDoc.lookup(SsidPswdProperty)].getValue().getStr().c_str();}
const char *HiveConfig::getDbHost() const {return mDoc[mDoc.lookup(DbHostProperty)].getValue().getStr().c_str();}
const char *HiveConfig::getDbUser() const {return mDoc[mDoc.lookup(DbUserProperty)].getValue().getStr().c_str();}
const char *HiveConfig::getDbPswd() const {return mDoc[mDoc.lookup(DbPswdProperty)].getValue().getStr().c_str();}
const int HiveConfig::getDbPort() const {return atoi(mDoc[mDoc.lookup(DbPortProperty)].getValue().getStr().c_str());}
const int HiveConfig::isSSL() const {return strcmp(mDoc[mDoc.lookup(IsSslProperty)].getValue().getStr().c_str(),"true")==0;}

const char *HiveConfig::getLogDbName() const {return "hive-sensor-log";}     // couchdb name
const char *HiveConfig::getConfigDbName() const {return "hive-config";}      // couchdb name
const char *HiveConfig::getChannelDbName() const {return "hive-channel";}    // couchdb name
const char *HiveConfig::getDesignDocId() const {return "_design/SensorLog";} // couchdb design document
const char *HiveConfig::getSensorByHiveViewName() const {return "by-hive-sensor";}

const char *HiveConfig::getHiveId() const
{
    return PlatformUtils::singleton().serialNumber();
}


bool HiveConfig::addProperty(const char *name, const char *value)
{
    TF("HiveConfig::addProperty");
    int index = mDoc.lookup(name);
    if (index >= 0) {
        if (mDoc[index].getValue().equals(value))
	    return false;
	
	CouchUtils::Doc newDoc;
	for (int i = 0; i < mDoc.getSz(); i++) {
	    newDoc.addNameValue(i==index ?
				new CouchUtils::NameValuePair(name,value) :
				new CouchUtils::NameValuePair(mDoc[i]));
	}
	setDoc(newDoc);
    } else {
        CouchUtils::Doc newDoc = mDoc;
	newDoc.addNameValue(new CouchUtils::NameValuePair(name, value));
	setDoc(newDoc);
    }

    Str dump;
    TRACE2("Just updated config: ", CouchUtils::toString(mDoc, &dump));
    
    return true;
}


const char *HiveConfig::getProperty(const char *name) const
{
    int index = mDoc.lookup(name);
    if (index >= 0 && mDoc[index].getValue().isStr())
        return mDoc[index].getValue().getStr().c_str();

    return NULL;
}


HiveConfig::UpdateFunctor *HiveConfig::onUpdate(HiveConfig::UpdateFunctor *functor)
{
    UpdateFunctor *prev = mUpdateFunctor;
    mUpdateFunctor = functor;
    return prev;
}


