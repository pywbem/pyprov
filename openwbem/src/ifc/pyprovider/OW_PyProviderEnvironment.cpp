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
#include "OW_PyProviderModule.hpp"
#include <openwbem/OW_OperationContext.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

#define COMPONENT_NAME "python"

//////////////////////////////////////////////////////////////////////////////
PyProviderEnvironment::PyProviderEnvironment(
	const ProviderEnvironmentIFCRef& env)
	: Py::PythonExtension<PyProviderEnvironment>()
	, m_env(env)
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
	CIMOMHandleIFCRef chdl = m_env->getCIMOMHandle();
	return PyCIMOMHandle::newObject(chdl);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getLogger(
	const Py::Tuple& args)
{
	String component;
	if (args.length() && !args[0].isNone())
	{
		component = Py::String(args[0]).as_ow_string();
	}
	if (component.empty())
	{
		component = COMPONENT_NAME;
	}
	LoggerRef logger = m_env->getLogger(component);
	return PyLogger::newObject(logger);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getUserName(
	const Py::Tuple& args)
{
	return Py::String(m_env->getUserName());
}

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

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderEnvironment::getCIMOMInfo(
	const Py::Tuple& args)
{
	Py::Tuple rt(2);
	rt[0] = Py::String("OpenWBEM");
	rt[1] = Py::String(OW_VERSION);
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
	add_varargs_method("get_context_value", &PyProviderEnvironment::getContextValue,
		"Get the string value associated with a given string key from the "
		"operation context");
	add_varargs_method("set_context_value", &PyProviderEnvironment::setContextValue,
		"Set a given string value for a given string key within the "
		"operation context");
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PyProviderEnvironment::newObject(
	const ProviderEnvironmentIFCRef& env,
	PyProviderEnvironment **penv)
{
	PyProviderEnvironment* ph = new PyProviderEnvironment(env);
	if (penv)
	{
		*penv = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC


