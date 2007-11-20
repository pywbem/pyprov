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
#include "PyInstanceProviderHandler.h"
#include "PG_PyConverter.h"

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
#include <Pegasus/Config/ConfigManager.h>
#include <Pegasus/Provider/CIMOMHandleQueryContext.h>
#include <Pegasus/ProviderManager2/CIMOMHandleContext.h>
#include <Pegasus/ProviderManager2/ProviderName.h>
#include <Pegasus/ProviderManager2/AutoPThreadSecurity.h>

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

namespace PythonProvIFC
{

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* 
InstanceProviderHandler::handleGetInstanceRequest(
	CIMRequestMessage* message, 
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleGetInstanceRequest()");

	CIMGetInstanceRequestMessage* request =
		dynamic_cast<CIMGetInstanceRequestMessage*>(message);
	PEGASUS_ASSERT(request != 0);

	AutoPtr<CIMGetInstanceResponseMessage> response(
		dynamic_cast<CIMGetInstanceResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	GetInstanceResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMObjectPath objectPath(
		System::getHostName(),
		request->nameSpace,
		request->instanceName.getClassName(),
		request->instanceName.getKeyBindings());

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->instanceName.getClassName(), false, true, true,
		CIMPropertyList());

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Callable pyfunc = getFunction(provrep.m_pyprov, "getInstance");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = getPyPropertyList(request->propertyList);
		args[3] = PGPyConv::PGClass2Py(cc);
		Py::Object pyci = pyfunc.apply(args);
		if (pyci.isNone())
		{
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("Error: Python provider $0 returned NONE "
					"on getInstance", provrep.m_path));
		}
		handler.deliver(PGPyConv::PyInst2PG(pyci,
			request->nameSpace.getString()));
		handler.complete();
	}
	HANDLECATCH(handler, provrep, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* 
InstanceProviderHandler::handleEnumerateInstancesRequest(
	CIMRequestMessage* message,
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleEnumerateInstanceRequest()");

    CIMEnumerateInstancesRequestMessage* request =
        dynamic_cast<CIMEnumerateInstancesRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMEnumerateInstancesResponseMessage> response(
		dynamic_cast<CIMEnumerateInstancesResponseMessage*>(
			request->buildResponse()));

	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	EnumerateInstancesResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->className, false, true, true, CIMPropertyList());

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "enumInstances");
		Py::Tuple args(5);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = Py::String(request->nameSpace.getString());							// Namespace
		args[2] = getPyPropertyList(request->propertyList);
		args[3] = PGPyConv::PGClass2Py(cc);
		args[4] = PGPyConv::PGClass2Py(cc);

		StatProviderTimeMeasurement providerTime(response.get());

		handler.processing();

		Py::Object wko = pyfunc.apply(args);
		PyObject* ito = PyObject_GetIter(wko.ptr());
		if (!ito)
		{
			PyErr_Clear();
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("enumInstances for provider $0 is NOT "
					"an iterable object", provrep.m_path));
		}
		Py::Object iterable(ito, true);	// Let Py::Object manage the ref count
		PyObject* item;
		while((item = PyIter_Next(ito)))
		{
			wko = Py::Object(item, true);
			handler.deliver(PGPyConv::PyInst2PG(wko, request->nameSpace.getString()));
		}
		if (PyErr_Occurred())
		{
			std::cerr << "PyErr_Occurred" << std::endl;
			throw Py::Exception();
		}
		handler.complete();
	}
	HANDLECATCH(handler, provrep, enumInstances)
	PEG_METHOD_EXIT();
	return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* 
