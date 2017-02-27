#include <couchutils.h>

#define NDEBUG
#include <strutils.h>

#include <Trace.h>

#include <str.h>

#include <Arduino.h>

#include <cstring>

#define NULL 0

/* STATIC */
int CouchUtils::Doc::s_instanceCnt = 0;
int CouchUtils::NameValuePair::s_instanceCnt = 0;


static void printNameValuePairImp(const CouchUtils::NameValuePair &nv, int indent = 0);
static void printDocImp(const CouchUtils::Doc &doc, int indent = 0);
static void printArrImp(const CouchUtils::Arr &arr, int indent = 0);



void CouchUtils::Doc::addNameValue(CouchUtils::NameValuePair *nv) {
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

bool CouchUtils::Doc::setValue(const char *name, const Item &value)
{
    int i = lookup(name);
    if (i >= 0) {
        return nvs[i]->setValue(value);
    }
    return false;
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
    TF("CouchUtils::Doc::Doc");
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


CouchUtils::Doc &CouchUtils::Doc::operator=(const CouchUtils::Doc &o)
{
    if (this == &o)
        return *this;

    clear();
    for (int i = 0; i < o.getSz(); i++) {
        addNameValue(new NameValuePair(o[i].getName(), o[i].getValue()));
    }
}


bool CouchUtils::Doc::equals(const CouchUtils::Doc &other) const
{
    if (this == &other)
        return true;

    if (getSz() != other.getSz())
        return false;
    
    for (int i = 0; i < getSz(); i++) {
        const NameValuePair &nv = (*this)[i];
	const NameValuePair &onv = other[i];
	if (!nv.getName().equals(onv.getName()))
	    return false;
	if (!nv.getValue().equals(onv.getValue()))
	    return false;
    }

    return true;
}


void CouchUtils::Doc::clear()
{
    for (int i = 0; i < numNVs; i++) {
        delete nvs[i];
	nvs[i] = NULL;
    }
    numNVs = 0;
}



CouchUtils::Item::Item(const CouchUtils::Item &o)
  : _str(o._str), _arr(o._arr ? new Arr(*o._arr) : 0), _doc(o._doc ? new Doc(*o._doc) : 0)
{
}


bool CouchUtils::NameValuePair::setValue(const Item &i)
{
    _item = i;
    return true;
}



bool CouchUtils::Item::equals(const CouchUtils::Item &other) const
{
    if (this == &other)
        return true;

    if (isDoc() && other.isDoc()) {
        return getDoc().equals(other.getDoc());
    } else if (isArr() && other.isArr()) {
        return getArr().equals(other.getArr());
    } else if (isStr() && other.isStr()) {
        return getStr().equals(other.getStr());
    } else
        return false;
}


const CouchUtils::Item &CouchUtils::Item::operator=(const CouchUtils::Item &o)
{
    if (&o == this)
        return *this;
    
    _str = o._str;
    
    if (_arr && o._arr)
        *_arr = *o._arr;
    else {
        delete _arr;
	_arr = o._arr ? new Arr(*o._arr) : 0;
    }

    if (_doc && o._doc)
        *_doc = *o._doc;
    else {
        delete _doc;
	_doc = o._doc ? new Doc(*o._doc) : 0;
    }
    
    return *this;
}



CouchUtils::Arr::Arr(const CouchUtils::Arr &o)
  : _arr(o._sz ? new Item[o._sz] : 0), _sz(o._sz)
{
    for (int i = 0; i < _sz; i++) {
        _arr[i] = o._arr[i];
    }
}


bool CouchUtils::Arr::equals(const CouchUtils::Arr &other) const
{
    if (this == &other)
        return true;

    if (getSz() == other.getSz()) {
        for (int i = 0; i < getSz(); i++) {
	    if (!(*this)[i].equals(other[i]))
	        return false;
	}
	return true;
    } else return false;
}


void CouchUtils::Arr::append(const CouchUtils::Item &item)
{
    Item *n = new Item[_sz+1];
    for (int i = 0; i < _sz; i++)
        n[i] = _arr[i];
    n[_sz++] = item;
    delete [] _arr;
    _arr = n;
}


const CouchUtils::Arr &CouchUtils::Arr::operator=(const CouchUtils::Arr &o)
{
    if (&o == this)
        return *this;

    delete [] _arr;
    _sz = o._sz;
    _arr = _sz ? new Item[_sz] : 0;

    for (int i = 0; i < _sz; i++) {
        _arr[i] = o._arr[i];
    }

    return *this;
}


/* STATIC */ 
const char *CouchUtils::parseArr(const char *rawtext, CouchUtils::Arr *arr) 
{
    TF("CouchUtils::parseArr");
    
    const char *marker = strstr(rawtext, "[");
    if (marker != NULL) {
        marker = StringUtils::eatWhitespace(marker+1);
	const char *firstClose = strstr(marker, "]");
	if (firstClose != NULL) {
	    while (true) {
	        marker = StringUtils::eatWhitespace(marker);

		const char *tmarker = StringUtils::eatPunctuation(marker, ']');
		if (tmarker != NULL) 
		    return tmarker;

		const char *nestedDocMarker = StringUtils::eatPunctuation(marker, '{');
		const char *nestedArrMarker = StringUtils::eatPunctuation(marker, '[');
		if ((nestedDocMarker == NULL) && (nestedArrMarker == NULL)) {
		    // primitive value 
		    TRACE("found a string");

		    Str value;
		    marker = StringUtils::unquote(marker, &value);
		    if (marker == NULL) {
		        // invalid Doc
		        return NULL;
		    }

		    TRACE2("parsed a primitive: ", value.c_str());
		    arr->append(Item(value));
		} else if (nestedDocMarker != NULL) {
		    // nested Doc
		    TRACE("found a document");
		    assert(nestedArrMarker == NULL, "nestedArrMarker == NULL");
		    
		    Doc nested; 
		    marker = parseDoc(marker, &nested);
		    if (marker == NULL) {
		        // invalid Doc
		        return NULL;
		    }

		    arr->append(Item(nested));
		} else {
		    // nested Array
		    TRACE("found an array");

		    Arr nested;
		    marker = parseArr(marker, &nested);
		    if (marker == NULL) {
		        // invalid Array
		        return NULL;
		    }

		    arr->append(Item(nested));
		}
		
		tmarker = StringUtils::eatPunctuation(marker, ',');
		if (tmarker == NULL) {
		    tmarker = StringUtils::eatPunctuation(marker, '}');
		    if (tmarker == NULL) {
		        return StringUtils::eatPunctuation(marker, ']');
		    } else {
		      return tmarker;
		    }
		}
		marker = tmarker;
	    } 
	} 
    } 
    return NULL;
}


/* STATIC */ 
const char *CouchUtils::parseDoc(const char *rawtext, CouchUtils::Doc *doc) 
{
    TF("CouchUtils::parseDoc");
    
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
		TRACE2("parsed a name: ", name.c_str());

		marker = StringUtils::eatPunctuation(marker, ':');
		marker = StringUtils::eatWhitespace(marker);

		const char *nestedDocMarker = StringUtils::eatPunctuation(marker, '{');
		const char *nestedArrMarker = StringUtils::eatPunctuation(marker, '[');
		if ((nestedDocMarker == NULL) && (nestedArrMarker == NULL)) {
		    // primitive value 
		    TRACE("found a string");

		    Str value;
		    marker = StringUtils::unquote(marker, &value);
		    if (marker == NULL) {
		        // invalid Doc
		        return NULL;
		    }

		    TRACE2("parsed a primitive value: ", value.c_str());
		    doc->addNameValue(new NameValuePair(name.c_str(), value.c_str()));
		} else if (nestedDocMarker != NULL) {
		    // nested Doc
		    TRACE("found a document");
		    assert(nestedArrMarker == NULL, "nestedArrMarker == NULL");
		    
		    Doc nested; 
		    marker = parseDoc(marker, &nested);
		    if (marker == NULL) {
		        // invalid Doc
		        return NULL;
		    }

		    doc->addNameValue(new NameValuePair(name.c_str(), nested));
		} else {
		    // nested Array
		    TRACE("found an array");

		    Arr nested;
		    marker = parseArr(marker, &nested);
		    if (marker == NULL) {
		        // invalid Array
		        return NULL;
		    }

		    doc->addNameValue(new NameValuePair(name.c_str(), nested));
		}
		
		tmarker = StringUtils::eatPunctuation(marker, ',');
		if (tmarker == NULL) {
		    tmarker = StringUtils::eatPunctuation(marker, '}');
		    if (tmarker == NULL) {
		        return StringUtils::eatPunctuation(marker, ']');
		    } else {
		      return tmarker;
		    }
		}
		marker = tmarker;
	    } 
	} 
    } 
    return NULL;
}


