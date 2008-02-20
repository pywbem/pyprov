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
#include "PG_PyProvIFCCommon.h"
#include "PythonProviderManager.h"

#include "PyInstanceProviderHandler.h"
#include "PyMethodProviderHandler.h"
#include "PyAssociatorProviderHandler.h"
#include "PyIndicationProviderHandler.h"
#include "PyIndConsumerProviderHandler.h"

#include <Pegasus/Common/CIMMessage.h>
#include <Pegasus/Common/OperationContext.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/StatisticalData.h>
#include <Pegasus/Common/Logger.h>
#include <Pegasus/Common/LanguageParser.h>
#include <Pegasus/Common/MessageLoader.h> //l10n
#include <Pegasus/Common/Constants.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/Mutex.h>
#include <Pegasus/Config/ConfigManager.h>
#include <Pegasus/Provider/CIMOMHandleQueryContext.h>
#include <Pegasus/ProviderManager2/CIMOMHandleContext.h>
#include <Pegasus/ProviderManager2/ProviderName.h>
#include <Pegasus/ProviderManager2/AutoPThreadSecurity.h>

#include "PG_PyConverter.h"

#include <unistd.h>

PEGASUS_USING_STD;
PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

namespace
{

Py::Object g_cimexobj;
Mutex g_provGuard;

void TRACE(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

}	// End of unnamed namespace

//////////////////////////////////////////////////////////////////////////////
bool
strEndsWith(const String& src, const String& tok)
{
	bool rc = false;
	Uint32 ndx = src.find(tok);
	return (ndx == PEG_NOT_FOUND) ? false : (ndx == (src.size()-tok.size()));
}

//////////////////////////////////////////////////////////////////////////////
String
getPyFile(const String& fname)
{
	String pyFile;
	if (strEndsWith(fname, ".pyc"))
	{
		pyFile = fname.subString(0, fname.size()-1);
	}
	else if (strEndsWith(fname, ".py"))
	{
		pyFile = fname;
	}
	else
	{
		pyFile = fname + ".py";
	}
	return pyFile;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
getPyPropertyList(const CIMPropertyList& pgpropList)
{
	if (!pgpropList.isNull())
	{
		Py::List plst;
		Uint32 len = pgpropList.size();
		for(Uint32 i = 0; i < len; i++)
		{
			plst.append(Py::String(pgpropList[i].getString()));
		}
		return Py::Object(plst);
	}
	return Py::None();
}

//////////////////////////////////////////////////////////////////////////////
time_t
getModTime(const String& fileName)
{
	time_t cc = 0;
	struct stat stbuf;
	if (::stat((const char *)fileName.getCString(), &stbuf) == 0)
	{
		cc = stbuf.st_mtime;
	}
	return cc;
}

//////////////////////////////////////////////////////////////////////////////
String
getFunctionName(
	const String& fnameArg)
{
	return String(PYFUNC_PREFIX + fnameArg);
}

//////////////////////////////////////////////////////////////////////////////
Py::Callable
getFunction(
	const Py::Object& obj,
	const String& fnameArg,
	bool throwIfNotFound)
{
	String fn = getFunctionName(fnameArg);
	try
	{
		Py::Callable pyfunc = obj.getAttr(fn);
		return pyfunc;
	}
	catch(Py::Exception& e)
	{
		e.clear();
	}
	if (throwIfNotFound)
	{
		THROW_NOSUCHMETH_EXC(fn);
	}
	return Py::Callable();	// Shouldn't hit this
}

//////////////////////////////////////////////////////////////////////////////
String 
processPyException(
	Py::Exception& thrownEx,
	int lineno, 
	const String& provPath, 
	OperationResponseHandler* pHandler)
{
	Py::Object etype, evalue;

	bool isCIMExc = PyErr_ExceptionMatches(g_cimexobj.ptr());
	String tb = LogPyException(thrownEx, __FILE__, lineno, etype,
		evalue, !isCIMExc);

	thrownEx.clear();

	if (!pHandler)
	{
		return tb;
	}

	if (!isCIMExc)
	{
		pHandler->setCIMException(CIMException(CIM_ERR_FAILED,
			Formatter::format("File: $0  Line: $1  From Python code. "
				"Trace: $2", __FILE__, lineno, tb)));
		return tb;
	}

	int errval = CIM_ERR_FAILED;
	String msg;
	if (provPath.size() > 0)
	{
		msg = Formatter::format("Thrown from Python provider: $0", provPath);
	}
	else
	{
		msg = "Thrown from unknown entity";
	}
	try
	{
		// Attempt to get information about the pywbem.CIMError
		// that occurred...
		bool haveInt = false;
		bool haveMsg = false;
		Py::Tuple exargs = evalue.getAttr("args");
		for (int i = 0; i < int(exargs.length()); i++)
		{
			Py::Object wko = exargs[i];
			if (wko.isInt() && !haveInt)
			{
				errval = int(Py::Int(wko));
				haveInt = true;
			}
			if (wko.isString() && !haveMsg)
			{
				msg = Py::String(wko).as_peg_string();
				haveMsg = true;
			}
			if (haveInt && haveMsg)
			{
				break;
			}
		}
	}
	catch(Py::Exception& theExc)
	{
		theExc.clear();
		pHandler->setCIMException(CIMException(CIM_ERR_FAILED,
			Formatter::format("Re-Thrown from python code. type: $0  value: $1",
				etype.as_string(), evalue.as_string())));
		return tb;
	}
	catch(...)
	{
		pHandler->setCIMException(CIMException(CIM_ERR_FAILED,
			Formatter::format("Caught unknown exception trying to process "
				"pywbem.CIMError. type: $0  value: $1",
				etype.as_string(), evalue.as_string())));
		return tb;
	}

	pHandler->setCIMException(CIMException(CIMStatusCode(errval), tb));
	return tb;
}

///////////////////////////////////////////////////////////////////////////////
PythonProviderManager::PythonProviderManager()
	: ProviderManager()
	, m_pywbemMod()
	, m_pPyExtensions(0)
	, m_provs()
	, m_mainPyThreadState(0)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::PythonProviderManager()");
    _subscriptionInitComplete = false;
    PEG_TRACE_CSTRING (
        TRC_PROVIDERMANAGER,
        Tracer::LEVEL2,
        "-- Python Provider Manager activated");

	_initPython();
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
PythonProviderManager::~PythonProviderManager()
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::~PythonProviderManager()");
	_stopAllProviders();
	PyEval_AcquireLock();
	PyThreadState_Swap(m_mainPyThreadState);
	Py_Finalize();
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
// Assumptions: Caller catches Py::Exception
Py::Object
PythonProviderManager::_loadProvider(
	const String& provPath,
	const OperationContext& opctx)
{
	PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_loadProvider()");
	Py::Object cim_provider = m_pywbemMod.getAttr("cim_provider"); 
	Py::Callable ctor = cim_provider.getAttr("ProviderProxy");
	Py::Tuple args(2);
	args[0] = PyProviderEnvironment::newObject(opctx, this, provPath);
	args[1] = Py::String(provPath);
	// Construct a CIMProvider python object
	Py::Object pyprov = ctor.apply(args);
    PEG_METHOD_EXIT();
	return pyprov;
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_stopAllProviders()
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_stopAllProviders()");
 
	ProviderMap::iterator it = m_provs.begin();
	while(it != m_provs.end())
	{
		_shutdownProvider(it->second, OperationContext());
		it++;
	}
	m_provs.clear();
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_shutdownProvider(
	const PyProviderRef& provref,
	const OperationContext& opctx)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_shutdownProvider()");
 
	Py::GILGuard gg;	// Acquire python's GIL
	try
	{
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "shutdown", false);
		if (!pyfunc.isCallable())
		{
			return;
		}
		Py::Tuple args(1);
		args[0] = PyProviderEnvironment::newObject(opctx, this, provref->m_path);
	    pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		String tb = processPyException(e, __LINE__, provref->m_path);
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.PythonProviderManager",
			"Caught python exception invoking 'shutdown' provider $0. $1",
			provref->m_path, tb);
	}
	catch(...)
	{
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught unknown exception invoking 'shutdown' provider $0.",
			provref->m_path);
	}
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
PyProviderRef
PythonProviderManager::_path2PyProviderRef(
	const String& provPath,
	const OperationContext& opctx)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_path2PyProviderRef()");
 
	AutoMutex am(g_provGuard);
	ProviderMap::iterator it = m_provs.find(provPath);
	if (it != m_provs.end())
	{
		it->second->m_lastAccessTime = ::time(NULL);
        time_t curModTime = getModTime(provPath);
        if ((curModTime <= it->second->m_fileModTime)
            || (it->second->m_canUnload == false) )
        {
            // not modified, or can't reload... return it
            return it->second;
        }
        else
        {
            //cleanup for reload on fall-thru
			_shutdownProvider(it->second, opctx);
            m_provs.erase(it);
        }
	}

	Py::GILGuard gg;	// Acquire python's GIL

	try
	{
		// Get the Python proxy provider
		Py::Object pyprov = _loadProvider(provPath, opctx);
		PyProviderRef entry(new PyProviderRep(provPath, pyprov));
		entry->m_fileModTime = getModTime(provPath);
		entry->m_lastAccessTime = ::time(NULL);
		m_provs[provPath] = entry;
		return entry;
	}
	catch(Py::Exception& e)
	{
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught exception loading provider $0.",
			provPath);
		String tb = processPyException(e, __LINE__, provPath);
		String msg = "Python Load Error: " + tb;
		THROW_NOSUCHPROV_EXC(msg);
	}

    PEG_METHOD_EXIT();
	// Shouldn't hit this
	return PyProviderRef(0);
}

#ifdef DEBUG
    void print(PEGASUS_STD(ostream)& os, CIMRequestMessage *msg)
    {
		os << "CIMRequestMessage\n";
		os << "{";

		os << "    messageId: " << msg->messageId << PEGASUS_STD(endl);
		os << "    messageType: " << MessageTypeToString(msg->getType()) << endl;

        os << "}";
    }
#endif

///////////////////////////////////////////////////////////////////////////////
Message*
PythonProviderManager::processMessage(Message * message)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::processMessage()");

    CIMRequestMessage* request = dynamic_cast<CIMRequestMessage*>(message);
    PEGASUS_ASSERT(request != 0);

	//print(cerr, request);

	// the following messages don't contain ProviderIdContainer,
	// and hence don't pass a providerRef into the handler:
	//     CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE
	//     CIM_DISABLE_MODULE_REQUEST_MESSAGE
	//     CIM_ENABLE_MODULE_REQUEST_MESSAGE
	CIMResponseMessage* response = 0;
	try
	{
		PyProviderRef provRef;
		if (request->operationContext.contains(ProviderIdContainer::NAME))
		{
			ProviderIdContainer providerId =
				request->operationContext.get(ProviderIdContainer::NAME);
			ProviderName name = _resolveProviderName(providerId);
			// If provider doesn't exist this call throws PyNoSuchProviderException
			provRef = _path2PyProviderRef(name.getLocation(),
				request->operationContext);
			// At this point we know we have a provider
		}

		// pass the request message to a handler method based on message type
		switch (request->getType())
		{
			case CIM_GET_INSTANCE_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleGetInstanceRequest(request, provRef, this);
				break;

			case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleEnumerateInstancesRequest(request, provRef, this);
				break;

			case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleEnumerateInstanceNamesRequest(request, provRef, this);
				break;

			case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleCreateInstanceRequest(request, provRef, this);
				break;

			case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleModifyInstanceRequest(request, provRef, this);
				break;

			case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleDeleteInstanceRequest(request, provRef, this);
				break;

			case CIM_GET_PROPERTY_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleGetPropertyRequest(request, provRef, this);
				break;

			case CIM_SET_PROPERTY_REQUEST_MESSAGE:
				response = InstanceProviderHandler::handleSetPropertyRequest(request, provRef, this);
				break;

			case CIM_INVOKE_METHOD_REQUEST_MESSAGE:
				response = MethodProviderHandler::handleInvokeMethodRequest(request, provRef, this);
				break;

			case CIM_ASSOCIATORS_REQUEST_MESSAGE:
				response = AssociatorProviderHandler::handleAssociatorsRequest(request, provRef, this);
				break;

			case CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE:
				response = AssociatorProviderHandler::handleAssociatorNamesRequest(request, provRef, this);
				break;

			case CIM_REFERENCES_REQUEST_MESSAGE:
				response = AssociatorProviderHandler::handleReferencesRequest(request, provRef, this);
				break;
			case CIM_REFERENCE_NAMES_REQUEST_MESSAGE:
				response = AssociatorProviderHandler::handleReferenceNamesRequest(request, provRef, this);
				break;

			case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
				_incActivationCount(request, provRef);
				response = IndicationProviderHandler::handleCreateSubscriptionRequest(request, provRef, this);
				break;

			case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
				_decActivationCount(request, provRef);
				response = IndicationProviderHandler::handleDeleteSubscriptionRequest(request, provRef, this);
				break;

			case CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE:
				response = _handleModifySubscriptionRequest (request, provRef);
				break;

			case CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE:
				response = _handleSubscriptionInitCompleteRequest (request, provRef);
				break;

			case CIM_EXPORT_INDICATION_REQUEST_MESSAGE:
				response = IndicationConsumerProviderHandler::handleExportIndicationRequest(
					request, provRef, this);
				break;

			case CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE:
				response = _handleStopAllProvidersRequest(request);
				break;

			case CIM_DISABLE_MODULE_REQUEST_MESSAGE:
				response = _handleDisableModuleRequest(request);
				break;

			case CIM_ENABLE_MODULE_REQUEST_MESSAGE:
				response = _handleEnableModuleRequest(request);
				break;

			case CIM_EXEC_QUERY_REQUEST_MESSAGE:
				// TODO?
				response = _handleExecQueryRequest(request, provRef);
				break;

	// Note: The PG_Provider AutoStart property is not yet supported
#if 0
			case CIM_INITIALIZE_PROVIDER_REQUEST_MESSAGE:
				response = _handleInitializeProviderRequest(request, provRef);
				break;
#endif
			default:
				response = _handleUnsupportedRequest(request, provRef);
				break;
		}
	}
	catch (CIMException& e)
	{
		PEG_TRACE_STRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
			"CIMException: " + e.getMessage());
		response = request->buildResponse();
		response->cimException = PEGASUS_CIM_EXCEPTION_LANG(
		e.getContentLanguages(), e.getCode(), e.getMessage());
	}
	catch(Py::Exception& e)
	{
		String tb = processPyException(e, __LINE__, String());
		String msg = "PythonProviderManager caught Python "
			"exception: " + tb;
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE, msg);
        response = request->buildResponse();
        response->cimException = PEGASUS_CIM_EXCEPTION(
            CIM_ERR_FAILED, msg);
	}
	catch (Exception& e)
    {
        PEG_TRACE_STRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
            "Exception: " + e.getMessage());
        response = request->buildResponse();
        response->cimException = PEGASUS_CIM_EXCEPTION_LANG(
            e.getContentLanguages(), CIM_ERR_FAILED, e.getMessage());
    }
	catch (...)
    {
        PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL2,
            "Exception: Unknown");
        response = request->buildResponse();
        response->cimException = PEGASUS_CIM_EXCEPTION(
            CIM_ERR_FAILED, "Unknown error.");
    }

    PEG_METHOD_EXIT();
    return(response);
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_incActivationCount(
	CIMRequestMessage* message,
	PyProviderRef& provref)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_incActivationCount()");
 
	AutoMutex am(g_provGuard);
	provref->m_activationCount++;
	if (provref->m_pIndicationResponseHandler)
	{
		return;
	}

	CIMCreateSubscriptionRequestMessage* request =
		dynamic_cast<CIMCreateSubscriptionRequestMessage*>(message);
	PEGASUS_ASSERT(request != 0);

	//  Save the provider instance from the request
    ProviderIdContainer pidc = (ProviderIdContainer)
        request->operationContext.get(ProviderIdContainer::NAME);
	provref->m_provInstance = pidc.getProvider();
	provref->m_pIndicationResponseHandler = 
		new EnableIndicationsResponseHandler(
			0,    // request
			0,    // response
			provref->m_provInstance,
			_indicationCallback,
			_responseChunkCallback);

    provref->m_pIndicationResponseHandler->processing();
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_decActivationCount(
	CIMRequestMessage* message,
	PyProviderRef& provref)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_decActivationCount()");
 
	AutoMutex am(g_provGuard);
	// Make sure we know about this provider
	ProviderMap::iterator it = m_provs.find(provref->m_path);
	if (it != m_provs.end())
	{
		provref->m_activationCount--;
		if (!provref->m_activationCount
			&& provref->m_pIndicationResponseHandler)
		{
			provref->m_pIndicationResponseHandler->complete();

			// Set indication response handle to NULL so indications
			// will not get generated from this provider
			provref->m_pIndicationResponseHandler = 0;
		}
	}
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::generateIndication(
	const String& provPath,
	const CIMInstance& indicationInstance)
{
	AutoMutex am(g_provGuard);
	ProviderMap::iterator it = m_provs.find(provPath);
	if (it == m_provs.end())
	{
		return;
	}
	PyProviderRef pref = it->second;
	if (!(pref->m_pIndicationResponseHandler))
	{
		return;
	}
	pref->m_pIndicationResponseHandler->deliver(
		CIMIndication(indicationInstance));
}

