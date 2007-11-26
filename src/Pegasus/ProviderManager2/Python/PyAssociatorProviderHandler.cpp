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
#include "PyAssociatorProviderHandler.h"
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
AssociatorProviderHandler::handleAssociatorsRequest(
	CIMRequestMessage* message,
	PyProviderRef& provref,
	PythonProviderManager* pmgr)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleAssociatorsRequest()");

	CIMAssociatorsRequestMessage* request =
		dynamic_cast<CIMAssociatorsRequestMessage*>(message);
	PEGASUS_ASSERT(request != 0);

	AutoPtr<CIMAssociatorsResponseMessage> response(
		dynamic_cast<CIMAssociatorsResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	AssociatorsResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMObjectPath objectPath(
		System::getHostName(),
		request->nameSpace,
		request->objectName.getClassName());
	objectPath.setKeyBindings(request->objectName.getKeyBindings());

	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "associators");
		Py::Tuple args(7);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = Py::String(request->assocClass.getString());
		args[3] = Py::String(request->resultClass.getString());
		args[4] = Py::String(request->role);
		args[5] = Py::String(request->resultRole);
		args[6] = getPyPropertyList(request->propertyList);

		Py::Object wko = pyfunc.apply(args);
		PyObject* ito = PyObject_GetIter(wko.ptr());
		if (!ito)
		{
			PyErr_Clear();
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("associators for provider $0 is NOT ",
					"an iterable object", provref->m_path));
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
			throw Py::Exception();
		}
		handler.complete();
	}
	HANDLECATCH(handler, provref, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage*
AssociatorProviderHandler::handleAssociatorNamesRequest(
	CIMRequestMessage* message,
	PyProviderRef& provref,
	PythonProviderManager* pmgr)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleAssociatorNamesRequest()");
    CIMAssociatorNamesRequestMessage* request =
        dynamic_cast<CIMAssociatorNamesRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMAssociatorNamesResponseMessage> response(
		dynamic_cast<CIMAssociatorNamesResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	AssociatorNamesResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMObjectPath objectPath(
		System::getHostName(),
		request->nameSpace,
		request->objectName.getClassName());
	objectPath.setKeyBindings(request->objectName.getKeyBindings());

	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "associatorNames");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = Py::String(request->assocClass.getString());
		args[3] = Py::String(request->resultClass.getString());
		args[4] = Py::String(request->role);
		args[5] = Py::String(request->resultRole);
		Py::Object wko = pyfunc.apply(args);
		PyObject* ito = PyObject_GetIter(wko.ptr());
		if (!ito)
		{
			PyErr_Clear();
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("associatorNames for provider $0 is NOT "
					"an iterable object", provref->m_path));
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
			throw Py::Exception();
		}
		handler.complete();
	}
	HANDLECATCH(handler, provref, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage*
AssociatorProviderHandler::handleReferencesRequest(
	CIMRequestMessage* message,
	PyProviderRef& provref,
	PythonProviderManager* pmgr)
{
	Py::GILGuard gg;	// Acquire Python's GIL

    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleReferencesRequest()");
    CIMReferencesRequestMessage* request =
        dynamic_cast<CIMReferencesRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMReferencesResponseMessage> response(
		dynamic_cast<CIMReferencesResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	ReferencesResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMObjectPath objectPath(
		System::getHostName(),
		request->nameSpace,
		request->objectName.getClassName());
	objectPath.setKeyBindings(request->objectName.getKeyBindings());

	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "references");
		Py::Tuple args(5);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = Py::String(request->resultClass.getString());
		args[3] = Py::String(request->role);
		args[4] = getPyPropertyList(request->propertyList);
		Py::Object wko = pyfunc.apply(args);
		PyObject* ito = PyObject_GetIter(wko.ptr());
		if (!ito)
		{
			PyErr_Clear();
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("references for provider $0 is NOT "
					"an iterable object", provref->m_path));
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
			throw Py::Exception();
		}
		handler.complete();
	}
	HANDLECATCH(handler, provref, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

///////////////////////////////////////////////////////////////////////////////
CIMResponseMessage*
AssociatorProviderHandler::handleReferenceNamesRequest(
	CIMRequestMessage* message,
	PyProviderRef& provref,
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleReferenceNamesRequest()");
    CIMReferenceNamesRequestMessage* request =
        dynamic_cast<CIMReferenceNamesRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMReferenceNamesResponseMessage> response(
		dynamic_cast<CIMReferenceNamesResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	ReferenceNamesResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	CIMObjectPath objectPath(
		System::getHostName(),
		request->nameSpace,
		request->objectName.getClassName());
	objectPath.setKeyBindings(request->objectName.getKeyBindings());

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "referenceNames");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = Py::String(request->resultClass.getString());
		args[3] = Py::String(request->role);
		Py::Object wko = pyfunc.apply(args);
		PyObject* ito = PyObject_GetIter(wko.ptr());
		if (!ito)
		{
			PyErr_Clear();
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("referenceNames for provider $0 is NOT "
					"an iterable object", provref->m_path));
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
			throw Py::Exception();
		}
		handler.complete();
	}
	HANDLECATCH(handler, provref, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

} // end of namespace PythonProvIFC
