#!/bin/sh
g++ -shared -o libpycxx.so *.cpp -lpthread -lopenwbem -lpython2.4
