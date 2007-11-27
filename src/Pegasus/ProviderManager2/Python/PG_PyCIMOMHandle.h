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
#ifndef PG_PYCIMOMHANDLE_H_GUARD
#define PG_PYCIMOMHANDLE_H_GUARD

#include "PyCxxObjects.h"
#include "PyCxxExtensions.h"
#include <Pegasus/Provider/CIMOMHandle.h>

using namespace Pegasus;

namespace PythonProvIFC
{

class PythonProviderManager;

//////////////////////////////////////////////////////////////////////////////
class PyCIMOMHandle
	: public Py::PythonExtension<PyCIMOMHandle>
{
public:
	PyCIMOMHandle(PythonProviderManager* pmgr, const String& provPath);
	~PyCIMOMHandle();

	Py::Object setDefaultNs(const Py::Tuple& args);
	Py::Object exportIndication(const Py::Tuple& args);

	// Method
	Py::Object invokeMethod(const Py::Tuple& args, const Py::Dict& kws);

	// Schema
	Py::Object enumClassNames(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object enumClass(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object getClass(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object createClass(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object deleteClass(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object modifyClass(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object enumQualifiers(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object getQualifier(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object setQualifier(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object deleteQualifier(const Py::Tuple& args, const Py::Dict& kws);

	// Instance
	Py::Object enumInstanceNames(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object enumInstances(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object getInstance(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object deleteInstance(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object createInstance(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object modifyInstance(const Py::Tuple& args, const Py::Dict& kws);

	// Associators
	Py::Object associators(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object associatorNames(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object references(const Py::Tuple& args, const Py::Dict& kws);
	Py::Object referenceNames(const Py::Tuple& args, const Py::Dict& kws);


	virtual bool accepts(PyObject *pyob) const;
	virtual Py::Object repr();
	virtual Py::Object getattr(const char *name);

	static void doInit();
	static Py::Object newObject(
		PythonProviderManager* pmgr,
		const String& provPath,
		PyCIMOMHandle **pchdl=0);

private:

	CIMOMHandle m_chdl;
	String m_defaultns;
	PythonProviderManager* m_pmgr;
	String m_provPath;
};

}	// End of namespace PythonProvIFC

#endif	// PG_PYCIMOMHANDLE_H_GUARD
