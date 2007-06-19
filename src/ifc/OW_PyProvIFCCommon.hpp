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
#ifndef OW_PYPROVIFCCOMMON_HPP_GUARD_
#define OW_PYPROVIFCCOMMON_HPP_GUARD_

#include "PyCxxObjects.hpp"
#include <openwbem/OW_Logger.hpp>
#include <openwbem/OW_Exception.hpp>
#include <openwbem/OW_ProviderEnvironmentIFC.hpp>
#include <openwbem/OW_CIMInstance.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

OW_DECLARE_EXCEPTION(PyProviderIFC);
OW_DECLARE_EXCEPTION(NoSuchMethod);

// All of the functions in a python provider that can be called by the
// provider interface will have a prefix that matches PYFUNC_PREFIX.
// For example: The getInstance method would be <PYFUNC_PREFIX>getInstance.
// which could be MI_getInstance.
#define PYFUNC_PREFIX "MI_"

#define COMPONENT_NAME "ow.provider.python.ifc"

inline LoggerRef
myLogger(
	const ProviderEnvironmentIFCRef& env)
{
	return env->getLogger(COMPONENT_NAME);
}

// The GILGuard class is used to acquire python global interpreter lock.
// It acquires the lock within its constructor and releases it in its
// destructor. Do use it just declare an instance of the GILGuard when
// you want to acquire the lock. When the instance goes out of scope,
// the lock will be released.
class GILGuard
{
public:
	GILGuard();
	~GILGuard();
	void release();
	void acquire();
private:
	PyGILState_STATE m_gstate;
	bool m_acquired;
};

class PyProviderReg
{
public:

	enum ProvType
	{
		E_INSTANCE = 1,
		E_SECONDARY_INSTANCE = 2,
		E_ASSOCIATION = 3,
		E_LIFECYCLE_INDICATION = 4,
		E_ALERT_INDICATION = 5,
		E_METHOD = 6,
		E_INDICATION_HANDLER = 7,
		E_POLLED = 8,
	};

	PyProviderReg(const CIMInstance& ci);
	PyProviderReg();
	PyProviderReg(const PyProviderReg& arg);
	PyProviderReg& operator=(const PyProviderReg& arg);

	String getModPath() const;
	String getInstanceId() const;
	StringArray getNameSpaceNames() const;
	String getClassName() const;
	UInt16Array getProviderTypes() const;
	StringArray getMethodNames() const;
	StringArray getExportHandlerClassNames() const;
	bool isNull() const { return (!m_ci) ? true : false; }
	
private:
	CIMInstance m_ci;
};

String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno,
	LoggerRef& lgr,
	Py::Object& etype,
	Py::Object& evalue,
	bool isError=false);

String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno,
	LoggerRef& lgr);

}	// End of namespace PythonProvIFC

#endif	// OW_PYPROVIFCCOMMON_HPP_GUARD_
