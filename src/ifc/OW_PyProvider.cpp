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
#include <openwbem/OW_config.h>
#include "OW_PyProvider.hpp"
#include "OW_PyProvIFCCommon.hpp"
#include "OW_PyProviderEnvironment.hpp"
#include "OW_PyResultHandlers.hpp"
#include "OW_PyConverter.hpp"
#include <openwbem/OW_CIMValue.hpp>
#include <openwbem/OW_CIMClass.hpp>
#include <openwbem/OW_CIMInstance.hpp>
#include <openwbem/OW_CIMObjectPath.hpp>
#include <openwbem/OW_CIMParamValue.hpp>
#include <openwbem/OW_CIMException.hpp>
#include <openwbem/OW_NoSuchProviderException.hpp>
#include <openwbem/OW_Format.hpp>

#include <iostream>
using std::cout;
using std::endl;

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

using namespace OW_NAMESPACE;
using namespace WBEMFlags;

namespace PythonProvIFC
{

OW_DEFINE_EXCEPTION(NoSuchMethod);

namespace
{

Py::Module g_pywbem;
Py::Object g_cimexObj;

//////////////////////////////////////////////////////////////////////////////
String
getPyFile(const String& fname)
{
	if (fname.endsWith(".pyc"))
	{
		String rfname = fname.substring(0, fname.length()-1);
		return rfname;
	}

	if (fname.endsWith(".py"))
		return fname;

	return String("");
}

//////////////////////////////////////////////////////////////////////////////
time_t
getModTime(const String& fileName)
{
	time_t cc = 0;
	struct stat stbuf;
	if (::stat(fileName.c_str(), &stbuf) == 0)
	{
		cc = stbuf.st_mtime;
	}
	return cc;
}

//////////////////////////////////////////////////////////////////////////////
void
setOutputParam(
	CIMParamValueArray& params,
	const String& paramName,
	const Py::Tuple& pyop)
{
	if (pyop.length() != 2)
	{
		OW_THROWCIMMSG(CIMException::FAILED, "Failed to convert output "
			"parameter from python invoke method. Expecting tuple of "
			"size 2");
	}

	if (!pyop[0].isString())
	{
		OW_THROWCIMMSG(CIMException::FAILED, "Failed to convert output "
			"parameter from python invoke method. Expecting tuple of "
			"size 2 where element 0 is a string");
	}

	if (pyop[1].isNone())
	{
		return;
	}

	CIMValue cv = OWPyConv::PyVal2OW(pyop);
	for (CIMParamValueArray::size_type i = 0; i < params.size(); i++)
	{
		if (paramName.equalsIgnoreCase(params[i].getName()))
		{
			params[i].setValue(cv);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
getPropertyList(const StringArray* propertyList)
{
	if (propertyList && propertyList->size())
	{
		Py::List plst;
		for(StringArray::size_type i = 0; i < propertyList->size(); i++)
		{
			plst.append(Py::String((*propertyList)[i]));
		}
		return Py::Object(plst);
	}
	return Py::None();
}

//////////////////////////////////////////////////////////////////////////////
Py::Bool
getLocalOnly(ELocalOnlyFlag flag)
{
	bool v = (flag == E_LOCAL_ONLY) ? true : false;
	return Py::Bool(v);
}

//////////////////////////////////////////////////////////////////////////////
Py::Bool
getDeep(EDeepFlag flag)
{
	bool v = (flag == E_DEEP) ? true : false;
	return Py::Bool(v);
}

//////////////////////////////////////////////////////////////////////////////
Py::Bool
getIncludeQualifiers(EIncludeQualifiersFlag flag)
{
	bool v = (flag == E_INCLUDE_QUALIFIERS) ? true : false;
	return Py::Bool(v);
}

//////////////////////////////////////////////////////////////////////////////
Py::Bool
getClassOrigin(EIncludeClassOriginFlag flag)
{
	bool v = (flag == E_INCLUDE_CLASS_ORIGIN) ? true : false;
	return Py::Bool(v);
}

//////////////////////////////////////////////////////////////////////////////
void
logCurPyError(
	LoggerRef& lgr)
{
	Py::Object etype, evalue;
	String tb = Py::getCurrentErrorInfo(etype, evalue);

	OW_LOG_ERROR(lgr, Format("Python exception value: %1",
			evalue.as_string()));
	OW_LOG_ERROR(lgr, Format("Python exception type: %1",
			etype.as_string()));
	OW_LOG_DEBUG(lgr, Format("Python trace: %1", tb));
}


//////////////////////////////////////////////////////////////////////////////
inline String
getFunctionName(
	const String& fnameArg)
{
	return String(PYFUNC_PREFIX + fnameArg);
}

//////////////////////////////////////////////////////////////////////////////
Py::Callable
getFunction(
	Py::Object& obj,
	const String& fnameArg)
{
	String fn = getFunctionName(fnameArg);
	try
	{
		Py::Callable pyfunc = obj.getAttr(fn);
		return pyfunc;
	}
	catch(Py::Exception& e)
	{
		e.clear();
	}

	OW_THROW(NoSuchMethodException, fn.c_str());
	return Py::Callable();	// Shouldn't hit this
}

}	// End of unnamed namespace

//////////////////////////////////////////////////////////////////////////////
// STATIC
void
PyProvider::setPyWbemMod(
	const Py::Module& pywbemMod)
{
	g_pywbem = pywbemMod;
	g_cimexObj = pywbemMod.getAttr("CIMError");
}

//////////////////////////////////////////////////////////////////////////////
PyProvider::PyProvider(
	const String& path, 
	const ProviderEnvironmentIFCRef& env,
	bool unloadableType)
	: IntrusiveCountableBase()
	, m_path(path)
	, m_pyprov()
	, m_dt(0)
	, m_fileModTime(0)
#if OW_OPENWBEM_MAJOR_VERSION == 3
	, m_activationCount(0)
#endif
	, m_unloadableType(unloadableType)
	, m_handlerClassNames()
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		// Get the Python proxy provider
		Py::Object cim_provider = g_pywbem.getAttr("cim_provider"); 
		Py::Callable ctor = cim_provider.getAttr("ProviderProxy");
		Py::Tuple args(2);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = Py::String(m_path);
		// Construct a CIMProvider python object
		m_pyprov = ctor.apply(args);
		m_fileModTime = getModTime(m_path);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_DEBUG(logger, Format("PyProvider caught exception loading "
			"provider %1", m_path));
		String tb = processPyException(e, __LINE__, logger, false);
		String msg = "Python Load Error: " + tb;
		OW_THROW(NoSuchProviderException, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
PyProvider::~PyProvider()
{
	try
	{
		GILGuard gg;	// Acquire python's GIL
		m_pyprov.release();
	}
	catch(Py::Exception& e)
	{
		e.clear();
		// Ignore?
	}
	catch(...)
	{
		// Ignore?
	}
}

//////////////////////////////////////////////////////////////////////////////
bool
PyProvider::providerChanged() const
{
	String pyfname = m_path;
	pyfname = getPyFile(m_path);
	time_t modTime = getModTime(pyfname);
	return (modTime > m_fileModTime);
}

//////////////////////////////////////////////////////////////////////////////
void 
PyProvider::updateAccessTime()
{
	m_dt.setToCurrent();
}

//////////////////////////////////////////////////////////////////////////////
DateTime 
PyProvider::getLastAccessTime() const
{
	return m_dt;
}

//////////////////////////////////////////////////////////////////////////////
String
PyProvider::getFileName() const
{
	return m_path;
}

//////////////////////////////////////////////////////////////////////////////
String
PyProvider::processPyException(
	Py::Exception& thrownEx,
	int lineno,
	LoggerRef& lgr,
	bool doThrow) const
{
	Py::Object etype, evalue;

	bool isCIMExc = PyErr_ExceptionMatches(g_cimexObj.ptr());
	String tb = LogPyException(thrownEx, __FILE__, lineno, lgr, etype,
		evalue, !isCIMExc);
	thrownEx.clear();

	if (!doThrow)
	{
		return tb;
	}

	if (!isCIMExc)
	{
		throw CIMException(__FILE__, lineno, CIMException::FAILED,
			Format("From python code. Trace: %1", tb).c_str());
	}

	int errval = CIMException::FAILED;
	String msg = Format("Thrown from Python provider: %1", m_path);
	try
	{
		// Attempt to get information about the pywbem.CIMError
		// that occurred...
		bool haveInt = false;
		bool haveMsg = false;
		Py::Tuple exargs = evalue.getAttr("args");
		for (int i = 0; i < int(exargs.length()); i++)
		{
			Py::Object wko = exargs[i];
			if (wko.isInt() && !haveInt)
			{
				errval = int(Py::Int(wko));
				haveInt = true;
			}
			if (wko.isString() && !haveMsg)
			{
				msg = Py::String(wko).as_ow_string();
				haveMsg = true;
			}
			if (haveInt && haveMsg)
			{
				break;
			}
		}
	}
	catch(Py::Exception& theExc)
	{
		theExc.clear();
		OW_THROWCIMMSG(CIMException::FAILED,
			Format("Re-Thrown from python code. type: %1  value: %2",
				etype.as_string(), evalue.as_string()).c_str());
	}
	catch(...)
	{
		OW_THROWCIMMSG(CIMException::FAILED,
			Format("Caught unknown exception trying to process "
				"pywbem.CIMError. type: %1  value: %2",
				etype.as_string(), evalue.as_string()).c_str());
	}

	throw CIMException(__FILE__, lineno,
		CIMException::ErrNoType(errval), msg.c_str());
	return tb;	// Won't hit this
}

//////////////////////////////////////////////////////////////////////////////
bool 
PyProvider::canShutDown(
	const ProviderEnvironmentIFCRef& env) const
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc;
		try
		{
			String fname = getFunctionName("canshutdown");
			pyfunc = m_pyprov.getAttr(fname);
		}
		catch(Py::Exception& e)
		{
			e.clear();
			return true;
		}
		Py::Tuple args(1);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		Py::Object wko = pyfunc.apply(args);
		return wko.isTrue();
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"canshutdown on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"canshutdown on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
void 
PyProvider::shutDown(
	const ProviderEnvironmentIFCRef& env)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc;
		try
		{
			String fname = getFunctionName("shutdown");
			pyfunc = m_pyprov.getAttr(fname);
		}
		catch(Py::Exception& e)
		{
			e.clear();
			return;
		}
		Py::Tuple args(1);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"shutdown on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"shutdown on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::enumInstanceNames(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const String& className,
	CIMObjectPathResultHandlerIFC& result,
	const CIMClass& cimClass)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "enumInstanceNames");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = Py::String(ns);							// Namespace
		args[2] = PyObjectPathResultHandler::newObject(result, ns);
		args[3] = OWPyConv::OWClass2Py(cimClass);			// CIM Class
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"enumInstanceNames on provider %1 for class %2",
			m_path, className));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"enumInstanceNames on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::enumInstances(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const String& className,
	CIMInstanceResultHandlerIFC& result,
	ELocalOnlyFlag localOnly, 
	EDeepFlag deep, 
	EIncludeQualifiersFlag includeQualifiers, 
	EIncludeClassOriginFlag includeClassOrigin,
	const StringArray* propertyList,
	const CIMClass& requestedClass,
	const CIMClass& cimClass)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "enumInstances");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = Py::String(ns);							// Namespace
		args[2] = PyInstanceResultHandler::newObject(result, ns);
		args[3] = getPropertyList(propertyList);
		args[4] = OWPyConv::OWClass2Py(requestedClass);
		args[5] = OWPyConv::OWClass2Py(cimClass);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"enumInstances on provider %1 for class %2",
			m_path, className));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"enumInstances on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
