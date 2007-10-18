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

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

//////////////////////////////////////////////////////////////////////////////
PyLogger::PyLogger(
	const LoggerRef& logger)
	: Py::PythonExtension<PyLogger>()
	, m_logger(logger)
{
}

//////////////////////////////////////////////////////////////////////////////
PyLogger::~PyLogger()
{
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyLogger::logFatalError(const Py::Tuple& args)
{
	if (args.length() && !args[0].isNone())
	{
		Py::String msg(args[0]);
		PYCXX_ALLOW_THREADS
		m_logger->logFatalError(String(msg));
		PYCXX_END_ALLOW_THREADS
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyLogger::logError(const Py::Tuple& args)
{
	if (args.length() && !args[0].isNone())
	{
		Py::String msg(args[0]);
		PYCXX_ALLOW_THREADS
		m_logger->logError(String(msg));
		PYCXX_END_ALLOW_THREADS
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyLogger::logInfo(const Py::Tuple& args)
{
	if (args.length() && !args[0].isNone())
	{
		Py::String msg(args[0]);
		PYCXX_ALLOW_THREADS
		m_logger->logInfo(String(msg));
		PYCXX_END_ALLOW_THREADS
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyLogger::logDebug(const Py::Tuple& args)
{
	if (args.length() && !args[0].isNone())
	{
		Py::String msg(args[0]);
		PYCXX_ALLOW_THREADS
		m_logger->logDebug(String(msg));
		PYCXX_END_ALLOW_THREADS
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
bool
PyLogger::accepts(
	PyObject *pyob) const
{
	return pyob && PyLogger::check(pyob);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyLogger::repr()
{
	return Py::String("Provider Logger");
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyLogger::getattr(
	const char *name)
{
	return getattr_methods(name);
}

//////////////////////////////////////////////////////////////////////////////
void
PyLogger::doInit()
{
	behaviors().name("Logger");
	behaviors().doc("Logger objects are retrieved by python providers through "
		"the given ProviderEnvironment objects");
	behaviors().supportRepr();
	behaviors().supportGetattr();
	add_varargs_method("log_fatal_error", &PyLogger::logFatalError,
		"Log a message at the fatal error level");
	add_varargs_method("log_error", &PyLogger::logError,
		"Log a message at the error level");
	add_varargs_method("log_info", &PyLogger::logInfo,
		"Log a message at the info level");
	add_varargs_method("log_debug", &PyLogger::logDebug,
		"Log a message at the debug level");
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PyLogger::newObject(
	const LoggerRef& logger,
	PyLogger **plogger)
{
	PyLogger* ph = new PyLogger(logger);
	if (plogger)
	{
		*plogger = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC
