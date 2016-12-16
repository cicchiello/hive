#include <couchutils.t.h>

#define NDEBUG
#include <strutils.h>

#include <couchutils.h>

#include <Arduino.h>

static bool success = true;
static CouchUtils::Doc *doc = NULL;

#define TestAssert(t,msg) if (!t) {PH("ASSERT FAILURE: "); PL(msg); success = false;}

bool CouchUtilsTest::setup() {
    PF("CouchUtilsTest::setup; ");
    
    // Print a welcome message

    TestAssert(CouchUtils::Doc::s_instanceCnt == 0, "CouchUtils::Doc::s_instanceCnt == 0");
    TestAssert(CouchUtils::Doc::NameValuePair::s_instanceCnt == 0, "CouchUtils::Doc::NameValuePair::s_instanceCnt == 0");

    doc = new CouchUtils::Doc();
    doc->addNameValue(new CouchUtils::Doc::NameValuePair("_rev", "4-0352417bf4356dcc4a621084d57aaf46"));
    doc->addNameValue(new CouchUtils::Doc::NameValuePair("content2", "Hi CouchDB!"));
    doc->addNameValue(new CouchUtils::Doc::NameValuePair("content", "Hi feather, if you can read this, the test worked!"));

    TestAssert(CouchUtils::Doc::s_instanceCnt == 1, "CouchUtils::Doc::s_instanceCnt == 1");
    
    TestAssert(CouchUtils::Doc::NameValuePair::s_instanceCnt == 3, "CouchUtils::Doc::NameValuePair::s_instanceCnt == 3");

    Str buf;
    CouchUtils::toString(*doc, &buf);
    const char *expected = "{\"_rev\":\"4-0352417bf4356dcc4a621084d57aaf46\",\"content2\":\"Hi CouchDB!\",\"content\":\"Hi feather, if you can read this, the test worked!\"}";
 
    TestAssert(strcmp(buf.c_str(), expected) == 0, "strcmp(buf.c_str(), expected) == 0");

    delete doc;

    TestAssert(CouchUtils::Doc::s_instanceCnt == 0, "CouchUtils::Doc::s_instanceCnt == 0");
    TestAssert(CouchUtils::Doc::NameValuePair::s_instanceCnt == 0, "CouchUtils::Doc::NameValuePair::s_instanceCnt == 0");


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
    
    m_didIt = true;

    return success;
}


bool CouchUtilsTest::loop() {
    return success;
}


