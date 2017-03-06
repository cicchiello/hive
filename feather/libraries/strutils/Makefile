str.t : str.cpp str.t.cpp str.h strbuf.h strbuf.cpp CygwinTrace.cpp
	g++ str.t.cpp str.cpp CygwinTrace.cpp strbuf.cpp -I. -o $@

str.o : str.h str.cpp
	g++ str.cpp -I. -o $@

str.t.o : str.h str.t.cpp
	g++ str.t.cpp -I. -o $@
