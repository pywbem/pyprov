#include "PG_PyProvIFCCommon.h"
#include "PyIndConsumerProviderHandler.h"
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
IndicationConsumerProviderHandler::handleExportIndicationRequest(
	CIMRequestMessage* message, 
	PyProviderRep& provrep,
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleExportIndicationRequest()");
    CIMExportIndicationRequestMessage* request =
        dynamic_cast<CIMExportIndicationRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMExportIndicationResponseMessage> response(
		dynamic_cast<CIMExportIndicationResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	OperationResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		Py::Callable pyfunc = getFunction(provrep.m_pyprov, "handleIndication");
		Py::Tuple args(3);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = Py::String(request->destinationPath);	
		args[2] = PGPyConv::PGInst2Py(request->indicationInstance);
		Py::Object pyv = pyfunc.apply(args);
	}
	HANDLECATCH(handler, provrep, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

}	// End of namespace PythonProvIFC