InstanceProviderHandler::handleEnumerateInstanceNamesRequest(
	CIMRequestMessage* message, 
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
	PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleEnumerateInstanceNamesRequest()");

	CIMEnumerateInstanceNamesRequestMessage* request =
		dynamic_cast<CIMEnumerateInstanceNamesRequestMessage*>(message);
	PEGASUS_ASSERT(request != 0);

	AutoPtr<CIMEnumerateInstanceNamesResponseMessage> response(
		dynamic_cast<CIMEnumerateInstanceNamesResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	EnumerateInstanceNamesResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->className, false, true, true, CIMPropertyList());

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();

		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "enumInstanceNames");
		Py::Tuple args(3);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = Py::String(request->nameSpace.getString());							// Namespace
		args[2] = PGPyConv::PGClass2Py(cc);

		Py::Object wko = pyfunc.apply(args);
		PyObject* ito = PyObject_GetIter(wko.ptr());
		if (!ito)
		{
			PyErr_Clear();
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("enumInstanceNames for provider $0 is NOT "
					"an iterable object", provrep.m_path));
		}
		Py::Object iterable(ito, true);	// Let Py::Object manage the ref count
		PyObject* item;
		while((item = PyIter_Next(ito)))
		{
			wko = Py::Object(item, true);
			handler.deliver(PGPyConv::PyRef2PG(wko, request->nameSpace.getString()));
		}
		if (PyErr_Occurred())
		{
			std::cerr << "PyErr_Occurred" << std::endl;
			throw Py::Exception();
		}
		handler.complete();
	}
	HANDLECATCH(handler, provrep, enumInstanceNames)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* 
