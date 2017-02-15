#include <hiveconfig.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>

#include <platformutils.h>

#include <couchutils.h>

HiveConfig::HiveConfig(const char *resetCause, const char *versionId)
  : mDoc(), mRCause(resetCause), mVersionId(versionId)
{
    TF("HiveConfig::HiveConfig");
    TRACE("entry");
    setDefault();
}


void HiveConfig::setDoc(const CouchUtils::Doc &doc)
{
    mDoc = doc;
}


void HiveConfig::setDefault()
{
    TF("HiveConfig::setDefault");
    TRACE("entry");
    mDoc.clear();
    mDoc.addNameValue(new CouchUtils::NameValuePair("hive-id", CouchUtils::Item(getHiveId())));
    mDoc.addNameValue(new CouchUtils::NameValuePair("timestamp", CouchUtils::Item("1400000000")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("ssid", CouchUtils::Item("JOE5")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("pswd", CouchUtils::Item("abcdef012345")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("db-host", CouchUtils::Item("jfcenterprises.cloudant.com")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("db-port", CouchUtils::Item("443")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("is-ssl", CouchUtils::Item("true")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("db-creds", CouchUtils::Item("YWZ0ZXB0c2VjdW1iZWhpc29tb3J0aGVyOmU0ZjI4NmJlMWVlZjUzNGYxY2RkZDYyNDBlZDAxMzNiOTY4YjFjOWE=")));
    TRACE("exit");
}


void HiveConfig::print() const
{
    CouchUtils::printDoc(getDoc());
}

inline bool isNumber(const char *s) {
  bool foundAtLeastOne = false;
  while (s && *s && (*s >= '0') && (*s <= '9')) {
      foundAtLeastOne = true;
      s++;
  }
  return foundAtLeastOne;
}


bool HiveConfig::isValid() const
{
    TF("HiveConfig::isValid");
    TRACE(mDoc.lookup("hive-id") >= 0);
    TRACE((strcmp(mDoc[mDoc.lookup("hive-id")].getValue().getStr().c_str(), getHiveId()) == 0));
    TRACE(mDoc.lookup("timestamp") >= 0);
    TRACE(mDoc.lookup("ssid") >= 0);
    TRACE(mDoc.lookup("pswd") >= 0);
    TRACE(mDoc.lookup("db-host") >= 0);
    TRACE(mDoc.lookup("db-port") >= 0);
    TRACE(mDoc.lookup("is-ssl") >= 0);
    TRACE(mDoc.lookup("db-creds") >= 0);
    TRACE(mDoc[mDoc.lookup("hive-id")].getValue().isStr());
    TRACE(mDoc[mDoc.lookup("timestamp")].getValue().isStr());
    TRACE(mDoc[mDoc.lookup("ssid")].getValue().isStr());
    TRACE(mDoc[mDoc.lookup("pswd")].getValue().isStr());
    TRACE(mDoc[mDoc.lookup("db-host")].getValue().isStr());
    TRACE(mDoc[mDoc.lookup("db-port")].getValue().isStr());
    TRACE(isNumber(mDoc[mDoc.lookup("db-port")].getValue().getStr().c_str()));
    TRACE(mDoc[mDoc.lookup("is-ssl")].getValue().isStr());
    TRACE((strcmp(mDoc[mDoc.lookup("is-ssl")].getValue().getStr().c_str(),"true") == 0) ||
	  (strcmp(mDoc[mDoc.lookup("is-ssl")].getValue().getStr().c_str(),"false") == 0));
    TRACE(mDoc[mDoc.lookup("db-creds")].getValue().isStr());
    
    return mDoc.lookup("hive-id") >= 0 &&
      (strcmp(mDoc[mDoc.lookup("hive-id")].getValue().getStr().c_str(), getHiveId()) == 0) &&
      mDoc.lookup("timestamp") >= 0 && 
      mDoc.lookup("ssid") >= 0 &&
      mDoc.lookup("pswd") >= 0 &&
      mDoc.lookup("db-host") >= 0 &&
      mDoc.lookup("db-port") >= 0 &&
      mDoc.lookup("is-ssl") >= 0 &&
      mDoc.lookup("db-creds") >= 0 &&
      mDoc[mDoc.lookup("hive-id")].getValue().isStr() &&
      mDoc[mDoc.lookup("timestamp")].getValue().isStr() &&
      mDoc[mDoc.lookup("ssid")].getValue().isStr() &&
      mDoc[mDoc.lookup("pswd")].getValue().isStr() &&
      mDoc[mDoc.lookup("db-host")].getValue().isStr() &&
      mDoc[mDoc.lookup("db-port")].getValue().isStr() &&
      isNumber(mDoc[mDoc.lookup("db-port")].getValue().getStr().c_str()) &&
      mDoc[mDoc.lookup("is-ssl")].getValue().isStr() &&
      ((strcmp(mDoc[mDoc.lookup("is-ssl")].getValue().getStr().c_str(),"true") == 0) ||
       (strcmp(mDoc[mDoc.lookup("is-ssl")].getValue().getStr().c_str(),"false") == 0)) &&
      mDoc[mDoc.lookup("db-creds")].getValue().isStr();
}


const char *HiveConfig::getSSID() const {return mDoc[mDoc.lookup("ssid")].getValue().getStr().c_str();}
const char *HiveConfig::getPSWD() const {return mDoc[mDoc.lookup("pswd")].getValue().getStr().c_str();}
const char *HiveConfig::getDbHost() const {return mDoc[mDoc.lookup("db-host")].getValue().getStr().c_str();}
const int HiveConfig::getDbPort() const {return atoi(mDoc[mDoc.lookup("db-port")].getValue().getStr().c_str());}
const int HiveConfig::isSSL() const {return strcmp(mDoc[mDoc.lookup("is-ssl")].getValue().getStr().c_str(),"true")==0;}
const char *HiveConfig::getDbCredentials() const 
  {return mDoc[mDoc.lookup("db-creds")].getValue().getStr().c_str();}

const char *HiveConfig::getLogDbName() const {return "hive-sensor-log";}     // couchdb name
const char *HiveConfig::getConfigDbName() const {return "hive-config";}      // couchdb name
const char *HiveConfig::getChannelDbName() const {return "hive-channel";}    // couchdb name
const char *HiveConfig::getDesignDocId() const {return "_design/SensorLog";}
const char *HiveConfig::getSensorByHiveViewName() const {return "by-hive-sensor";}

const char *HiveConfig::getHiveId() const
{
    return PlatformUtils::singleton().serialNumber();
}
