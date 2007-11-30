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
#include "PG_PyConverter.h"
#include <Pegasus/Common/CIMProperty.h>
#include <Pegasus/Common/CIMDateTime.h>
#include <Pegasus/Common/CIMQualifierDecl.h>
#include <Pegasus/Common/CIMDateTime.h>
#include <Pegasus/Common/CIMDateTime.h>
#include <Pegasus/Common/CIMFlavor.h>
#include <Pegasus/Common/CIMScope.h>
#include <Pegasus/Common/Array.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
using std::cout;
using std::endl;

using namespace Pegasus;

namespace PythonProvIFC
{

String
_int2Str(int val)
{
	char bfr[16];
	snprintf(bfr, sizeof(bfr), "%d", val);
	return String(bfr);
}

String
_dataConvMsg(const String& file, int lineno, const String& msg)
{
	char bfr[512];
	CString cfile = file.getCString();
	CString cmsg = msg.getCString();
	snprintf(bfr, sizeof(bfr), "Python Data Conversion. File: %s, Line: %d, "
		"Msg: %s", (const char*)cfile, lineno, (const char*)cmsg);
	return String(bfr);
}

PyConversionException::PyConversionException()
	: Exception("Python Data Conversion")
{
}

PyConversionException::PyConversionException(const String& msg)
	: Exception(msg)
{
};

PyConversionException::PyConversionException(const String& file, int lineno,
		const String& msg)
	: Exception(_dataConvMsg(file, lineno, msg))
{
}

#define THROW_CONV_EXC(msg) throw PyConversionException(__FILE__, __LINE__, msg)

Py::Module g_modpywbem;

namespace
{

//////////////////////////////////////////////////////////////////////////////
void
_py2ConversionException(
	Py::Exception& e,
	int lineno)
{
	Py::Object etype, evalue;
	String tb = Py::getCurrentErrorInfo(etype, evalue);
	e.clear();
	String msg("ConversionException: type: ");
	msg.append(etype.as_string());
	msg.append(", value: ");
	msg.append(evalue.as_string());
	msg.append(". ");
	msg.append(tb);
	throw PyConversionException(__FILE__, lineno, msg);
}
#define PY2CONVEXC(e) _py2ConversionException(e, __LINE__)

//////////////////////////////////////////////////////////////////////////////
CIMName
_stringAttr2CIMName(
	const Py::Object& pyobj,
	const char* attrName=0)
{
	CIMName cnm;
	Py::Object attrobj = Py::None();
	if (attrName)
	{
		if (pyobj.hasAttr(attrName))
		{
			attrobj = pyobj.getAttr(attrName);
		}
	}
	else
	{
		attrobj = pyobj;
	}

	if (attrobj.isString())
	{
		String wkstr = Py::String(attrobj).as_peg_string();
		if (wkstr.size())
		{
			cnm = CIMName(wkstr);
		}
	}
	return cnm;
}

//////////////////////////////////////////////////////////////////////////////
String
_stringAttr(
	const Py::Object& pyobj,
	const String& attrName)
{
	String rv;
	if (pyobj.hasAttr(attrName))
	{
		Py::Object attrobj = pyobj.getAttr(attrName);
		if (attrobj.isString())
		{
			rv = Py::String(attrobj).as_peg_string();
		}
	}
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
CIMDateTime
_convertPyDateTime(
	Py::Object arg)
{
    CIMDateTime cdt; 
	Py::Object dtclass = g_modpywbem.getAttr("CIMDateTime");
	if (!arg.isInstanceOf(dtclass))
	{
		THROW_CONV_EXC("Unknown python type converting to PG CIMDateTime");
	}
	Py::Object dt = arg.getAttr("datetime");
	Py::Object td = arg.getAttr("timedelta");
	if (!dt.isNone())
	{
		Uint32 year = Py::Int(dt.getAttr("year")).asLong();
		Uint32 month = Py::Int(dt.getAttr("month")).asLong();
		Uint32 day = Py::Int(dt.getAttr("day")).asLong();
		Uint32 hour = Py::Int(dt.getAttr("hour")).asLong();
		Uint32 minute = Py::Int(dt.getAttr("minute")).asLong();
		Uint32 second = Py::Int(dt.getAttr("second")).asLong();
		Uint32 microseconds = Uint64(Py::Int(dt.getAttr("microsecond")).asLong());
		Sint32 offset = Py::Int(arg.getAttr("minutes_from_utc")).asLong();
		cdt.setTimeStamp(year, month, day, hour, minute, second,
			microseconds, 6, offset);
	}
	else if (!td.isNone())
	{
		int days = Py::Int(td.getAttr("days")).asLong();
		int seconds = Py::Int(td.getAttr("seconds")).asLong();
		int microseconds = Py::Int(td.getAttr("microseconds")).asLong();
		int hours = seconds / 3600; 
		int minutes = (seconds - hours * 3600) / 60; 
		seconds = seconds - (hours * 3600) - (minutes * 60);
		cdt.setInterval(days, hours, minutes, seconds, microseconds, 6);
	}
	else
	{
		THROW_CONV_EXC("Invalid CIMDateTime object");
	}
    return cdt; 
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
_convertPGDateTime(const CIMDateTime& dt)
{
	try
	{
		Py::Callable func = g_modpywbem.getAttr("CIMDateTime");
		Py::Tuple cdtarg(1);
		cdtarg[0] = Py::String(dt.toString());
		return func.apply(cdtarg); 
	}
	catch(Py::Exception& e)
	{
		PY2CONVEXC(e);
	}
	return Py::None();
}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
Py::Object
_numericPG2Py(
	const char* format,
	const char* func, 
	const CIMValue& pegval)
{
	Py::Callable pyfunc = g_modpywbem.getAttr(func);
    if (pegval.isArray())
    {
		Array<T> val;
		pegval.get(val); 
		Py::List vlist;

		for (Uint32 i = 0; i < val.size(); ++i)
		{
			PyObject* pVal = PyObject_CallFunction(pyfunc.ptr(),
				const_cast<char*>(format), val[i]);
			if (!pVal)
			{
				String msg("Calling function ");
				msg.append(func);
				throw Py::Exception(msg);
			}
			vlist.append(Py::Object(pVal, true));
		}

		return vlist; 
    }

	T val;
	pegval.get(val); 
	PyObject* pVal = PyObject_CallFunction(pyfunc.ptr(),
		const_cast<char*>(format), val);
	if (!pVal)
	{
		String msg("Calling function ");
		msg.append(func);
		throw Py::Exception(msg);
	}
	return Py::Object(pVal, true);
}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
Py::Dict
_makeQualDict(
	const T& cobj)
{
	Py::Dict pyquals;
	Uint32 qcount = cobj.getQualifierCount();
	for (Uint32 i = 0; i < qcount; i++)
	{
		CIMConstQualifier qual = cobj.getQualifier(i);
		pyquals[qual.getName().getString()] = PGPyConv::PGQual2Py(qual);
	}
	return pyquals;
}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
void
_setQuals(
	T& cobj, 
	const Py::Mapping& pyquals)
{
	Py::List items = pyquals.values();
	int len = int(items.length());
	for(int i = 0; i < len; i++)
	{
		cobj.addQualifier(PGPyConv::PyQual2PG(items[i]));
	}
}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
void
_setProps(T& cobj, const Py::Mapping& pyprops)
{
	Py::List propList = pyprops.values();
	int len = int(propList.length());
	for(int i = 0; i < len; i++)
	{
		cobj.addProperty(PGPyConv::PyProperty2PG(propList[i]));
	}
}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
Py::Dict
_makePropDict(
	const T& cobj)
{
	Py::Dict props;
	Uint32 propCount = cobj.getPropertyCount();
	for(Uint32 i = 0; i < propCount; i++)
	{
		CIMConstProperty cprop = cobj.getProperty(i);
		props[cprop.getName().getString()] = PGPyConv::PGProperty2Py(cprop);
	}

	return props;
}

//////////////////////////////////////////////////////////////////////////////
Py::Dict
_makeMethDict(
	const CIMConstClass& cc)
{
	Py::Dict meths;
	Uint32 mcount = cc.getMethodCount();
	for(Uint32 i = 0; i < mcount; i++)
	{
		CIMConstMethod meth = cc.getMethod(i);
		meths[meth.getName().getString()] = PGPyConv::PGMeth2Py(meth);
	}
	return meths;
}

}	// End of unnamed namespace

//////////////////////////////////////////////////////////////////////////////
// STATIC
void
PGPyConv::setPyWbemMod(
	const Py::Module& mod)
{
	g_modpywbem = mod;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
String
PGPyConv::PGDataType2Py(CIMType dt)
{
	String strtype;
	switch (dt)
	{
		// Not implemented
		case CIMTYPE_CHAR16:	// Not handled by pywbem
			strtype = "char16";
			break;
		case CIMTYPE_REFERENCE:
			strtype = "reference";
			break;
		case CIMTYPE_DATETIME:
			strtype = "datetime";
			break;
		case CIMTYPE_BOOLEAN:
			strtype = "boolean";
			break;
		case CIMTYPE_REAL32:
			strtype = "real32";
			break;
		case CIMTYPE_REAL64:
			strtype = "real64";
			break;
		case CIMTYPE_SINT8:
			strtype = "sint8";
			break;
		case CIMTYPE_SINT16:
			strtype = "sint16";
			break;
		case CIMTYPE_SINT32:
			strtype = "sint32";
			break;
		case CIMTYPE_SINT64:
			strtype = "sint64";
			break;
		case CIMTYPE_UINT8:
			strtype = "uint8";
			break;
		case CIMTYPE_UINT16:
			strtype = "uint16";
			break;
		case CIMTYPE_UINT32:
			strtype = "uint32";
			break;
		case CIMTYPE_UINT64:
			strtype = "uint64";
			break;
		case CIMTYPE_STRING:
			strtype = "string";
			break;
		case CIMTYPE_INSTANCE:
			strtype = "instance";
			break;
		default:
			break;
    }
	return strtype;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMType
PGPyConv::PyDataType2PG(
	const String& strt)
{
	CIMType dtype;
	if (strt == "char16")
		dtype = CIMTYPE_CHAR16;
	else if (strt == "reference")
		dtype =  CIMTYPE_REFERENCE;
	else if (strt == "datetime")
		dtype =  CIMTYPE_DATETIME;
	else if (strt == "boolean")
		dtype =  CIMTYPE_BOOLEAN;
	else if (strt == "real32")
		dtype =  CIMTYPE_REAL32;
	else if (strt == "real64")
		dtype =  CIMTYPE_REAL64;
	else if (strt == "sint8")
		dtype =  CIMTYPE_SINT8;
	else if (strt == "sint16")
		dtype =  CIMTYPE_SINT16;
	else if (strt == "sint32")
		dtype =  CIMTYPE_SINT32;
	else if (strt == "sint64")
		dtype =  CIMTYPE_SINT64;
	else if (strt == "uint8")
		dtype =  CIMTYPE_UINT8;
	else if (strt == "uint16")
		dtype =  CIMTYPE_UINT16;
	else if (strt == "uint32")
		dtype =  CIMTYPE_UINT32;
	else if (strt == "uint64")
		dtype =  CIMTYPE_UINT64;
	else if (strt == "string")
		dtype =  CIMTYPE_STRING;
	else if (strt == "instance")
		dtype =  CIMTYPE_INSTANCE;
	else
	{
		String msg("Unknown python type encountered in PyDataType2PG: ");
		msg.append(strt);
		THROW_CONV_EXC(msg);
	}
	return dtype;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGVal2Py(const CIMValue& pegval)
{
	// Assume pegval is NOT NULL
	Py::Object ro;
	switch (pegval.getType())
	{
		// Not implemented
		case CIMTYPE_CHAR16:	// Not handled by pywbem
			THROW_CONV_EXC("Conversion from PG Char16 to python not supported");
			break;
		case CIMTYPE_INSTANCE:
		{
			if (pegval.isArray())
			{
				Array<CIMInstance> val;
				pegval.get(val);
				Py::List blist;
				for (Uint32 i = 0; i < val.size(); ++i)
				{
					blist.append(PGInst2Py(val[i]));
				}
				ro = blist;
			}
			else
			{
				CIMInstance ci;
				pegval.get(ci);
				ro =  PGInst2Py(ci);
			}
			break;
		}
		case CIMTYPE_REFERENCE:
		{
			if (pegval.isArray())
			{
				Array<CIMObjectPath> val;
				pegval.get(val);
				Py::List blist;
				for (Uint32 i = 0; i < val.size(); ++i)
				{
					blist.append(PGRef2Py(val[i]));
				}
				ro =  blist;
			}
			else
			{
				ro = RefValPG2Py(pegval);
			}
			break;
		}
		case CIMTYPE_DATETIME:
			ro = DateTimeValPG2Py(pegval);
			break;
		case CIMTYPE_BOOLEAN:
		{
			if (pegval.isArray())
			{
				Array<Boolean> val;
				pegval.get(val);
				Py::List blist;
				for (Uint32 i = 0; i < val.size(); ++i)
				{
					blist.append(Py::Bool(val[i]));
				}
				ro = blist;
			}
			else
			{
				Boolean b;
				pegval.get(b);
				ro = Py::Bool(b);
			}
			break;
		}
		case CIMTYPE_REAL32:
			ro = _numericPG2Py<Real32>("(d)", "Real32", pegval);
			break;
		case CIMTYPE_REAL64:
			ro = _numericPG2Py<Real64>("(d)", "Real64", pegval);
			break;
		case CIMTYPE_SINT8:
			ro = _numericPG2Py<Sint8>("(b)", "Sint8", pegval);
			break;
		case CIMTYPE_SINT16:
			ro = _numericPG2Py<Sint16>("(h)", "Sint16", pegval);
			break;
		case CIMTYPE_SINT32:
			ro = _numericPG2Py<Sint32>("(i)", "Sint32", pegval);
			break;
		case CIMTYPE_SINT64:
			ro = _numericPG2Py<Sint64>("(L)", "Sint64", pegval);
			break;
		case CIMTYPE_UINT8:
			ro = _numericPG2Py<Uint8>("(k)", "Uint8", pegval);
			break;
		case CIMTYPE_UINT16:
			ro = _numericPG2Py<Uint16>("(k)", "Uint16", pegval);
			break;
		case CIMTYPE_UINT32:
			ro = _numericPG2Py<Uint32>("(k)", "Uint32", pegval);
			break;
		case CIMTYPE_UINT64:
			ro = _numericPG2Py<Uint64>("(K)", "Uint64", pegval);
			break;
		case CIMTYPE_STRING:
		{
			if (pegval.isArray())
			{
				Array<String> sa;
				pegval.get(sa);
				Py::List vlist;
				for (Uint32 i = 0; i < sa.size(); ++i)
				{
					vlist.append(Py::String(sa[i]));
				}
				ro = vlist;
			}
			else
			{
				String s;
				pegval.get(s);
				ro = Py::String(s);
			}
			break;
		}
		default:
			THROW_CONV_EXC("Unknown numeric data type while converting from PG "
				"to python");
			break;
    }

	return ro;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGRef2Py(const CIMObjectPath& cop)
{
	const Array<CIMKeyBinding>& kra = cop.getKeyBindings();
	if (kra.size() == 0)	// No keys. Assume classpath
	{
		Py::Callable pyfunc = g_modpywbem.getAttr("CIMClassName");
		Py::Tuple args(3);
		args[0] = Py::String(cop.getClassName().getString());
		args[1] = Py::String(cop.getHost());
		args[2] = Py::String(cop.getNameSpace().getString());
		return pyfunc.apply(args);
	}

	Py::Callable pyfunc = g_modpywbem.getAttr("CIMInstanceName");
	Py::Dict dict;
	for (Uint32 i = 0; i < kra.size(); i++)
	{
		CIMKeyBinding kb = kra[i];
		String kname = kb.getName().getString();
		String sv = kb.getValue();
		switch(kb.getType())
		{
			case CIMKeyBinding::REFERENCE:
			{
				CIMObjectPath lcop(sv);
				dict[kname] = PGRef2Py(lcop);
				break;
			}
			case CIMKeyBinding::NUMERIC:
			{
				unsigned long v = strtoul((const char*) sv.getCString(),
					NULL, 10);
				dict[kname] = Py::Long(v);
				break;
			}
			default:
				dict[kname] = Py::String(kb.getValue());
		}
	}
	Py::Tuple fargs(4);
	fargs[0] = Py::String(cop.getClassName().getString());
	fargs[1] = dict;
	fargs[2] = Py::String(cop.getHost());
	fargs[3] = Py::String(cop.getNameSpace().getString());

	// Return results of CIMInstanceName call.
	// Should be a pywbem.CIMInstance object
	return pyfunc.apply(fargs);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGInst2Py(const CIMConstInstance& ci, const String& nsArg)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMInstance");
	Py::Tuple pyarg(4);
	pyarg[0] = Py::String(ci.getClassName().getString());
	pyarg[1] = _makePropDict(ci);
	pyarg[2] = _makeQualDict(ci);
	CIMObjectPath icop = ci.getPath();
	String ns = icop.getNameSpace().getString();
	if (ns.size() == 0 && nsArg.size() > 0)
	{
		ns = nsArg;
		icop.setNameSpace(ns);
	}

	// If icop is a class path, then that means we didn't get
	// the key properties or their 'key' qualifiers because
	// of the type of instance that was given. This could
	// be the result of having the 'ci' parameter built
	// with includeQualifiers=false or localOnly=true or
	// a restrictive propertyList or any/all of the above.
	
	const Array<CIMKeyBinding>& kbs = icop.getKeyBindings();
	if (kbs.size() == 0)
		pyarg[3] = Py::None();	// No way to determine keys
	else
		pyarg[3] = PGRef2Py(icop);
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGQual2Py(const CIMConstQualifier& qual)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMQualifier");
	Py::Tuple pyarg(8);
	pyarg[0] = Py::String(qual.getName().getString());
	Py::Object qval;
	CIMValue cv = qual.getValue();
	if (!cv.isNull())
	{
		pyarg[1] = PGVal2Py(cv);	// value
		pyarg[2] = Py::String(PGDataType2Py(cv.getType()));
	}
	else
	{
		pyarg[1] = Py::Object();	// value
		pyarg[2] = Py::String(PGDataType2Py(qual.getType()));	// type
	}
	pyarg[3] = Py::Bool(bool(qual.getPropagated()));				// propagated
	const CIMFlavor& flavor = qual.getFlavor();
	pyarg[4] = Py::Bool(flavor.hasFlavor(CIMFlavor::OVERRIDABLE));
	pyarg[5] = Py::Bool(flavor.hasFlavor(CIMFlavor::TOSUBCLASS));
	pyarg[6] = Py::Bool(flavor.hasFlavor(CIMFlavor::TOINSTANCE));
	pyarg[7] = Py::Bool(flavor.hasFlavor(CIMFlavor::TRANSLATABLE));
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGQualType2Py(const CIMConstQualifierDecl& qualt)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMQualifierDeclaration");
	Py::Tuple pyarg(10);
	pyarg[0] = Py::String(qualt.getName().getString());		// name
	Py::Object pqvalt;
	pyarg[1] = Py::String(PGDataType2Py(qualt.getType()));
	CIMValue cv = qualt.getValue();
	pyarg[2] = (cv.isNull()) ? Py::Object() : PGVal2Py(cv);
	if (qualt.isArray())
	{
		pyarg[3] = Py::Bool(true);
		pyarg[4] = (qualt.getArraySize()) ? Py::Int(int(qualt.getArraySize()))
			: Py::Object();
	}
	else
	{
		pyarg[3] = Py::Bool(false);
		pyarg[4] = Py::Object();
	}
	bool added = false;
	Py::Dict scopes;
	CIMScope cscope = qualt.getScope();
	if (cscope.hasScope(CIMScope::CLASS))
	{
		scopes["CLASS"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::ASSOCIATION))
	{
		scopes["ASSOCIATION"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::INDICATION))
	{
		scopes["INDICATION"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::PROPERTY))
	{
		scopes["PROPERTY"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::REFERENCE))
	{
		scopes["REFERENCE"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::METHOD))
	{
		scopes["METHOD"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::PARAMETER))
	{
		scopes["PARAMETER"] = Py::Bool(true);
		added = true;
	}
	if (cscope.hasScope(CIMScope::ANY))
	{
		scopes["ANY"] = Py::Bool(true);
		added = true;
	}
	pyarg[5] = (added) ? scopes : Py::Object();

	added = false;
	Py::Dict flavors;
	const CIMFlavor cflavor = qualt.getFlavor();
	pyarg[6] = Py::Bool(cflavor.hasFlavor(CIMFlavor::OVERRIDABLE));
	pyarg[7] = Py::Bool(cflavor.hasFlavor(CIMFlavor::TOSUBCLASS));
	pyarg[8] = Py::Bool(cflavor.hasFlavor(CIMFlavor::TOINSTANCE));
	pyarg[9] = Py::Bool(cflavor.hasFlavor(CIMFlavor::TRANSLATABLE));
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGCIMParam2Py(const CIMConstParameter& param)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMParameter");
	Py::Tuple pyarg(6);
	pyarg[0] = Py::String(param.getName().getString());	// name
	pyarg[1] = Py::String(PGDataType2Py(param.getType()));
	String wks = param.getReferenceClassName().getString();
	if (wks.size())
		pyarg[2] = Py::String(wks);						// reference_class
	else
		pyarg[2] = Py::Object();						// reference_class
	pyarg[3] = Py::Bool(param.isArray());				// is_array
	pyarg[4] = Py::Int(int(param.getArraySize()));
	pyarg[5] = _makeQualDict(param);					// Qualifiers
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGMeth2Py(const CIMConstMethod& meth)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMMethod");
	Py::Tuple pyarg(6);
	pyarg[0] = Py::String(meth.getName().getString());
	pyarg[1] = Py::String(PGDataType2Py(meth.getType()));

	Py::Dict params;
	Uint32 pcount = meth.getParameterCount();
	for (Uint32 i = 0; i < pcount; i++)
	{
		CIMConstParameter param = meth.getParameter(i);
		params[param.getName().getString()] = PGCIMParam2Py(param);
	}
	pyarg[2] = params;
	pyarg[3] = Py::String(meth.getClassOrigin().getString());
	pyarg[4] = Py::Bool(meth.getPropagated());
	pyarg[5] = _makeQualDict(meth);
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
PGPyConv::PGProperty2Py(const CIMConstProperty& prop)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMProperty");
	Py::Tuple pyarg(10);
	pyarg[0] = Py::String(prop.getName().getString());	// name
	CIMValue cv = prop.getValue();
	if (!cv.isNull())
	{
		pyarg[1] = PGVal2Py(cv);	// value
		pyarg[2] = Py::String(PGDataType2Py(cv.getType()));	// type
	}
	else
	{
		pyarg[1] = Py::Object();	// value
		pyarg[2] = Py::String(PGDataType2Py(prop.getType()));	// type
	}

	String wks = prop.getClassOrigin().getString();
	if (!wks.size())
	{
		pyarg[3] = Py::Object();	// class_origin
	}
	else
	{
		pyarg[3] = Py::String(wks);	// class_origin
	}

	pyarg[4] = Py::Int(int(prop.getArraySize()));			// array_size
	pyarg[5] = Py::Bool(prop.getPropagated());	// propagated
	pyarg[6] = Py::Bool(prop.isArray());		// is_array
	wks = prop.getReferenceClassName().getString();
	if (wks.size())
	{
		pyarg[7] = Py::String(wks);	// reference_class
	}
	else
	{
		pyarg[7] = Py::Object();					// reference_class
	}

	pyarg[8] = _makeQualDict(prop);
	if (prop.getType() == CIMTYPE_INSTANCE)
	{
		pyarg[9] = Py::String("instance");
	}
	else
	{
		pyarg[9] = Py::None();
	}
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::PGClass2Py(const CIMConstClass& cls)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMClass");
	Py::Tuple pyarg(5);
	pyarg[0] = Py::String(cls.getClassName().getString());
	pyarg[1] = _makePropDict(cls);
	pyarg[2] = _makeMethDict(cls);

	String superClass = cls.getSuperClassName().getString();
	if(!superClass.size())
	{
		pyarg[3] = Py::Object();
	}
	else
	{
		pyarg[3] = Py::String(superClass);
	}

	pyarg[4] = _makeQualDict(cls);
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMInstance
PGPyConv::PyInst2PG(const Py::Object& pyci, const String& nsArg)
{
    CIMInstance inst(_stringAttr2CIMName(pyci, "classname"));
	CIMObjectPath cop;
	if (pyci.hasAttr("path"))
	{
		Py::Object pyref = pyci.getAttr("path");
		if (!pyref.isNone())
		{
			cop = PyRef2PG(pyref, nsArg);
			inst.setPath(cop);
		}
	}
	Py::Mapping props = pyci.getAttr("properties");
	_setProps(inst, props);
    return inst; 
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMObjectPath
PGPyConv::PyRef2PG(
	const Py::Object& pycop,
	const String& nsArg)
{
	CIMName className = _stringAttr2CIMName(pycop, "classname");
	String ns = _stringAttr(pycop, "namespace");
	if (!ns.size())
	{
		ns = nsArg;
	}
	Py::Mapping kb = pycop.getAttr("keybindings");
	Py::List items = kb.items();
	Py::Object pciName = g_modpywbem.getAttr("CIMInstanceName");
	Py::Object pciClassName = g_modpywbem.getAttr("CIMClassName");
	Py::Object pciDateTime = g_modpywbem.getAttr("CIMDateTime");

	Array<CIMKeyBinding> ckbs;
	for (int i = 0; i < int(items.length()); i++)
	{
		Py::Tuple tup(items[i]);
		if (tup.length() != 2)
		{
			String msg("Py Ref Conversion: keybinding tuple has size of ");
			msg.append(_int2Str(tup.length()));
			THROW_CONV_EXC(msg);
		}

		CIMName kname = _stringAttr2CIMName(tup[0]);
		Py::Object pkval = tup[1];
		CIMValue cv;

		if (pkval.isBool())
		{
			cv = PyVal2PG("boolean", pkval);
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::BOOLEAN));
		}
		else if (pkval.isString())
		{
			cv = PyVal2PG("string", pkval);
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::STRING));
		}
		else if (pkval.isInt())
		{
			try
			{
				cv = CIMValue(Sint64(Py::Int(pkval).asLongLong()));
			}
			catch(Py::OverflowError& err)
			{
				err.clear();
				// This can throw
				cv = CIMValue(Sint64(Py::Int(pkval).asUnsignedLongLong()));
			}
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::NUMERIC));
		}
		else if (pkval.isLong())
		{
			try
			{
				cv = CIMValue(Sint64(Py::Long(pkval).asLongLong()));
			}
			catch(Py::OverflowError& err)
			{
				err.clear();
				// This can throw
				cv = CIMValue(Sint64(Py::Long(pkval).asUnsignedLongLong()));
			}
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::NUMERIC));
		}
		else if (pkval.isFloat())
		{
			cv = PyVal2PG("real64", pkval); 
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::NUMERIC));
		}
		else if (pkval.isInstanceOf(pciClassName)
			|| pkval.isInstanceOf(pciName))
		{
			cv = PyVal2PG("reference", pkval); 
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::REFERENCE));
		}
		else if (pkval.isInstanceOf(pciDateTime))
		{
			cv = PyVal2PG("datetime", pkval);
			ckbs.append(CIMKeyBinding(kname, cv.toString(), CIMKeyBinding::STRING));
		}
		else
		{
			String msg("Py Ref Conversion: unhandled value for key: ");
			msg.append(kname.getString());
			msg.append(" type: ");
			msg.append(pkval.type().as_string());
			THROW_CONV_EXC(msg);
		}
	}

	return CIMObjectPath("", ns, className, ckbs);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMValue
PGPyConv::PyVal2PG(
	const String& type,
	const Py::Object& pyval)
{
	if (type == "boolean")
	{
		if (pyval.isList())
		{
			Py::List bl(pyval);
			Uint32 sz = bl.length();
			Array<Boolean> bra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
					bra[i] = bl[i].isTrue();
			}
			return CIMValue(bra);
		}
		return CIMValue(Boolean(pyval.isTrue()));
	}
	else if (type == "string")
	{
		if (pyval.isList())
		{
			Py::List sl(pyval);
			Uint32 sz = sl.length();
			Array<String> sra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				sra[i] = Py::String(sl[i]).as_peg_string();
			}
			return CIMValue(sra);
		}
		return CIMValue(Py::String(pyval).as_peg_string());
	}
	else if (type == "uint8")
	{
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Uint8> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Uint8(Py::Int(il[i]).asUnsignedLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Uint8(Py::Int(pyval).asUnsignedLong()));
	}
	else if (type == "sint8")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Sint8> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Sint8(Py::Int(il[i]).asLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Sint8(Py::Int(pyval).asLong()));
	}
	else if (type == "uint16")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Uint16> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Uint16(Py::Int(il[i]).asUnsignedLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Uint16(Py::Int(pyval).asUnsignedLong()));
	}
	else if (type == "sint16")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Sint16> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Sint16(Py::Int(il[i]).asLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Sint16(Py::Int(pyval).asLong()));
	}
	else if (type == "uint32")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Uint32> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Uint32(Py::Int(il[i]).asUnsignedLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Uint32(Py::Int(pyval).asUnsignedLong()));
	}
	else if (type == "sint32")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Sint32> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Sint32(Py::Int(il[i]).asLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Sint32(Py::Int(pyval).asLong()));
	}
	else if (type == "uint64")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Uint64> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Uint64(Py::LongLong(il[i]).asUnsignedLongLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Uint64(Py::LongLong(pyval).asUnsignedLongLong()));
	}
	else if (type == "sint64")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			Uint32 sz = il.length();
			Array<Sint64> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Sint64(Py::LongLong(il[i]).asLongLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Sint64(Py::LongLong(pyval).asLongLong()));
	}
	else if (type == "real32")
	{
		if (pyval.isList())
		{
			Py::List fl(pyval);
			Uint32 sz = fl.length();
			Array<Real32> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Real32(Py::Float(fl[i]).as_double());
			}
			return CIMValue(nra);
		}
		return CIMValue(Real32(Py::Float(pyval).as_double()));
	}
	else if (type == "real64")
	{
		if (pyval.isList())
		{
			Py::List fl(pyval);
			Uint32 sz = fl.length();
			Array<Real64> nra(sz);
			for(Uint32 i = 0; i < sz; i++)
			{
				nra[i] = Real64(Py::Float(fl[i]).as_double());
			}
			return CIMValue(nra);
		}
		return CIMValue(Real64(Py::Float(pyval).as_double()));
	}
	else if (type == "char16")
    {
		THROW_CONV_EXC("Unable to convert to PG from python char16");
    }
	else if (type == "datetime")
    {
		if (pyval.isList())
		{
			Py::List dl(pyval);
			Uint32 sz = dl.length();
			Array<CIMDateTime> ra(sz); 
			for (Uint32 i = 0; i < sz; ++i)
			{
				ra[i] = _convertPyDateTime(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(_convertPyDateTime(pyval)); 
    }
	else if (type == "reference")
    {
		if (pyval.isList())
		{
			Py::List dl(pyval);
			Uint32 sz = dl.length();
			Array<CIMObjectPath> ra(sz); 
			for (Uint32 i = 0; i < sz; ++i)
			{
				ra[i] = PyRef2PG(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(PyRef2PG(pyval)); 
    }
	else if (type == "instance")
	{
		if (pyval.isList())
		{
			Py::List dl(pyval);
			Uint32 sz = dl.length();
			Array<CIMInstance> ra(sz); 
			for (Uint32 i = 0; i < sz; ++i)
			{
				ra[i] = PyInst2PG(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(PyInst2PG(pyval));
	}
	/* Pegasus doesn't support embedded class?
	else if (type == "class")
	{
		if (pyval.isList())
		{
			Py::List dl(pyval);
			Uint32 sz = dl.length();
			Array<CIMClass> ra(sz); 
			for (Uint32 i = 0; i < sz; ++i)
			{
				ra[i] = PyClass2PG(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(PyClass2PG(pyval));
	}
	*/

	String msg("Unknown python data type for conversion: ");
	msg.append(type);
	THROW_CONV_EXC(msg);
    return CIMValue(); 	// Never hit here
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMValue
PGPyConv::PyVal2PG(
	const Py::Tuple& tuple)
{
	if (tuple.length() != 2)
	{
		String msg("Invalid python tuple given for data type conversion. "
			"len: ");
		msg.append(_int2Str(tuple.length()));
		THROW_CONV_EXC(msg);
	}
	Py::Object wko = tuple[0];
	if (!wko.isString())
	{
		THROW_CONV_EXC("Invalid python tuple given for "
			"data type conversion. first element is not a string");
	}
	String dtname = Py::String(wko).as_peg_string();
	return PyVal2PG(dtname, tuple[1]);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMClass
PGPyConv::PyClass2PG(const Py::Object& pycls)
{
	Py::Object wko;
	CIMName theName = _stringAttr2CIMName(pycls, "classname");
	CIMClass theClass(theName);
	CIMName wkcn = _stringAttr2CIMName(pycls, "superclass");
	if (!wkcn.isNull())
	{	
		theClass.setSuperClassName(wkcn);
	}
	Py::Mapping pymap = pycls.getAttr("properties");
	_setProps(theClass, pymap);

	pymap = pycls.getAttr("qualifiers");
	_setQuals(theClass, pymap);

	pymap = pycls.getAttr("methods");
	Py::List methList = pymap.values();
	int len = int(methList.length());
	for(int i = 0; i < len; i++)
	{
		theClass.addMethod(PyMeth2PG(methList[i]));
	}
	return theClass;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMProperty
PGPyConv::PyProperty2PG(const Py::Object& pyprop)
{
	Py::Object wko;
	CIMName theName = _stringAttr2CIMName(pyprop, "name");
	Boolean propagated = false;
	if (pyprop.getAttr("propagated").isTrue())
	{
		propagated = true;
	}
	CIMName classOrigin = _stringAttr2CIMName(pyprop, "class_origin");
	CIMName refClass = _stringAttr2CIMName(pyprop, "reference_class");
	// Convert data type
	String strtype = Py::String(pyprop.getAttr("type")).as_peg_string();
	wko = pyprop.getAttr("embedded_object");
	if (!wko.isNone())
	{
		String stremb = Py::String(wko).as_peg_string();
		if (String::equalNoCase(stremb, "instance"))
			strtype = "instance";
		else
			THROW_CONV_EXC("Embedded classes not supported");
	}
	Uint32 arraySize = 0;
	wko = pyprop.getAttr("array_size");
	if (!wko.isNone())
	{
		arraySize = Uint32(Py::Int(wko));
	}
	CIMValue theValue;
	wko = pyprop.getAttr("value");
	if (!wko.isNone())
	{
		theValue = PyVal2PG(strtype, wko);
	}
	else
	{
		CIMType dt = PGPyConv::PyDataType2PG(strtype);
		wko = pyprop.getAttr("is_array");
		bool isArray = wko.isTrue();
		theValue = CIMValue(dt, isArray, arraySize);
	}

	CIMProperty theProp(theName, theValue, arraySize, refClass,
		classOrigin, propagated);

	Py::Mapping pyqualDict(pyprop.getAttr("qualifiers"));
	_setQuals(theProp, pyqualDict);
	return theProp;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMParameter
PGPyConv::PyCIMParam2PG(const Py::Object& pyparam)
{
	Py::Object wko;
	CIMName theName = _stringAttr2CIMName(pyparam, "name");
	// Convert data type
	String strtype = Py::String(pyparam.getAttr("type")).as_peg_string();
	CIMType dt = PyDataType2PG(strtype);
	Uint32 raSize = 0;
	Boolean isArray = false;
	if (pyparam.getAttr("is_array").isTrue())
	{
		isArray = true;
		wko = pyparam.getAttr("array_size");
		if (!wko.isNone())
		{
			raSize = Uint32(Py::Int(wko));
		}
	}
	CIMName refClass = _stringAttr2CIMName(pyparam, "reference_class");
	CIMParameter theParam(theName, dt, isArray, raSize, refClass);
	// Set the qualifiers for the parameter
	Py::Mapping pyqualDict(pyparam.getAttr("qualifiers"));
	_setQuals(theParam, pyqualDict);
	return theParam;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMQualifier
PGPyConv::PyQual2PG(const Py::Object& pyqual)
{
	Py::Object wko;
	CIMName theName = _stringAttr2CIMName(pyqual, "name");
	String strtype = Py::String(pyqual.getAttr("type")).as_peg_string();
	CIMValue theValue;
	Py::Object qv = pyqual.getAttr("value");
	if (!qv.isNone())
	{
		theValue = PyVal2PG(strtype, qv);
	}
	Boolean propagated = pyqual.getAttr("propagated").isTrue();
	CIMFlavor flavor;
	if (pyqual.getAttr("overridable").isTrue())
	{
		flavor.addFlavor(CIMFlavor::OVERRIDABLE);
	}
	if (pyqual.getAttr("tosubclass").isTrue())
	{
		flavor.addFlavor(CIMFlavor::TOSUBCLASS);
	}
	if (pyqual.getAttr("toinstance").isTrue())
	{
		flavor.addFlavor(CIMFlavor::TOINSTANCE);
	}
	if (pyqual.getAttr("translatable").isTrue())
	{
		flavor.addFlavor(CIMFlavor::TRANSLATABLE);
	}
	return CIMQualifier(theName, theValue, flavor, propagated);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMQualifierDecl
PGPyConv::PyQualType2PG(const Py::Object& pyqualt)
{
	Py::Object wko;
	CIMName theName = _stringAttr2CIMName(pyqualt, "name");
	String strtype = Py::String(pyqualt.getAttr("type")).as_peg_string();
	CIMValue theValue;
	wko = pyqualt.getAttr("value");
	if (!wko.isNone())
	{
		theValue = PyVal2PG(strtype, wko);
	}
	Uint32 raSize = 0;
	wko = pyqualt.getAttr("array_size");
	if (!wko.isNone())
	{
		raSize = Uint32(Py::Int(wko).asLong());
	}
	CIMScope theScope;
	wko = pyqualt.getAttr("scopes");
	if (!wko.isNone())
	{
		Py::Mapping scopes(wko);
		if (scopes.hasKey("CLASS"))
		{
			if (scopes["CLASS"].isTrue())
				theScope.addScope(CIMScope::CLASS);
		}
		if (scopes.hasKey("ASSOCIATION"))
		{
			if (scopes["ASSOCIATION"].isTrue())
				theScope.addScope(CIMScope::ASSOCIATION);
		}
		if (scopes.hasKey("REFERENCE"))
		{
			if (scopes["REFERENCE"].isTrue())
				theScope.addScope(CIMScope::REFERENCE);
		}
		if (scopes.hasKey("PROPERTY"))
		{
			if (scopes["PROPERTY"].isTrue())
				theScope.addScope(CIMScope::PROPERTY);
		}
		if (scopes.hasKey("METHOD"))
		{
			if (scopes["METHOD"].isTrue())
				theScope.addScope(CIMScope::METHOD);
		}
		if (scopes.hasKey("PARAMETER"))
		{
			if (scopes["PARAMETER"].isTrue())
				theScope.addScope(CIMScope::PARAMETER);
		}
		if (scopes.hasKey("INDICATION"))
		{
			if (scopes["INDICATION"].isTrue())
				theScope.addScope(CIMScope::INDICATION);
		}
	}
	CIMFlavor flavor;
	if (pyqualt.getAttr("overridable").isTrue())
	{
		flavor.addFlavor(CIMFlavor::OVERRIDABLE);
	}
	if (pyqualt.getAttr("tosubclass").isTrue())
	{
		flavor.addFlavor(CIMFlavor::TOSUBCLASS);
	}
	if (pyqualt.getAttr("toinstance").isTrue())
	{
		flavor.addFlavor(CIMFlavor::TOINSTANCE);
	}
	if (pyqualt.getAttr("translatable").isTrue())
	{
		flavor.addFlavor(CIMFlavor::TRANSLATABLE);
	}

	return CIMQualifierDecl(theName, theValue, theScope, flavor, raSize);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMMethod
PGPyConv::PyMeth2PG(const Py::Object& pymeth)
{
	Py::Object wko;
	CIMName theName = _stringAttr2CIMName(pymeth, "name");
	CIMName classOrigin = _stringAttr2CIMName(pymeth, "class_origin");
	Boolean propagated = pymeth.getAttr("propagated").isTrue();
	String strtype = Py::String(pymeth.getAttr("return_type")).as_peg_string();
	CIMType returnType = PyDataType2PG(strtype);
	CIMMethod theMethod(theName, returnType, classOrigin, propagated);
	Py::Mapping pyqualDict(pymeth.getAttr("qualifiers"));
	_setQuals(theMethod, pyqualDict);
	wko = pymeth.getAttr("parameters");
	if (!wko.isNone())
	{
		Py::Mapping parmDict(wko);
		Py::List items(parmDict.items());
		for (int i = 0; i < int(items.length()); i++)
		{
			theMethod.addParameter(PyCIMParam2PG(items[i]));
		}
	}
	return theMethod;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::RefValPG2Py(const CIMValue& pegval)
{
	CIMObjectPath cop;
	pegval.get(cop);
	return PGRef2Py(cop);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
PGPyConv::DateTimeValPG2Py(const CIMValue& pegval)
{
	if (!pegval.isArray())
	{
		CIMDateTime dt;
		pegval.get(dt);
		return _convertPGDateTime(dt);
	}

	Array<CIMDateTime> val;
	pegval.get(val);
	Py::List dtlist;
	for (Uint32 i = 0; i < val.size(); ++i)
	{
		dtlist.append(_convertPGDateTime(val[i]));
	}

	return dtlist;
}

}	// End of namespace PythonProvIFC
