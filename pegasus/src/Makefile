
PY_VER = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_python_version()")
PY_LIB = python$(PY_VER)
PY_INC = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_python_inc()")
PY_LINK = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LINKFORSHARED')")
PY_LIBS = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LOCALMODLIBS'), distutils.sysconfig.get_config_var('LIBS')")
EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(PY_INC)
EXTRA_LINK_FLAGS += $(PY_LINK) -l$(PY_LIB) $(PY_LIBS)

PEG_CFLAGS = $(shell pkg-config --cflags tog-pegasus)

LOCAL_DEFINES = -DPEGASUS_PYTHONPM_INTERNAL -DPEGASUS_INTERNALONLY


CFLAGS = $(PEG_CFLAGS) $(EXTRA_INCLUDES) -I/usr/include/Pegasus-internal \
		 $(LOCAL_DEFINES)

LIBRARIES = \
    -lpegprovider \
    -lpegprovidermanager \
    -lpegprm \
    -lpegclient \
    -lpegquerycommon \
    -lpegwql \
	-lpegcql

LIBRARIES += \
    -lpegquerycommon \
    -lpegconfig \
    -lpegcommon


SOURCES = \
	PythonProviderManager.cpp \
	PG_PyProvIFCCommon.cpp \
	PyCxxExtensions.cpp \
	PyCxxObjects.cpp \
	PyCxxPythonWrap.cpp \
	PyCxxSupport.cpp \
	PG_PyExtensions.cpp \
	PG_PyLogger.cpp \
	PG_PyCIMOMHandle.cpp \
	PG_PyProviderEnvironment.cpp \
	PyInstanceProviderHandler.cpp \
	PyMethodProviderHandler.cpp \
	PyAssociatorProviderHandler.cpp \
	PyIndicationProviderHandler.cpp \
	PyIndConsumerProviderHandler.cpp \
	PG_PyConverter.cpp

OBJECTS = \
	PythonProviderManager.o \
	PG_PyProvIFCCommon.o \
	PyCxxExtensions.o \
	PyCxxObjects.o \
	PyCxxPythonWrap.o \
	PyCxxSupport.o \
	PG_PyExtensions.o \
	PG_PyLogger.o \
	PG_PyCIMOMHandle.o \
	PG_PyProviderEnvironment.o \
	PyInstanceProviderHandler.o \
	PyMethodProviderHandler.o \
	PyAssociatorProviderHandler.o \
	PyIndicationProviderHandler.o \
	PyIndConsumerProviderHandler.o \
	PG_PyConverter.o

.cpp.o : 
	c++ -g $(CFLAGS) -c -o $@ $<

libPGPythonProviderManager.so : $(OBJECTS)
	g++ -g -shared \
		-Wl,-hlibPGPythonProviderManager.so \
		-Xlinker -export-dynamic \
		$(LIBRARIES) $(EXTRA_LINK_FLAGS)\
		-o $@ *.o
	    
clean: 
	rm -f libPGPythonProviderManager.so $(OBJECTS)
