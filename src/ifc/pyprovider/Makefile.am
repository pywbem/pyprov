include $(top_srcdir)/Makefile.incl.am

INCLUDES = -I$(top_builddir) -I$(top_srcdir)/src/pycxx -I$(top_srcdir)/src/ifc

noinst_LTLIBRARIES = \
	libowpyprovider.la

libowpyprovider_la_SOURCES = \
	OW_PyConverter.cpp \
	OW_PyConverter.hpp \
	OW_PyProviderModule.cpp \
	OW_PyProviderModule.hpp \
	OW_PyResultHandlers.cpp \
	OW_PyResultHandlers.hpp \
	OW_PyCIMOMHandle.cpp \
	OW_PyCIMOMHandle.hpp \
	OW_PyProviderEnvironment.cpp \
	OW_PyProviderEnvironment.hpp \
	OW_PyLogger.cpp \
	OW_PyLogger.hpp
