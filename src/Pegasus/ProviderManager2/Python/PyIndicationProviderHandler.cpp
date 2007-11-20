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
IndicationProviderHandler::handleCreateSubscriptionRequest(CIMRequestMessage* message, 
														   PyProviderRep& provrep, 
														   PythonProviderManager* pmgr)
{
	cout << "**** handleCreateSubscriptionRequest called..." << endl;

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
		//handler.processing();

		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "activate_filter");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = Py::String(request->query);
		// TODO: What to do with event type... I don't get that from Pegasus request
		args[2] = Py::String();//request->eventType);
		args[3] = Py::String(request->nameSpace.getString());
		Py::List pyclasses;
		for (unsigned long i = 0; i < request->classNames.size(); i++)
		{
			pyclasses.append(Py::String(request->classNames[i].getString()));
		}
		args[4] = pyclasses;
		provrep.m_activationCount++;
		args[5] = Py::Bool(provrep.m_activationCount == 1);// whether first activation or not
		pyfunc.apply(args);

		//handler.complete();
	}
	HANDLECATCH(handler, provrep, createSubscription)
	PEG_METHOD_EXIT();
	return response.release();
}

/*
CIMResponseMessage* 
IndicationProviderHandler::handleModifySubscriptionRequest(CIMRequestMessage* message, 
														   PyProviderRep& provrep, 
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
	HANDLECATCH(handler, provrep, modifySubscription)
	PEG_METHOD_EXIT();
	return response.release();
}
*/

CIMResponseMessage* 
IndicationProviderHandler::handleDeleteSubscriptionRequest(CIMRequestMessage* message, 
														   PyProviderRep& provrep, 
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

		Py::Object pyProv = provrep.m_pyprov;
		Py::Callable pyfunc = getFunction(pyProv, "deactivate_filter");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = Py::String();//request->query);
		// TODO: What to do with event type... I don't get that from Pegasus request
		args[2] = Py::String();//request->eventType);
		args[3] = Py::String(request->nameSpace.getString());
		Py::List pyclasses;
		for (unsigned long i = 0; i < request->classNames.size(); i++)
		{
			pyclasses.append(Py::String(request->classNames[i].getString()));
		}
		args[4] = pyclasses;
		provrep.m_activationCount--;
		args[5] = Py::Bool(provrep.m_activationCount == 0);// whether last activation or not
		pyfunc.apply(args);

		//handler.complete();
	}
	HANDLECATCH(handler, provrep, deleteSubscription)
	PEG_METHOD_EXIT();
	return response.release();
}

}	// End of namespace PythonProvIFC

