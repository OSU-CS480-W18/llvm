all: main.cpp
	g++ -std=c++11 main.cpp  `llvm-config --cppflags --ldflags --libs --system-libs core` -o main
