include $(top_srcdir)/Makefile.incl.am

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/src/pycxx -I$(top_srcdir)/src/ifc

noinst_LTLIBRARIES = \
	libpegpyprovider.la

libpegpyprovider_la_SOURCES = \
	PG_PyConverter.cpp \
	PG_PyConverter.hpp \
	PG_PyProviderModule.cpp \
	PG_PyProviderModule.hpp