CIMInstance
PyProvider::getInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMObjectPath& instanceName,
	ELocalOnlyFlag localOnly,
	EIncludeQualifiersFlag includeQualifiers, 
	EIncludeClassOriginFlag includeClassOrigin,
	const StringArray* propertyList, 
	const CIMClass& cimClass)
{
	GILGuard gg;	// Acquire python's GIL
	LoggerRef logger = myLogger(env);

	try
	{
		CIMObjectPath lcop(instanceName);
		if (lcop.getNameSpace().empty())	// Ensure namespace set
		{
			lcop.setNameSpace(ns);
		}

		Py::Callable pyfunc = getFunction(m_pyprov, "getInstance");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = OWPyConv::OWRef2Py(lcop);
		args[2] = getPropertyList(propertyList);
		args[3] = OWPyConv::OWClass2Py(cimClass);
		Py::Object pyci = pyfunc.apply(args);
		if (pyci.isNone())
		{
			OW_THROWCIMMSG(CIMException::FAILED,
				Format("Error: Python provider: %1 returned NONE on "
					"getInstance", m_path).c_str());
		}
		CIMInstance ci = OWPyConv::PyInst2OW(pyci, ns);
		return ci;
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"getInstance on provider %1 for class %2",
			m_path, cimClass.getName()));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"getInstance on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}

	OW_THROWCIM(CIMException::NOT_SUPPORTED);	// Shouldn't hit this
	return CIMInstance(CIMNULL);
}

