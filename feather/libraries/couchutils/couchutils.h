#ifndef couch_utils_h
#define couch_utils_h

#include <str.h>

class IPAddress;
class StrBuf;

class CouchUtils {
public:

  class Item;
  class Arr;
  class Doc;
  class NameValuePair;

  class Item {
  private:
    Str _str;
    Arr *_arr;
    Doc *_doc;
    
  public:
    Item() : _arr(0), _doc(0) {}
    Item(const char *cstr) : _str(cstr), _arr(0), _doc(0) {}
    Item(const Str &str) : _str(str), _arr(0), _doc(0) {}
    Item(const Arr &arr) : _arr(new Arr(arr)), _doc(0) {}
    Item(const Doc &doc) : _arr(0), _doc(new Doc(doc)) {}
    Item(const Item &o);
    ~Item() {delete _arr; delete _doc;}
    
    const Item &operator=(const Item &o);
    
    bool isDoc() const {return _doc != 0;}
    bool isArr() const {return _arr != 0;}
    bool isStr() const {return !isDoc() && !isArr();}

    const Str &getStr() const {return _str;}
    const Doc &getDoc() const {return *_doc;}
    const Arr &getArr() const {return *_arr;}

    bool equals(const Item &other) const;
  };
  
  class Arr {
  private:
    Item *_arr;
    int _sz;

  public:
    Arr() : _sz(0), _arr(0) {};
    Arr(const Arr &arr);
    ~Arr() {delete [] _arr;}
    const Arr &operator=(const Arr &);

    void append(const Item &item);

    int getSz() const {return _sz;}

    const Item &operator[](int i) const {return _arr[i];}

    bool equals(const Arr &other) const;
  };

  
  class NameValuePair {
  private:
    Str _name;
    Item _item;

    const NameValuePair&operator=(const NameValuePair&); // unimplemented
    
  public:
    static int s_instanceCnt;
    
    NameValuePair(const char *n, const Item &v) : _name(n), _item(v) {s_instanceCnt++;}
    NameValuePair(const Str &n, const Item &v) : _name(n), _item(v) {s_instanceCnt++;}
    NameValuePair(const NameValuePair &o) : _name(o._name), _item(o._item) {s_instanceCnt++;}
    ~NameValuePair() {s_instanceCnt--;}

    const Str &getName() const {return _name;}
    const Item &getValue() const {return _item;}

    bool setValue(const Item &i);
  };

    
  class Doc {
  private:
    int numNVs, arrSz;
    NameValuePair **nvs;

  public:
    static int s_instanceCnt;
    
    Doc() : nvs(new NameValuePair*[10]), numNVs(0), arrSz(10) {s_instanceCnt++;}
    Doc(const Doc &d);
    ~Doc();

    Doc &operator=(const Doc&);
    
    void clear();
    
    void addNameValue(NameValuePair *nv);
    bool setValue(const char *name, const Item &value);
    bool setValue(const Str &name, const Item &value);

    int getSz() const {return numNVs;}
    bool isEmpty() const {return numNVs == 0;}

    int lookup(const char *name) const;
    int lookup(const Str &name) const {return lookup(name.c_str());}
    const NameValuePair &operator[](int i) const {return *nvs[i];}

    bool equals(const Doc &other) const;
  };


  // on success returns pointer to the string just past the closing '}', NULL on failure
  static const char *parseDoc(const char *rawtext, Doc *doc);

  static void printArr(const Arr &arr);
  static void printDoc(const Doc &doc);

  static const char *toURL(const char *db, const char *docid, StrBuf *page);
  static const char *urlEncode(const char *msg, StrBuf *url);
  static const char *toAttachmentPutURL(const char *db, const char *docid, 
					const char *attachName, const char *revision, StrBuf *result);
  static const char *toAttachmentGetURL(const char *db, const char *docid, 
					const char *attachName, StrBuf *result);

  static const char *toString(const Doc &doc, StrBuf *buf);
  static const char *toString(const Arr &arr, StrBuf *buf);

 private:
  static const char *parseArr(const char *rawtext, Arr *arr);
};

#endif
