g++-10  -fexceptions -std=c++20 -m64 -c hmi.cpp -o hmi.o
g++-10  -o hmi hmi.o -s -m64 -lpthread

