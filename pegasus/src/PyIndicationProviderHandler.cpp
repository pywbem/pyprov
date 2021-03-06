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
#include "PyIndicationProviderHandler.h"
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

CIMResponseMessage* 
IndicationProviderHandler::handleCreateSubscriptionRequest(
	CIMRequestMessage* message, 
	PyProviderRef& provref, 
	PythonProviderManager* pmgr)
{
	PEG_METHOD_ENTER(
		TRC_PROVIDERMANAGER,
		"PythonProviderManager::handleCreateSubscriptionRequest()");
	CIMCreateSubscriptionRequestMessage* request =
		dynamic_cast<CIMCreateSubscriptionRequestMessage *>(message);
	PEGASUS_ASSERT(request != 0 );
/*
    CIMNamespaceName nameSpace;
    CIMInstance subscriptionInstance;
    Array <CIMName> classNames;
    CIMPropertyList propertyList;
    Uint16 repeatNotificationPolicy;
    String query;

	String authType;
	String userName;
*/
	AutoPtr<CIMCreateSubscriptionResponseMessage> response(
		dynamic_cast<CIMCreateSubscriptionResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	OperationResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		Py::Object pyProv = provref->m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "activateFilter");
		Py::Tuple args(5);
		args[0] = PyProviderEnvironment::newObject(request->operationContext,
			pmgr, provref->m_path);
		args[1] = Py::String(request->query);
		args[2] = Py::String(request->nameSpace.getString());
		Py::List pyclasses;
		for (Uint32 i = 0; i < request->classNames.size(); i++)
		{
			pyclasses.append(Py::String(request->classNames[i].getString()));
		}
		args[3] = pyclasses;
		args[4] = Py::Bool(provref->m_activationCount == 1);// whether first activation or not
		pyfunc.apply(args);
	}
	HANDLECATCH(handler, provref, createSubscription)
	PEG_METHOD_EXIT();
	return response.release();
}

/*
CIMResponseMessage* 
IndicationProviderHandler::handleModifySubscriptionRequest(CIMRequestMessage* message, 
														   PyProviderRef& provref, 
														   PythonProviderManager* pmgr)
{
	PEG_METHOD_ENTER(
		TRC_PROVIDERMANAGER,
		"PythonProviderManager::handleModifySubscriptionRequest()");
	CIMModifySubscriptionRequestMessage* request =
		dynamic_cast<CIMModifySubscriptionRequestMessage *>(message);
	PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMModifySubscriptionResponseMessage> response(
		dynamic_cast<CIMModifySubscriptionResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	ModifySubscriptionResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();

		// do modify work here

		handler.complete();
	}
	HANDLECATCH(handler, provref, modifySubscription)
	PEG_METHOD_EXIT();
	return response.release();
}
*/

CIMResponseMessage* 
IndicationProviderHandler::handleDeleteSubscriptionRequest(CIMRequestMessage* message, 
														   PyProviderRef& provref, 
														   PythonProviderManager* pmgr)
{
	PEG_METHOD_ENTER(
		TRC_PROVIDERMANAGER,
		"PythonProviderManager::handleDeleteSubscriptionRequest()");
	CIMDeleteSubscriptionRequestMessage* request =
		dynamic_cast<CIMDeleteSubscriptionRequestMessage *>(message);
	PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMDeleteSubscriptionResponseMessage> response(
		dynamic_cast<CIMDeleteSubscriptionResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	OperationResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		//handler.processing();

		Py::Object pyProv = provref->m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "deactivateFilter");
		Py::Tuple args(5);
		args[0] = PyProviderEnvironment::newObject(request->operationContext,
			pmgr, provref->m_path);
		args[1] = Py::String();//request->query);
		args[2] = Py::String(request->nameSpace.getString());
		Py::List pyclasses;
		for (unsigned long i = 0; i < request->classNames.size(); i++)
		{
			pyclasses.append(Py::String(request->classNames[i].getString()));
		}
		args[3] = pyclasses;
		provref->m_activationCount--;
		args[4] = Py::Bool(provref->m_activationCount == 0);// whether last activation or not
		pyfunc.apply(args);

		//handler.complete();
	}
	HANDLECATCH(handler, provref, deleteSubscription)
	PEG_METHOD_EXIT();
	return response.release();
}

}	// End of namespace PythonProvIFC

