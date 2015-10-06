SUBDIRS = LogicalFile

noinst_PROGRAMS = test

test_SOURCES = \
	test.cpp \
	converter.cpp \
	converter.hpp \
	common.hpp

test_LDADD = \
	$(PYTHON_LDFLAGS) \
	$(PYTHON_EXTRA_LDFLAGS) \
	$(PYTHON_EXTRA_LIBS) \
	-lopenwbem
test_LDFLAGS = -Xlinker -export-dynamic
test_CFLAGS = -DDEBUG
test_CXXFLAGS = -DDEBUG

EXTRA_DIST = test.py