//////////////////////////////////////////////////////////////////////////////
CIMObjectPath
PyProvider::createInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMInstance& cimInstance)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "createInstance");
		Py::Tuple args(2);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = OWPyConv::OWInst2Py(cimInstance, ns);		// New instance
		Py::Object pycop = pyfunc.apply(args);
		if (pycop.isNone())
		{
			OW_THROWCIMMSG(CIMException::FAILED,
				Format("Error: Python provider: %1 returned NONE on "
					"createInstance", m_path).c_str());
		}
		CIMObjectPath cop = OWPyConv::PyRef2OW(pycop, ns);
		return cop;
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"createInstance on provider %1 for class %2",
			m_path, cimInstance.getClassName()));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"createInstance on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}

	OW_THROWCIM(CIMException::NOT_SUPPORTED);	// Shouldn't hit this
	return CIMObjectPath(CIMNULL);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::modifyInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMInstance& modifiedInstance,
	const CIMInstance& previousInstance,
	EIncludeQualifiersFlag includeQualifiers,
	const StringArray* propertyList,
	const CIMClass& theClass)
{
	GILGuard gg;	// Acquire python's GIL
	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "modifyInstance");
		Py::Tuple args(5);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = OWPyConv::OWInst2Py(modifiedInstance, ns);
		args[2] = OWPyConv::OWInst2Py(previousInstance, ns);
		args[3] = getPropertyList(propertyList);
		args[4] = OWPyConv::OWClass2Py(theClass);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"modifyInstance on provider %1 for class %2",
			m_path, theClass.getName()));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"modifyInstance on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::deleteInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMObjectPath& cop)
{
	GILGuard gg;	// Acquire python's GIL
	LoggerRef logger = myLogger(env);

	try
	{
		CIMObjectPath lcop(cop);
		if (lcop.getNameSpace().empty())
		{
			lcop.setNameSpace(ns);
		}
		Py::Callable pyfunc = getFunction(m_pyprov, "deleteInstance");
		Py::Tuple args(2);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = OWPyConv::OWRef2Py(lcop);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"deleteInstance on provider %1 for class %2",
			m_path, cop.getClassName()));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"deleteInstance on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::associators(
	const ProviderEnvironmentIFCRef& env,
	CIMInstanceResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& assocClass,
	const String& resultClass,
	const String& role,
	const String& resultRole,
	EIncludeQualifiersFlag includeQualifiers,
	EIncludeClassOriginFlag includeClassOrigin,
	const StringArray* propertyList)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		CIMObjectPath lcop(objectName);
		if (lcop.getNameSpace().empty())
		{
			lcop.setNameSpace(ns);
		}
		Py::Callable pyfunc = getFunction(m_pyprov, "associators");
		Py::Tuple args(8);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = PyInstanceResultHandler::newObject(result, ns);
		args[2] = OWPyConv::OWRef2Py(lcop);
		args[3] = Py::String(assocClass);
		args[4] = Py::String(resultClass);
		args[5] = Py::String(role);
		args[6] = Py::String(resultRole);
		args[7] = getPropertyList(propertyList);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"associators on provider %1 for assoc class %2",
			m_path, assocClass));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"associators on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::associatorNames(
	const ProviderEnvironmentIFCRef& env,
	CIMObjectPathResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& assocClass,
	const String& resultClass,
	const String& role,
	const String& resultRole)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		CIMObjectPath lcop(objectName);
		if (lcop.getNameSpace().empty())
		{
			lcop.setNameSpace(ns);
		}
		Py::Callable pyfunc = getFunction(m_pyprov, "associatorNames");
		Py::Tuple args(7);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = PyObjectPathResultHandler::newObject(result, ns);
		args[2] = OWPyConv::OWRef2Py(lcop);
		args[3] = Py::String(assocClass);
		args[4] = Py::String(resultClass);
		args[5] = Py::String(role);
		args[6] = Py::String(resultRole);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"associatorNames on provider %1 for assoc class %2",
			m_path, assocClass));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"associatorNames on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::references(
	const ProviderEnvironmentIFCRef& env,
	CIMInstanceResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& resultClass,
	const String& role,
	EIncludeQualifiersFlag includeQualifiers,
	EIncludeClassOriginFlag includeClassOrigin,
	const StringArray* propertyList)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		CIMObjectPath lcop(objectName);
		if (lcop.getNameSpace().empty())
		{
			lcop.setNameSpace(ns);
		}
		Py::Callable pyfunc = getFunction(m_pyprov, "references");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = PyInstanceResultHandler::newObject(result, ns);
		args[2] = OWPyConv::OWRef2Py(lcop);
		args[3] = Py::String(resultClass);
		args[4] = Py::String(role);
		args[5] = getPropertyList(propertyList);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"references on provider %1 for result class %2",
			m_path, resultClass));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"references on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::referenceNames(
	const ProviderEnvironmentIFCRef& env,
	CIMObjectPathResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& resultClass,
	const String& role)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		CIMObjectPath lcop(objectName);
		if (lcop.getNameSpace().empty())
		{
			lcop.setNameSpace(ns);
		}
		Py::Callable pyfunc = getFunction(m_pyprov, "referenceNames");
		Py::Tuple args(5);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = PyObjectPathResultHandler::newObject(result, ns);
		args[2] = OWPyConv::OWRef2Py(lcop);
		args[3] = Py::String(resultClass);
		args[4] = Py::String(role);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"referenceNames on provider %1 for result class %2",
			m_path, resultClass));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"referenceNames on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
CIMValue
PyProvider::invokeMethod(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMObjectPath& path,
	const String& methodName,
	const CIMParamValueArray& in,
	CIMParamValueArray& out)
{
	GILGuard gg;	// Acquire python's GIL
	LoggerRef logger = myLogger(env);

	CIMObjectPath lpath(path);
	if (lpath.getNameSpace().empty())
	{
		lpath.setNameSpace(ns);
	}

	// Get the method object off of the class
	CIMOMHandleIFCRef lch = env->getCIMOMHandle();
	CIMClass cc = lch->getClass(ns, lpath.getClassName(),
		E_NOT_LOCAL_ONLY, E_INCLUDE_QUALIFIERS);
	CIMMethod method = cc.getMethod(methodName);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "invokeMethod");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(env); 	// Provider Environment
		args[1] = OWPyConv::OWRef2Py(lpath);
		args[2] = OWPyConv::OWMeth2Py(method);
		// Build input parameter dictionary
		Py::Dict aDict;
		for (CIMParamValueArray::size_type i = 0; i < in.size(); i++)
		{
			CIMValue cv = in[i].getValue();
			if (cv)
			{
				aDict[in[i].getName()] = OWPyConv::OWVal2Py(cv);
			}
			else
			{
				aDict[in[i].getName()] = Py::None();
			}
		}
		args[3] = aDict;
		
		//-----------------------------------------------------
		// Invoke the method in the Python provider
		//-----------------------------------------------------
		Py::Object pyv = pyfunc.apply(args);
		//-----------------------------------------------------
		// Process output from Python provider's invokeMethod
		//-----------------------------------------------------
		if (!pyv.isTuple())
		{
			OW_LOG_ERROR(logger, Format("Python provider %1 did not return "
				"a valid value for invokeMethod on %2. Should be a tuple",
					m_path, methodName));
			OW_THROWCIMMSG(CIMException::FAILED, Format("Python provider %1 "
				"did not return a value for invokeMethod on %2. Should "
				"be a tuple", m_path, methodName).c_str());
		}
		// The tuple we are excpecting here is of the following format
		// Tuple[0] = subTuple(2): subTuple[0]: dataType name. subTuple[1]:data
		// Tuple[1] = Dict where key is the param name and value is a tuple
		// that looks like: tuple[0]:dataType tuple[1]:data
		Py::Tuple rt(pyv);
		if (rt.length() != 2)
		{
			OW_LOG_ERROR(logger, Format("Python provider %1 did not return "
				"tuple of length 2 on invokeMethod on %2", m_path,
					methodName));
			OW_THROWCIMMSG(CIMException::FAILED, Format("Python provider %1 "
				"did not return tuple of length 2 on invokeMethod on %2",
					m_path, methodName).c_str());
		}

		// Get the tuple that hold the return value
		Py::Tuple vt(rt[0]);
		// Convert the return value tuple to a CIMValue
		CIMValue rv = OWPyConv::PyVal2OW(vt);
		// Get the output parameter dictionary
		aDict = rt[1];
		if (!aDict.length())
		{
			// Provider did not return any output parameters
			return rv;
		}

		// Get the list of output parameter names from the python
		// dictionary of output parameters
		Py::List keys(aDict.keys());
		// Now loop through the dictionary of output parameters
		// and put them in the output parameters the CIMOM gave us.
		for (int i = 0; i < int(keys.length()); i++)
		{
			// Convert parameter name to an ow string
			String pname = Py::String(keys[i]).as_ow_string();
			// Get the tuple that holds the parameter value
			vt = aDict[pname];
			// Convert to CIMValue and set the parameter in the
			// given output parameter array
			setOutputParam(out, pname, vt);
		}
		return rv;
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"invokeMethod on provider %1 for method name %2",
			m_path, methodName));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"invokeMethod on provider %1. MethodName: %2 Exception Message: "
			"%3", m_path, methodName, e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
	
	// Shouldn't hit this
	return CIMValue(UInt32(1));		// Not supported ?
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::activateFilter(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes
#if OW_OPENWBEM_MAJOR_VERSION >= 4
	, bool firstActivation
#endif
	)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "activateFilter");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(env);
		args[1] = Py::String(filter.toString());
		args[2] = Py::String(eventType);
		args[3] = Py::String(nameSpace);
		Py::List pyclasses;
		for (StringArray::size_type i = 0; i < classes.size(); i++)
		{
			pyclasses.append(Py::String(classes[i]));
		}
		args[4] = pyclasses;
