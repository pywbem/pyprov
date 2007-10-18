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
#include "OW_PyConverter.hpp"
#include <openwbem/OW_CIMQualifier.hpp>
#include <openwbem/OW_CIMQualifierType.hpp>
#include <openwbem/OW_CIMException.hpp>
#include <openwbem/OW_String.hpp>
#include <openwbem/OW_Array.hpp>
#include <openwbem/OW_ResultHandlers.hpp>
#include <openwbem/OW_Format.hpp>
#include <openwbem/OW_CIMParamValue.hpp>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace OW_NAMESPACE;
using namespace WBEMFlags;

namespace PythonProvIFC
{

namespace
{

//////////////////////////////////////////////////////////////////////////////
String
getPythonTraceBack(
	Py::Exception& thrownEx)
{
	Py::Object etype, evalue;
	return Py::getCurrentErrorInfo(etype, evalue);
}

//////////////////////////////////////////////////////////////////////////////
CIMParameter
getCIMParam(
	const String& paramName,
	const CIMParameterArray& params)
{
	for(CIMParameterArray::size_type i = 0; i < params.size(); i++)
	{
		if (paramName.equalsIgnoreCase(params[i].getName()))
		{
			return params[i];
		}
	}
	return CIMParameter(CIMNULL);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
getParam(
	const Py::Dict& kws,
	const String& pname)
{
	if (kws.hasKey(pname.c_str()))
	{
		Py::Object rv = kws[pname.c_str()];
		if (!rv.isNone())
		{
			return rv;
		}
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
void
throwPyCIMException(
	CIMException::ErrNoType theErrno,
	const String& message)
{
	// Get pywbem module ref
	Py::Module pywbemMod = PyProviderModule::getWBEMMod();

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
throwPyCIMException(
	const CIMException& theException)
{
	CIMException::ErrNoType theErrno = theException.getErrNo();
	String message = theException.getDescription();
	if (message.empty())
	{
		message = theException.getMessage();
	}
	throwPyCIMException(theErrno, message);
}

//////////////////////////////////////////////////////////////////////////////
void
throwPyException(
	const Exception& theException)
{
	String message = theException.getMessage();
	throwPyCIMException(CIMException::FAILED, message);
}

//////////////////////////////////////////////////////////////////////////////
String
getStringParam(
	const Py::Dict& kws,
	const String& key,
	const String& exceptionMsg=String())
{
	if (!kws.hasKey(key))
	{
		if (exceptionMsg.length())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER, exceptionMsg);
		}
		return String();
	}

	return Py::String(kws[key]).as_ow_string();
}

//////////////////////////////////////////////////////////////////////////////
class PyClassResultHandler : public CIMClassResultHandlerIFC
{
public:
	PyClassResultHandler(Py::Object& pycb)
		: CIMClassResultHandlerIFC()
		, m_res()
		, m_pycb()
	{
		if (pycb.isCallable())
		{
			m_pycb = pycb;
		}
	}

	virtual void doHandle(const CIMClass& cc)
	{
		Py::GILGuard gg;	// Acquire python's GIL
		if (!m_pycb.isNone())
		{
			try
			{
				Py::Tuple args(1);
				args[0] = OWPyConv::OWClass2Py(cc);
				m_pycb.apply(args);
			}
			catch(Py::Exception& e)
			{
				String msg = Format("Python exception caught: %1",
					getPythonTraceBack(e));
				e.clear();
				throwPyCIMException(CIMException::FAILED, msg.c_str());
			}
		}
		else
		{
			m_res.append(OWPyConv::OWClass2Py(cc));
		}
	}

	Py::Object getResult() const 
	{ 
		return (!m_pycb.isNone()) ? Py::Nothing() : Py::Object(m_res);
	}

private:
	Py::List m_res;
	Py::Callable m_pycb;
};

class PyQualResultHandler : public CIMQualifierTypeResultHandlerIFC
{
public:
	PyQualResultHandler(Py::Object& pycb)
		: CIMQualifierTypeResultHandlerIFC()
		, m_res()
		, m_pycb()
	{
		if (pycb.isCallable())
		{
			m_pycb = pycb;
		}
	}

	virtual void doHandle(const CIMQualifierType& cqt)
	{
		Py::GILGuard gg;	// Acquire python's GIL
		if (!m_pycb.isNone())
		{
			try
			{
				Py::Tuple args(1);
				args[0] = OWPyConv::OWQualType2Py(cqt);
				m_pycb.apply(args);
			}
			catch(Py::Exception& e)
			{
				String msg = Format("Python exception caught: %1",
					getPythonTraceBack(e));
				e.clear();
				throwPyCIMException(CIMException::FAILED, msg.c_str());
			}
		}
		else
		{
			m_res.append(OWPyConv::OWQualType2Py(cqt));
		}
	}

	Py::Object getResult() const 
	{ 
		return (!m_pycb.isNone()) ? Py::Nothing() : Py::Object(m_res);
	}

private:
	Py::List m_res;
	Py::Callable m_pycb;
};

class PyOpResultHandler : public CIMObjectPathResultHandlerIFC
{
public:
	PyOpResultHandler(const String& ns, Py::Object& pycb)
		: CIMObjectPathResultHandlerIFC()
		, m_res()
		, m_ns(ns)
		, m_pycb()
	{
		if (pycb.isCallable())
		{
			m_pycb = pycb;
		}
	}

	virtual void doHandle(const CIMObjectPath& cop)
	{
		Py::GILGuard gg;	// Acquire python's GIL
		CIMObjectPath lcop(cop);
		if (lcop.getNameSpace().empty())
		{
			lcop.setNameSpace(m_ns);
		}

		if (!m_pycb.isNone())
		{
			try
			{
				Py::Tuple args(1);
				args[0] = OWPyConv::OWRef2Py(lcop);
				m_pycb.apply(args);
			}
			catch(Py::Exception& e)
			{
				String msg = Format("Python exception caught: %1",
					getPythonTraceBack(e));
				e.clear();
				throwPyCIMException(CIMException::FAILED, msg.c_str());
			}
		}
		else
		{
			m_res.append(OWPyConv::OWRef2Py(lcop));
		}
	}

	Py::Object getResult() const 
	{ 
		return (!m_pycb.isNone()) ? Py::Nothing() : Py::Object(m_res);
	}

private:
	Py::List m_res;
	String m_ns;
	Py::Callable m_pycb;
};

class PyInstResultHandler : public CIMInstanceResultHandlerIFC
{
public:
	PyInstResultHandler(const String& ns, Py::Object& pycb)
		: CIMInstanceResultHandlerIFC()
		, m_res()
		, m_ns(ns)
		, m_pycb()
	{
		if (pycb.isCallable())
		{
			m_pycb = pycb;
		}
	}

	virtual void doHandle(const CIMInstance& ci)
	{
		Py::GILGuard gg;	// Acquire python's GIL
		if (!m_pycb.isNone())
		{
			try
			{
				Py::Tuple args(1);
				args[0] = OWPyConv::OWInst2Py(ci, m_ns);
				m_pycb.apply(args);
			}
			catch(Py::Exception& e)
			{
				String msg = Format("Python exception caught: %1",
					getPythonTraceBack(e));
				e.clear();
				throwPyCIMException(CIMException::FAILED, msg.c_str());
			}
		}
		else
		{
			m_res.append(OWPyConv::OWInst2Py(ci, m_ns));
		}
	}

	Py::Object getResult() const 
	{ 
		return (!m_pycb.isNone()) ? Py::Nothing() : Py::Object(m_res);
	}

private:
	Py::List m_res;
	String m_ns;
	Py::Callable m_pycb;
};

class PyStringResultHandler : public StringResultHandlerIFC
{
public:
	PyStringResultHandler(Py::Object& pycb)
		: StringResultHandlerIFC()
		, m_res()
		, m_pycb()
	{
		if (pycb.isCallable())
		{
			m_pycb = pycb;
		}
	}

	virtual void doHandle(const String& arg)
	{
		Py::GILGuard gg;	// Acquire python's GIL
		if (!m_pycb.isNone())
		{
			try
			{
				Py::Tuple args(1);
				args[0] = Py::String(arg);
				m_pycb.apply(args);
			}
			catch(Py::Exception& e)
			{
				String msg = Format("Python exception caught: %1",
					getPythonTraceBack(e));
				e.clear();
				throwPyCIMException(CIMException::FAILED, msg.c_str());
			}
		}
		else
		{
			m_res.append(Py::String(arg));
		}
	}

	Py::Object getResult() const 
	{ 
		return (!m_pycb.isNone()) ? Py::Nothing() : Py::Object(m_res);
	}

private:
	Py::List m_res;
	Py::Callable m_pycb;
};

}	// End of unnamed namespace

//////////////////////////////////////////////////////////////////////////////
PyCIMOMHandle::PyCIMOMHandle(
	CIMOMHandleIFCRef& chdl)
	: Py::PythonExtension<PyCIMOMHandle>()
	, m_chdl(chdl)
	, m_defaultns()
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
		m_defaultns = Py::String(args[0]).as_ow_string();
	}
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::exportIndication(
	const Py::Tuple& args)
{
	CIMInstance ci(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		ci = OWPyConv::PyInst2OW(args[0]);
	}

	if (!ci)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'indication_instance' must be given for exportIndication");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->exportIndication(ci, ns);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
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
	String methodName;
	if (args.length() && !args[0].isNone())
	{
		methodName = Py::String(args[0]);
	}

	if (methodName.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'MethodName' is a required parameter");
	}

	CIMObjectPath objectName(CIMNULL);
	if (args.length() > 1 && !args[1].isNone())
	{
		objectName = OWPyConv::PyRef2OW(args[1]);
	}

	if (!objectName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ObjectName' is a required parameter");
	}

	String ns = objectName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'ObjectName' does not have a namespace");
		}
	}

