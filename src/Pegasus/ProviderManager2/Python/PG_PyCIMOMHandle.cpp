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
#include "PG_PyConverter.h"
#include <Pegasus/Common/CIMQualifier.h>
#include <Pegasus/Common/CIMQualifierDecl.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/CIMStatusCode.h>
#include <Pegasus/Common/Exception.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/Array.h>
//#include <Pegasus/Common/ResultHandlers.h>
#include <Pegasus/Common/Formatter.h>
#include <Pegasus/Common/CIMParamValue.h>
#include <Pegasus/Client/CIMClient.h>

#include "PythonProviderManager.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace Pegasus;

namespace PythonProvIFC
{

namespace
{

//////////////////////////////////////////////////////////////////////////////
CIMName
_pyStr2CIMName(const Py::Object& pobj)
{
	String wstr;
	CIMName rv;
	if (!pobj.isNone())
	{
		String wstr = Py::String(pobj).as_peg_string();
		if (wstr.size())
			rv = wstr;
	}
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
CIMParameter
_getCIMParam(
	const String& paramName,
	CIMMethod& theMethod)
{
	CIMParameter param;
	Uint32 ndx = theMethod.findParameter(paramName);
	if (ndx != PEG_NOT_FOUND)
	{
		param = theMethod.getParameter(ndx);
	}
	return param;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_getParam(
	const Py::Dict& kws,
	const String& pname)
{
	if (kws.hasKey(pname))
	{
		Py::Object rv = kws[pname];
		if (!rv.isNone())
		{
			return rv;
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
String
_getNameSpace(const CIMObjectPath& cop)
{
	String ns;
	CIMNamespaceName cnsn = cop.getNameSpace();
	if (!cnsn.isNull())
	{
		ns = cnsn.getString();
	}
	return ns;
}

//////////////////////////////////////////////////////////////////////////////
String
_getPythonTraceBack(
	Py::Exception& thrownEx)
{
	Py::Object etype, evalue;
	return Py::getCurrentErrorInfo(etype, evalue);
}

//////////////////////////////////////////////////////////////////////////////
void
_throwPyCIMException(
	CIMStatusCode theErrno,
	const String& message)
{
	// Get pywbem module ref
	Py::Module pywbemMod = PyExtensions::getWBEMMod();

	// Get CTOR of pywbem.CIMError
	Py::Callable excctor = pywbemMod.getAttr("CIMError");
	// Build args for pywbem.CIMError
	Py::Tuple args(2);
	args[0] = Py::Int(int(theErrno));
	args[1] = Py::String(message);
	throw Py::Exception(excctor, args);
}

//////////////////////////////////////////////////////////////////////////////
void
_throwPyCIMException(
	const CIMException& theException)
{
	CIMStatusCode theErrno = theException.getCode();
	String message = theException.getMessage();
	_throwPyCIMException(theErrno, message);
}

//////////////////////////////////////////////////////////////////////////////
void
_throwPyException(
	const Exception& theException)
{
	String message = theException.getMessage();
	_throwPyCIMException(CIM_ERR_FAILED, message);
}

//////////////////////////////////////////////////////////////////////////////
class PyCIMOMHandleException : public CIMException
{
public:
	PyCIMOMHandleException(CIMStatusCode code, const String& msg)
		: CIMException(code, msg)
	{
	}

	PyCIMOMHandleException(const String& file, int lineno,
		CIMStatusCode code, const String& msg)
		: CIMException(code, Formatter::format("Msg: $0  File: $1  Line: $2",
			msg, file, lineno))
	{
	}
};
#define PY_THROW_CIMMSG(code, msg) throw PyCIMOMHandleException(__FILE__, __LINE__, code, msg)

//////////////////////////////////////////////////////////////////////////////
Py::Object
_processCIMNameResults(
	const Array<CIMName>& cnames,
	Py::Callable& cb)
{
	if (cb.isNone())
	{
		// No given callback, so just return the results as a list
		Py::List res;
		for (Uint32 i = 0; i < cnames.size(); i++)
		{
			res.append(Py::String(cnames[i].getString()));
		}
		return res;
	}
	else
	{
		try
		{
			Py::Tuple args(1);
			for (Uint32 i = 0; i < cnames.size(); i++)
			{
				args[0] = Py::String(cnames[i].getString());
				cb.apply(args);
			}
		}
		catch(Py::Exception& e)
		{
			String msg = Formatter::format("Python exception caught: $0",
				_getPythonTraceBack(e));
			e.clear();
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER, msg);
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_processCIMClassResults(
	const Array<CIMClass>& classes,
	Py::Callable& cb)
{
	if (cb.isNone())
	{
		// No given callback, so just return the results as a list
		Py::List res;
		for (Uint32 i = 0; i < classes.size(); i++)
		{
			res.append(PGPyConv::PGClass2Py(classes[i]));
		}
		return res;
	}
	else
	{
		try
		{
			Py::Tuple args(1);
			for (Uint32 i = 0; i < classes.size(); i++)
			{
				args[0] = PGPyConv::PGClass2Py(classes[i]);
				cb.apply(args);
			}
		}
		catch(Py::Exception& e)
		{
			String msg = Formatter::format("Python exception caught: $0",
				_getPythonTraceBack(e));
			e.clear();
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER, msg);
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_processCIMInstanceResults(
	const Array<CIMInstance>& instances,
	const String& ns,
	Py::Callable& cb)
{
	if (cb.isNone())
	{
		// No given callback, so just return the results as a list
		Py::List res;
		for (Uint32 i = 0; i < instances.size(); i++)
		{
			res.append(PGPyConv::PGInst2Py(instances[i], ns));
		}
		return res;
	}
	else
	{
		try
		{
			Py::Tuple args(1);
			for (Uint32 i = 0; i < instances.size(); i++)
			{
				args[0] = PGPyConv::PGInst2Py(instances[i], ns);
				cb.apply(args);
			}
		}
		catch(Py::Exception& e)
		{
			String msg = Formatter::format("Python exception caught: $0",
				_getPythonTraceBack(e));
			e.clear();
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER, msg);
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_processCIMObjectResults(
	const Array<CIMObject>& objects,
	const String& ns,
	Py::Callable& cb)
{
	if (cb.isNone())
	{
		// No given callback, so just return the results as a list
		Py::List res;
		for (Uint32 i = 0; i < objects.size(); i++)
		{
			if (objects[i].isClass())
			{
				res.append(PGPyConv::PGClass2Py(CIMConstClass(objects[i])));
			}
			else
			{
				res.append(PGPyConv::PGInst2Py(CIMConstInstance(objects[i]),
					ns));
			}
		}
		return res;
	}
	else
	{
		try
		{
			Py::Tuple args(1);
			for (Uint32 i = 0; i < objects.size(); i++)
			{
				if (objects[i].isClass())
				{
					args[0] = PGPyConv::PGClass2Py(CIMConstClass(objects[i]));
				}
				else
				{
					args[0] = PGPyConv::PGInst2Py(CIMConstInstance(objects[i]),
						ns);
				}
				cb.apply(args);
			}
		}
		catch(Py::Exception& e)
		{
			String msg = Formatter::format("Python exception caught: $0",
				_getPythonTraceBack(e));
			e.clear();
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER, msg);
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_processCIMQualResults(
	const Array<CIMQualifierDecl>& quals,
	Py::Callable& cb)
{
	if (cb.isNone())
	{
		// No given callback, so just return the results as a list
		Py::List res;
		for (Uint32 i = 0; i < quals.size(); i++)
		{
			res.append(PGPyConv::PGQualType2Py(quals[i]));
		}
		return res;
	}
	else
	{
		try
		{
			Py::Tuple args(1);
			for (Uint32 i = 0; i < quals.size(); i++)
			{
				args[0] = PGPyConv::PGQualType2Py(quals[i]);
				cb.apply(args);
			}
		}
		catch(Py::Exception& e)
		{
			String msg = Formatter::format("Python exception caught: $0",
				_getPythonTraceBack(e));
			e.clear();
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER, msg);
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_processCIMObjectPathResults(
	Array<CIMObjectPath>& cops,
	Py::Callable& cb,
    const String& ns)
{
	if (cb.isNone())
	{
		// No given callback, so just return the results as a list
		Py::List res;
		for (Uint32 i = 0; i < cops.size(); i++)
		{
            cops[i].setNameSpace(ns);
			res.append(PGPyConv::PGRef2Py(cops[i]));
		}
		return res;
	}
	else
	{
		try
		{
			Py::Tuple args(1);
			for (Uint32 i = 0; i < cops.size(); i++)
			{
                cops[i].setNameSpace(ns);
				args[0] = PGPyConv::PGRef2Py(cops[i]);
				cb.apply(args);
			}
		}
		catch(Py::Exception& e)
		{
			String msg = Formatter::format("Python exception caught: $0",
				_getPythonTraceBack(e));
			e.clear();
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER, msg);
		}
	}
	return Py::Nothing();
}

}	// End of unnamed namespace

//////////////////////////////////////////////////////////////////////////////
PyCIMOMHandle::PyCIMOMHandle(
		PythonProviderManager* pmgr,
		const String& provPath)
	: Py::PythonExtension<PyCIMOMHandle>()
	, m_chdl()
	, m_defaultns()
	, m_pmgr(pmgr)
	, m_provPath(provPath)
{
}

//////////////////////////////////////////////////////////////////////////////
PyCIMOMHandle::~PyCIMOMHandle()
{
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::setDefaultNs(
	const Py::Tuple& args)
{
	if (args.length() > 0 && !args[0].isNone())
	{
		m_defaultns = Py::String(args[0]).as_peg_string();
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::exportIndication(
	const Py::Tuple& args)
{
	try
	{
		CIMInstance ci;
		if (args.length() && !args[0].isNone())
		{
			ci = PGPyConv::PyInst2PG(args[0]);
		}

		if (ci.isUninitialized())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'indication_instance' must be given for exportIndication");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}
		PYCXX_ALLOW_THREADS
		m_pmgr->generateIndication(m_provPath, ci, ns);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}

	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
// args:
// 	0: string - ns
// 	1: CIMInstanceName/CIMClassName - path
// 	2: string - methodName
// 	3: 
Py::Object
PyCIMOMHandle::invokeMethod(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String methodName;
		if (args.length() && !args[0].isNone())
		{
			methodName = Py::String(args[0]);
		}

		if (!methodName.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'MethodName' is a required parameter");
		}

		CIMObjectPath objectName;
		if (args.length() > 1 && !args[1].isNone())
		{
			objectName = PGPyConv::PyRef2PG(args[1]);
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ObjectName' is a required parameter");
		}

		String ns = _getNameSpace(objectName);
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'ObjectName' does not have a namespace");
			}
		}
		CIMName className = objectName.getClassName();
		CIMClass cc;
		PYCXX_ALLOW_THREADS
		cc = m_chdl.getClass(OperationContext(), ns, className, false, 
			true, true, CIMPropertyList());
		PYCXX_END_ALLOW_THREADS
		Uint32 ndx = cc.findMethod(methodName);
		if (ndx == PEG_NOT_FOUND)
		{
			PY_THROW_CIMMSG(CIM_ERR_METHOD_NOT_FOUND,
				Formatter::format("Class $0 has no method called $1", 
					className.getString(), methodName));
		}
		CIMMethod method = cc.getMethod(ndx);

		// Get Input parameters
		Array<CIMParamValue> inParams;
		Py::List values = kws.values();
		Py::List keys = kws.keys();
		for (int i = 0; i < int(keys.length()); i++)
		{
			// Get parameter name
			String pname = Py::String(keys[i]).as_peg_string();
			CIMParameter pgparam = _getCIMParam(pname, method);
			if (!pgparam.isUninitialized())
			{
				CIMType dt = pgparam.getType();
				String pydt = PGPyConv::PGDataType2Py(dt);
				CIMValue cv = PGPyConv::PyVal2PG(pydt, values[i]);
				inParams.append(CIMParamValue(pname, dt, true));
			}
		}

		Array<CIMParamValue> outParams;
		CIMValue rcv;
		PYCXX_ALLOW_THREADS
		rcv = m_chdl.invokeMethod(OperationContext(), ns, objectName, methodName,
			inParams, outParams);
		PYCXX_END_ALLOW_THREADS

		Py::Tuple rtuple(2);
		rtuple[0] = rcv.isNull() ? Py::None() : PGPyConv::PGVal2Py(rcv);
		Py::Dict pyoutParams;
		for(Uint32 i = 0; i < outParams.size(); i++)
		{
			String pname = outParams[i].getParameterName();
			if (outParams[i].isUninitialized())
			{
				pyoutParams[pname] = Py::None();
			}
			else
			{
				CIMValue cv = outParams[i].getValue();
				if (cv.isNull())
				{
					pyoutParams[pname] = Py::None();
				}
				else
				{
					pyoutParams[pname] = PGPyConv::PGVal2Py(cv);
				}
			}
		}
		rtuple[1] = pyoutParams;
		return rtuple;
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}

	// Shouldn't hit this
	throw Py::RuntimeError("invokeMethod Logic error");
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumClassNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String ns;
		if (args.length() > 0 && !args[0].isNone())
		{
			ns = Py::String(args[0]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		String className;
		Py::Object wko = _getParam(kws, "ClassName");
		if (!wko.isNone())
		{
			className = Py::String(wko).as_peg_string();
		}

		bool deepflg = false;
		wko = _getParam(kws, "DeepInheritance");
		if (!wko.isNone())
		{
			if (wko.isTrue())
			{
				deepflg = true;
			}
		}

		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}

		Array<CIMName> cnames;
		PYCXX_ALLOW_THREADS
		cnames = m_chdl.enumerateClassNames(OperationContext(), ns, className, deepflg);
		PYCXX_END_ALLOW_THREADS
		return _processCIMNameResults(cnames, cb);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String ns;
		if (args.length() > 0 && !args[0].isNone())
		{
			ns = Py::String(args[0]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		String className;
		Py::Object wko = _getParam(kws, "ClassName");
		if (!wko.isNone())
		{
			className = Py::String(wko).as_peg_string();
		}

		bool deepFlg = false;
		wko = _getParam(kws, "DeepInheritance");
		if (!wko.isNone())
		{
			if (wko.isTrue())
			{
				deepFlg = true;
			}
		}

		bool localOnlyFlag = true;
		wko = _getParam(kws, "LocalOnly");
		if (!wko.isNone())
		{
			if (!wko.isTrue())
			{
				localOnlyFlag = false;
			}
		}

		bool incQualsFlag = true;
		wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone())
		{
			if (!wko.isTrue())
			{
				incQualsFlag = false;
			}
		}

		bool classOriginFlag = false;
		wko = _getParam(kws, "IncludeClassOrigin");
		if (!wko.isNone())
		{
			if (wko.isTrue())
			{
				classOriginFlag = true;
			}
		}

		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}
		Array<CIMClass> classes;
		PYCXX_ALLOW_THREADS
		classes = m_chdl.enumerateClasses(OperationContext(), ns, className,
			deepFlg, localOnlyFlag, incQualsFlag, classOriginFlag);
		PYCXX_END_ALLOW_THREADS
		return _processCIMClassResults(classes, cb);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String className;
		if (args.length() && !args[0].isNone())
		{
			className = Py::String(args[0]).as_peg_string();
		}

		if (!className.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ClassName' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		bool localOnlyFlag = true;
		Py::Object wko = _getParam(kws, "LocalOnly");
		if (!wko.isNone())
		{
			if (!wko.isTrue())
			{
				localOnlyFlag = false;
			}
		}

		bool incQualsFlag = true;
		wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone())
		{
			if (!wko.isTrue())
			{
				incQualsFlag = false;
			}
		}

		bool classOriginFlag = false;
		wko = _getParam(kws, "IncludeClassOrigin");
		if (!wko.isNone())
		{
			if (wko.isTrue())
			{
				classOriginFlag = true;
			}
		}

		CIMPropertyList propList;
		wko = _getParam(kws, "PropertyList");
		if (!wko.isNone())
		{
			Array<CIMName> namera;
			Py::List pl(wko);
			for (Py::List::size_type i = 0; i < pl.length(); i++)
			{
				namera.append(CIMName(Py::String(pl[i]).as_peg_string()));
			}
			propList.set(namera);
		}

		CIMClass cc;
		PYCXX_ALLOW_THREADS
		cc = m_chdl.getClass(OperationContext(), ns, className, localOnlyFlag,
			incQualsFlag, classOriginFlag, propList);
		PYCXX_END_ALLOW_THREADS
		return PGPyConv::PGClass2Py(cc);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::createClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMClass cc;
		if (args.length() && !args[0].isNone())
		{
			cc = PGPyConv::PyClass2PG(args[0]);
		}

		if (cc.isUninitialized())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'NewClass' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}
		PYCXX_ALLOW_THREADS
		m_chdl.createClass(OperationContext(), ns, cc);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::deleteClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String className;
		if (args.length() && !args[0].isNone())
		{
			className = Py::String(args[0]).as_peg_string();
		}

		if (!className.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ClassName' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}
		PYCXX_ALLOW_THREADS
		m_chdl.deleteClass(OperationContext(), ns, className);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::modifyClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMClass cc;
		if (args.length() && !args[0].isNone())
		{
			cc = PGPyConv::PyClass2PG(args[0]);
		}

		if (cc.isUninitialized())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ModifiedClass' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}
		PYCXX_ALLOW_THREADS
		m_chdl.modifyClass(OperationContext(), ns, cc);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumQualifiers(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String ns;
		if (args.length() > 0 && !args[0].isNone())
		{
			ns = Py::String(args[0]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		Py::Callable cb;
		Py::Object wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}

		CIMClient client;
		client.connectLocal();
		Array<CIMQualifierDecl> quals;
		PYCXX_ALLOW_THREADS
		quals = client.enumerateQualifiers(ns);
		PYCXX_END_ALLOW_THREADS
		client.disconnect();
		return _processCIMQualResults(quals, cb);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getQualifier(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String qualName;
		if (args.length() && !args[0].isNone())
		{
			qualName = Py::String(args[0]).as_peg_string();
		}
		if (!qualName.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'QualifierName' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}
		CIMClient client;
		client.connectLocal();
		CIMQualifierDecl cqt;
		PYCXX_ALLOW_THREADS
		cqt = client.getQualifier(ns, qualName);
		PYCXX_END_ALLOW_THREADS
		client.disconnect();
		return PGPyConv::PGQualType2Py(cqt);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::setQualifier(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMQualifierDecl cqt;
		if (args.length() && !args[0].isNone())
		{
			cqt = PGPyConv::PyQualType2PG(args[0]);
		}

		if (cqt.isUninitialized())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'QualifierDeclaration' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		CIMClient client;
		client.connectLocal();
		PYCXX_ALLOW_THREADS
		client.setQualifier(ns, cqt);
		PYCXX_END_ALLOW_THREADS
		client.disconnect();
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::deleteQualifier(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String qualName;
		if (args.length() && !args[0].isNone())
		{
			qualName = Py::String(args[0]);
		}

		if (!qualName.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'QualifierName' is a required parameter");
		}

		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}
		CIMClient client;
		client.connectLocal();
		PYCXX_ALLOW_THREADS
		client.deleteQualifier(ns, qualName);
		PYCXX_END_ALLOW_THREADS
		client.disconnect();
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumInstanceNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String className;
		if (args.length() && !args[0].isNone())
		{
			className = Py::String(args[0]).as_peg_string();
		}
		if (!className.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ClassName' is a required parameter");
		}
		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		Py::Callable cb;
		Py::Object wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}

		Array<CIMObjectPath> cops;
		PYCXX_ALLOW_THREADS
        cerr << " CH Namespace: " << ns << endl;
		cops = m_chdl.enumerateInstanceNames(OperationContext(), ns, className);
		PYCXX_END_ALLOW_THREADS
		return _processCIMObjectPathResults(cops, cb, ns);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumInstances(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		String className;
		if (args.length() && !args[0].isNone())
		{
			className = Py::String(args[0]).as_peg_string();
		}
		if (!className.size())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ClassName' is a required parameter");
		}
		String ns;
		if (args.length() > 1 && !args[1].isNone())
		{
			ns = Py::String(args[1]).as_peg_string();
		}

		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'namespace' is a required parameter");
			}
		}

		bool deepFlg = false;
		Py::Object wko = _getParam(kws, "DeepInheritance");
		if (!wko.isNone() && wko.isTrue())
		{
			deepFlg = true;
		}

		bool localOnlyFlag = true;
		wko = _getParam(kws, "LocalOnly");
		if (!wko.isNone() && !wko.isTrue())
		{
			localOnlyFlag = false;
		}

		bool incQualsFlag = true;
		wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone() && !wko.isTrue())
		{
			incQualsFlag = false;
		}

		bool classOriginFlag = false;
		wko = _getParam(kws, "IncludeClassOrigin");
		if (!wko.isNone() && wko.isTrue())
		{
			classOriginFlag = true;
		}

		CIMPropertyList propList;
		wko = _getParam(kws, "PropertyList");
		if (!wko.isNone())
		{
			Array<CIMName> namera;
			Py::List pl(wko);
			for (Py::List::size_type i = 0; i < pl.length(); i++)
			{
				namera.append(CIMName(Py::String(pl[i]).as_peg_string()));
			}
			propList.set(namera);
		}

		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}

		Array<CIMInstance> instances;
		PYCXX_ALLOW_THREADS
		instances = m_chdl.enumerateInstances(OperationContext(), ns, className, deepFlg, localOnlyFlag,
			incQualsFlag, classOriginFlag, propList);
		PYCXX_END_ALLOW_THREADS
		return _processCIMInstanceResults(instances, ns, cb);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMObjectPath instanceName;
		if (args.length() && !args[0].isNone())
		{
			instanceName = PGPyConv::PyRef2PG(Py::Object(args[0]));
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'InstanceName' is a required parameter");
		}

		String ns = instanceName.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'InstanceName' parameter does not have a namespace");
			}
		}

		bool localOnlyFlag = true;
		Py::Object wko = _getParam(kws, "LocalOnly");
		if (!wko.isNone() && !wko.isTrue())
		{
			localOnlyFlag = false;
		}

		bool incQualsFlag = true;
		wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone() && !wko.isTrue())
		{
			incQualsFlag = false;
		}

		bool classOriginFlag = false;
		wko = _getParam(kws, "IncludeClassOrigin");
		if (!wko.isNone() && wko.isTrue())
		{
			classOriginFlag = true;
		}

		CIMPropertyList propList;
		wko = _getParam(kws, "PropertyList");
		if (!wko.isNone())
		{
			Array<CIMName> namera;
			Py::List pl(wko);
			for (Py::List::size_type i = 0; i < pl.length(); i++)
			{
				namera.append(CIMName(Py::String(pl[i]).as_peg_string()));
			}
			propList.set(namera);
		}
		CIMInstance ci;
		PYCXX_ALLOW_THREADS
		ci = m_chdl.getInstance(OperationContext(), ns, instanceName,
			localOnlyFlag, incQualsFlag, classOriginFlag, propList);
		ci.setPath(instanceName);
		PYCXX_END_ALLOW_THREADS
		return PGPyConv::PGInst2Py(ci, ns);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::deleteInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMObjectPath instanceName;
		if (args.length() && !args[0].isNone())
		{
			instanceName = PGPyConv::PyRef2PG(Py::Object(args[0]));
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'InstanceName' is a required parameter");
		}

		String ns = instanceName.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'InstanceName' parameter does not have a namespace");
			}
		}
		PYCXX_ALLOW_THREADS
		m_chdl.deleteInstance(OperationContext(), ns, instanceName);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::createInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMInstance ci;
		if (args.length() && !args[0].isNone())
		{
			ci = PGPyConv::PyInst2PG(args[0]);
		}

		if (ci.isUninitialized())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'NewInstance' is a required parameter");
		}

		CIMObjectPath cop = ci.getPath();
		String ns = cop.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'NewInstance.path' does not contain a namespace");
			}
		}
		CIMObjectPath mcop;
		PYCXX_ALLOW_THREADS
		mcop = m_chdl.createInstance(OperationContext(), ns, ci);
		mcop.setNameSpace(ns);
		PYCXX_END_ALLOW_THREADS
		return PGPyConv::PGRef2Py(mcop);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::modifyInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMInstance ci;
		if (args.length() && !args[0].isNone())
		{
			ci = PGPyConv::PyInst2PG(args[0]);
		}

		if (ci.isUninitialized())
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ModifiedInstance' is a required parameter");
		}

		CIMObjectPath cop = ci.getPath();
		String ns = cop.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'ModifiedInstance.path' does not contain a namespace");
			}
		}

		bool incQualsFlag = true;
		Py::Object wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone() && !wko.isTrue())
		{
			incQualsFlag = false;
		}

		CIMPropertyList propList;
		wko = _getParam(kws, "PropertyList");
		if (!wko.isNone())
		{
			Array<CIMName> namera;
			Py::List pl(wko);
			for (Py::List::size_type i = 0; i < pl.length(); i++)
			{
				namera.append(CIMName(Py::String(pl[i]).as_peg_string()));
			}
			propList.set(namera);
		}

		PYCXX_ALLOW_THREADS
		m_chdl.modifyInstance(OperationContext(), ns, ci,
			incQualsFlag, propList);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::associators(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMObjectPath objectName;
		if (args.length() && !args[0].isNone())
		{
			objectName = PGPyConv::PyRef2PG(args[0]);
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ObjectName' is a required parameter");
		}

		String ns = objectName.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'ObjectName' parameter does not have a namespace");
			}
		}

		CIMName assocClass;
		Py::Object wko = _getParam(kws, "AssocClass");
		if (!wko.isNone())
		{
			assocClass = _pyStr2CIMName(wko);
		}
		CIMName resultClass;
		wko = _getParam(kws, "ResultClass");
		if (!wko.isNone())
		{
			resultClass = _pyStr2CIMName(wko);
		}
		String role;
		wko = _getParam(kws, "Role");
		if (!wko.isNone())
		{
			role = Py::String(wko).as_peg_string();
		}
		String resultRole;
		wko = _getParam(kws, "ResultRole");
		if (!wko.isNone())
		{
			resultRole = Py::String(wko).as_peg_string();
		}

		bool incQualsFlag = true;
		wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone() && !wko.isTrue())
		{
			incQualsFlag = false;
		}

		bool classOriginFlag = false;
		wko = _getParam(kws, "IncludeClassOrigin");
		if (!wko.isNone() && wko.isTrue())
		{
			classOriginFlag = true;
		}

		CIMPropertyList propList;
		wko = _getParam(kws, "PropertyList");
		if (!wko.isNone())
		{
			Array<CIMName> namera;
			Py::List pl(wko);
			for (Py::List::size_type i = 0; i < pl.length(); i++)
			{
				namera.append(CIMName(Py::String(pl[i]).as_peg_string()));
			}
			propList.set(namera);
		}

		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}