///////////////////////////////////////////////////////////////////////////////
Boolean PythonProviderManager::hasActiveProviders()
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::hasActiveProviders()");
 
	bool cc = false;
	AutoMutex am(g_provGuard);
	try
	{
		time_t currtime = ::time(NULL);
		ProviderMap::iterator it = m_provs.begin();
		while(it != m_provs.end())
		{
			if (it->second->m_isIndicationConsumer
				|| it->second->m_pIndicationResponseHandler)
			{
				cc = true;
				break;
			}
			else
			{
				time_t tdiff = currtime - it->second->m_lastAccessTime;
				if (tdiff < PYPROV_SECS_TO_LIVE)
				{
					cc = true;
					break;
				}
			}
			it++;
		}
	}
	catch(...)
	{
		// TODO
		cc = true;
	}
    PEG_METHOD_EXIT();
	return cc;
}

///////////////////////////////////////////////////////////////////////////////
void PythonProviderManager::unloadIdleProviders()
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::unloadIdleProviders()");
 
	AutoMutex am(g_provGuard);
	time_t currtime = ::time(NULL);
	ProviderMap::iterator it = m_provs.begin();
	while(it != m_provs.end())
	{
		if (!(it->second->m_isIndicationConsumer)
			&& !(it->second->m_pIndicationResponseHandler))
		{
			time_t tdiff = currtime - it->second->m_lastAccessTime;
			if (tdiff >= PYPROV_SECS_TO_LIVE)
			{
				_shutdownProvider(it->second, OperationContext());
				m_provs.erase(it++);
				continue;
			}
		}
		it++;
	}
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
void PythonProviderManager::setAsIndicationConsumer(
	PyProviderRef& provref)
{
	AutoMutex am(g_provGuard);
	ProviderMap::iterator it = m_provs.find(provref->m_path);
	if (it != m_provs.end())
	{
		it->second->m_isIndicationConsumer = true;
		provref->m_isIndicationConsumer = true;
	}
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleExecQueryRequest(
	CIMRequestMessage* message,
	PyProviderRef& provref)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleExecQueryRequest()");

    CIMRequestMessage* request =
        dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
    response->cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED,
				"ExecQuery not yet implemented");
    PEG_METHOD_EXIT();
    return response;
}

