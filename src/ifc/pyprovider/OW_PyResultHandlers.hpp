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
#ifndef PYRESULTHANDLERS_HPP_GUARD
#define PYRESULTHANDLERS_HPP_GUARD

#include "PyCxxObjects.hpp"
#include "PyCxxExtensions.hpp"
#include <openwbem/OW_IfcsFwd.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
class PyInstanceResultHandler
	: public Py::PythonExtension<PyInstanceResultHandler>
{
public:
	PyInstanceResultHandler(CIMInstanceResultHandlerIFC& result,
		const String& ns);
	~PyInstanceResultHandler();
	Py::Object handle(const Py::Tuple& args);
	virtual bool accepts(PyObject *pyob) const;
	virtual Py::Object repr();
	virtual Py::Object getattr(const char *name);
	virtual Py::Object call(const Py::Object& args, const Py::Object& kws);
	void invalidate() { m_valid = false; }

	static void doInit();
	static Py::Object newObject(CIMInstanceResultHandlerIFC& result,
		const String& ns, PyInstanceResultHandler **presHandler=0);

private:
	CIMInstanceResultHandlerIFC &m_result;
	bool m_valid;
	String m_ns;
};

//////////////////////////////////////////////////////////////////////////////
class PyObjectPathResultHandler
	: public Py::PythonExtension<PyObjectPathResultHandler>
{
public:
	PyObjectPathResultHandler(CIMObjectPathResultHandlerIFC& result,
		const String& ns);
	~PyObjectPathResultHandler();
	Py::Object handle(const Py::Tuple& args);
	virtual bool accepts(PyObject *pyob) const;
	virtual Py::Object repr();
	virtual Py::Object getattr(const char *name);
	virtual Py::Object call(const Py::Object& args, const Py::Object& kws);
	void invalidate() { m_valid = false; }

	static void doInit();
	static Py::Object newObject(CIMObjectPathResultHandlerIFC& result,
		const String& ns, PyObjectPathResultHandler **presHandler=0);

private:
	CIMObjectPathResultHandlerIFC &m_result;
	bool m_valid;
	String m_ns;
};

}	// End of namespace PythonProvIFC

#endif	// PYRESULTHANDLERS_HPP_GUARD
