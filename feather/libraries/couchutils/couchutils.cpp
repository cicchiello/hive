#include <couchutils.h>

//#define NDEBUG
#include <strutils.h>

#include <str.h>

#include <Arduino.h>

#include <cstring>

#define NULL 0

/* STATIC */
int CouchUtils::Doc::s_instanceCnt = 0;
int CouchUtils::Doc::NameValuePair::s_instanceCnt = 0;


static void printNameValuePairImp(const CouchUtils::Doc::NameValuePair &nv, int indent = 0);
static void printDocImp(const CouchUtils::Doc &doc, int indent = 0);



void CouchUtils::Doc::addNameValue(CouchUtils::Doc::NameValuePair *nv) {
    if (numNVs == arrSz) {
        NameValuePair **old = nvs;
	nvs = new NameValuePair*[2*arrSz];
	for (int i = 0; i < arrSz; i++)
	    nvs[i] = old[i];
	for (int i = arrSz; i < 2*arrSz; i++)
	    nvs[i] = 0;
	delete [] old;
	arrSz *= 2;
    }
    nvs[numNVs++] = nv;
}

int CouchUtils::Doc::lookup(const char *name) const 
{
    for (int i = 0; i < numNVs; i++)
        if (strcmp(nvs[i]->getName().c_str(), name) == 0)
	    return i;
    return -1;
}

CouchUtils::Doc::Doc(const CouchUtils::Doc &d)
  : nvs(new NameValuePair*[d.arrSz]), numNVs(d.numNVs), arrSz(d.arrSz)
{
    s_instanceCnt++;
    assert(numNVs <= arrSz, "numNVs <= arrSz");
    for (int i = 0; i < arrSz; i++)
        nvs[i] = NULL;
    for (int i = 0; i < numNVs; i++)
        nvs[i] = new NameValuePair(d[i]);
}

CouchUtils::Doc::~Doc() 
{
    for (int i = 0; i < numNVs; i++) 
        delete nvs[i];
    delete [] nvs;

    s_instanceCnt--;
}


void CouchUtils::Doc::clear()
{
    for (int i = 0; i < numNVs; i++) {
        delete nvs[i];
	nvs[i] = NULL;
    }
    numNVs = 0;
}


CouchUtils::Doc::NameValuePair::NameValuePair(const Str &name, const Str &value)
  : _name(name), _value(value)
{
    _doc = NULL;

    s_instanceCnt++;
}

CouchUtils::Doc::NameValuePair::NameValuePair(const char *name, const char *value)
  : _name(name), _value(value)
{
    _doc = NULL;

    s_instanceCnt++;
}

CouchUtils::Doc::NameValuePair::NameValuePair(const char *name, const CouchUtils::Doc *doc)
  : _name(name)
{
    _doc = doc;

    s_instanceCnt++;
}

CouchUtils::Doc::NameValuePair::NameValuePair(const Str &name, const CouchUtils::Doc *doc) 
  : _name(name)
{
    _doc = doc;

    s_instanceCnt++;
}

CouchUtils::Doc::NameValuePair::NameValuePair(const CouchUtils::Doc::NameValuePair &p)
  : _name(p._name), _doc(p.isDoc() ? new Doc(*p.getDoc()) : NULL), _value(p._value)
{
    assert((_doc == NULL) == (p._doc == NULL), "Doc not copied properly");
}

CouchUtils::Doc::NameValuePair::~NameValuePair()
{
    delete _doc;

    s_instanceCnt--;
}


/* STATIC */ 
const char *CouchUtils::parseDoc(const char *rawtext, CouchUtils::Doc *doc) 
{
    const char *marker = strstr(rawtext, "{");
    if (marker != NULL) {
        marker = StringUtils::eatWhitespace(marker+1);
	const char *firstClose = strstr(marker, "}");
	if (firstClose != NULL) {
	    while (true) {
	        marker = StringUtils::eatWhitespace(marker);

		const char *tmarker = StringUtils::eatPunctuation(marker, '}');
		if (tmarker != NULL) 
		    return tmarker;

		Str name;
		marker = StringUtils::unquote(marker, &name);
		if (marker == NULL) {
		    // error -- DONE
		    return NULL;
		}

		marker = StringUtils::eatPunctuation(marker, ':');
		marker = StringUtils::eatWhitespace(marker);

		tmarker = StringUtils::eatPunctuation(marker, '{');
		if (tmarker == NULL) {
		    // primitive value 
		    Str value;
		    marker = StringUtils::unquote(marker, &value);
		    if (marker == NULL) {
		        // invalid Doc
		        return NULL;
		    }

		    doc->addNameValue(new Doc::NameValuePair(name.c_str(), value.c_str()));
		} else {
		    // nested Doc
		    Doc *nested = new Doc();
		    marker = parseDoc(marker, nested);
		    if (marker == NULL) {
		        // invalid Doc
		        return NULL;
		    }

		    doc->addNameValue(new Doc::NameValuePair(name.c_str(), nested));
		}

		tmarker = StringUtils::eatPunctuation(marker, ',');
		if (tmarker == NULL) 
		    return StringUtils::eatPunctuation(marker, '}');
		marker = tmarker;
	    } 
	} 
    } 
    return NULL;
}


