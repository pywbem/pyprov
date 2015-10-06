#!/bin/sh

# Make rangeTest
g++ -o example python.cpp example.cpp range.cpp rangetest.cpp pycxx_iter.cpp -I.. -I/usr/include/python -lpython2.4 -L. -lpycxx
