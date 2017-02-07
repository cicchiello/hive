#include <hiveconfig.h>

#include <Arduino.h>

#define HEADLESS
#define NDEBUG

#include <Trace.h>


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


#ifdef foo
HiveConfig::HiveConfig(const CouchUtils::Doc &d)
  : mDoc(d), mRCause(new Str(*d.mRCause)), mVersionId(new Str(*d.mVersionId))
{
    TF("HiveConfig::HiveConfig(const CouchUtils::Doc&)");
    //TRACE("doc: ");
    //CouchUtils::printDoc(d);
    //CouchUtils::printDoc(mDoc);
}

HiveConfig::HiveConfig(const HiveConfig &o)
  : mDoc(o.getDoc())
{
}
#endif


void HiveConfig::setDefault()
{
    TF("HiveConfig::setDefault");
    TRACE("entry");
    mDoc.clear();
    mDoc.addNameValue(new CouchUtils::NameValuePair("SSID", CouchUtils::Item("JOE5")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("PSWD", CouchUtils::Item("abcdef012345")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("DBHOST", CouchUtils::Item("jfcenterprises.cloudant.com")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("DBPORT", CouchUtils::Item("443")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("ISSSL", CouchUtils::Item("true")));
    mDoc.addNameValue(new CouchUtils::NameValuePair("DBCREDS", CouchUtils::Item("YWZ0ZXB0c2VjdW1iZWhpc29tb3J0aGVyOmU0ZjI4NmJlMWVlZjUzNGYxY2RkZDYyNDBlZDAxMzNiOTY4YjFjOWE=")));
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
    TRACE(mDoc.lookup("SSID") >= 0);
    TRACE(strcmp(mDoc[mDoc.lookup("SSID")].getValue().getStr().c_str(), "JOE5") == 0);
    TRACE(mDoc.lookup("PSWD") >= 0);
    TRACE(strcmp(mDoc[mDoc.lookup("PSWD")].getValue().getStr().c_str(), "abcdef012345") == 0);
    TRACE(mDoc.lookup("DBHOST") >= 0);
    TRACE(mDoc.lookup("DBPORT") >= 0);
    TRACE(mDoc.lookup("ISSSL") >= 0);
    TRACE(mDoc.lookup("DBCREDS") >= 0);
    TRACE(!mDoc[mDoc.lookup("SSID")].isDoc());
    TRACE(!mDoc[mDoc.lookup("PSWD")].isDoc());
    TRACE(!mDoc[mDoc.lookup("DBHOST")].isDoc());
    TRACE(!mDoc[mDoc.lookup("DBPORT")].isDoc());
    TRACE(!mDoc[mDoc.lookup("ISSSL")].isDoc());
    TRACE(!mDoc[mDoc.lookup("DBCREDS")].isDoc());
    TRACE((strcmp(mDoc[mDoc.lookup("ISSSL")].getValue().getStr().c_str(),"true") == 0) ||
	  (strcmp(mDoc[mDoc.lookup("ISSSL")].getValue().getStr().c_str(),"false") == 0));
    TRACE(isNumber(mDoc[mDoc.lookup("DBPORT")].getValue().getStr().c_str()));
    
    return mDoc.lookup("SSID") >= 0 &&
      (strcmp(mDoc[mDoc.lookup("SSID")].getValue().getStr().c_str(), "JOE5") == 0) && 
      mDoc.lookup("PSWD") >= 0 &&
      (strcmp(mDoc[mDoc.lookup("PSWD")].getValue().getStr().c_str(), "abcdef012345") == 0) && 
      mDoc.lookup("DBHOST") >= 0 &&
      mDoc.lookup("DBPORT") >= 0 &&
      mDoc.lookup("ISSSL") >= 0 &&
      mDoc.lookup("DBCREDS") >= 0 &&
      !mDoc[mDoc.lookup("SSID")].getValue().isDoc() &&
      !mDoc[mDoc.lookup("PSWD")].getValue().isDoc() &&
      !mDoc[mDoc.lookup("DBHOST")].getValue().isDoc() &&
      !mDoc[mDoc.lookup("DBPORT")].getValue().isDoc() &&
      !mDoc[mDoc.lookup("ISSSL")].getValue().isDoc() &&
      !mDoc[mDoc.lookup("DBCREDS")].getValue().isDoc() &&
      ((strcmp(mDoc[mDoc.lookup("ISSSL")].getValue().getStr().c_str(),"true") == 0) ||
       (strcmp(mDoc[mDoc.lookup("ISSSL")].getValue().getStr().c_str(),"false") == 0)) &&
      isNumber(mDoc[mDoc.lookup("DBPORT")].getValue().getStr().c_str());
}


const char *HiveConfig::getSSID() const {return mDoc[mDoc.lookup("SSID")].getValue().getStr().c_str();}
const char *HiveConfig::getPSWD() const {return mDoc[mDoc.lookup("PSWD")].getValue().getStr().c_str();}
const char *HiveConfig::getDbHost() const {return mDoc[mDoc.lookup("DBHOST")].getValue().getStr().c_str();}
const int HiveConfig::getDbPort() const {return atoi(mDoc[mDoc.lookup("DBPORT")].getValue().getStr().c_str());}
const int HiveConfig::isSSL() const {return strcmp(mDoc[mDoc.lookup("ISSSL")].getValue().getStr().c_str(),"true")==0;}
const char *HiveConfig::getDbCredentials() const 
  {return mDoc[mDoc.lookup("DBCREDS")].getValue().getStr().c_str();}

const char *HiveConfig::getLogDbName() const {return "hive-sensor-log";}     // couchdb name
const char *HiveConfig::getDesignDocId() const {return "_design/SensorLog";}
const char *HiveConfig::getSensorByHiveViewName() const {return "by-hive-sensor";}

