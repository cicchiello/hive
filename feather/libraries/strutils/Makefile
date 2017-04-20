str.t : str.cpp str.t.cpp str.h strbuf.h strbuf.cpp CygwinTrace.cpp mempool.cpp mempool.h
	g++ str.t.cpp str.cpp mempool.cpp CygwinTrace.cpp strbuf.cpp -I. -o $@

str.o : str.h str.cpp
	g++ str.cpp -I. -o $@

mempool.o : mempool.h mempool.cpp
	g++ mempool.cpp -I. -o $@

str.t.o : str.h str.t.cpp
	g++ str.t.cpp -I. -o $@
