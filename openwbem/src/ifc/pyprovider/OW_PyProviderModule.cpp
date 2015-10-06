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

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
PyProviderModule::PyProviderModule()
	: Py::ExtensionModule<PyProviderModule>("pycimmb")
{
	PyCIMOMHandle::doInit();
	PyLogger::doInit();
	PyProviderEnvironment::doInit();

	initialize("Supporting Classes/Objects for the Python Provider Interface");
}

//////////////////////////////////////////////////////////////////////////////
PyProviderModule::~PyProviderModule()
{
}

static PyProviderModule* g_pymod = 0;
static Py::Module g_pywbemmod;
//////////////////////////////////////////////////////////////////////////////
// STATIC
void
PyProviderModule::doInit(
	const Py::Module& pywbemMod)
{
	g_pywbemmod = pywbemMod;
	g_pymod = new PyProviderModule;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
PyProviderModule*
PyProviderModule::getModulePtr()
{
	return g_pymod;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Module
PyProviderModule::getWBEMMod()
{
	return g_pywbemmod;
}

}	// End of namespace PythonProvIFC
