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
#ifndef PG_PYPROVIDERENVIRONMENT_H_GUARD
#define PG_PYPROVIDERENVIRONMENT_H_GUARD

#include "PyCxxObjects.h"
#include "PyCxxExtensions.h"
#include <Pegasus/Common/OperationContext.h>

using namespace Pegasus;

namespace PythonProvIFC
{

class PythonProviderManager;

//////////////////////////////////////////////////////////////////////////////
class PyProviderEnvironment
	: public Py::PythonExtension<PyProviderEnvironment>
{
public:
	PyProviderEnvironment(
		const OperationContext& opctx,
		PythonProviderManager* pmgr,
		const String& provPath);

	~PyProviderEnvironment();

	Py::Object getCIMOMHandle(const Py::Tuple& args);
	Py::Object getLogger(const Py::Tuple& args);
	Py::Object getUserName(const Py::Tuple& args);

#if 0
	Py::Object getContextValue(const Py::Tuple& args);
	Py::Object setContextValue(const Py::Tuple& args);
#endif

	Py::Object getCIMOMInfo(const Py::Tuple& args);

	virtual bool accepts(PyObject *pyob) const;
	virtual Py::Object repr();
	virtual Py::Object getattr(const char *name);

	static void doInit();

	static Py::Object newObject(
		const OperationContext& opctx,
		PythonProviderManager* pmgr,
		const String& provPath,
		PyProviderEnvironment **penv=0);

private:

	OperationContext m_opctx;
	PythonProviderManager* m_pmgr;
	String m_provPath;
};

}	// End of namespace PythonProvIFC

#endif	// PG_PYPROVIDERENVIRONMENT_H_GUARD
