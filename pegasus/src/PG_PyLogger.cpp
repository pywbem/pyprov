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
#include "PG_PyExtensions.h"
#include <Pegasus/Config/ConfigManager.h>

#include <iostream>

extern "C"
{
#include <unistd.h>
}

PEGASUS_USING_STD;
PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

namespace
{
bool g_toStdOut = false;
}

//////////////////////////////////////////////////////////////////////////////
PyLogger::PyLogger()
	: Py::PythonExtension<PyLogger>()
{
	try
	{
		String daemon = ConfigManager::getInstance()->getCurrentValue("daemon");
		daemon.toLower();
		g_toStdOut = (daemon == "false") ? true : false;
	}
	catch(...)
	{
		// Ignore
	}
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
		if (g_toStdOut)
		{
			cout << "[F-" << ::getpid() << "] " << msg << endl;
		}
		else
		{
			PYCXX_ALLOW_THREADS
			Logger::put(Logger::STANDARD_LOG, "PGPythonProvider",
				Logger::FATAL, msg);
			PYCXX_END_ALLOW_THREADS
		}
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
		if (g_toStdOut)
		{
			cout << "[E-" << ::getpid() << "] " << msg << endl;
		}
		else
		{
			PYCXX_ALLOW_THREADS
			Logger::put(Logger::STANDARD_LOG, "PGPythonProvider",
				Logger::SEVERE, msg);
			PYCXX_END_ALLOW_THREADS
		}
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
		if (g_toStdOut)
		{
			cout << "[I-" << ::getpid() << "] " << msg << endl;
		}
		else
		{
			PYCXX_ALLOW_THREADS
			Logger::put(Logger::STANDARD_LOG, "PGPythonProvider",
				Logger::INFORMATION, msg);
			PYCXX_END_ALLOW_THREADS
		}
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
		if (g_toStdOut)
		{
			cout << "[D-" << ::getpid() << "] " << msg << endl;
		}
		else
		{
			PYCXX_ALLOW_THREADS
			Logger::put(Logger::STANDARD_LOG, "PGPythonProvider",
				Logger::TRACE, msg);
			PYCXX_END_ALLOW_THREADS
		}
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
	PyLogger **plogger)
{
	PyLogger* ph = new PyLogger();
	if (plogger)
	{
		*plogger = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC
