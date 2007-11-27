ROOT = /usr/src/packages/BUILD/pegasus

DIR = Pegasus/ProviderManager2/Python

include $(ROOT)/mak/config.mak

PY_VER = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_python_version()")
PY_LIB = python$(PY_VER)
PY_INC = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_python_inc()")
PY_LINK = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LINKFORSHARED')")
PY_LIBS = $(shell python -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LOCALMODLIBS'), distutils.sysconfig.get_config_var('LIBS')")
EXTRA_INCLUDES = $(SYS_INCLUDES) -I$(PY_INC)
EXTRA_LINK_FLAGS += $(PY_LINK) -l$(PY_LIB) $(PY_LIBS)


LOCAL_DEFINES = -DPEGASUS_PYTHONPM_INTERNAL -DPEGASUS_INTERNALONLY

LIBRARY = PGPythonProviderManager

ifeq ($(OS_TYPE),vms)
 VMS_VECTOR = PegasusCreateProviderManager
endif

LIBRARIES = \
    pegprovider \
    pegprovidermanager \
    pegprm \
    pegclient \
    pegquerycommon \
    pegwql

ifndef PEGASUS_DISABLE_CQL
    LIBRARIES += \
       pegcql
endif

LIBRARIES += \
    pegquerycommon \
    pegconfig \
    pegcommon


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

include $(ROOT)/mak/dynamic-library.mak