/* STATIC */
void printNameValuePairImp(const CouchUtils::NameValuePair &nv, int indent)
{
    TF("::printNameValuePairImp");
    
    P("\"");
    P(nv.getName().c_str());
    P("\" : ");
    if (nv.getValue().isDoc()) {
        PL("{");
	printDocImp(nv.getValue().getDoc(), indent+4);
	for (int j = 0; j < indent; j++)
	    P(' ');
	P("}");
    } else if (nv.getValue().isArr()) {
	P("[");
	printArrImp(nv.getValue().getArr(), indent+4);
	P("]");
    } else {
        assert(nv.getValue().isStr(), "nv.isStr()");
        P("\"");
	P(nv.getValue().getStr().c_str());
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
void printArrImp(const CouchUtils::Arr &arr, int indent)
{
    TF("::printArrImp");
    int sz = arr.getSz();
    for (int i = 0; i < sz; i++) {
        if (arr[i].isDoc()) {
	    PL("{");
	    printDocImp(arr[i].getDoc(), indent);
	    PL("}");
	} else if (arr[i].isArr())
	    printArrImp(arr[i].getArr(), indent);
	else {
            P("\"");
	    P(arr[i].getStr().c_str());
	    P("\"");
	}
	if (i+1 < sz) 
	    P(",");
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
void CouchUtils::printArr(const CouchUtils::Arr &arr) 
{
    P("[");
    printArrImp(arr, 4);
    PL("]");
}

/* STATIC */
const char *CouchUtils::urlEncode(const char* msg, Str *buf)
{
    // according to https://tools.ietf.org/html/rfc3986, the following are the legal characters within a url
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;="
    // (however, any that may be context specific are also escaped here)
    static const char *hex = "0123456789abcdef";

    int i = 0;
    while (*msg!='\0'){
        if( ('a' <= *msg && *msg <= 'z') ||
	    ('A' <= *msg && *msg <= 'Z') ||
	    ('0' <= *msg && *msg <= '9') ||
	    ('/' == *msg) || ('&' == *msg) ||
	    ('=' == *msg) || ('?' == *msg) || (',' == *msg) ||
	    ('_' == *msg) || ('.' == *msg) || ('-' == *msg)) {
	    buf->add(*msg);
        } else {
	    buf->add('%');
	    buf->add(hex[*msg >> 4]);
	    buf->add(hex[*msg & 0x0f]);
        }
        msg++;
    }
    buf->add(0);
    return buf->c_str();
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
	if (doc[i].getValue().isDoc()) {
	    Str nested;
	    toString(doc[i].getValue().getDoc(), &nested);
	    requiredLen += nested.len()+4;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(strcat(strcat(strcat(strcat(nonConstBuf, "\""), name.c_str()), "\":"), nested.c_str()), "");
	    if (i+1 < doc.getSz()) 
	        strcat(nonConstBuf, ",");
	} else if (doc[i].getValue().isArr()) {
	    Str nested;
	    toString(doc[i].getValue().getArr(), &nested);
	    requiredLen += nested.len()+4;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(strcat(strcat(strcat(strcat(nonConstBuf, "\""), name.c_str()), "\":"), nested.c_str()), "");
	    if (i+1 < doc.getSz()) 
	        strcat(nonConstBuf, ",");
	} else {
	    const Str &value = doc[i].getValue().getStr();
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


const char *CouchUtils::toString(const CouchUtils::Arr &arr, Str *buf)
{
    buf->expand(2);

    buf->add('[');
    buf->set(0, 1);
    for (int i = 0; i < arr.getSz(); i++) {
        const Item &item = arr[i];
	int requiredLen = buf->len()+1;
	if (item.isDoc()) {
	    Str nested;
	    toString(item.getDoc(), &nested);
	    requiredLen += nested.len()+2;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(nonConstBuf, nested.c_str());
	    if (i+1 < arr.getSz()) 
	        strcat(nonConstBuf, ",");
	} else if (item.isArr()) {
	    Str nested;
	    toString(item.getArr(), &nested);
	    requiredLen += nested.len()+2;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(nonConstBuf, nested.c_str());
	    if (i+1 < arr.getSz()) 
	        strcat(nonConstBuf, ",");
	} else {
	    const Str &value = item.getStr();
	    requiredLen += value.len()+2;
	    buf->expand(requiredLen);
	    char *nonConstBuf = (char*) buf->c_str();
	    strcat(nonConstBuf, value.c_str());
	    if (i+1 < arr.getSz()) 
	        strcat(nonConstBuf, ",");
	}
    }
    char *nonConstBuf = (char*) buf->c_str();
    return strcat(nonConstBuf, "]");
}
