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


#ifndef OW_PYWBEM_converter_hpp__
#define OW_PYWBEM_converter_hpp__

#include <Python.h>
#include <openwbem/OW_config.h>
#include <openwbem/OW_CIMFwd.hpp>

namespace OW_Py_Converter
{

PyObject* CIMInstance_OW2Py(const OpenWBEM::CIMInstance& owinst); 
OpenWBEM::CIMInstance CIMInstance_Py2OW(PyObject* pyInst); 
void setModulePyWBEM(PyObject* mod);
void setModuleTimeDelta(PyObject* mod);

}
#endif // #ifndef OW_PYWBEM_converter_hpp__
