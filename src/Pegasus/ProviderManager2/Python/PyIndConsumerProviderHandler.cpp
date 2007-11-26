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
	PyProviderRef& provref,
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
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "handleIndication");
		Py::Tuple args(3);
		args[0] = PyProviderEnvironment::newObject(request->operationContext); 	// Provider Environment
		args[1] = Py::String(request->destinationPath);	
		args[2] = PGPyConv::PGInst2Py(request->indicationInstance);
		Py::Object pyv = pyfunc.apply(args);
	}
	HANDLECATCH(handler, provref, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

}	// End of namespace PythonProvIFC