#if OW_OPENWBEM_MAJOR_VERSION >= 4
		args[5] = Py::Bool(firstActivation);
#elif OW_OPENWBEM_MAJOR_VERSION == 3
		m_activationCount++;
		args[5] = Py::Bool(m_activationCount == 1);
#endif
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"activateFilter on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"activateFilter on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::authorizeFilter(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes,
	const String& owner)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "authorizeFilter");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(env);
		args[1] = Py::String(filter.toString());
		args[2] = Py::String(eventType);
		args[3] = Py::String(nameSpace);
		Py::List pyclasses;
		for (StringArray::size_type i = 0; i < classes.size(); i++)
		{
			pyclasses.append(Py::String(classes[i]));
		}
		args[4] = pyclasses;
		args[5] = Py::String(owner);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"authorizeFilter on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"authorizeFilter on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::deActivateFilter(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes
#if OW_OPENWBEM_MAJOR_VERSION >= 4
	, bool lastActivation
#endif
	)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "deActivateFilter");
		Py::Tuple args(6);
		args[0] = PyProviderEnvironment::newObject(env);
		args[1] = Py::String(filter.toString());
		args[2] = Py::String(eventType);
		args[3] = Py::String(nameSpace);
		Py::List pyclasses;
		for (StringArray::size_type i = 0; i < classes.size(); i++)
		{
			pyclasses.append(Py::String(classes[i]));
		}
		args[4] = pyclasses;
