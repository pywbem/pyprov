#!/bin/sh
g++ -shared -I/usr/include/python2.4 -DPEGASUS_PLATFORM_LINUX_IX86_GNU -o libpycxx.so *.cpp -lpthread -lpegcommon -lpython2.4