#ifdef DEBUG
void printDMR(PEGASUS_STD(ostream)& os, CIMDisableModuleRequestMessage *msg)
{
	os << "CIMDisableModuleRequestMessage\n" << endl;
	os << "{" << endl;
	os << "    messageId: " << msg->messageId << PEGASUS_STD(endl);
	os << "    messageType: " << MessageTypeToString(msg->getType()) << endl;
	os << "    disableProvidersOnly: " << (msg->disableProviderOnly?"true":"false") << endl;
	os << "    authType: " << msg->authType << endl;
	os << "    userName: " << msg->userName << endl;
	os << "    providerModule: " << msg->providerModule.getPath().toString() << endl;
	String physName = msg->providerModule.getProperty(msg->providerModule.findProperty("Location")).getValue().toString();
	os << "        physName: " << physName << endl;
	os << "    Providers (and if indicationProvider):" << endl;
	Array<CIMInstance> _pInstances = msg->providers;
	Array<Boolean> _indProvs = msg->indicationProviders;
	for (Uint32 i=0, n=_pInstances.size(); i<n; i++)
	{
		String provName;
		_pInstances[i].getProperty(_pInstances[i].findProperty(CIMName("Name"))).getValue().get(provName);
		os << "        " << provName << ":" << (_indProvs[i]?"true":"false") << endl;
	}
	os << "}" << endl << endl;
}
#endif

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleDisableModuleRequest(
    CIMRequestMessage* message)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleDisableModuleRequest()");

	CIMDisableModuleRequestMessage* request = 
		dynamic_cast<CIMDisableModuleRequestMessage*>(message);
    PEGASUS_ASSERT(request != 0 );

	//printDMR(cerr, request);

	Array<Uint16> operationalStatus;
	CIMException cimException;
	String loc;
	try
	{
		// first find the provider ref
		loc = request->providerModule.getProperty(request->providerModule.findProperty("Location")).getValue().toString();
		// ProviderMap is a map of:  Location -> ProviderRef
		ProviderMap::iterator it = m_provs.find(loc);
		if (it != m_provs.end())
		{
			_shutdownProvider(it->second, request->operationContext);
			m_provs.erase(it);
			operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_STOPPED);
		}
		else
		{
            // no providers were loaded... still set it to stopped
			operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_STOPPED);
		}
	}
	catch (Py::Exception& e)
	{
		String tb = processPyException(e, __LINE__, loc, false);
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught exception processing Disable Module Request. "
			"Provider $0. $1", loc, tb);
		String msg = "Python UnLoad Error: " + tb;
        cimException = CIMException(CIM_ERR_FAILED, msg);
		operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);
	}
	catch (CIMException& e)
    {
        PEG_TRACE_STRING(TRC_PROVIDERMANAGER, Tracer::LEVEL4,
                         "Exception: " + e.getMessage());
        cimException = e;
		operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);
    }
    catch (Exception& e)
    {
        PEG_TRACE_STRING(TRC_PROVIDERMANAGER, Tracer::LEVEL4,
            "Exception: " + e.getMessage());
        cimException = CIMException(CIM_ERR_FAILED, e.getMessage());
		operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);
    }
    catch (...)
    {
        PEG_TRACE_CSTRING(TRC_PROVIDERMANAGER, Tracer::LEVEL4,
            "Exception: Unknown");
        cimException = PEGASUS_CIM_EXCEPTION_L(
            CIM_ERR_FAILED, "PythonProviderManager. Disable Module Request "
			"failed with unknown exception");
		operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);
    }

    CIMDisableModuleResponseMessage* response =
        dynamic_cast<CIMDisableModuleResponseMessage*>(
            request->buildResponse());
	/*
    CIMDisableModuleResponseMessage * response =
        new CIMDisableModuleResponseMessage(
        request->messageId,
        CIMException(),
        request->queueIds.copyAndPop(),
        operationalStatus);
	response->setHttpMethod(request->getHttpMethod());
	*/

    PEGASUS_ASSERT(response != 0);
	response->operationalStatus = operationalStatus;
    PEG_METHOD_EXIT();
    return response;
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleEnableModuleRequest(
    CIMRequestMessage* message)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleEnableModuleRequest()");
    CIMEnableModuleRequestMessage* request =
        dynamic_cast<CIMEnableModuleRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	Array<Uint16> operationalStatus;
	operationalStatus.append(CIM_MSE_OPSTATUS_VALUE_OK);

	// let the CIMOM know that we received the message - indicating
	// that a provider registration has been made
	CIMEnableModuleResponseMessage* response =
	    dynamic_cast<CIMEnableModuleResponseMessage*>(
	        request->buildResponse());
	PEGASUS_ASSERT(response != 0);
	response->operationalStatus = operationalStatus;

	PEG_METHOD_EXIT();
    return response;
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleStopAllProvidersRequest(
    CIMRequestMessage* message)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleStopAllProvidersRequest()");
    CIMStopAllProvidersRequestMessage* request =
        dynamic_cast<CIMStopAllProvidersRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
	_stopAllProviders();
    PEG_METHOD_EXIT();
	return response;
}