	try
	{
		String className = objectName.getClassName();
		CIMClass cc;
		PYCXX_ALLOW_THREADS
		cc = m_chdl->getClass(ns, className, E_NOT_LOCAL_ONLY,
			E_INCLUDE_QUALIFIERS, E_INCLUDE_CLASS_ORIGIN);
		PYCXX_END_ALLOW_THREADS
		CIMMethod method = cc.getMethod(methodName);
		if (!method)
		{
			OW_THROWCIMMSG(CIMException::METHOD_NOT_FOUND,
				Format("Class %1 has no method called %2", className,
					methodName).c_str());
		}

		// Get Input parameters
		CIMParamValueArray inParams;
		CIMParameterArray methInParams = method.getINParameters();
		Py::List values = kws.values();
		Py::List keys = kws.keys();
		for (int i = 0; i < int(keys.length()); i++)
		{
			// Get parameter name
			String pname = Py::String(keys[i]).as_ow_string();
			CIMParameter owparam = getCIMParam(pname, methInParams);
			if (owparam)
			{
				CIMDataType::Type dt = owparam.getType().getType();
				String pydt = OWPyConv::OWDataType2Py(dt);
				CIMValue cv = OWPyConv::PyVal2OW(pydt, values[i]);
				inParams.append(CIMParamValue(pname, cv));
			}
		}

		CIMParamValueArray outParams;
		CIMValue rcv(CIMNULL);
		PYCXX_ALLOW_THREADS
		rcv = m_chdl->invokeMethod(ns, objectName, methodName,
			inParams, outParams);
		PYCXX_END_ALLOW_THREADS

		Py::Tuple rtuple(2);
		rtuple[0] = OWPyConv::OWVal2Py(rcv);
		Py::Dict pyoutParams;
		for(CIMParamValueArray::size_type i = 0; i < outParams.size(); i++)
		{
			String pname = outParams[i].getName();
			CIMValue cv = outParams[i].getValue();
			if (!cv)
			{
				pyoutParams[pname] = Py::None();
			}
			else
			{
				pyoutParams[pname] = OWPyConv::OWVal2Py(cv);
			}
		}

		rtuple[1] = pyoutParams;
		return rtuple;
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
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
	String ns;
	if (args.length() > 0 && !args[0].isNone())
	{
		ns = Py::String(args[0]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	String className;
	Py::Object wko = getParam(kws, "ClassName");
	if (!wko.isNone())
	{
		className = Py::String(wko).as_ow_string();
	}

	EDeepFlag flg = E_SHALLOW;
	wko = getParam(kws, "DeepInheritance");
	if (!wko.isNone())
	{
		if (wko.isTrue())
		{
			flg = E_DEEP;
		}
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyStringResultHandler rhandler(cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->enumClassNames(ns, className, rhandler, flg);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}
	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String ns;
	if (args.length() > 0 && !args[0].isNone())
	{
		ns = Py::String(args[0]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	String className;
	Py::Object wko = getParam(kws, "ClassName");
	if (!wko.isNone())
	{
		className = Py::String(wko).as_ow_string();
	}

	EDeepFlag deepFlg = E_SHALLOW;
	wko = getParam(kws, "DeepInheritance");
	if (!wko.isNone())
	{
		if (wko.isTrue())
		{
			deepFlg = E_DEEP;
		}
	}

	ELocalOnlyFlag localOnlyFlag = E_LOCAL_ONLY;
	wko = getParam(kws, "LocalOnly");
	if (!wko.isNone())
	{
		if (!wko.isTrue())
		{
			localOnlyFlag = E_NOT_LOCAL_ONLY;
		}
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone())
	{
		if (!wko.isTrue())
		{
			incQualsFlag = E_EXCLUDE_QUALIFIERS;
		}
	}

	EIncludeClassOriginFlag classOriginFlag = E_EXCLUDE_CLASS_ORIGIN;
	wko = getParam(kws, "IncludeClassOrigin");
	if (!wko.isNone())
	{
		if (wko.isTrue())
		{
			classOriginFlag = E_INCLUDE_CLASS_ORIGIN;
		}
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyClassResultHandler rhandler(cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->enumClass(ns, className, rhandler, deepFlg, localOnlyFlag,
			incQualsFlag, classOriginFlag);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String className;
	if (args.length() && !args[0].isNone())
	{
		className = Py::String(args[0]).as_ow_string();
	}

	if (className.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ClassName' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	ELocalOnlyFlag localOnlyFlag = E_LOCAL_ONLY;
	Py::Object wko = getParam(kws, "LocalOnly");
	if (!wko.isNone())
	{
		if (!wko.isTrue())
		{
			localOnlyFlag = E_NOT_LOCAL_ONLY;
		}
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone())
	{
		if (!wko.isTrue())
		{
			incQualsFlag = E_EXCLUDE_QUALIFIERS;
		}
	}

	EIncludeClassOriginFlag classOriginFlag = E_EXCLUDE_CLASS_ORIGIN;
	wko = getParam(kws, "IncludeClassOrigin");
	if (!wko.isNone())
	{
		if (wko.isTrue())
		{
			classOriginFlag = E_INCLUDE_CLASS_ORIGIN;
		}
	}

	StringArray propList;
	StringArray *pPropList = 0;
	wko = getParam(kws, "PropertyList");
	if (!wko.isNone())
	{
		Py::List pl(wko);
		for (Py::List::size_type i = 0; i < pl.length(); i++)
		{
			propList.append(Py::String(pl[i]).as_ow_string());
		}
		pPropList = &propList;
	}

	Py::Object pycc;
	try
	{
		CIMClass cc;
		PYCXX_ALLOW_THREADS
		cc = m_chdl->getClass(ns, className, localOnlyFlag,
			incQualsFlag, classOriginFlag, pPropList);
		PYCXX_END_ALLOW_THREADS
		pycc = OWPyConv::OWClass2Py(cc);
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return pycc;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::createClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMClass cc(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		cc = OWPyConv::PyClass2OW(args[0]);
	}

	if (!cc)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'NewClass' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->createClass(ns, cc);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::deleteClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String className;
	if (args.length() && !args[0].isNone())
	{
		className = Py::String(args[0]).as_ow_string();
	}

	if (className.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ClassName' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->deleteClass(ns, className);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::modifyClass(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMClass cc(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		cc = OWPyConv::PyClass2OW(args[0]);
	}

	if (!cc)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ModifiedClass' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->modifyClass(ns, cc);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumQualifiers(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String ns;
	if (args.length() > 0 && !args[0].isNone())
	{
		ns = Py::String(args[0]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyQualResultHandler rhandler(cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->enumQualifierTypes(ns, rhandler);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getQualifier(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String qualName;
	if (args.length() && !args[0].isNone())
	{
		qualName = Py::String(args[0]).as_ow_string();
	}
	if (qualName.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'QualifierName' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	Py::Object pyqual;
	try
	{
		CIMQualifierType cqt;
		PYCXX_ALLOW_THREADS
		cqt = m_chdl->getQualifierType(ns, qualName);
		PYCXX_END_ALLOW_THREADS
		pyqual = OWPyConv::OWQualType2Py(cqt);
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return pyqual;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::setQualifier(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMQualifier cq(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		cq = OWPyConv::PyQual2OW(args[0]);
	}

	if (!cq)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'QualifierDeclaration' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	try
	{
		CIMQualifierType cqt = cq.getDefaults();
		cqt.setName(cq.getName());
		CIMValue cv = cq.getValue();
		cqt.setDefaultValue(cv);
		PYCXX_ALLOW_THREADS
		m_chdl->setQualifierType(ns, cqt);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::deleteQualifier(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String qualName;
	if (args.length() && !args[0].isNone())
	{
		qualName = Py::String(args[0]);
	}

	if (!qualName.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'QualifierName' is a required parameter");
	}

	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->deleteQualifierType(ns, qualName);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}
	
	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumInstanceNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String className;
	if (args.length() && !args[0].isNone())
	{
		className = Py::String(args[0]).as_ow_string();
	}
	if (className.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ClassName' is a required parameter");
	}
	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyOpResultHandler rhandler(ns, cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->enumInstanceNames(ns, className, rhandler);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::enumInstances(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	String className;
	if (args.length() && !args[0].isNone())
	{
		className = Py::String(args[0]).as_ow_string();
	}
	if (className.empty())
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ClassName' is a required parameter");
	}
	String ns;
	if (args.length() > 1 && !args[1].isNone())
	{
		ns = Py::String(args[1]).as_ow_string();
	}

	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'namespace' is a required parameter");
		}
	}

	EDeepFlag deepFlg = E_SHALLOW;
	Py::Object wko = getParam(kws, "DeepInheritance");
	if (!wko.isNone() && wko.isTrue())
	{
		deepFlg = E_DEEP;
	}

	ELocalOnlyFlag localOnlyFlag = E_LOCAL_ONLY;
	wko = getParam(kws, "LocalOnly");
	if (!wko.isNone() && !wko.isTrue())
	{
		localOnlyFlag = E_NOT_LOCAL_ONLY;
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone() && !wko.isTrue())
	{
		incQualsFlag = E_EXCLUDE_QUALIFIERS;
	}

	EIncludeClassOriginFlag classOriginFlag = E_EXCLUDE_CLASS_ORIGIN;
	wko = getParam(kws, "IncludeClassOrigin");
	if (!wko.isNone() && wko.isTrue())
	{
		classOriginFlag = E_INCLUDE_CLASS_ORIGIN;
	}

	StringArray propList;
	StringArray *pPropList = 0;
	wko = getParam(kws, "PropertyList");
	if (!wko.isNone())
	{
		Py::List pl(wko);
		for (Py::List::size_type i = 0; i < pl.length(); i++)
		{
			propList.append(Py::String(pl[i]).as_ow_string());
		}
		pPropList = &propList;
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyInstResultHandler rhandler(ns, cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->enumInstances(ns, className, rhandler, deepFlg, localOnlyFlag,
			incQualsFlag, classOriginFlag, pPropList);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::getInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMObjectPath instanceName(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		instanceName = OWPyConv::PyRef2OW(Py::Object(args[0]));
	}

	if (!instanceName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'InstanceName' is a required parameter");
	}

	String ns = instanceName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'InstanceName' parameter does not have a namespace");
		}
	}

	ELocalOnlyFlag localOnlyFlag = E_LOCAL_ONLY;
	Py::Object wko = getParam(kws, "LocalOnly");
	if (!wko.isNone() && !wko.isTrue())
	{
		localOnlyFlag = E_NOT_LOCAL_ONLY;
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone() && !wko.isTrue())
	{
		incQualsFlag = E_EXCLUDE_QUALIFIERS;
	}

	EIncludeClassOriginFlag classOriginFlag = E_EXCLUDE_CLASS_ORIGIN;
	wko = getParam(kws, "IncludeClassOrigin");
	if (!wko.isNone() && wko.isTrue())
	{
		classOriginFlag = E_INCLUDE_CLASS_ORIGIN;
	}

	StringArray propList;
	StringArray *pPropList = 0;
	wko = getParam(kws, "PropertyList");
	if (!wko.isNone())
	{
		Py::List pl(wko);
		for (Py::List::size_type i = 0; i < pl.length(); i++)
		{
			propList.append(Py::String(pl[i]).as_ow_string());
		}
		pPropList = &propList;
	}

	Py::Object pyinst;	
	try
	{
		CIMInstance ci;
		PYCXX_ALLOW_THREADS
		ci = m_chdl->getInstance(ns, instanceName, localOnlyFlag,
			incQualsFlag, classOriginFlag, pPropList);
		PYCXX_END_ALLOW_THREADS
		pyinst = OWPyConv::OWInst2Py(ci, ns);
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return pyinst;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::deleteInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMObjectPath instanceName(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		instanceName = OWPyConv::PyRef2OW(Py::Object(args[0]));
	}
	if (!instanceName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'InstanceName' is a required parameter");
	}

	String ns = instanceName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'InstanceName' parameter does not have a namespace");
		}
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->deleteInstance(ns, instanceName);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::createInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMInstance ci(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		ci = OWPyConv::PyInst2OW(args[0]);
	}

	if (!ci)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'NewInstance' is a required parameter");
	}

	String ns = ci.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'NewInstance.path' does not contain a namespace");
		}
	}

	Py::Object pyref;
	try
	{
		CIMObjectPath mcop;
		PYCXX_ALLOW_THREADS
		CIMObjectPath mcop = m_chdl->createInstance(ns, ci);
		PYCXX_END_ALLOW_THREADS
		pyref = OWPyConv::OWRef2Py(mcop);
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return pyref;
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::modifyInstance(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMInstance ci(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		ci = OWPyConv::PyInst2OW(args[0]);
	}

	if (!ci)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ModifiedInstance' is a required parameter");
	}

	String ns = ci.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'ModifiedInstance.path' does not contain a namespace");
		}
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	Py::Object wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone() && !wko.isTrue())
	{
		incQualsFlag = E_EXCLUDE_QUALIFIERS;
	}

	StringArray propList;
	StringArray *pPropList = 0;
	wko = getParam(kws, "PropertyList");
	if (!wko.isNone())
	{
		Py::List pl(wko);
		for (Py::List::size_type i = 0; i < pl.length(); i++)
		{
			propList.append(Py::String(pl[i]).as_ow_string());
		}
		pPropList = &propList;
	}

	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->modifyInstance(ns, ci, incQualsFlag, pPropList);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return Py::Nothing();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::associators(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMObjectPath objectName(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		objectName = OWPyConv::PyRef2OW(args[0]);
	}
	if (!objectName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ObjectName' is a required parameter");
	}

	String ns = objectName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'ObjectName' parameter does not have a namespace");
		}
	}

	String assocClass;
	Py::Object wko = getParam(kws, "AssocClass");
	if (!wko.isNone())
	{
		assocClass = Py::String(wko).as_ow_string();
	}
	String resultClass;
	wko = getParam(kws, "ResultClass");
	if (!wko.isNone())
	{
		resultClass = Py::String(wko).as_ow_string();
	}
	String role;
	wko = getParam(kws, "Role");
	if (!wko.isNone())
	{
		role = Py::String(wko).as_ow_string();
	}
	String resultRole;
	wko = getParam(kws, "ResultRole");
	if (!wko.isNone())
	{
		resultRole = Py::String(wko).as_ow_string();
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone() && !wko.isTrue())
	{
		incQualsFlag = E_EXCLUDE_QUALIFIERS;
	}

	EIncludeClassOriginFlag classOriginFlag = E_EXCLUDE_CLASS_ORIGIN;
	wko = getParam(kws, "IncludeClassOrigin");
	if (!wko.isNone() && wko.isTrue())
	{
		classOriginFlag = E_INCLUDE_CLASS_ORIGIN;
	}

	StringArray propList;
	StringArray *pPropList = 0;
	wko = getParam(kws, "PropertyList");
	if (!wko.isNone())
	{
		Py::List pl(wko);
		for (Py::List::size_type i = 0; i < pl.length(); i++)
		{
			propList.append(Py::String(pl[i]).as_ow_string());
		}
		pPropList = &propList;
	}
	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyInstResultHandler rhandler(ns, cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->associators(ns, objectName, rhandler, assocClass, resultClass,
			role, resultRole, incQualsFlag, classOriginFlag, pPropList);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::associatorNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMObjectPath objectName(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		objectName = OWPyConv::PyRef2OW(args[0]);
	}
	if (!objectName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ObjectName' is a required parameter");
	}

	String ns = objectName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'ObjectName' parameter does not have a namespace");
		}
	}

	String assocClass;
	Py::Object wko = getParam(kws, "AssocClass");
	if (!wko.isNone())
	{
		assocClass = Py::String(wko).as_ow_string();
	}
	String resultClass;
	wko = getParam(kws, "ResultClass");
	if (!wko.isNone())
	{
		resultClass = Py::String(wko).as_ow_string();
	}
	String role;
	wko = getParam(kws, "Role");
	if (!wko.isNone())
	{
		role = Py::String(wko).as_ow_string();
	}
	String resultRole;
	wko = getParam(kws, "ResultRole");
	if (!wko.isNone())
	{
		resultRole = Py::String(wko).as_ow_string();
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyOpResultHandler rhandler(ns, cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->associatorNames(ns, objectName, rhandler, assocClass,
				resultClass, role, resultRole);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::references(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMObjectPath objectName(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		objectName = OWPyConv::PyRef2OW(args[0]);
	}
	if (!objectName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ObjectName' is a required parameter");
	}

	String ns = objectName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'ObjectName' parameter does not have a namespace");
		}
	}

	String resultClass;
	Py::Object wko = getParam(kws, "ResultClass");
	if (!wko.isNone())
	{
		resultClass = Py::String(wko).as_ow_string();
	}
	String role;
	wko = getParam(kws, "Role");
	if (!wko.isNone())
	{
		role = Py::String(wko).as_ow_string();
	}

	EIncludeQualifiersFlag incQualsFlag = E_INCLUDE_QUALIFIERS;
	wko = getParam(kws, "IncludeQualifiers");
	if (!wko.isNone() && !wko.isTrue())
	{
		incQualsFlag = E_EXCLUDE_QUALIFIERS;
	}

	EIncludeClassOriginFlag classOriginFlag = E_EXCLUDE_CLASS_ORIGIN;
	wko = getParam(kws, "IncludeClassOrigin");
	if (!wko.isNone() && wko.isTrue())
	{
		classOriginFlag = E_INCLUDE_CLASS_ORIGIN;
	}

	StringArray propList;
	StringArray *pPropList = 0;
	wko = getParam(kws, "PropertyList");
	if (!wko.isNone())
	{
		Py::List pl(wko);
		for (Py::List::size_type i = 0; i < pl.length(); i++)
		{
			propList.append(Py::String(pl[i]).as_ow_string());
		}
		pPropList = &propList;
	}
	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyInstResultHandler rhandler(ns, cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->references(ns, objectName, rhandler, resultClass, role, 
			incQualsFlag, classOriginFlag, pPropList);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PyCIMOMHandle::referenceNames(
	const Py::Tuple& args,
	const Py::Dict& kws)
{
	CIMObjectPath objectName(CIMNULL);
	if (args.length() && !args[0].isNone())
	{
		objectName = OWPyConv::PyRef2OW(args[0]);
	}
	if (!objectName)
	{
		throwPyCIMException(CIMException::INVALID_PARAMETER,
			"'ObjectName' is a required parameter");
	}

	String ns = objectName.getNameSpace();
	if (ns.empty())
	{
		ns = m_defaultns;
		if (ns.empty())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'ObjectName' parameter does not have a namespace");
		}
	}

	String resultClass;
	Py::Object wko = getParam(kws, "ResultClass");
	if (!wko.isNone())
	{
		resultClass = Py::String(wko).as_ow_string();
	}
	String role;
	wko = getParam(kws, "Role");
	if (!wko.isNone())
	{
		role = Py::String(wko).as_ow_string();
	}

	Py::Object cb = getParam(kws, "Handler");
	if (!cb.isNone())
	{
		if (!cb.isCallable())
		{
			throwPyCIMException(CIMException::INVALID_PARAMETER,
				"'Handler' parameter must be a callable object");
		}
	}

	PyOpResultHandler rhandler(ns, cb);
	try
	{
		PYCXX_ALLOW_THREADS
		m_chdl->referenceNames(ns, objectName, rhandler, resultClass, role);
		PYCXX_END_ALLOW_THREADS
	}
	catch(const CIMException& e)
	{
		throwPyCIMException(e);
	}
	catch(const Exception& e)
	{
		throwPyException(e);
	}
	catch(...)
	{
		throwPyCIMException(CIMException::FAILED, "Unknown exception");
	}

	return rhandler.getResult();
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
PyCIMOMHandle::newObject(CIMOMHandleIFCRef& chdl,
	PyCIMOMHandle **pchdl)
{
	PyCIMOMHandle* ph = new PyCIMOMHandle(chdl);
	if (pchdl)
	{
		*pchdl = ph;
	}

	return Py::asObject(ph);
}

}	// End of namespace PythonProvIFC
