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
#include "PG_PyExtensions.h"
#include "PythonProviderManager.h"

using namespace Pegasus;

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
PyProviderEnvironment::PyProviderEnvironment(
		const OperationContext& opctx,
		PythonProviderManager* pmgr,
		const String& provPath)
	: Py::PythonExtension<PyProviderEnvironment>()
	, m_opctx(opctx)
	, m_pmgr(pmgr)
	, m_provPath(provPath)
{
}

//////////////////////////////////////////////////////////////////////////////
PyProviderEnvironment::~PyProviderEnvironment()
{
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getCIMOMHandle(
	const Py::Tuple& args)
{
	return PyCIMOMHandle::newObject(m_pmgr, m_provPath);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getLogger(
	const Py::Tuple& args)
{
	return PyLogger::newObject();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getUserName(
	const Py::Tuple& args)
{
	IdentityContainer container(m_opctx.get(IdentityContainer::NAME));
	String userName(container.getUserName());
	return Py::String(userName);
}

#if 0
//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getContextValue(
	const Py::Tuple& args)
{
	args.verify_length(1);
	String strKey;
	if (!args[0].isNone())
	{
		String strKey = Py::String(args[0]).as_ow_string();
	}
	if (strKey.empty())
	{
		throw Py::ValueError("'Key' parameter is required");
	}

	OperationContext &ctx = m_env->getOperationContext();
	String strVal;
	try
	{
		strVal = ctx.getStringData(strKey);
	}
	catch(const ContextDataNotFoundException&)
	{
		return Py::None();
	}
	return Py::String(strVal);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::setContextValue(
	const Py::Tuple& args)
{
	args.verify_length(2);
	if (args[0].isNone() || args[1].isNone())
	{
		throw Py::ValueError("Parameter 1 should be a key and "
			"parameter 2 should be its value");
	}

	String strKey = Py::String(args[0]).as_ow_string();
	String strVal = Py::String(args[1]).as_ow_string();
	if (strKey.empty() || strVal.empty())
	{
		throw Py::ValueError("Parameter 1 should be a key and "
			"parameter 2 should be its value");
	}
	OperationContext &ctx = m_env->getOperationContext();
	ctx.setStringData(strKey, strVal);
	return Py::Nothing();
}
#endif

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getCIMOMInfo(
	const Py::Tuple& args)
{
	Py::Tuple rt(2);
	rt[0] = Py::String("OpenPegasus");
	rt[1] = Py::String("Unknown");
	return rt;
}

//////////////////////////////////////////////////////////////////////////////
bool
PyProviderEnvironment::accepts(
	PyObject *pyob) const
{
	return pyob && PyProviderEnvironment::check(pyob);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::repr()
{
	return Py::String("Provider Environment");
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getattr(
	const char *name)
{
	return getattr_methods(name);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
void
PyProviderEnvironment::doInit()
{
	behaviors().name("ProviderEnvironment");
	behaviors().doc("ProviderEnvironment object are given to python providers "
		"when they are called to service a request. Other objects can be "
		"obtained through the ProviderEnvironement objects: CIMOMHandle, "
		"Logger, UserName and Context objects.");
	behaviors().supportRepr();
	behaviors().supportGetattr();

	add_varargs_method("get_cimom_handle", &PyProviderEnvironment::getCIMOMHandle,
		"Get the CIMOM handle object that can be used to make CIM requests "
		"to the hosting CIMOM");
	add_varargs_method("get_logger", &PyProviderEnvironment::getLogger,
		"Get the logger object that can be used to log to the CIMOM's logger");
	add_varargs_method("get_user_name", &PyProviderEnvironment::getUserName,
		"Get the name of the user making the CIM request");
#if 0
	add_varargs_method("get_context_value", &PyProviderEnvironment::getContextValue,
		"Get the string value associated with a given string key from the "
		"operation context");
	add_varargs_method("set_context_value", &PyProviderEnvironment::setContextValue,
		"Set a given string value for a given string key within the "
		"operation context");
#endif
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object 
PyProviderEnvironment::newObject(
	const OperationContext& opctx,
	PythonProviderManager* pmgr,
	const String& provPath,
	PyProviderEnvironment **penv)
{
	PyProviderEnvironment* ph = new PyProviderEnvironment(opctx, pmgr, provPath);
	if (penv)
	{
		*penv = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC


