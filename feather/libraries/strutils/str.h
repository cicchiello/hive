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

  int capacity() const;
  int len() const;

  bool equals(const Str &other) const;
  bool lessThan(const Str &other) const;
  
  bool endsWith(const char *cmp) const;
  
  const char *c_str() const;

  static Str sEmpty; // equiv to Str as constructed by default ctor
  
  static int sBytesConsumed;
  static unsigned int hash_str(const char* s);
  static int cacheSz();
  
 private:
  
  class Item {
  private:
    Item(const char *s);
    
  public:
    static Item *lookupOrCreate(const char *s);
  
    Item(int sz);
    ~Item();

    bool equals(const Item &other) const;
    
    void expand(int capacity);
    
    char *buf;
    unsigned short cap;
    unsigned short len;
    unsigned short refs;
  };
  
  Str(Item *);

  Item *item;
  bool deleted;

  static void expand(int required, unsigned short *cap, char **buf);

  friend class StrBuf;
  friend int main();
};


inline Str::Str() : item(sEmpty.item), deleted(false) {item->refs++;}
inline Str::Str(const Str &str) : item(str.item), deleted(false) {item->refs++;}
inline const char *Str::c_str() const {return item->buf;}
inline int Str::capacity() const {return item->cap;}
inline Str::~Str() {
  if (--item->refs == 0) {
    delete item;
    sBytesConsumed -= sizeof(Item);
  }
  item = 0;
  deleted = true;
}
inline bool Str::equals(const Str &other) const
{
    if (this->item == other.item)
        return true;

    return this->item->equals(*other.item);
}


inline
bool operator<(const Str &l, const Str &r) {
    return l.lessThan(r);
}


#endif
