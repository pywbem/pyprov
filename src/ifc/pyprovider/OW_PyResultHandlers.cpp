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
#include "OW_PyConverter.hpp"
#include <openwbem/OW_CIMInstance.hpp>
#include <openwbem/OW_CIMObjectPath.hpp>
#include <openwbem/OW_ResultHandlers.hpp>
#include <openwbem/OW_Format.hpp>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
PyInstanceResultHandler::PyInstanceResultHandler(
	CIMInstanceResultHandlerIFC& result,
	const String& ns)
	: Py::PythonExtension<PyInstanceResultHandler>()
	, m_result(result)
	, m_valid(true)
	, m_ns(ns)
{
}

//////////////////////////////////////////////////////////////////////////////
PyInstanceResultHandler::~PyInstanceResultHandler()
{
	invalidate();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyInstanceResultHandler::handle(
	const Py::Tuple& args)
{
	if (!m_valid)
	{
		throw Py::RuntimeError("InstanceResultHandler is no longer valid");
	}
	args.verify_length(1);
	if (args[0].isNone())
	{
		throw Py::TypeError("Invalid CIM instance given to InstanceResultHandler");
	}

	try
	{
		m_result.handle(OWPyConv::PyInst2OW(args[0], m_ns));
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Invalid CIM Instance given to "
			"InstanceResultHandler. Msg: %1", e.getMessage());
		throw Py::TypeError(msg.c_str());
	}

	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyInstanceResultHandler::call(
	const Py::Object& args,
	const Py::Object& kws)
{
	return handle(args);
}

//////////////////////////////////////////////////////////////////////////////
bool
PyInstanceResultHandler::accepts(
	PyObject *pyob) const
{
	return pyob && PyInstanceResultHandler::check(pyob);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyInstanceResultHandler::repr()
{
	return Py::String("Provider CIM Instance Result Handler");
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyInstanceResultHandler::getattr(
	const char *name)
{
	return getattr_methods(name);
}

//////////////////////////////////////////////////////////////////////////////
void
PyInstanceResultHandler::doInit()
{
	behaviors().name("InstanceResultHandler");
	behaviors().doc("InstanceResultHandlers are used by CIM providers "
		"to return instances from methods such as enumInstances and "
		"associators");
	behaviors().supportRepr();
	behaviors().supportGetattr();
	behaviors().supportCall();
	add_varargs_method("handle", &PyInstanceResultHandler::handle,
		"Return a CIM instance to the CIMOM");
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PyInstanceResultHandler::newObject(
	CIMInstanceResultHandlerIFC& result,
	const String& ns,
	PyInstanceResultHandler **presHandler)
{
	PyInstanceResultHandler* ph = new PyInstanceResultHandler(result, ns);
	if (presHandler)
	{
		*presHandler = ph;
	}

	return Py::asObject(ph);
}

//////////////////////////////////////////////////////////////////////////////
PyObjectPathResultHandler::PyObjectPathResultHandler(
	CIMObjectPathResultHandlerIFC& result,
	const String& ns)
	: Py::PythonExtension<PyObjectPathResultHandler>()
	, m_result(result)
	, m_valid(true)
	, m_ns(ns)
{
}

//////////////////////////////////////////////////////////////////////////////
PyObjectPathResultHandler::~PyObjectPathResultHandler()
{
	invalidate();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyObjectPathResultHandler::handle(
	const Py::Tuple& args)
{
	if (!m_valid)
	{
		throw Py::RuntimeError("ObjectPathResultHandler is no longer valid");
	}
	args.verify_length(1);
	if (args[0].isNone())
	{
		throw Py::TypeError("Invalid CIM Object Path given to "
			"ObjectPathResultHandler");
	}

	try
	{
		m_result.handle(OWPyConv::PyRef2OW(args[0], m_ns));
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Invalid CIM Object Path given to "
			"ObjectPathResultHandler. Msg: %1", e.getMessage());
		throw Py::TypeError(msg.c_str());
	}

	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyObjectPathResultHandler::call(
	const Py::Object& args,
	const Py::Object& kws)
{
	return handle(args);
}


//////////////////////////////////////////////////////////////////////////////
bool
PyObjectPathResultHandler::accepts(
	PyObject *pyob) const
{
	return pyob && PyObjectPathResultHandler::check(pyob);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyObjectPathResultHandler::repr()
{
	return Py::String("Provider CIM Instance Result Handler");
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyObjectPathResultHandler::getattr(
	const char *name)
{
	return getattr_methods(name);
}

//////////////////////////////////////////////////////////////////////////////
void
PyObjectPathResultHandler::doInit()
{
	behaviors().name("ObjectPathResultHandler");
	behaviors().doc("ObjectPathResultHandlers are used by CIM providers "
		"to return CIM Object paths (REFS) from methods such as "
		"enumInstanceNames and associatorNames");
	behaviors().supportRepr();
	behaviors().supportGetattr();
	behaviors().supportCall();
	add_varargs_method("handle", &PyObjectPathResultHandler::handle, 
		"Return a CIM Objectpath to the CIMOM");
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PyObjectPathResultHandler::newObject(
	CIMObjectPathResultHandlerIFC& result,
	const String& ns,
	PyObjectPathResultHandler **presHandler)
{
	PyObjectPathResultHandler* ph = new PyObjectPathResultHandler(result, ns);
	if (presHandler)
	{
		*presHandler = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC
