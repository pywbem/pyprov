/*****************************************************************************
* (C) Copyright 2007 Novell, Inc. 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*   
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*   
* You should have received a copy of the GNU Lesser General Public
* License along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*****************************************************************************/
#ifndef Pegasus_PythonProviderManager_h
#define Pegasus_PythonProviderManager_h

#include "PyCxxObjects.h"
#include "PG_PyExtensions.h"
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/HashTable.h>
#include <Pegasus/ProviderManager2/ProviderName.h>
#include <Pegasus/ProviderManager2/ProviderManager.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Config/ConfigManager.h>
#include <Pegasus/ProviderManager2/OperationResponseHandler.h>
#include <Pegasus/ProviderManager2/Python/Linkage.h>
#include <Pegasus/Provider/CIMOMHandleQueryContext.h>

#include "Reference.h"
#include "PyCxxObjects.h"
#include "PG_PyExtensions.h"

#include <ctime>
#include <map>

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

namespace PythonProvIFC
{

struct Python_SelectExp;


class PyProviderRep
{
public:
	PyProviderRep()
		: m_path()
		, m_pyprov(Py::None())
		, m_canUnload(true)
		, m_lastAccessTime(time_t(0))
		, m_fileModTime(time_t(0))
		, m_activationCount(0)
		, m_provInstance()
		, m_pIndicationResponseHandler(0)
		, m_isIndicationConsumer(false)
	{
	}

	PyProviderRep(const String& path, const Py::Object pyprov,
		bool canUnload=true)
		: m_path(path)
		, m_pyprov(pyprov)
		, m_canUnload(canUnload)
		, m_lastAccessTime(time_t(0))
		, m_fileModTime(time_t(0))
		, m_activationCount(0)
		, m_provInstance()
		, m_pIndicationResponseHandler(0)
		, m_isIndicationConsumer(false)
	{
	}

	~PyProviderRep()
	{
		m_pyprov.release();
		if (m_pIndicationResponseHandler)
			delete m_pIndicationResponseHandler;
	}

	String m_path;
	Py::Object m_pyprov;
	bool m_canUnload;
	time_t m_lastAccessTime;
	time_t m_fileModTime;
	int m_activationCount;
	CIMInstance m_provInstance;
	EnableIndicationsResponseHandler *m_pIndicationResponseHandler;
	bool m_isIndicationConsumer;
private:

	// These are unimplemented. Copy not allowed
	PyProviderRep(const PyProviderRep& arg);
	PyProviderRep& operator=(const PyProviderRep& arg);
};

typedef Reference<PyProviderRep> PyProviderRef;
typedef std::map<String, PyProviderRef> ProviderMap;

class PEGASUS_PYTHONPM_LINKAGE PythonProviderManager : public ProviderManager
{
public:

    PythonProviderManager();
    virtual ~PythonProviderManager();

	PyExtensions* getPyExtensionsMod() const
	{
		return m_pPyExtensions;
	}
	Py::Module getWBEMMod() const
	{
		return m_pywbemMod;
	}

    virtual Message * processMessage(Message * request);

    virtual Boolean hasActiveProviders();
    virtual void unloadIdleProviders();

    virtual Boolean supportsRemoteNameSpaces()
    {
		// TODO: What does this mean?
        return true;
    }

	void generateIndication(const String& provPath,
		const CIMInstance& indicationInstance);

	void setAsIndicationConsumer(PyProviderRef& provref);

protected:

    CIMResponseMessage* _handleUnsupportedRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleExecQueryRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleDisableModuleRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleEnableModuleRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleStopAllProvidersRequest(CIMRequestMessage * message, PyProviderRef& provref);
	CIMResponseMessage* _handleModifySubscriptionRequest(CIMRequestMessage* message, PyProviderRef& provref);
    CIMResponseMessage* _handleSubscriptionInitCompleteRequest (CIMRequestMessage * message, PyProviderRef& provref);

//  Note: The PG_Provider AutoStart property is not yet supported
#if 0
    CIMResponseMessage* _handleInitializeProviderRequest(const Message * message, PyProviderRef& provref);
#endif


    ProviderName _resolveProviderName(const ProviderIdContainer & providerId);
	String _resolvePhysicalName(String physicalName);
	void _initPython();

private:

	Py::Object _loadProvider(const String& provPath,
		const OperationContext& opctx);
	void _shutdownProvider(const PyProviderRef& provref,
		const OperationContext& opctx);
	PyProviderRef _path2PyProviderRef(const String& provPath,
		const OperationContext& opctx);
	void _incActivationCount(CIMRequestMessage* message, PyProviderRef& provref);
	void _decActivationCount(CIMRequestMessage* message, PyProviderRef& provref);
	void _stopAllProviders();

	Py::Module m_pywbemMod;
	Py::Object m_cimexobj;
	PyExtensions* m_pPyExtensions;
	ProviderMap m_provs;
	PyThreadState* m_mainPyThreadState;

	friend class InstanceProviderHandler;
	friend class MethodProviderHandler;
	friend class AssociatorProviderHandler;
	friend class IndicationConsumerProviderHandler;
	friend class IndicationProviderHandler;
};

bool strEndsWith(const String& src, const String& tok);
String getPyFile(const String& fname);
Py::Object getPyPropertyList(const CIMPropertyList& pgpropList);
time_t getModTime(const String& fileName);
String getFunctionName(const String& fnameArg);
Py::Callable getFunction(const Py::Object& obj, const String& fnameArg, bool throwIfNotFound=true);
String processPyException(Py::Exception& thrownEx, int lineno, 
	const String& provPath, OperationResponseHandler* pHandler=0);

}	// End of namespace PythonProvIFC

#endif

