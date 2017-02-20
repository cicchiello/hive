#ifndef mutex_h
#define mutex_h

class Mutex {
 private:
  const void *owner;
 public:
  Mutex() :owner(0) {}
  bool own(const void *tag);
  void release(const void *tag);

  bool isAvailable() const {return owner == 0;}
};

inline
bool Mutex::own(const void *tag)
{
    if (!owner || (owner == tag)) {
        owner = tag;
	return true;
    } else {
        return false;
    }
}

#ifndef assert
#define assert(a,b)
#endif

inline
void Mutex::release(const void *tag)
{
    assert(owner == tag, "Mutex release failure");
    owner = 0;
}

#endif
