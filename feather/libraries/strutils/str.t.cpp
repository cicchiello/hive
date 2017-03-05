#include <str.h>

//#define HEADLESS
//#define NDEBUG

#include <CygwinTrace.h>

#include <strbuf.h>

#include <string.h>

void test()
{
  TF("::test");

  Str s1("");
  Str s2("foo");
  Str s3(s2);
  Str s4("foo");
  s4 = "bar";
  Str s5 = "hello";
  Str s6 = "there";
  StrBuf s7(s5.c_str());
  s7 = s7.append(" ").append(s6.c_str());
  Str s8(&s7.c_str()[6]);
  StrBuf s9;
  s9 = "";
  s9.add('h');
  s9.add('i');
  s9.add('!');
  StrBuf s10;
  s10 = "";
  StrBuf s11("Now is the time");
  StrBuf s12("for all good men");
  s11 = s12;
  Str s13("Now is the time again");
  Str s14("for all good men to do something");
  s13 = s14;

  PH2("hash(s1): ", Str::hash_str(s1.c_str()));
  PH2("hash(s2): ", Str::hash_str(s2.c_str()));
  PH2("hash(s3): ", Str::hash_str(s3.c_str()));
  PH2("hash(s4): ", Str::hash_str(s4.c_str()));
  PH2("hash(s9): ", Str::hash_str(s9.c_str()));
  PH2("hash(s10): ", Str::hash_str(s10.c_str()));

  PH2("s1: ", s1.c_str());
  PH2("s2: ", s2.c_str());
  PH2("s3: ", s3.c_str());
  PH2("s4: ", s4.c_str());
  PH2("s5: ", s5.c_str());
  PH2("s7: ", s7.c_str());
  PH2("s8: ", s8.c_str());
  PH2("s9: ", s9.c_str());
  PH2("s10: ", s10.c_str());
  PH2("s11: ", s11.c_str());
  PH2("s12: ", s12.c_str());
  PH2("s13: ", s13.c_str());
  PH2("s14: ", s14.c_str());

  assert(s4.len() == strlen(s4.c_str()), "s4.len() == strlen(s4.c_str())");
  assert(s7.len() == strlen(s7.c_str()), "s7.len() == strlen(s7.c_str())");
  assert(s8.len() == strlen(s8.c_str()), "s8.len() == strlen(s8.c_str())");
  
  PH2("cacheSz: ", Str::cacheSz());
}


int main()
{
  test();
  
  PH2("Str malloc report: ", Str::sBytesConsumed);

  assert(Str::sBytesConsumed == 0, "Str::sBytesConsumed == 0");
  
  PH("That's all folks!");
}