#if 0
///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleInitializeProviderRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleSubscriptionInitCompleteRequest()");
    CIMRequestMessage* request =
        dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
    response->cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED,
				"SubscriptionInitComplete not yet implemented");
    PEG_METHOD_EXIT();
    return response;
}
#endif

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleModifySubscriptionRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleModifySubscriptionRequest()");
	CIMModifySubscriptionRequestMessage* request =
		dynamic_cast<CIMModifySubscriptionRequestMessage*>(message);
    PEGASUS_ASSERT(request != 0);

    AutoPtr<CIMModifySubscriptionResponseMessage> response(
        dynamic_cast<CIMModifySubscriptionResponseMessage*>(
            request->buildResponse()));
    PEGASUS_ASSERT(response.get() != 0);

	// Ignore?

	return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleSubscriptionInitCompleteRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleSubscriptionInitCompleteRequest()");

    CIMSubscriptionInitCompleteRequestMessage* request =
        dynamic_cast<CIMSubscriptionInitCompleteRequestMessage*>(message);
    PEGASUS_ASSERT(request != 0);

	CIMSubscriptionInitCompleteResponseMessage* response =
		dynamic_cast<CIMSubscriptionInitCompleteResponseMessage*>(
			request->buildResponse());
	PEGASUS_ASSERT(response != 0);
	_subscriptionInitComplete = true;
    PEG_METHOD_EXIT();
    return response;
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleUnsupportedRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleUnsupportedRequest()");
    CIMRequestMessage* request =
        dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
    response->cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED, String::EMPTY);

    PEG_METHOD_EXIT();
    return response;
}

