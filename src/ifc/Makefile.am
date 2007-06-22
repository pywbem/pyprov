SUBDIRS = pyprovider

include $(top_srcdir)/Makefile.incl.am

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/src/pycxx -I$(top_srcdir)/src/ifc/pyprovider 

providerifc_LTLIBRARIES = \
	libpythonprovifc.la

##############################################################################
libpythonprovifc_la_SOURCES = \
	OW_PyProviderIFC.cpp \
	OW_PyProviderIFC.hpp \
	OW_PyProvider.cpp \
	OW_PyProvider.hpp \
	OW_PyProxyProvider.cpp \
	OW_PyProxyProvider.hpp \
	OW_PyProvIFCCommon.cpp \
	OW_PyProvIFCCommon.hpp

libpythonprovifc_la_LDFLAGS = \
	$(PYTHON_LDFLAGS) \
	$(PYTHON_EXTRA_LDFLAGS) \
	$(PYTHON_EXTRA_LIBS) \
	-lpthread \
	-lopenwbem \
	-lowprovider \
	-lowwql \
	-version-info 1

libpythonprovifc_la_LIBADD = \
	$(top_builddir)/src/pycxx/libowpycxx.la \
	$(top_builddir)/src/ifc/pyprovider/libowpyprovider.la