		Array<CIMObject> cimobjs;
		PYCXX_ALLOW_THREADS
		cimobjs = m_chdl.associators(OperationContext(), ns, objectName, 
			assocClass, resultClass, role, resultRole, incQualsFlag,
			classOriginFlag, propList);
		PYCXX_END_ALLOW_THREADS
		return _processCIMObjectResults(cimobjs, ns, cb);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::associatorNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
        cerr << " entering CH:assocNames " << endl;
		CIMObjectPath objectName;
		if (args.length() && !args[0].isNone())
		{
            cerr << " CH:assocNames:objectName/args[0]: " << args[0] << endl;
			objectName = PGPyConv::PyRef2PG(args[0]);
            cerr << " CH:assocNames:objectName: " << objectName.toString() << endl;
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ObjectName' is a required parameter");
		}

		String ns = objectName.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'ObjectName' parameter does not have a namespace");
			}
		}

		CIMName assocClass;
		Py::Object wko = _getParam(kws, "AssocClass");
		if (!wko.isNone())
		{
			assocClass = _pyStr2CIMName(wko);
		}
		CIMName resultClass;
		wko = _getParam(kws, "ResultClass");
		if (!wko.isNone())
		{
			resultClass = _pyStr2CIMName(wko);
		}
		String role;
		wko = _getParam(kws, "Role");
		if (!wko.isNone())
		{
			role = Py::String(wko).as_peg_string();
		}
		String resultRole;
		wko = _getParam(kws, "ResultRole");
		if (!wko.isNone())
		{
			resultRole = Py::String(wko).as_peg_string();
		}

		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}
		Array<CIMObjectPath> names;
		PYCXX_ALLOW_THREADS
        cerr << " CH:assocNames ns: " << ns << endl;
		names = m_chdl.associatorNames(OperationContext(), ns, objectName,
			assocClass, resultClass, role, resultRole);
		PYCXX_END_ALLOW_THREADS
		return _processCIMObjectPathResults(names, cb, ns);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::references(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMObjectPath objectName;
		if (args.length() && !args[0].isNone())
		{
			objectName = PGPyConv::PyRef2PG(args[0]);
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ObjectName' is a required parameter");
		}

		String ns = objectName.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'ObjectName' parameter does not have a namespace");
			}
		}

		CIMName resultClass;
		Py::Object wko = _getParam(kws, "ResultClass");
		if (!wko.isNone())
		{
			resultClass = Py::String(wko).as_peg_string();
		}
		String role;
		wko = _getParam(kws, "Role");
		if (!wko.isNone())
		{
			role = Py::String(wko).as_peg_string();
		}

		bool incQualsFlag = true;
		wko = _getParam(kws, "IncludeQualifiers");
		if (!wko.isNone() && !wko.isTrue())
		{
			incQualsFlag = false;
		}

		bool classOriginFlag = false;
		wko = _getParam(kws, "IncludeClassOrigin");
		if (!wko.isNone() && wko.isTrue())
		{
			classOriginFlag = true;
		}

		CIMPropertyList propList;
		wko = _getParam(kws, "PropertyList");
		if (!wko.isNone())
		{
			Array<CIMName> namera;
			Py::List pl(wko);
			for (Py::List::size_type i = 0; i < pl.length(); i++)
			{
				namera.append(CIMName(Py::String(pl[i]).as_peg_string()));
			}
			propList.set(namera);
		}

		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}
		Array<CIMObject> cimobjs;
		PYCXX_ALLOW_THREADS
		cimobjs = m_chdl.references(OperationContext(), ns, objectName, resultClass, role, 
			incQualsFlag, classOriginFlag, propList);
		PYCXX_END_ALLOW_THREADS
		return _processCIMObjectResults(cimobjs, ns, cb);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::referenceNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	try
	{
		CIMObjectPath objectName;
		if (args.length() && !args[0].isNone())
		{
			objectName = PGPyConv::PyRef2PG(args[0]);
		}
		else
		{
			PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
				"'ObjectName' is a required parameter");
		}

		String ns = objectName.getNameSpace().getString();
		if (!ns.size())
		{
			ns = m_defaultns;
			if (!ns.size())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'ObjectName' parameter does not have a namespace");
			}
		}

		CIMName resultClass;
		Py::Object wko = _getParam(kws, "ResultClass");
		if (!wko.isNone())
		{
			resultClass = _pyStr2CIMName(wko);
		}
		String role;
		wko = _getParam(kws, "Role");
		if (!wko.isNone())
		{
			role = Py::String(wko).as_peg_string();
		}
		Py::Callable cb;
		wko = _getParam(kws, "Handler");
		if (!wko.isNone())
		{
			if (!wko.isCallable())
			{
				PY_THROW_CIMMSG(CIM_ERR_INVALID_PARAMETER,
					"'Handler' parameter must be a callable object");
			}
			cb = wko;
		}
		Array<CIMObjectPath> cops;
		PYCXX_ALLOW_THREADS
		cops = m_chdl.referenceNames(OperationContext(), ns, objectName,
			resultClass, role);
		PYCXX_END_ALLOW_THREADS
		return _processCIMObjectPathResults(cops, cb, ns);
	}
	catch(const CIMException& e)
	{
		_throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		_throwPyException(e);
	}
	catch(...)
	{
		_throwPyCIMException(CIM_ERR_FAILED, "Unknown exception");
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
bool
PyCIMOMHandle::accepts(
	PyObject *pyob) const
{
	return pyob && PyCIMOMHandle::check(pyob);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::repr()
{
	return Py::String("Provider CIM Instance Result Handler");
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getattr(
	const char *name)
{
	return getattr_methods(name);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
void
PyCIMOMHandle::doInit()
{
	behaviors().name("CIMOMHandle");
	behaviors().doc("CIMOMHandle objects allow providers to communicate with "
		"the hosting CIMOM");
	behaviors().supportRepr();
	behaviors().supportGetattr();

	add_varargs_method("set_default_namespace", &PyCIMOMHandle::setDefaultNs, 
		"Set the default namespace that will used for CIMOM up call operations. "
		"If this is not called, then the namespace parameter must be supplied "
		"for call calls into the CIMOM");

	add_varargs_method("export_indication", &PyCIMOMHandle::exportIndication,
		"Export a given instance of an indication.This will cause all "
		"CIMListerners that are interested in this type of indication "
		"to be notified.");

	add_keyword_method("InvokeMethod", &PyCIMOMHandle::invokeMethod, "Doc for InvokeMethod");
	add_keyword_method("EnumerateClassNames", &PyCIMOMHandle::enumClassNames, "Doc for EnumerateClassNames");
	add_keyword_method("EnumerateClasses", &PyCIMOMHandle::enumClass, "Doc for EnumerateClasses");
	add_keyword_method("GetClass", &PyCIMOMHandle::getClass, "Doc for GetClass");
	add_keyword_method("CreateClass", &PyCIMOMHandle::createClass, "Doc for CreateClass");
	add_keyword_method("DeleteClass", &PyCIMOMHandle::deleteClass, "Doc for DeleteClass");
	add_keyword_method("ModifyClass", &PyCIMOMHandle::modifyClass, "Doc for ModifyClass");
	add_keyword_method("EnumerateQualifiers", &PyCIMOMHandle::enumQualifiers, "Doc for EnumerateQualifiers");
	add_keyword_method("GetQualifier", &PyCIMOMHandle::getQualifier, "Doc for GetQualifier");
	add_keyword_method("SetQualifier", &PyCIMOMHandle::setQualifier, "Doc for SetQualifier");
	add_keyword_method("DeleteQualifier", &PyCIMOMHandle::deleteQualifier, "Doc for DeleteQualifier");
	add_keyword_method("EnumerateInstanceNames", &PyCIMOMHandle::enumInstanceNames, "Doc for EnumerateInstanceNames");
	add_keyword_method("EnumerateInstances", &PyCIMOMHandle::enumInstances, "Doc for EnumerateInstances");
	add_keyword_method("GetInstance", &PyCIMOMHandle::getInstance, "Doc for GetInstance");
	add_keyword_method("DeleteInstance", &PyCIMOMHandle::deleteInstance, "Doc for DeleteInstance");
	add_keyword_method("CreateInstance", &PyCIMOMHandle::createInstance, "Doc for CreateInstance");
	add_keyword_method("ModifyInstance", &PyCIMOMHandle::modifyInstance, "Doc for ModifyInstance");
	add_keyword_method("Associators", &PyCIMOMHandle::associators, "Doc for Associators");
	add_keyword_method("AssociatorNames", &PyCIMOMHandle::associatorNames, "Doc for AssociatorNames");
	add_keyword_method("References", &PyCIMOMHandle::references, "Doc for References");
	add_keyword_method("ReferenceNames", &PyCIMOMHandle::referenceNames, "Doc for ReferenceNames");
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PyCIMOMHandle::newObject(
	PythonProviderManager* pmgr,
	const String& provPath,
	PyCIMOMHandle **pchdl)
{
	PyCIMOMHandle* ph = new PyCIMOMHandle(pmgr, provPath);
	if (pchdl)
	{
		*pchdl = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC
