all: main.cc
	g++ -std=c++14 -DV1 main.cc -o v1.out
	g++ -std=c++14 main.cc
