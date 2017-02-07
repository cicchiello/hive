#ifndef trace_h
#define trace_h


#ifndef HEADLESS
#define P(args) Serial.print(args)
#define PL(args) Serial.println(args)
#else
#ifndef NDEBUG
#   define NDEBUG
#endif
#define P(args) do {} while (0)
#define PL(args) do {} while (0)
#endif

#ifndef NDEBUG
#define D(args) P(args)
#define DL(args) PL(args)
#else
#define D(args)
#define DL(args)
#endif

class TraceScope {
  private:
    TraceScope *parent;
    const char *func, *file;
    int line;
  public:
    static TraceScope *sCurrScope;
     
    TraceScope(const char *_func, const char *_file, const int _line)
      : func(_func), file(_file), line(_line), parent(0)
    {
        if (sCurrScope)
	    parent=sCurrScope;
	sCurrScope = this;
    }
    ~TraceScope() {sCurrScope = parent;}
    const char *getFunc() const {return func;}
    const char *getFile() const {return file;}
    int getLine() const {return line;}
    const TraceScope *getParent() const {return parent;}
};

#ifndef HEADLESS
#   define TF(f) TraceScope tscope(f,__FILE__,__LINE__);
#else
#   define TF(f) do {} while (0);
#endif



#ifndef NDEBUG

#  define _TAG(TAG) D(TAG); D("; ")
#  define _TAGTOK(TAG,MSG) D(TAG); D(MSG); D("; ")
#  define _FL(FILE,LINE) _TAGTOK("file: ",FILE); _TAGTOK("line: ",LINE)
#  define _L(LINE) _TAGTOK("line: ",LINE)
#  define _TWHERE_FILE(TAG,FILE,LINE,MSG) _TAG(TAG); _TAG(FILE); _L(LINE); DL(MSG);
#  define _TWHERE_FILE2(TAG,FILE,LINE,MSG1,MSG2) _TAG(TAG); _TAG(FILE); _L(LINE); D(MSG1); DL(MSG2);
#  define _TWHERE_FILE3(TAG,FILE,LINE,MSG1,MSG2,MSG3) _TAG(TAG); _TAG(FILE); _L(LINE); D(MSG1); D(MSG2); DL(MSG3);
#  define _TWHERE_FUNC(TAG,FUNC,LINE,MSG) _TAG(TAG); _TAG(FUNC); _L(LINE); DL(MSG);
#  define _TWHERE_FUNC2(TAG,FUNC,LINE,MSG1,MSG2) _TAG(TAG); _TAG(FUNC); _L(LINE); D(MSG1); DL(MSG2);
#  define _TWHERE_FUNC3(TAG,FUNC,LINE,MSG1,MSG2,MSG3) _TAG(TAG); _TAG(FUNC); _L(LINE); D(MSG1); D(MSG2); DL(MSG3);
#  define _TWHERE(TAG,FUNC,FILE,LINE,MSG) if(FUNC==0){_TWHERE_FILE(TAG,FILE,LINE,MSG)}else{_TWHERE_FUNC(TAG,FUNC,LINE,MSG)}
#  define _TWHERE2(TAG,FUNC,FILE,LINE,MSG1,MSG2) if(FUNC==0){_TWHERE_FILE2(TAG,FILE,LINE,MSG1,MSG2)}else{_TWHERE_FUNC2(TAG,FUNC,LINE,MSG1,MSG2)}
#  define _TWHERE3(TAG,FUNC,FILE,LINE,MSG1,MSG2,MSG3) if(FUNC==0){_TWHERE_FILE3(TAG,FILE,LINE,MSG1,MSG2,MSG3)}else{_TWHERE_FUNC3(TAG,FUNC,LINE,MSG1,MSG2,MSG3)}

   static void _TRECURSE(const char *tag, const TraceScope *scope) {
     while (scope) {
       _TWHERE(tag, scope->getFunc(), scope->getFile(), scope->getLine(), "");
       scope = scope->getParent();
     }
   }

#   define TRACE(msg) _TWHERE("TRACE",tscope.getFunc(),__FILE__,__LINE__,msg);
#   define TRACE2(msg1,msg2) _TWHERE2("TRACE",tscope.getFunc(),__FILE__,__LINE__,msg1,msg2);
#   define TRACE3(msg1,msg2,msg3) _TWHERE3("TRACE",tscope.getFunc(),__FILE__,__LINE__,msg1,msg2,msg3);
#   define TRACEDUMP(msg) _TRECURSE("TRACEDUMP",&tscope)
#   define ERR(action) TRACEDUMP("CRASH"); action
#   define assert(c,msg) if (!(c)) {TF(msg);TRACE("assertion");do{}while(1);}


#else

#   define TRACE(msg) do {} while (0);
#   define TRACE2(msg1,msg2) while (0);
#   define TRACE3(msg1,msg2,msg3) while (0);
#   define TRACEDUMP(msg) do {} while (0);
#   define ERR(action) action
#   define assert(c,msg) do {} while (0)

#endif

#endif
