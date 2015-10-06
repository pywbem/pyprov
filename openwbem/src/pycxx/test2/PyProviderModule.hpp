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
#ifndef PYPROVIDERMODULE_HPP_GUARD
#define PYPROVIDERMODULE_HPP_GUARD

#include "PyCxxObjects.hpp"
#include "PyCxxExtensions.hpp"

class PyProviderModule
	: public Py::ExtensionModule<PyProviderModule>
{
public:
	PyProviderModule();
	virtual ~PyProviderModule();
	static void doInit();

	Py::Object new_InstanceResultHandler(CIMInstanceResultHandlerIFC& result);
	Py::Object new_ObjectPathResultHandler(CIMObjectPathResultHandlerIFC& result);
};

#endif	// PYPROVIDERMODULE_HPP_GUARD