///////////////////////////////////////////////////////////////////////////////
String PythonProviderManager::_resolvePhysicalName(String physicalName)
{
	String defProvDir = "/usr/lib/pycim";
	String provDir;
	//String provDir = ConfigManager::getInstance()->getCurrentValue("pythonProvDir");

	if (provDir == String::EMPTY)
		provDir = defProvDir;
	if (!strEndsWith(provDir, "/"))
		provDir.append("/");
	
	String pyFileName = getPyFile(physicalName);
	String fullPath;
	if (pyFileName[0] == '/')
	{
		fullPath = pyFileName;
	}
	else
	{
		fullPath = provDir + pyFileName;
	}
	
	return fullPath;
}

///////////////////////////////////////////////////////////////////////////////
ProviderName PythonProviderManager::_resolveProviderName(
    const ProviderIdContainer & providerId)
{
    String providerName;
    String fileName;
    String location;
    String moduleName;
    CIMValue genericValue;

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_resolveProviderName()");

    genericValue = providerId.getModule().getProperty(
        providerId.getModule().findProperty("Name")).getValue();
    genericValue.get(moduleName);

    genericValue = providerId.getProvider().getProperty(
        providerId.getProvider().findProperty("Name")).getValue();
    genericValue.get(providerName);

    genericValue = providerId.getModule().getProperty(
        providerId.getModule().findProperty("Location")).getValue();
    genericValue.get(location);
    fileName = _resolvePhysicalName(location);

    // An empty file name is only for interest if we are in the 
    // local name space. So the message is only issued if not
    // in the remote Name Space.
    if (fileName == String::EMPTY && (!providerId.isRemoteNameSpace()))
    {
        genericValue.get(location);
        //String fullName = FileSystem::buildLibraryFileName(location);
        String fullName = location;
        Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
            "ProviderManager.Python.PythonProviderManager.CANNOT_FIND_MODULE",
            "For provider $0 module $1 was not found.", 
            providerName, fullName);

    }
    ProviderName name(moduleName, providerName, fileName);
    name.setLocation(fileName);
    PEG_METHOD_EXIT();
    return name;
}

