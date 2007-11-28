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
	String msg = Formatter::format("Thrown from Python provider: $0", provPath);
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
		// TESTING
		Py::Object etype1, evalue1;
		bool isCIMExc = PyErr_ExceptionMatches(g_cimexobj.ptr());
		String tb1 = LogPyException(thrownEx, __FILE__, lineno, etype1,
			evalue1, false);
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

	pHandler->setCIMException(CIMException(CIMStatusCode(errval),
		Formatter::format("$0 File: $1  Line: $2",
			msg, __FILE__, lineno)));
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
	cerr << "*****In PythonProviderManager::ctor" << endl;
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
	cerr << "*****In PythonProviderManager::~dtor. pid: " << ::getpid() << endl;
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::~PythonProviderManager()");
	ProviderMap::iterator it = m_provs.begin();
	while(it != m_provs.end())
	{
		try
		{
			_shutdownProvider(it->second, OperationContext());
		}
		catch(...)
		{
			// Ignore
		}
		it++;
	}
	PyEval_AcquireLock();
	PyThreadState_Swap(m_mainPyThreadState);
	Py_Finalize();
    PEG_METHOD_EXIT();
}

///////////////////////////////////////////////////////////////////////////////
Py::Object
PythonProviderManager::_loadProvider(
	const String& provPath,
	const OperationContext& opctx)
{
	try
	{
		Py::Object cim_provider = m_pywbemMod.getAttr("cim_provider"); 
		Py::Callable ctor = cim_provider.getAttr("ProviderProxy");
		Py::Tuple args(2);
		args[0] = PyProviderEnvironment::newObject(opctx, this, provPath);
		args[1] = Py::String(provPath);
		// Construct a CIMProvider python object
		Py::Object pyprov = ctor.apply(args);
		return pyprov;
	}
	catch(Py::Exception& e)
	{
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught exception loading provider $0.",
			provPath);
		String tb = processPyException(e, __LINE__, provPath, false);
		String msg = "Python Load Error: " + tb;
		THROW_NOSUCHPROV_EXC(msg);
	}
	return Py::None();
}


///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_shutdownProvider(
	const PyProviderRef& provref,
	const OperationContext& opctx)
{
	Py::GILGuard gg;	// Acquire python's GIL
	try
	{
cerr << "******* about to call 'shutdown' (for reload?  or for shutdown?) *****" << endl;
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "shutdown");
		if (!pyfunc.isCallable())
		{
			return;
		}
		Py::Tuple args(1);
		args[0] = PyProviderEnvironment::newObject(opctx, this, provref->m_path);
	    pyfunc.apply(args);
cerr << "******* Done with 'shutdown' and cleanup *****" << endl;
	}
	catch(Py::Exception& e)
	{
cerr << "******* Got exception unloading provider *****" << endl;
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught python exception invoking 'shutdown' provider $0.",
			provref->m_path);
		String tb = processPyException(e, __LINE__, provref->m_path);
		String msg = "Python Unload Error: " + tb;
//cerr << (const char *)msg.getCString() << endl;
cerr << msg << endl;
	}
	catch(...)
	{
cerr << "******* Got unknown exception unloading provider *****" << endl;
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught unknown exception invoking 'shutdown' provider $0.",
			provref->m_path);
		String msg = "Python Unload Error: Unknown error";
cerr << msg << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////
PyProviderRef
PythonProviderManager::_path2PyProviderRef(
	const String& provPath,
	const OperationContext& opctx)
{
	AutoMutex am(g_provGuard);

cerr << "*** in _path2PyProviderRef" << endl;
	ProviderMap::iterator it = m_provs.find(provPath);
	if (it != m_provs.end())
	{
		it->second->m_lastAccessTime = ::time(NULL);
        time_t curModTime = getModTime(provPath);
        if ((curModTime <= it->second->m_fileModTime)
            || (it->second->m_canUnload == false) )
        {
cerr << "*** _path2PyProviderRef Found EXISTING returning" << endl;
            // not modified, or can't reload... return it
            return it->second;
        }
        else
        {
            //cleanup for reload on fall-thru
			try
			{
				_shutdownProvider(it->second, opctx);
			}
			catch(...)
			{
				// Ignore?
			}
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
cerr << "*** _path2PyProviderRef Created NEW returning" << endl;
		return entry;
	}
	catch(Py::Exception& e)
	{
cerr << "*** _path2PyProviderRef caught Py::exception" << endl;
		Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"ProviderManager.Python.PythonProviderManager",
			"Caught exception loading provider $0.",
			provPath);
		String tb = processPyException(e, __LINE__, provPath);
		String msg = "Python Load Error: " + tb;
		THROW_NOSUCHPROV_EXC(msg);
	}

	// Shouldn't hit this
	return PyProviderRef(0);
}

