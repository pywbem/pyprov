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
#include "PyCxxResultHandlers.hpp"
#include "Peg_PyConverter.hpp"
//#include <openwbem/OW_CIMInstance.hpp>
//#include <openwbem/OW_CIMObjectPath.hpp>
//#include <openwbem/OW_ResultHandlers.hpp>

//using namespace OW_NAMESPACE;

//////////////////////////////////////////////////////////////////////////////
PyInstanceResultHandler::PyInstanceResultHandler(
	CIMInstanceResultHandlerIFC& result)
	: Py::PythonExtension<PyInstanceResultHandler>()
	, m_result(result)
{
}

//////////////////////////////////////////////////////////////////////////////
PyInstanceResultHandler::~PyInstanceResultHandler()
{
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyInstanceResultHandler::handle(
	const Py::Tuple& args)
{
	args.verify_length(1);
	CIMInstance convci = OWPyConv::PyInst2OW(args[0]);
	m_result.handle(convci);
	return Py::Nothing();
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
	behaviors().name("ObjectPathResultHandler");
	behaviors().doc("ObjectPathResultHandlers are used by CIM providers "
		"to return CIM Object paths (REFS) from methods such as "
		"enumInstanceNames and associatorNames");
	behaviors().supportRepr();
	behaviors().supportGetattr();
	add_varargs_method("handle", 
	&PyInstanceResultHandler::handle, "Return a CIM Objectpath to the "
		"CIMOM");
}

//////////////////////////////////////////////////////////////////////////////
PyObjectPathResultHandler::PyObjectPathResultHandler(
	CIMObjectPathResultHandlerIFC& result)
	: Py::PythonExtension<PyObjectPathResultHandler>()
	, m_result(result)
{
}

//////////////////////////////////////////////////////////////////////////////
PyObjectPathResultHandler::~PyObjectPathResultHandler()
{
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyObjectPathResultHandler::handle(
	const Py::Tuple& args)
{
	args.verify_length(1);
	CIMObjectPath concop = OWPyConv::PyRef2OW(args[0]);
	m_result.handle(concop);
	return Py::Nothing();
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
	behaviors().name("InstanceResultHandler");
	behaviors().doc("InstanceResultHandlers are used by CIM providers "
		"to return instances from methods such as enumInstances and "
		"associators");
	behaviors().supportRepr();
	behaviors().supportGetattr();
	add_varargs_method("handle", 
	&PyObjectPathResultHandler::handle, "Return a CIM instance to the "
		"CIMOM");
}