InstanceProviderHandler::handleCreateInstanceRequest(
	CIMRequestMessage* message, 
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleCreateInstanceRequest()");

    CIMCreateInstanceRequestMessage* request =
        dynamic_cast<CIMCreateInstanceRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMCreateInstanceResponseMessage> response(
		dynamic_cast<CIMCreateInstanceResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	CreateInstanceResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->newInstance.getClassName(), false, true, true,
		CIMPropertyList());
	CIMObjectPath objectPath = request->newInstance.buildPath(cc);
	objectPath.setNameSpace(request->nameSpace);
	objectPath.setHost(System::getHostName());
	request->newInstance.setPath(objectPath);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "createInstance");
		Py::Tuple args(2);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGInst2Py(request->newInstance, request->nameSpace.getString());
		Py::Object pycop = pyfunc.apply(args);
		if (pycop.isNone())
		{
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("Error: Python provider: $0 returned NONE "
				"on createInstance", provrep.m_path));
		}
		handler.deliver(PGPyConv::PyRef2PG(pycop));
		handler.complete();
	}
	HANDLECATCH(handler, provrep, createInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* 
InstanceProviderHandler::handleModifyInstanceRequest(
	CIMRequestMessage* message, 
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleModifyInstanceRequest()");

    CIMModifyInstanceRequestMessage* request =
        dynamic_cast<CIMModifyInstanceRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMModifyInstanceResponseMessage> response(
		dynamic_cast<CIMModifyInstanceResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	ModifyInstanceResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->modifiedInstance.getClassName(), false, true, true,
		CIMPropertyList());

	CIMObjectPath objectPath = request->modifiedInstance.buildPath(cc);
	objectPath.setNameSpace(request->nameSpace);
	objectPath.setHost(System::getHostName());
	request->modifiedInstance.setPath(objectPath);

	CIMInstance prevInstance = chdl.getInstance(OperationContext(),
		request->nameSpace, objectPath, false, true, true, CIMPropertyList());
	prevInstance.setPath(objectPath);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "modifyInstance");

		Py::Tuple args(5);
		String ns = request->nameSpace.getString();
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGInst2Py(request->modifiedInstance, ns);
		args[2] = PGPyConv::PGInst2Py(prevInstance, ns);
		args[3] = getPyPropertyList(request->propertyList);
		args[4] = PGPyConv::PGClass2Py(cc);
		pyfunc.apply(args);
		handler.complete();
	}
	HANDLECATCH(handler, provrep, modifyInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage* 
InstanceProviderHandler::handleDeleteInstanceRequest(
	CIMRequestMessage* message, 
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleDeleteInstanceRequest()");

    CIMDeleteInstanceRequestMessage* request =
        dynamic_cast<CIMDeleteInstanceRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMDeleteInstanceResponseMessage> response(
		dynamic_cast<CIMDeleteInstanceResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	DeleteInstanceResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "deleteInstance");
		Py::Tuple args(2);
		String ns = request->nameSpace.getString();
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(request->instanceName);
		pyfunc.apply(args);
		handler.complete();
	}
	HANDLECATCH(handler, provrep, deleteInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage*
InstanceProviderHandler::handleGetPropertyRequest(
	CIMRequestMessage* message,
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleGetPropertyRequest()");

    CIMGetPropertyRequestMessage* request =
        dynamic_cast<CIMGetPropertyRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMGetPropertyResponseMessage> response(
		dynamic_cast<CIMGetPropertyResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	GetPropertyResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	// Do GetProperty through getInstance

    CIMObjectPath objectPath(
        System::getHostName(),
        request->nameSpace,
        request->instanceName.getClassName(),
        request->instanceName.getKeyBindings());

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->instanceName.getClassName(), false, true, true,
		CIMPropertyList());

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Callable pyfunc = getFunction(provrep.m_pyprov, "getInstance");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = Py::None();
		args[3] = PGPyConv::PGClass2Py(cc);
		Py::Object pyci = pyfunc.apply(args);
		if (pyci.isNone())
		{
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("Error: Python provider $0 returned NONE "
					"on getInstance", provrep.m_path));
		}
		CIMInstance ci = PGPyConv::PyInst2PG(pyci, request->nameSpace.getString());
		gg.release();
		Uint32 ndx = ci.findProperty(request->propertyName);
		if (ndx == PEG_NOT_FOUND)
		{
			THROWCIMMSG(CIM_ERR_NO_SUCH_PROPERTY,
				Formatter::format("Error: Python provider $0 did not return "
					"property $1", provrep.m_path,
					request->propertyName.getString()));
		}
		CIMProperty prop = ci.getProperty(ndx);
		handler.deliver(prop.getValue());
		handler.complete();
	}
	HANDLECATCH(handler, provrep, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage*
InstanceProviderHandler::handleSetPropertyRequest(
	CIMRequestMessage* message,
	PyProviderRep& provrep, 
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleSetPropertyRequest()");

    CIMSetPropertyRequestMessage* request =
        dynamic_cast<CIMSetPropertyRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMSetPropertyResponseMessage> response(
		dynamic_cast<CIMSetPropertyResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	SetPropertyResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	// Do SetProperty through modifyInstance
	//
    CIMObjectPath objectPath(
        System::getHostName(),
        request->nameSpace,
        request->instanceName.getClassName(),
        request->instanceName.getKeyBindings());

	// Build modified instance for call
    CIMInstance instance(request->instanceName.getClassName());
    instance.addProperty(CIMProperty(
        request->propertyName, request->newValue));
    instance.setPath(objectPath);

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(OperationContext(), request->nameSpace,
		request->instanceName.getClassName(), false, true, true,
		CIMPropertyList());

	CIMInstance prevInstance = chdl.getInstance(OperationContext(),
		request->nameSpace, objectPath, false, true, true, CIMPropertyList());
	prevInstance.setPath(objectPath);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "modifyInstance");

		Py::Tuple args(5);
		String ns = request->nameSpace.getString();
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGInst2Py(instance, ns);
		args[2] = PGPyConv::PGInst2Py(prevInstance, ns);
		Py::List pList;
		pList.append(Py::String(request->propertyName.getString()));
		args[3] = pList;
		args[4] = PGPyConv::PGClass2Py(cc);
		pyfunc.apply(args);
		handler.complete();
	}
	HANDLECATCH(handler, provrep, setProperty)
    PEG_METHOD_EXIT();
    return response.release();
}

}	// End of namespace PythonProvIFC