///////////////////////////////////////////////////////////////////////////////
Message*
PythonProviderManager::processMessage(Message * message)
{
	cerr << "*****In PythonProviderManager::processMessage" << endl;
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::processMessage()");

    CIMRequestMessage* request = dynamic_cast<CIMRequestMessage*>(message);
    PEGASUS_ASSERT(request != 0);

	ProviderIdContainer providerId =
		request->operationContext.get(ProviderIdContainer::NAME);

	ProviderName name = _resolveProviderName(providerId);

	// If provider doesn't exist this call throws PyNoSuchProviderException
	PyProviderRef provRef = _path2PyProviderRef(name.getLocation(),
		request->operationContext);

	// At this point we know we have a provider
	CIMResponseMessage* response = 0;

	// pass the request message to a handler method based on message type
	switch (request->getType())
	{
		case CIM_GET_INSTANCE_REQUEST_MESSAGE:
			cerr << "CIM_GET_INSTANCE_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleGetInstanceRequest(request, provRef, this);
			break;

		case CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE:
			cerr << "CIM_ENUMERATE_INSTANCES_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleEnumerateInstancesRequest(request, provRef, this);
			break;

		case CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE:
			cerr << "CIM_ENUMERATE_INSTANCE_NAMES_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleEnumerateInstanceNamesRequest(request, provRef, this);
			break;

		case CIM_CREATE_INSTANCE_REQUEST_MESSAGE:
			cerr << "CIM_CREATE_INSTANCE_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleCreateInstanceRequest(request, provRef, this);
			break;

		case CIM_MODIFY_INSTANCE_REQUEST_MESSAGE:
			cerr << "CIM_MODIFY_INSTANCE_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleModifyInstanceRequest(request, provRef, this);
			break;

		case CIM_DELETE_INSTANCE_REQUEST_MESSAGE:
			cerr << "CIM_DELETE_INSTANCE_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleDeleteInstanceRequest(request, provRef, this);
			break;

		case CIM_GET_PROPERTY_REQUEST_MESSAGE:
			cerr << "CIM_GET_PROPERTY_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleGetPropertyRequest(request, provRef, this);
			break;

		case CIM_SET_PROPERTY_REQUEST_MESSAGE:
			cerr << "CIM_SET_PROPERTY_REQUEST_MESSAGE" << endl;
			response = InstanceProviderHandler::handleSetPropertyRequest(request, provRef, this);
			break;

		case CIM_INVOKE_METHOD_REQUEST_MESSAGE:
			cerr << "CIM_INVOKE_METHOD_REQUEST_MESSAGE" << endl;
			response = MethodProviderHandler::handleInvokeMethodRequest(request, provRef, this);
			break;

		case CIM_ASSOCIATORS_REQUEST_MESSAGE:
			cerr << "CIM_ASSOCIATORS_REQUEST_MESSAGE" << endl;
			response = AssociatorProviderHandler::handleAssociatorsRequest(request, provRef, this);
			break;

		case CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE:
			cerr << "CIM_ASSOCIATOR_NAMES_REQUEST_MESSAGE" << endl;
			response = AssociatorProviderHandler::handleAssociatorNamesRequest(request, provRef, this);
			break;

		case CIM_REFERENCES_REQUEST_MESSAGE:
			cerr << "CIM_REFERENCES_REQUEST_MESSAGE" << endl;
			response = AssociatorProviderHandler::handleReferencesRequest(request, provRef, this);
			break;
		case CIM_REFERENCE_NAMES_REQUEST_MESSAGE:
			cerr << "CIM_REFERENCE_NAMES_REQUEST_MESSAGE" << endl;
			response = AssociatorProviderHandler::handleReferenceNamesRequest(request, provRef, this);
			break;

		case CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE:
			cerr << "CIM_CREATE_SUBSCRIPTION_REQUEST_MESSAGE" << endl;
			_incActivationCount(request, provRef);
			response = IndicationProviderHandler::handleCreateSubscriptionRequest(request, provRef, this);
			break;

		case CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE:
			cerr << "CIM_DELETE_SUBSCRIPTION_REQUEST_MESSAGE" << endl;
			_decActivationCount(request, provRef);
			response = IndicationProviderHandler::handleDeleteSubscriptionRequest(request, provRef, this);
			break;

		case CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE:
			cerr << "CIM_MODIFY_SUBSCRIPTION_REQUEST_MESSAGE" << endl;
			response = _handleModifySubscriptionRequest (request, provRef);
			break;

		case CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE:
			cerr << "CIM_SUBSCRIPTION_INIT_COMPLETE_REQUEST_MESSAGE" << endl;
			response = _handleSubscriptionInitCompleteRequest (request, provRef);
			break;

		case CIM_EXPORT_INDICATION_REQUEST_MESSAGE:
			cerr << "CIM_EXPORT_INDICATION_REQUEST_MESSAGE" << endl;
			response = IndicationConsumerProviderHandler::handleExportIndicationRequest(
				request, provRef, this);
			break;

		case CIM_EXEC_QUERY_REQUEST_MESSAGE:
			cerr << "CIM_EXEC_QUERY_REQUEST_MESSAGE" << endl;
			// TODO
			response = _handleExecQueryRequest(request, provRef);
			break;

		case CIM_DISABLE_MODULE_REQUEST_MESSAGE:
			cerr << "CIM_DISABLE_MODULE_REQUEST_MESSAGE" << endl;
			// TODO
			response = _handleDisableModuleRequest(request, provRef);
			break;

		case CIM_ENABLE_MODULE_REQUEST_MESSAGE:
			cerr << "CIM_ENABLE_MODULE_REQUEST_MESSAGE" << endl;
			// TODO
			response = _handleEnableModuleRequest(request, provRef);
			break;

		case CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE:
			cerr << "CIM_STOP_ALL_PROVIDERS_REQUEST_MESSAGE" << endl;
			// TODO
			response = _handleStopAllProvidersRequest(request, provRef);
			break;

// Note: The PG_Provider AutoStart property is not yet supported
#if 0
		case CIM_INITIALIZE_PROVIDER_REQUEST_MESSAGE:
			response = _handleInitializeProviderRequest(request, provRef);
			break;
#endif
		default:
			cerr << "!!!!! UNKNOWN MESSAGE" << endl;
			response = _handleUnsupportedRequest(request, provRef);
			break;
	}
    PEG_METHOD_EXIT();
	cerr << "processMessage returning response" << endl;
    return(response);
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_incActivationCount(
	CIMRequestMessage* message,
	PyProviderRef& provref)
{
cerr << "!!! _incActionCount called with provider: " << provref->m_path << endl;

	AutoMutex am(g_provGuard);
	provref->m_activationCount++;
	if (provref->m_pIndicationResponseHandler)
	{
cerr << "!!! _incActionCount already has response handler. Just returning" << endl;
		return;
	}

	CIMCreateSubscriptionRequestMessage* request =
		dynamic_cast<CIMCreateSubscriptionRequestMessage*>(message);
cerr << "!!! _incActionCount trace 1" << endl;
	PEGASUS_ASSERT(request != 0);
cerr << "!!! _incActionCount trace 2" << endl;

	//  Save the provider instance from the request
    ProviderIdContainer pidc = (ProviderIdContainer)
        request->operationContext.get(ProviderIdContainer::NAME);
	provref->m_provInstance = pidc.getProvider();
cerr << "!!! _incActionCount setting response handler" << endl;
	provref->m_pIndicationResponseHandler = 
		new EnableIndicationsResponseHandler(
			0,    // request
			0,    // response
			provref->m_provInstance,
			_indicationCallback,
			_responseChunkCallback);
cerr << "!!! _incActionCount response handler: " << (void*) provref->m_pIndicationResponseHandler << endl;
cerr << "!!! _incActionCount returning" << endl;
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::_decActivationCount(
	CIMRequestMessage* message,
	PyProviderRef& provref)
{
	AutoMutex am(g_provGuard);
	ProviderMap::iterator it = m_provs.find(provref->m_path);
	if (it != m_provs.end())
	{
		it->second->m_activationCount--;
		provref->m_activationCount = it->second->m_activationCount;
	}
}

///////////////////////////////////////////////////////////////////////////////
void
PythonProviderManager::generateIndication(
	const String& provPath,
	const CIMInstance& indicationInstance)
{
cerr << "!!!! ProviderManager generateIndication called with provider: " << provPath << endl;
	AutoMutex am(g_provGuard);
	ProviderMap::iterator it = m_provs.find(provPath);
	if (it == m_provs.end())
	{
cerr << "!!!! ProviderManager generateIndication DID NOT FIND PROVIDER" << endl;
		return;
	}
	PyProviderRef pref = it->second;
cerr << "!!!! ProviderManager generateIndication found provider: " << pref->m_path << endl;
cerr << "!!!! ProviderManager response handler: " << (void*) pref->m_pIndicationResponseHandler << endl;
	if (!(pref->m_pIndicationResponseHandler))
	{
cerr << "!!!! ProviderManager generateIndication NO RESPONSE HANDLER" << endl;
		return;
	}

	pref->m_pIndicationResponseHandler->deliver(
		CIMIndication(indicationInstance));
cerr << "!!!! ProviderManager generateIndication returning" << endl;
}

///////////////////////////////////////////////////////////////////////////////
Boolean PythonProviderManager::hasActiveProviders()
{
	// TODO
	cerr << "**** hasActiveProviders called pid: " << ::getpid() << endl;
	//return false;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
void PythonProviderManager::unloadIdleProviders()
{
	// TODO
	cerr << "**** unloadIdleProviders called pid: " << ::getpid() << endl;
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

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleDisableModuleRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleDisableModuleRequest()");
    CIMRequestMessage* request =
        dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
    response->cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED,
				"DisableModuleRequest not yet implemented");
    PEG_METHOD_EXIT();
    return response;
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleEnableModuleRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleEnableModuleRequest()");
    CIMRequestMessage* request =
        dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
    response->cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED,
				"EnableModuleRequest not yet implemented");
    PEG_METHOD_EXIT();
    return response;
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* PythonProviderManager::_handleStopAllProvidersRequest(
    CIMRequestMessage* message,
	PyProviderRef& provref)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::_handleStopAllProvidersRequest()");
    CIMRequestMessage* request =
        dynamic_cast<CIMRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

    CIMResponseMessage* response = request->buildResponse();
    response->cimException =
        PEGASUS_CIM_EXCEPTION(CIM_ERR_NOT_SUPPORTED,
				"StopAllProviders not yet implemented");
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
cerr << "$$$$$ Setting _subscriptionInitComplete = true $$$$$" << endl;
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
	//cerr << "cfgProvDir : " << provDir << endl;

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
cerr << "######## moduleName: " << moduleName << endl;

    genericValue = providerId.getProvider().getProperty(
        providerId.getProvider().findProperty("Name")).getValue();
    genericValue.get(providerName);
cerr << "######## providerName: " << providerName << endl;

    genericValue = providerId.getModule().getProperty(
        providerId.getModule().findProperty("Location")).getValue();
    genericValue.get(location);
cerr << "######## location: " << location << endl;
	cerr << "About to resolvePhysicalName for : " << location << endl;
    fileName = _resolvePhysicalName(location);
cerr << "######## fileName: " << fileName << endl;

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
	cerr << "*****In PythonProviderManager::_initPython" << endl;
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
		// Explicitly importing 'threading' right here seems to get rid of the
		// atexit error we get when exiting after a provider has imported
		// threading. Might have something to do with importing threading
		// from the main thread...
		//Py::Module tmod = Py::Module("threading", true);	// Explicity import threading

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
		//m_disabled = true;
		cerr << "*****In PythonProviderManager::_initPython... caught exception loading pywbem module" << endl;
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
    
