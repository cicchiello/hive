#include <couchutils.t.h>

//#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <couchutils.h>

#include <str.h>

#include <Arduino.h>

static bool success = true;
static CouchUtils::Doc *sDoc = NULL;

#define TestAssert(t,msg) if (!t) {PH("ASSERT FAILURE: "); PL(msg); success = false;}

int StrBytesAtStart = 0;

bool CouchUtilsTest::setup() {
    TF("CouchUtilsTest::setup");
    
    StrBytesAtStart = Str::sBytesConsumed;
    
    // Print a welcome message

    TestAssert(CouchUtils::Doc::s_instanceCnt == 0, "CouchUtils::Doc::s_instanceCnt == 0");
    TestAssert(CouchUtils::NameValuePair::s_instanceCnt == 0, "CouchUtils::NameValuePair::s_instanceCnt == 0");

    sDoc = new CouchUtils::Doc();
    sDoc->addNameValue(new CouchUtils::NameValuePair("_rev", "4-0352417bf4356dcc4a621084d57aaf46"));
    sDoc->addNameValue(new CouchUtils::NameValuePair("content2", "Hi CouchDB!"));
    sDoc->addNameValue(new CouchUtils::NameValuePair("content", "Hi feather, if you can read this, the test worked!"));

    TestAssert(CouchUtils::Doc::s_instanceCnt == 1, "CouchUtils::Doc::s_instanceCnt == 1");
    
    TestAssert(CouchUtils::NameValuePair::s_instanceCnt == 3, "CouchUtils::NameValuePair::s_instanceCnt == 3");

    Str buf;
    CouchUtils::toString(*sDoc, &buf);
    const char *expected = "{\"_rev\":\"4-0352417bf4356dcc4a621084d57aaf46\",\"content2\":\"Hi CouchDB!\",\"content\":\"Hi feather, if you can read this, the test worked!\"}";
 
    TestAssert(strcmp(buf.c_str(), expected) == 0, "strcmp(buf.c_str(), expected) == 0");

    delete sDoc;
    
    TestAssert(CouchUtils::Doc::s_instanceCnt == 0, "CouchUtils::Doc::s_instanceCnt == 0");
    TestAssert(CouchUtils::NameValuePair::s_instanceCnt == 0, "CouchUtils::NameValuePair::s_instanceCnt == 0");


    const char *stringToParse = 
      "HTTP/1.1 201 Created\n"
      "Server: CouchDB/1.6.1 (Erlang OTP/17)\n"
      "Location: http://192.168.2.85/test-persistent-enum/feather-get-test\n"
      "ETag: \"19-08a91348b0b3841d33350166025cd9ff\"\n"
      "Date: Wed, 18 May 2016 21:34:20 GMT\n"
      "Content-Type: text/plain; charset=utf-8\n"
      "Content-Length: 80\n"
      "Cache-Control: must-revalidate\n"
      "\n"
      "{\"ok\":true,\"id\":\"feather-get-test\",\"rev\":\"19-08a91348b0b3841d33350166025cd9ff\"}\n"
      "\n";
    expected = "{\"ok\":\"true\",\"id\":\"feather-get-test\",\"rev\":\"19-08a91348b0b3841d33350166025cd9ff\"}";
    CouchUtils::Doc doc2;
    TestAssert(CouchUtils::parseDoc(stringToParse, &doc2) != NULL, "CouchUtils::parseDoc(stringToParse, &doc2) != NULL");

    Str buf2;
    CouchUtils::toString(doc2, &buf2);
    TestAssert(strcmp(expected, buf2.c_str()) == 0, "strcmp(expected, buf2.c_str()) == 0");

    const char *rawdoc3 = "{\"_id\":\"feather-get-test\",\"_rev\":\"7-477f7b016f691940f63da508c712fa37\",\"content2\":\"Hi CouchDB!\",\"content\":\"Hi feather, if you can read this, the test worked!\",\"_attachments\":{\"wav\":{\"content_type\":\"application/x-www-form-urlencoded\",\"revpos\":7,\"digest\":\"md5-2mAAFXEzk+JAIKoESbDF0Q==\",\"length\":133,\"stub\":true}}}";
    expected = "{\"_id\":\"feather-get-test\",\"_rev\":\"7-477f7b016f691940f63da508c712fa37\",\"content2\":\"Hi CouchDB!\",\"content\":\"Hi feather, if you can read this, the test worked!\",\"_attachments\":{\"wav\":{\"content_type\":\"application/x-www-form-urlencoded\",\"revpos\":\"7\",\"digest\":\"md5-2mAAFXEzk+JAIKoESbDF0Q==\",\"length\":\"133\",\"stub\":\"true\"}}}";

    Str buf3;
    CouchUtils::Doc doc3;
    CouchUtils::parseDoc(rawdoc3, &doc3);
    CouchUtils::toString(doc3, &buf3);
    TestAssert(strcmp(expected, buf3.c_str()) == 0, "strcmp(expected, buf3.c_str()) == 0");

    CouchUtils::Doc doc4(doc3);
    Str buf4;
    CouchUtils::toString(doc4, &buf4);
    TestAssert(strcmp(expected, buf4.c_str()) == 0, "strcmp(expected, buf4.c_str()) == 0");

    Str tstr("0123456789abcdef0123456789abcdef");
    TRACE2("tstr: ", tstr.c_str());
    CouchUtils::Doc tstrdoc;
    tstrdoc.addNameValue(new CouchUtils::NameValuePair("test", CouchUtils::Item(Str(tstr.c_str()))));
    CouchUtils::printDoc(tstrdoc);
    
    CouchUtils::Doc pt;
    pt.addNameValue(new CouchUtils::NameValuePair("total_rows", CouchUtils::Item(Str("20088"))));
    pt.addNameValue(new CouchUtils::NameValuePair("offset", CouchUtils::Item(Str("3102"))));
    CouchUtils::Doc doc;
    doc.addNameValue(new CouchUtils::NameValuePair("id", CouchUtils::Item(Str("b2616c9c249a6e23b6cb1288477bac2f"))));
    CouchUtils::Arr k1;
    k1.append(CouchUtils::Item(Str("F0-17-66-FC-5E-A1")));
    k1.append(CouchUtils::Item(Str("sample-rate")));
    k1.append(CouchUtils::Item(Str("1484857822")));
    doc.addNameValue(new CouchUtils::NameValuePair("key", CouchUtils::Item(k1)));
    CouchUtils::Arr k2;
    k2.append(CouchUtils::Item(Str("F0-17-66-FC-5E-A1")));
    k2.append(CouchUtils::Item(Str("sample-rate")));
    k2.append(CouchUtils::Item(Str("1484857822")));
    k2.append(CouchUtils::Item(Str("10")));
    doc.addNameValue(new CouchUtils::NameValuePair("value", CouchUtils::Item(k2)));
    CouchUtils::Arr rows;
    rows.append(CouchUtils::Item(doc));
    pt.addNameValue(new CouchUtils::NameValuePair("rows", CouchUtils::Item(rows)));
    CouchUtils::printArr(rows);
    
    CouchUtils::Doc k1doc;
    k1doc.addNameValue(new CouchUtils::NameValuePair("hi", CouchUtils::Item(k1)));
    CouchUtils::printDoc(k1doc);
    
    CouchUtils::printArr(k1);
    CouchUtils::printDoc(doc);
    CouchUtils::printDoc(pt);
    
		    
    const char *rawdoc5 = "{\"total_rows\":20088,\"offset\":3102,\"rows\":[\{\"id\":\"b2616c9c249a6e23b6cb1288477bac2f\",\"key\":[\"F0-17-66-FC-5E-A1\",\"sample-rate\",\"1484857822\"],\"value\":[\"F0-17-66-FC-5E-A1\",\"sample-rate\",\"1484857822\",\"10\"]}\]}";
    Str buf5;
    CouchUtils::Doc doc5;
    CouchUtils::parseDoc(rawdoc5, &doc5);

    CouchUtils::printDoc(doc5);
    
    bool hasRate = false;
    int rate = -1;
    int ir = doc5.lookup("rows");
    if ((ir>=0) && doc5[ir].getValue().isArr()) {
        const CouchUtils::Arr &rows = doc5[ir].getValue().getArr();
	if ((rows.getSz() == 1) && rows[0].isDoc()) {
	    const CouchUtils::Doc &record = rows[0].getDoc();
	    int ival = record.lookup("value");
	    if ((ival >= 0) && record[ival].getValue().isArr()) {
                const CouchUtils::Arr &val = record[ival].getValue().getArr();
		if ((val.getSz() == 4) && !val[3].isDoc() && !val[3].isArr()) {
                    const Str &rateStr = val[3].getStr();
		    rate = atoi(rateStr.c_str());
		    hasRate = true;
		}
	    }
	}
    }
    assert(hasRate, "hasRate");
    assert(rate == 10, "rate == 10");
		    

    const char *rawdoc6 = "{\"total_rows\":22851,\"offset\":22654,\"rows\":[|\n|]}|";
    Str s6 = rawdoc6;
    for (int i = 0; i < s6.len(); i++)
      if (s6.c_str()[i] == '|')
	s6.set(0x0d, i);
    TRACE2("s6.c_str(): ", s6.c_str());
    
    CouchUtils::Doc doc6;
    CouchUtils::parseDoc(s6.c_str(), &doc6);
    CouchUtils::printDoc(doc6);
    
    m_didIt = true;

    return success;
}


bool CouchUtilsTest::loop() {
    TF("CouchUtilsTest::loop");

    int bytesLeaked = Str::sBytesConsumed - StrBytesAtStart;
    TRACE2("bytesLeaked of those tracked by class Str: ", bytesLeaked);
    
    return success;
}


