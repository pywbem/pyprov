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
#ifndef PG_PYPROVIFCCOMMON_H_GUARD_
#define PG_PYPROVIFCCOMMON_H_GUARD_

#include "PyCxxObjects.h"
#include <Pegasus/Common/Exception.h>
#include <Pegasus/Common/CIMInstance.h>

// All of the functions in a python provider that can be called by the
// provider interface will have a prefix that matches PYFUNC_PREFIX.
// For example: The getInstance method would be <PYFUNC_PREFIX>getInstance.
// which could be MI_getInstance.
#define PYFUNC_PREFIX "MI_"
#define PYSYSTEM_ID "PyProviderManager"

using namespace Pegasus;

namespace PythonProvIFC
{

class PyProviderIFCException : public Exception
{
public:
	PyProviderIFCException();
	PyProviderIFCException(const String& msg);
	PyProviderIFCException(const String& file, int lineno, const String& msg);
};
#define THROW_PYIFC_EXC(msg) throw PyProviderIFCException(__FILE__, __LINE__, msg)

class PyNoSuchMethodException : public Exception
{
public:
	PyNoSuchMethodException();
	PyNoSuchMethodException(const String& msg);
	PyNoSuchMethodException(const String& file, int lineno, const String& msg);
};
#define THROW_NOSUCHMETH_EXC(msg) throw PyNoSuchMethodException(__FILE__, __LINE__, msg)

class PyNoSuchProviderException : public Exception
{
public:
	PyNoSuchProviderException();
	PyNoSuchProviderException(const String& msg);
	PyNoSuchProviderException(const String& file, int lineno, const String& msg);
};
#define THROW_NOSUCHPROV_EXC(msg) throw PyNoSuchProviderException(__FILE__, __LINE__, msg)


void 
throwCIMException(
	CIMStatusCode code,
	const String& msg,
	int lineno, 
	const char* filename);
#define THROWCIMMSG(code, msg) throwCIMException(code, msg, __LINE__, __FILE__)

String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno,
	Py::Object& etype,
	Py::Object& evalue,
	bool isError=false);

String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno);


#define HANDLECATCH(handler, provrep, operation) \
	catch(Py::Exception& e) \
	{ \
        Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE, \
			"Caught python exception invoking "#operation" on provider " \
			"$0", provrep.m_path); \
		processPyException(e, __LINE__, provrep.m_path, &handler); \
	} \
	catch(const PyConversionException& e) \
	{ \
		String msg = Formatter::format( \
			"Caught python conversion exception calling"#operation" on " \
			"provider $0. Exception Msg: $1", provrep.m_path, e.getMessage()); \
        Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE, msg); \
		handler.setCIMException(CIMException(CIM_ERR_FAILED, msg)); \
	} \
	catch(const Exception& e) \
	{ \
		String msg = Formatter::format( \
			"Caught Exception calling"#operation" on " \
			"provider $0. Exception Msg: $1", provrep.m_path, e.getMessage()); \
        Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE, msg); \
		handler.setStatus(CIM_ERR_FAILED, e.getMessage()); \
	} \
	catch(const std::exception& e) \
	{ \
		String msg = Formatter::format( \
			"Caught std::exception calling"#operation" on " \
			"provider $0. Exception: $1", provrep.m_path, e.what()); \
        Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE, msg); \
		handler.setStatus(CIM_ERR_FAILED, e.what()); \
	} \
	catch(...) \
	{ \
		String msg = Formatter::format( \
			"Caught unknown exception calling"#operation" on " \
			"provider $0.", provrep.m_path); \
        Logger::put(Logger::ERROR_LOG, PYSYSTEM_ID, Logger::SEVERE, msg); \
		handler.setStatus(CIM_ERR_FAILED, "Unknown exception"); \
	} \

}	// End of namespace PythonProvIFC

#endif	// PG_PYPROVIFCCOMMON_H_GUARD_
