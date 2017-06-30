#ifndef ARDUINO

#define NDEBUG
#include <CygwinTrace.h>

#include <strutils.h>

#include <jparse.h>
#include <couchdoc_consumer.h>

#include <string.h>
#include <stdlib.h>


static bool success = true;
static CouchUtils::Doc *sDoc = NULL;

#define TestAssert(t,msg) if (!(t)) {PH("ASSERT FAILURE: "); PL(msg); success = false;}

int StrBytesAtStart = 0;


void StreamString(const char *strdoc, int snippetSz, JParse *parser)
{
    TF("::StreamString");
    int len = strlen(strdoc);
    int i = 0;
    while ((i < len) && parser->streamIsValid()) {
      StrBuf snippet;
      int j = 0;
      while ((j < snippetSz) && (j+i < len)) {
	snippet.add(strdoc[j+i]);
	j++;
      }
      i += j;
      if (CouchDocConsumer::sVerbose) {PH2("snippet: ", snippet.c_str());}
      parser->streamParseDoc(snippet.c_str());
      TestAssert(parser->streamIsValid(), "invalid stream");
    }
}


#define SKIP_WORKING

bool test() {
    TF("::JParseTest");

#ifndef SKIP_WORKING    
    TestAssert(CouchUtils::Doc::s_instanceCnt == 0, "CouchUtils::Doc::s_instanceCnt == 0");
    TestAssert(CouchUtils::NameValuePair::s_instanceCnt == 0, "CouchUtils::NameValuePair::s_instanceCnt == 0");
    sDoc = new CouchUtils::Doc();
    sDoc->addNameValue(new CouchUtils::NameValuePair("_rev", "4-0352417bf4356dcc4a621084d57aaf46"));
    sDoc->addNameValue(new CouchUtils::NameValuePair("content2", "Hi CouchDB!"));
    sDoc->addNameValue(new CouchUtils::NameValuePair("content", "Hi feather, if you can read this, the test worked!"));

    TestAssert(CouchUtils::Doc::s_instanceCnt == 1, "CouchUtils::Doc::s_instanceCnt == 1");
    
    TestAssert(CouchUtils::NameValuePair::s_instanceCnt == 3, "CouchUtils::NameValuePair::s_instanceCnt == 3");

    StrBuf buf;
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
    TestAssert(JParse().parseDoc(stringToParse, &doc2) != 0, "JParse::nonConstSingleton().parseDoc(stringToParse, &doc2) != NULL");

    StrBuf buf2;
    CouchUtils::toString(doc2, &buf2);
    TestAssert(strcmp(expected, buf2.c_str()) == 0, "strcmp(expected, buf2.c_str()) == 0");

    const char *rawdoc3 = "{\"_id\":\"feather-get-test\",\"_rev\":\"7-477f7b016f691940f63da508c712fa37\",\"content2\":\"Hi CouchDB!\",\"content\":\"Hi feather, if you can read this, the test worked!\",\"_attachments\":{\"wav\":{\"content_type\":\"application/x-www-form-urlencoded\",\"revpos\":7,\"digest\":\"md5-2mAAFXEzk+JAIKoESbDF0Q==\",\"length\":133,\"stub\":true}}}";
    expected = "{\"_id\":\"feather-get-test\",\"_rev\":\"7-477f7b016f691940f63da508c712fa37\",\"content2\":\"Hi CouchDB!\",\"content\":\"Hi feather, if you can read this, the test worked!\",\"_attachments\":{\"wav\":{\"content_type\":\"application/x-www-form-urlencoded\",\"revpos\":\"7\",\"digest\":\"md5-2mAAFXEzk+JAIKoESbDF0Q==\",\"length\":\"133\",\"stub\":\"true\"}}}";

    StrBuf buf3;
    CouchUtils::Doc doc3;
    JParse().parseDoc(rawdoc3, &doc3);
    CouchUtils::toString(doc3, &buf3);
    TestAssert(strcmp(expected, buf3.c_str()) == 0, "strcmp(expected, buf3.c_str()) == 0");

    CouchUtils::Doc doc4(doc3);
    StrBuf buf4;
    CouchUtils::toString(doc4, &buf4);
    TestAssert(strcmp(expected, buf4.c_str()) == 0, "strcmp(expected, buf4.c_str()) == 0");

    StrBuf tstr("0123456789abcdef0123456789abcdef");
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
#endif
    
		    
    const char *rawdoc5 = "{\"total_rows\":20088,\"offset\":3102,\"rows\":[\{\"id\":\"b2616c9c249a6e23b6cb1288477bac2f\",\"key\":[\"F0-17-66-FC-5E-A1\",\"sample-rate\",\"1484857822\"],\"value\":[\"F0-17-66-FC-5E-A1\",\"sample-rate\",\"1484857822\",\"10\"]}\]}";
    StrBuf buf5;
    CouchUtils::Doc doc5;
    CouchUtils::parseDoc(rawdoc5, &doc5);

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
    TestAssert(hasRate, "hasRate");
    TestAssert(rate == 10, "rate == 10");

    const char *rawdoc6 = "{\"total_rows\":22851,\"offset\":22654,\"rows\":[|\n|]}|";
    StrBuf s6;
    StringUtils::replace(&s6, rawdoc6, "|", "\n");
    
    CouchUtils::Doc doc6;
    CouchUtils::parseDoc(s6.c_str(), &doc6);

    CouchUtils::Doc doc7;
    const char *rawdoc7 = "adfadf {\"ok\":\"true\"}";
    CouchUtils::parseDoc(rawdoc7, &doc7);
    TestAssert(doc7.lookup("ok")>=0, "ok lookup");
    TestAssert(doc7[doc7.lookup("ok")].getValue().isStr(), "ok's value isn't a string");
    TestAssert(doc7[doc7.lookup("ok")].getValue().getStr().equals("true"), "ok's value isn't true");

    CouchDocConsumer consumer7;
    JParse parser7(&consumer7);
    StreamString(rawdoc7, 5, &parser7);
    TestAssert(consumer7.getDoc().equals(doc7), "stream-parsed doc7 isn't equal to conventionally parsed");
    
    CouchUtils::Doc doc8;
    CouchDocConsumer consumer8;
    JParse parser8(&consumer8);
    const char *rawdoc8 = "adfadf {\"ok\":\"true\", \"stat\" : \"hi\" }";
    CouchUtils::parseDoc(rawdoc8, &doc8);
    StreamString(rawdoc8, 5, &parser8);
    TestAssert(consumer8.getDoc().lookup("ok")>=0, "ok lookup");
    TestAssert(consumer8.getDoc()[consumer8.getDoc().lookup("ok")].getValue().isStr(),
	       "ok's value isn't a string");
    TestAssert(consumer8.getDoc()[consumer8.getDoc().lookup("ok")].getValue().getStr().equals("true"),
	       "ok's value isn't true");
    TestAssert(consumer8.getDoc().lookup("stat")>=0,
	       "stat lookup");
    TestAssert(consumer8.getDoc()[consumer8.getDoc().lookup("stat")].getValue().isStr(),
	       "stat's value isn't a string");
    TestAssert(consumer8.getDoc()[consumer8.getDoc().lookup("stat")].getValue().getStr().equals("hi"),
	       "stat's value isn't hi");
    TestAssert(consumer8.getDoc().equals(doc8), "stream-parsed doc8 isn't equal to conventionally parsed");

    CouchDocConsumer consumer6;
    JParse parser6(&consumer6);
    StreamString(s6.c_str(), 5, &parser6);
    if (!consumer6.getDoc().equals(doc6)) {
      StrBuf dump1, dump2;
      CouchUtils::toString(consumer6.getDoc(), &dump1);
      CouchUtils::toString(doc6, &dump2);
      PH2("consumer6.getDoc(): ", dump1.c_str());
      PH2("doc6: ", dump2.c_str());
      PH("stream-parsed doc6 isn't equal to conventionally parsed");
    }
    TestAssert(consumer6.getDoc().equals(doc6), "stream-parsed doc6 isn't equal to conventionally parsed");

    for (int l = 1; l < strlen(rawdoc5); l++) {
      CouchDocConsumer consumer5;
      JParse parser5(&consumer5);
      StreamString(rawdoc5, l, &parser5);
      TestAssert(consumer5.getDoc().equals(doc5), "stream-parsed doc5 isn't equal to conventionally parsed");
    }
    

    const char *fs0 = "{";
    const char *fs1 = "\"ssid\" : \"JOE3\",";
    const char *fs2 = "\"db-host\" : \"hivewiz.cloudant.com\",";
    const char *fs3 = "\"db-user\" : \"gromespecorgingeoughtnev\",";
    const char *fs4 = "\"db-pswd\" : \"075b14312a355c8563a77bd05c91fe519873fdf4\",";
    const char *fs5 = "\"db-port\" : \"443\",";
    const char *fs6 = "\"is-ssl\" : \"true\",";
    const char *fs7 = "\"pswd\" : \"abcdef012345\",";
    const char *fs8 = "\"hive-id\" : \"d359371b544d4d51202020470b0b01ff\",";
    const char *fs9 = "\"hive-version\" : \"1.0.504\",";
    const char *fs10 = "\"timestamp\" : \"1493655579\",";
    const char *fs11 = "\"latch-trip-temp\" : \"37\",";
    const char *fs12 = "\"latch-iscw\" : \"false\",";
    const char *fs13 = "\"latch-ulticks\" : \"44\",";
    const char *fs14 = "\"sensor-rate-seconds\" : \"600\",";
    const char *fs15 = "\"steps-per_second\" : \"10\",";
    const char *fs16 = "\"app-channel-msgid\" : \"f15f2c053b0f4742eed857439e322080\"";
    const char *fs17 = "}";
    const char *lines[] = {fs0, fs1, fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10,
			   fs11, fs12, fs13, fs14, fs15, fs16, fs17, 0};
    CouchDocConsumer fileConsumer;
    JParse fileParser(&fileConsumer);
    for (int i = 0; lines[i]; i++)
      fileParser.streamParseDoc(lines[i]);
    TestAssert(fileParser.streamIsValid(), "invalid stream");
    //StrBuf dump;
    //CouchUtils::toString(fileConsumer.getDoc(), &dump);
    //PH2("file contents: ", dump.c_str());



    const char *unchunkedStr = "{\"results\":[{\"seq\":\"231-g1AAAAJIeJzLYWBg4MhgTmGQS0lKzi9KdUhJMtZLSszMyizVS87JL01JzCvRy0styQGqY0pkSJL___9_VgZzokAuUIDd1NQyyTgllaBuYi1IUgCSSfYgOxKliNbkANIUD9bET7SmBJCmeqhvSsG-MbEws0g1tqSab_JYgCRDA5ACWjOfBMdBNC6AaNwPciAf2IFJBmYWllQMbog9ByD23CfZgQ8gGoHBzpsFAITfv1M\",\"id\":\"d359371b544d4d51202020470b0b01ff-app\",\"changes\":[{\"rev\":\"102-d5ee710e72f2b23176ad33ce09dcf00b\"}]}],\"last_seq\":\"259-g1AAAAMzeJyt0cEOgjAMANBFTfRkNB68qfEDCBkMxkn-RNdtBAhCYuCsf6J_on-if4LbIPGIh13apGnz0rZACM3SsUAbAby6yFiA5wDL8qxxeFE1gpW1U8q6UH0jhmDbtm2ejtnirApTQiLwhByc_heAnYpw6I29MTCn0hPYnhFr49gba2MkPGE4DO0ZJ21ce6Mxhk8DtUhkzSgnKqKbSoq5a2dlHOn6hAKx7Dw656mduXHADWhk8fed8-qct3aWxqGJT0IiLDufzml_dwMsCMHDd8u_sgAIXg\",\"pending\":0}";
    CouchUtils::Doc unchunkedDoc;
    CouchUtils::parseDoc(unchunkedStr, &unchunkedDoc);
    StrBuf dumpUnchunked;
    CouchUtils::toString(unchunkedDoc, &dumpUnchunked);
    //PH2("unchunked doc, conventionally parsed: ", dumpUnchunked.c_str());
    //PH("");
    //PH("");
    
    const char *chunkedExample = "d&|{'results':[|&2a5&|{'seq':'231-g1AAAAJIeJzLYWBg4MhgTmGQS0lKzi9KdUhJMtZLSszMyizVS87JL01JzCvRy0styQGqY0pkSJL___9_VgZzokAuUIDd1NQyyTgllaBuYi1IUgCSSfYgOxKliNbkANIUD9bET7SmBJCmeqhvSsG-MbEws0g1tqSab_JYgCRDA5ACWjOfBMdBNC6AaNwPciAf2IFJBmYWllQMbog9ByD23CfZgQ8gGoHBzpsFAITfv1M','id':'d359371b544d4d51202020470b0b01ff-app','changes':[{'rev':'102-d5ee710e72f2b23176ad33ce09dcf00b'}]}|],|'last_seq':'259-g1AAAAMzeJyt0cEOgjAMANBFTfRkNB68qfEDCBkMxkn-RNdtBAhCYuCsf6J_on-if4LbIPGIh13apGnz0rZACM3SsUAbAby6yFiA5wDL8qxxeFE1gpW1U8q6UH0jhmDbtm2ejtnirApTQiLwhByc_heAnYpw6I29MTCn0hPYnhFr49gba2MkPGE4DO0ZJ21ce6Mxhk8DtUhkzSgnKqKbSoq5a2dlHOn6hAKx7Dw656mduXHADWhk8fed8-qct3aWxqGJT0IiLDufzml_dwMsCMHDd8u_sgAIXg','pending':0}|&|1&||&|0&|&|&|";
    
    StrBuf chunks(chunkedExample);
    StrBuf cleanedChunks0, cleanedChunks1, cleanedChunks2;
    StringUtils::replace(&cleanedChunks0, chunks.c_str(), "|", "\n");
    StringUtils::replace(&cleanedChunks1, cleanedChunks0.c_str(), "&", "\r");
    StringUtils::replace(&cleanedChunks2, cleanedChunks1.c_str(), "'", "\"");
    
    CouchDocConsumer chunkedConsumer;
    JParse chunkParser(&chunkedConsumer);
    chunkParser.setIsChunked(true);
    StreamString(cleanedChunks2.c_str(), 40, &chunkParser);

    TestAssert(chunkedConsumer.getDoc().equals(unchunkedDoc),
	       "stream-parsed chunked example isn't equal to conventionally parsed");
    
    return success;
}


int main()
{
  TF("::main");
  
  int initial = Str::sBytesConsumed;
  
  test();
 
  PH2("Str malloc report: ", Str::sBytesConsumed);

  TestAssert(Str::sBytesConsumed == initial, "Str::sBytesConsumed == 0");
  
  PH("That's all folks!");
}


#endif
