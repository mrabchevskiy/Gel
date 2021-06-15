g++-10 -Wall -fexceptions -std=c++20 -m64 -c terminal.cpp -o terminal.o
g++-10 -o terminal terminal.o -s -m64 -lpthread