//////////////////////////////////////////////////////////////////////////////
void PythonProviderManager::_initPython()
{
	PEG_METHOD_ENTER(
		TRC_PROVIDERMANAGER,
		"PythonProviderManager::_initPython()");

	// Initialize embedded python interpreter
	Py_Initialize();

	PyEval_InitThreads();
	m_mainPyThreadState = PyGILState_GetThisThreadState();
	PyEval_ReleaseThread(m_mainPyThreadState);

	Py::GILGuard gg;	// Acquire python's GIL

	try
	{
		// Load the pywbem module for use in interacting
		// with python providers
		PEG_TRACE_CSTRING (
			TRC_PROVIDERMANAGER,
			Tracer::LEVEL2,
			"Python provider manager loading pywbem module...");

		m_pywbemMod = Py::Module("pywbem", true);	// load pywbem module
		g_cimexobj = m_pywbemMod.getAttr("CIMError");
		PGPyConv::setPyWbemMod(m_pywbemMod);
	}
	catch (Py::Exception& e)
	{
		String msg = "Python provider manager caught exception "
			"loading pywbem module:";
		Logger::put(Logger::STANDARD_LOG, PYSYSTEM_ID, Logger::FATAL, msg);
		String tb = LogPyException(e, __FILE__, __LINE__);
		e.clear();
		msg.append(tb);
		THROW_PYIFC_EXC(msg);
	}

	try
	{
		// Initialize the pycimmb module (python provider support module)
		Logger::put(Logger::DEBUG_LOG, PYSYSTEM_ID, Logger::TRACE,
			"Python provider mananger initializing the pycimmb module...");
		PyExtensions::doInit(m_pywbemMod);
		m_pPyExtensions = PyExtensions::getModulePtr();
	}
	catch (Py::Exception& e)
	{
		String msg = "Python provider manager caught exception "
			"initializing pycim module:";
		Logger::put(Logger::STANDARD_LOG, PYSYSTEM_ID, Logger::FATAL, msg);
		String tb = LogPyException(e, __FILE__, __LINE__);
		e.clear();
		msg.append(tb);
		THROW_PYIFC_EXC(msg);
	}
}

}	// End of namespace PythonProvIFC

///////////////////////////////////////////////////////////////////////////////
extern "C" PEGASUS_EXPORT ProviderManager * PegasusCreateProviderManager(
    const Pegasus::String & providerManagerName)
{
    if (Pegasus::String::equalNoCase(providerManagerName, "Python"))
    {
        return(new PythonProvIFC::PythonProviderManager());
    }
    return(0);
}

