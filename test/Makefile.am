SUBDIRS = LogicalFile

noinst_PROGRAMS = test

test_SOURCES = \
	test.cpp \
	converter.cpp \
	converter.hpp \
	common.hpp

test_LDADD = -lpython2.4 -lopenwbem
test_LDFLAGS = -Xlinker -export-dynamic
test_CFLAGS = -DDEBUG
test_CXXFLAGS = -DDEBUG

EXTRA_DIST = test.py
