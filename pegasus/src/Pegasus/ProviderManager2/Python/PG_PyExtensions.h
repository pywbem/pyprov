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

#include "PyCxxObjects.h"
#include "PyCxxExtensions.h"
#include "PG_PyCIMOMHandle.h"
#include "PG_PyProviderEnvironment.h"
#include "PG_PyLogger.h"

namespace PythonProvIFC
{

class PyExtensions
	: public Py::ExtensionModule<PyExtensions>
{
public:
	PyExtensions();
	virtual ~PyExtensions();
	static void doInit(const Py::Module& pywbemMod);
	static PyExtensions* getModulePtr();
	static Py::Module getWBEMMod();
};

}	// End of namespace PythonProvIFC

#endif	// PYPROVIDERMODULE_HPP_GUARD


