PKGS=raylib
CXXFLAGS=-Wall -Wextra -ggdb -pedantic -std=c++20 `pkg-config --cflags --static $(PKGS)`
LIBS=`pkg-config --libs --static $(PKGS)`

build: main.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp $(LIBS)

clean:
	rm -rf main main.dSYM

run: clean build
	./main
