#!/bin/sh
g++ -o provtest *.cpp -I.. -lopenwbem -lpthread -lpython2.4 -L. -lpycxx