#if OW_OPENWBEM_MAJOR_VERSION >= 4
		args[5] = Py::Bool(lastActivation);
#elif OW_OPENWBEM_MAJOR_VERSION == 3
		m_activationCount--;
		args[5] = Py::Bool(m_activationCount == 0);
#endif
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"authorizeFilter on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"deActivateFilter on provider %1. Exception Message: %2", m_path,
			e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
StringArray
PyProvider::getHandlerClassNames()
{
	return m_handlerClassNames;
}

//////////////////////////////////////////////////////////////////////////////
void
PyProvider::exportIndication(
	const ProviderEnvironmentIFCRef& env, 
	const String& ns,
	const CIMInstance& indHandlerInst,
	const CIMInstance& indicationInst)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);

	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "handleIndication");
		Py::Tuple args(4);
		args[0] = PyProviderEnvironment::newObject(env);
		args[1] = Py::String(ns);
		args[2] = OWPyConv::OWInst2Py(indHandlerInst, ns);
		args[3] = OWPyConv::OWInst2Py(indicationInst, ns);
		pyfunc.apply(args);
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"handleIndication on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"exportIndication on provider %1. Exception Message: %2",
			m_path, e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
Int32
PyProvider::poll(
	const ProviderEnvironmentIFCRef& env)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);
	Int32 rv = 0;
	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "poll");
		Py::Tuple args(1);
		args[0] = PyProviderEnvironment::newObject(env);
		Py::Object wko = pyfunc.apply(args);
		if (!wko.isNone())
		{
			rv = Int32(Py::Int(wko).asLong());
		}
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"poll on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"poll on provider %1. Exception Message: %2",
			m_path, e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
Int32
PyProvider::getInitialPollingInterval(
	const ProviderEnvironmentIFCRef& env)
{
	GILGuard gg;	// Acquire python's GIL

	LoggerRef logger = myLogger(env);
	Int32 rv = 0;
	try
	{
		Py::Callable pyfunc = getFunction(m_pyprov, "getInitialPollingInterval");
		Py::Tuple args(1);
		args[0] = PyProviderEnvironment::newObject(env);
		Py::Object wko = pyfunc.apply(args);
		if (!wko.isNone())
		{
			rv = Int32(Py::Int(wko).asLong());
		}
	}
	catch(Py::Exception& e)
	{
		OW_LOG_ERROR(logger, Format("Caught python exception invoking "
			"getInitialPollingInterval on provider %1", m_path));

		// Rethrow as an exception OW understands
		processPyException(e, __LINE__, logger);
	}
	catch(const PyConversionException& e)
	{
		String msg = Format("Caught python conversion exception calling "
			"getInitialPollingInterval on provider %1. Exception Message: %2",
			m_path, e.getMessage());
		OW_LOG_ERROR(logger, msg);
		OW_THROWCIMMSG(CIMException::FAILED, msg.c_str());
	}
	return rv;
}

} // end namespace PythonProvIFC

