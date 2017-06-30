#ifndef str_h
#define str_h

class Str {
 public:
  Str();
  Str(const char *s);
  Str(const Str &str);
  ~Str();

  Str &operator=(const Str &);
  Str &operator=(const char *);

  int len() const;

  bool equals(const Str &other) const;
  bool lessThan(const Str &other) const;
  
  bool endsWith(const char *cmp) const;
  
  const char *c_str() const;

  static Str sEmpty; // equiv to Str as constructed by default ctor
  
  static int sBytesConsumed;
  static unsigned int hash_str(const char* s);


  class Item {
  private:
    Item(const char *s);
    
    unsigned char cap;
    unsigned short refs;
    
  public:
    static Item *lookupOrCreate(const char *s);

    Item() : buf(0), cap(0), refs(0) {}
    Item(int sz);
    ~Item();

    bool equals(const Item &other) const;
    
    void expand(int capacity);
    int inc();
    int dec() {return --refs;}
    int refcnt() const {return refs;}

    int getLen() const;

    unsigned short getCapacity() const {return ((unsigned short) cap) << 4;}
    void setCapacity(unsigned short c);

    static const char *classname() {return "Str::Item";}
    
    char *buf;
  };

  Str(Item *);

  Item *item;
  bool deleted:1;

  static void expand(int required, unsigned short *cap, char **buf);

  friend class StrBuf;
  friend int main();
};


inline Str::Str() : deleted(false) {item = sEmpty.item; item->inc();}


inline
bool operator<(const Str &l, const Str &r) {
    return l.lessThan(r);
}


#endif
