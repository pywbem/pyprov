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
#include "PyMethodProviderHandler.h"
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
MethodProviderHandler::handleInvokeMethodRequest(
	CIMRequestMessage* message,
	PyProviderRef& provref,
	PythonProviderManager* pmgr)
{
    PEG_METHOD_ENTER(
        TRC_PROVIDERMANAGER,
        "PythonProviderManager::handleInvokeMethodRequest()");
    CIMInvokeMethodRequestMessage* request =
        dynamic_cast<CIMInvokeMethodRequestMessage *>(message);
    PEGASUS_ASSERT(request != 0 );

	AutoPtr<CIMInvokeMethodResponseMessage> response(
		dynamic_cast<CIMInvokeMethodResponseMessage*>(
			request->buildResponse()));
	PEGASUS_ASSERT(response.get() != 0);

	// create a handler for this request
	InvokeMethodResponseHandler handler(
		request, response.get(), pmgr->_responseChunkCallback);

	OperationContext ctx(request->operationContext);

	CIMObjectPath objectPath(
		System::getHostName(),
		request->nameSpace,
		request->instanceName.getClassName(),
		request->instanceName.getKeyBindings());

	CIMOMHandle chdl;
	CIMClass cc = chdl.getClass(ctx, request->nameSpace,
		request->instanceName.getClassName(), false, true, true,
		CIMPropertyList());

	Py::GILGuard gg;	// Acquire Python's GIL
	try
	{
		StatProviderTimeMeasurement providerTime(response.get());
		handler.processing();
		Uint32 i = cc.findMethod(request->methodName);
		if (i == PEG_NOT_FOUND)
		{
			THROWCIMMSG(CIM_ERR_METHOD_NOT_AVAILABLE,
				Formatter::format("Python provider $0 called with invalid "
					"method: $1", provref->m_path,
					request->methodName.getString()));
		}
		CIMMethod method = cc.getMethod(i);
		Py::Callable pyfunc = getFunction(provref->m_pyprov, "invokeMethod");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(request->operationContext,
			pmgr, provref->m_path);
		args[1] = PGPyConv::PGRef2Py(objectPath);
		args[2] = PGPyConv::PGMeth2Py(method);
		// Build input parameter dictionary
		Py::Dict aDict;
		Uint32 len = request->inParameters.size();
		for (i = 0; i < len; i++)
		{
			CIMParamValue pv = request->inParameters[i];
			CIMValue cv = pv.getValue();
			if (cv.isNull())
			{
				aDict[pv.getParameterName()] = Py::None();
			}
			else
			{
				aDict[pv.getParameterName()] = PGPyConv::PGVal2Py(cv);
			}
		}
		// Now add any missing parameters with a None value
		len = method.getParameterCount();
		for (i = 0; i < len; i++)
		{
			CIMParameter param = method.getParameter(i);
			String pname = param.getName().getString();
			if (aDict.hasKey(pname))
			{
				// Already have parameter
				continue;
			}

			// No see if it is an input param
			Uint32 ndx = param.findQualifier("IN");
			if (ndx == PEG_NOT_FOUND)
			{
				// 'IN' qualifier not specified.
				// According to DSP0004 assume default is TRUE
				aDict[pname] = Py::None();
				continue;
			}

			CIMQualifier qual = param.getQualifier(ndx);
			CIMValue cv = qual.getValue();
			if (cv.isNull())
			{
				// 'IN' qualifier not specified.
				// According to DSP0004 assume default is TRUE
				aDict[pname] = Py::None();
				continue;
			}

			Boolean isIn;
			cv.get(isIn);
			if (isIn)
			{
				// 'IN' qual specified and 'TRUE'
				aDict[pname] = Py::None();
			}
		}
		
		args[3] = aDict;
		Py::Object pyv = pyfunc.apply(args);
		if (!pyv.isTuple())
		{
			THROWCIMMSG(CIM_ERR_FAILED,
				Formatter::format("Python provider $0 did not return a valid "
					"value for invokeMethod on $1. Should be a tuple",
						provref->m_path, request->methodName.getString()));
		}
		// The tuple we are excpecting here is of the following format
		// Tuple[0] = subTuple(2): subTuple[0]: dataType name. subTuple[1]:data
		// Tuple[1] = Dict where key is the param name and value is a tuple
		// that looks like: tuple[0]:dataType tuple[1]:data
		Py::Tuple rt(pyv);
		if (rt.length() != 2)
		{
			THROWCIMMSG(CIM_ERR_FAILED, Formatter::format("Python provider $0 "
				"did not return tuple of length 2 on invokeMethod on $1",
					provref->m_path, request->methodName.getString()));
		}
		// Get the return value
		Py::Tuple vt(rt[0]);
		// Convert the return value to a CIMValue
		CIMValue rv = PGPyConv::PyVal2PG(vt);
		// Get the output parameter dictionary
		aDict = rt[1];
		if (aDict.length())
		{
			Py::List keys(aDict.keys());
			for (i = 0; i < keys.length(); i++)
			{
				String pname = Py::String(keys[i]).as_peg_string();
				// get the tuple that holds the parameter value
				vt = aDict[pname];
				if (vt.length() != 2)
				{
					THROWCIMMSG(CIM_ERR_FAILED, "Failed to convert output "
						"parameter from python invoke method. Expecting tuple of "
						"size 2");
				}
				if (!vt[0].isString())
				{
					THROWCIMMSG(CIM_ERR_FAILED, "Failed to convert output "
						"parameter from python invoke method. Expecting "
						"tuple of size 2 where element 0 is a string");
				}
				handler.deliverParamValue(CIMParamValue(pname,
					PGPyConv::PyVal2PG(vt)));
			}
		}
		handler.deliver(rv);
		handler.complete();
	}
	HANDLECATCH(handler, provref, getInstance)
    PEG_METHOD_EXIT();
    return response.release();
}

}	// End of namespace PythonProvIFC

