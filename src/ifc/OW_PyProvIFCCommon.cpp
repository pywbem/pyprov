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
#include "OW_PyProvIFCCommon.hpp"
#include <openwbem/OW_CIMException.hpp>
#include <openwbem/OW_Format.hpp>
#include <openwbem/OW_CIMValue.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno,
	LoggerRef& lgr,
	Py::Object& etype,
	Py::Object& evalue,
	bool isError)
{
	String tb = Py::getCurrentErrorInfo(etype, evalue);
	if (isError)
	{
		OW_LOG_ERROR(lgr, Format("Python exception value: %1",
				evalue.as_string()));
		OW_LOG_ERROR(lgr, Format("Python exception type: %1",
				etype.as_string()));
		OW_LOG_ERROR(lgr, Format("Python trace: %1", tb));
	}
	else
	{
		OW_LOG_DEBUG(lgr, Format("Python exception value: %1",
				evalue.as_string()));
		OW_LOG_DEBUG(lgr, Format("Python exception type: %1",
				etype.as_string()));
		OW_LOG_DEBUG(lgr, Format("Python trace: %1", tb));
	}
	return tb;
}

//////////////////////////////////////////////////////////////////////////////
String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno,
	LoggerRef& lgr)
{
	Py::Object etype, evalue;
	return LogPyException(thrownEx, fileName, lineno, lgr, etype, evalue, true);
}

//////////////////////////////////////////////////////////////////////////////
GILGuard::GILGuard()
	: m_gstate()
	, m_acquired(true)
{
	m_gstate = PyGILState_Ensure();
}

//////////////////////////////////////////////////////////////////////////////
GILGuard::~GILGuard()
{
	release();
}

//////////////////////////////////////////////////////////////////////////////
void
GILGuard::release()
{
	if (m_acquired)
	{
		PyGILState_Release(m_gstate);
		m_acquired = false;
	}
}

//////////////////////////////////////////////////////////////////////////////
void
GILGuard::acquire()
{
	if (!m_acquired)
	{
		m_gstate = PyGILState_Ensure();
		m_acquired = true;
	}
}

//////////////////////////////////////////////////////////////////////////////
PyProviderReg::PyProviderReg(const CIMInstance& ci)
	: m_ci(ci)
{
}

//////////////////////////////////////////////////////////////////////////////
PyProviderReg::PyProviderReg()
	: m_ci(CIMNULL)
{
}

//////////////////////////////////////////////////////////////////////////////
PyProviderReg::PyProviderReg(const PyProviderReg& arg)
	: m_ci(arg.m_ci)
{
}

//////////////////////////////////////////////////////////////////////////////
PyProviderReg&
PyProviderReg::operator=(const PyProviderReg& arg)
{
	m_ci = arg.m_ci;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
String
PyProviderReg::getInstanceId() const
{
	String rv;
	CIMValue cv = m_ci.getPropertyValue("InstanceID");
	if (cv)
		cv.get(rv);
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
String
PyProviderReg::getModPath() const
{
	String rv;
	CIMValue cv = m_ci.getPropertyValue("ModulePath");
	if (cv)
		cv.get(rv);
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
StringArray
PyProviderReg::getNameSpaceNames() const
{
	StringArray rv;
	CIMValue cv = m_ci.getPropertyValue("NamespaceNames");
	if (cv)
		cv.get(rv);
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
String
PyProviderReg::getClassName() const
{
	String rv;
	CIMValue cv = m_ci.getPropertyValue("ClassName");
	if (cv)
		cv.get(rv);
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
UInt16Array
PyProviderReg::getProviderTypes() const
{
	UInt16Array rv;
	CIMValue cv = m_ci.getPropertyValue("ProviderTypes");
	if (cv)
		cv.get(rv);
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
StringArray
PyProviderReg::getMethodNames() const
{
	StringArray rv;
	CIMValue cv = m_ci.getPropertyValue("MethodNames");
	if (cv)
		cv.get(rv);
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
StringArray
PyProviderReg::getExportHandlerClassNames() const
{
	StringArray rv;
	CIMValue cv = m_ci.getPropertyValue("IndicationExportHandlerClassNames");
	if (cv)
		cv.get(rv);
	return rv;
}

}	// End of namespace PythonProvIFC
