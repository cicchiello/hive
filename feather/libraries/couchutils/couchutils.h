#ifndef couch_utils_h
#define couch_utils_h

#include <str.h>

class IPAddress;


class CouchUtils {
public:

  class Doc {
  public:

    class NameValuePair {
    private:
      const Str _name, _value;
      const Doc *_doc;
      
    public:
      NameValuePair(const char *name, const char *value);
      NameValuePair(const char *name, const Doc *value);
      NameValuePair(const Str &name, const Str &value);
      NameValuePair(const Str &name, const Doc *value);
      NameValuePair(const NameValuePair &);
      ~NameValuePair();

      bool isDoc() const {return _doc != 0;}
      const Str &getName() const {return _name;}
      const Str &getValue() const {return _value;}
      const Doc *getDoc() const {return _doc;}

      static int s_instanceCnt;

    private:
    };

  private:

    int numNVs, arrSz;
    NameValuePair **nvs;

  public:
    Doc()
      : nvs(new NameValuePair*[10]), numNVs(0), arrSz(10)
      {s_instanceCnt++;}
    Doc(const Doc &d);
    ~Doc();

    void clear();
    
    void addNameValue(NameValuePair *nv);

    int getSz() const {return numNVs;}
    bool isEmpty() const {return numNVs == 0;}

    int lookup(const char *name) const;
    const NameValuePair &operator[](int i) const {return *nvs[i];}

    static int s_instanceCnt;
  };


  static const char *parseDoc(const char *rawtext, Doc *doc);

  static void printDoc(const Doc &doc);

  static const char *toURL(const char *db, const char *docid, Str *page);
  static const char *toAttachmentPutURL(const char *db, const char *docid, 
					const char *attachName, const char *revision, Str *result);
  static const char *toAttachmentGetURL(const char *db, const char *docid, 
					const char *attachName, Str *result);

  static const char *toString(const Doc &doc, Str *buf);
};

#endif
