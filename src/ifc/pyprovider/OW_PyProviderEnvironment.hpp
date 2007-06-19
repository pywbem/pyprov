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
#ifndef OW_PYPROVIDERENVIRONMENT_HPP_GUARD
#define OW_PYPROVIDERENVIRONMENT_HPP_GUARD

#include "PyCxxObjects.hpp"
#include "PyCxxExtensions.hpp"
#include <openwbem/OW_ProviderEnvironmentIFC.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
class PyProviderEnvironment
	: public Py::PythonExtension<PyProviderEnvironment>
{
public:
	PyProviderEnvironment(const ProviderEnvironmentIFCRef& env);
	~PyProviderEnvironment();

	Py::Object getCIMOMHandle(const Py::Tuple& args);
	Py::Object getLogger(const Py::Tuple& args);
	Py::Object getUserName(const Py::Tuple& args);
	Py::Object getContextValue(const Py::Tuple& args);

	Py::Object setContextValue(const Py::Tuple& args);
	Py::Object getCIMOMInfo(const Py::Tuple& args);

	virtual bool accepts(PyObject *pyob) const;
	virtual Py::Object repr();
	virtual Py::Object getattr(const char *name);

	static void doInit();
	static Py::Object newObject(const ProviderEnvironmentIFCRef& env,
		PyProviderEnvironment **penv=0);

private:
	ProviderEnvironmentIFCRef m_env;
};

}	// End of namespace PythonProvIFC

#endif