/* STATIC */
void printNameValuePairImp(const CouchUtils::Doc::NameValuePair &nv, int indent)
{
//PL("trace 1");
    P("\"");
//PL("trace 2");
    P(nv.getName().c_str());
//PL("trace 3");
    P("\" : ");
//PL("trace 4");
    if (nv.isDoc()) {
//PL("trace 5");
        PL("{");
	printDocImp(*nv.getDoc(), indent+4);
	for (int j = 0; j < indent; j++)
	    P(' ');
	P("}");
    } else {
//PL("trace 6");
        P("\"");
	P(nv.getValue().c_str());
	P("\"");
    }
}


/* STATIC */
void printDocImp(const CouchUtils::Doc &doc, int indent)
{
    int sz = doc.getSz();
    for (int i = 0; i < sz; i++) {
        for (int j = 0; j < indent; j++)
	    P(' ');
	printNameValuePairImp(doc[i], indent);
	if (i+1 < sz) 
	    PL(",");
	else
	    PL();
    }
}


/* STATIC */
void CouchUtils::printDoc(const CouchUtils::Doc &doc) 
{
    PL("{");
    printDocImp(doc, 4);
    PL("}");
}


/* STATIC */
const char *CouchUtils::toURL(const char *db, const char *docid, Str *page)
{
    int len = strlen(db)+1+strlen(docid)+1+1;
    page->expand(len);
    char *nonConstBuf = (char*) page->c_str();
    return strcat(strcat(strcat(strcpy(nonConstBuf, "/"), db), "/"), docid);
}

/* STATIC */
const char *CouchUtils::toAttachmentPutURL(const char *db, const char *docid, 
					   const char *attachName, const char *revision, Str *page)
{
    toURL(db, docid, page);
    int len = page->len()+1+strlen(attachName)+1+4+strlen(revision)+1;
    page->expand(len);
    char *nonConstBuf = (char*) page->c_str();
    return strcat(strcat(strcat(strcat(nonConstBuf, "/"), attachName), "?rev="), revision);
}


/* STATIC */
const char *CouchUtils::toAttachmentGetURL(const char *db, const char *docid, 
					   const char *attachName, Str *page)
{
    toURL(db, docid, page);
    int len = page->len()+1+strlen(attachName)+1+4;
    page->expand(len);
    char *nonConstBuf = (char*) page->c_str();
    return strcat(strcat(nonConstBuf, "/"), attachName);
}



const char *CouchUtils::toString(const CouchUtils::Doc &doc, Str *buf)
{
    buf->expand(2);

    buf->add('{');
    buf->set(0, 1);
    for (int i = 0; i < doc.getSz(); i++) {
        const Str &name = doc[i].getName();
	int requiredLen = buf->len()+5+name.len();
	if (doc[i].isDoc()) {
	    Str nested;
	    toString(*doc[i].getDoc(), &nested);
	    requiredLen += nested.len()+4;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(strcat(strcat(strcat(strcat(nonConstBuf, "\""), name.c_str()), "\":"), nested.c_str()), "");
	    if (i+1 < doc.getSz()) 
	        strcat(nonConstBuf, ",");
	} else {
	    const Str &value = doc[i].getValue();
	    requiredLen += value.len()+4;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(strcat(strcat(strcat(strcat(nonConstBuf, "\""), name.c_str()), "\":\""), value.c_str()), "\"");
	    if (i+1 < doc.getSz()) 
	        strcat(nonConstBuf, ",");
	}
    }
    char *nonConstBuf = (char*) buf->c_str();
    return strcat(nonConstBuf, "}");
}
