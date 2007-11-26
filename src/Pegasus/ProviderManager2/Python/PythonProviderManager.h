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


struct PyProviderRep
{
	PyProviderRep()
		: m_path()
		, m_pyprov(Py::None())
		, m_canUnload(true)
		, m_lastAccessTime(time_t(0))
		, m_fileModTime(time_t(0))
		, m_activationCount(0)
		, m_pIndicationResponseHandler(0)
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
		, m_pIndicationResponseHandler(0)
	{
	}

	PyProviderRep(const PyProviderRep& arg)
		: m_path(arg.m_path)
		, m_pyprov(arg.m_pyprov)
		, m_canUnload(arg.m_canUnload)
		, m_lastAccessTime(arg.m_lastAccessTime)
		, m_fileModTime(arg.m_fileModTime)
		, m_activationCount(arg.m_activationCount)
		, m_pIndicationResponseHandler(arg.m_pIndicationResponseHandler)
	{
	}

	PyProviderRep& operator= (const PyProviderRep& arg)
	{
		m_path = arg.m_path;
		m_pyprov = arg.m_pyprov;
		m_canUnload = arg.m_canUnload;
		m_lastAccessTime = arg.m_lastAccessTime;
		m_fileModTime = arg.m_fileModTime;
		m_activationCount = arg.m_activationCount;
		m_pIndicationResponseHandler = arg.m_pIndicationResponseHandler;
		return *this;
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
	EnableIndicationsResponseHandler *m_pIndicationResponseHandler;
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
        return true;
    }

    struct indProvRecord
    {
        indProvRecord() : enabled(false), count(1), handler(NULL)
        {
        }
        Boolean enabled;
        int count;
#ifdef PEGASUS_ENABLE_REMOTE_PYTHON
        String remoteInfo; 
#endif
        EnableIndicationsResponseHandler* handler;
    };

    struct indSelectRecord
    {
        indSelectRecord() : eSelx(NULL), count(1)
        {
        }
        Python_SelectExp *eSelx;
        CIMOMHandleQueryContext *qContext;
        int count;
    };

    typedef HashTable<String,indProvRecord*, \
        EqualFunc<String>,HashFunc<String> > IndProvTab;

    typedef HashTable<CIMObjectPath,indSelectRecord*, \
        EqualFunc<CIMObjectPath>,HashFunc<CIMObjectPath> > IndSelectTab;

protected:

    CIMResponseMessage* _handleUnsupportedRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleExecQueryRequest(CIMRequestMessage * message, PyProviderRef& provref);

    CIMResponseMessage* _handleCreateSubscriptionRequest(CIMRequestMessage * message, PyProviderRef& provref);
//    CIMResponseMessage* handleModifySubscriptionRequest(const Message * message, PyProviderRef& provref);
    CIMResponseMessage* _handleDeleteSubscriptionRequest(CIMRequestMessage * message, PyProviderRef& provref);

    CIMResponseMessage* _handleExportIndicationRequest(CIMRequestMessage * message, PyProviderRef& provref);

    CIMResponseMessage* _handleDisableModuleRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleEnableModuleRequest(CIMRequestMessage * message, PyProviderRef& provref);
    CIMResponseMessage* _handleStopAllProvidersRequest(CIMRequestMessage * message, PyProviderRef& provref);
//  Note: The PG_Provider AutoStart property is not yet supported
#if 0
    CIMResponseMessage* _handleInitializeProviderRequest(const Message * message, PyProviderRef& provref);
#endif
    CIMResponseMessage* _handleSubscriptionInitCompleteRequest (CIMRequestMessage * message, PyProviderRef& provref);

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

	void _incActivationCount(PyProviderRef& provref);
	void _decActivationCount(PyProviderRef& provref);

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
Py::Callable getFunction(const Py::Object& obj, const String& fnameArg);
String processPyException(Py::Exception& thrownEx, int lineno, 
	const String& provPath, OperationResponseHandler* pHandler=0);

}	// End of namespace PythonProvIFC

#endif

