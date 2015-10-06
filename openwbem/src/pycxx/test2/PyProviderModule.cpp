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
#include "PyProviderModule.hpp"
#include "PyCxxResultHandlers.hpp"

//////////////////////////////////////////////////////////////////////////////
PyProviderModule::PyProviderModule()
	: Py::ExtensionModule<PyProviderModule>("pyprovider")
{
	PyInstanceResultHandler::doInit();
	PyObjectPathResultHandler::doInit();

	/*
	add_varargs_method("InstanceResultHandler",
		&PyProviderModule::new_InstanceResultHandler,
		"InstanceResultHandler.handle(CIMInstance)");
	add_varargs_method("ObjectPathResultHandler",
		&PyProviderModule::new_ObjectPathResultHandler,
		"ObjectPathResultHandler.handle(CIMInstanceName|"
		"CIMClassName)");
	*/
	initialize("Supporting Classes/Objects for the Python Provider Interface");
}

//////////////////////////////////////////////////////////////////////////////
PyProviderModule::~PyProviderModule()
{
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
void
PyProviderModule::doInit()
{
	static PyProviderModule* pymod = new PyProviderModule;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderModule::new_InstanceResultHandler(
	CIMInstanceResultHandlerIFC& result)
{
	return Py::asObject(new PyInstanceResultHandler(result));
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyProviderModule::new_ObjectPathResultHandler(
	CIMObjectPathResultHandlerIFC& result)
{
	return Py::asObject(new PyObjectPathResultHandler(result));
}


