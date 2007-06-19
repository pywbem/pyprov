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
#include "OW_PyConverter.hpp"
#include <openwbem/OW_CIMProperty.hpp>
#include <openwbem/OW_CIMDateTime.hpp>
#include <openwbem/OW_CIMQualifierType.hpp>
#include <openwbem/OW_Bool.hpp>
#include <openwbem/OW_DateTime.hpp>
#include <openwbem/OW_CIMDateTime.hpp>
#include <openwbem/OW_CIMFlavor.hpp>
#include <openwbem/OW_Format.hpp>

#include <iostream>
using std::cout;
using std::endl;

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

OW_DEFINE_EXCEPTION(PyConversion);

Py::Module g_modpywbem;

namespace
{

//////////////////////////////////////////////////////////////////////////////
void
py2ConversionException(
	Py::Exception& e,
	int lineno)
{
	Py::Object etype, evalue;
	String tb = Py::getCurrentErrorInfo(etype, evalue);
	e.clear();
	String msg = Format("ConversionException: type: %1, value: %2. %3",
		etype.as_string(), evalue.as_string(), tb);
	throw PyConversionException(__FILE__, lineno, msg.c_str());
}
#define PY2CONVEXC(e) py2ConversionException(e, __LINE__)

//////////////////////////////////////////////////////////////////////////////
Py::Object
bool2Py(bool v)
{
	PyObject *p = (v) ? Py_True : Py_False;
	return Py::Object(p);
}

//////////////////////////////////////////////////////////////////////////////
String
stringAttr(
	const Py::Object& pyobj,
	const String& attrName)
{
	String rv;
	if (pyobj.hasAttr(attrName))
	{
		Py::Object attrobj = pyobj.getAttr(attrName);
		if (!attrobj.isNone() && attrobj.isString())
		{
			rv = Py::String(attrobj).as_ow_string();
		}
	}
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
CIMDateTime
convertPyDateTime(
	Py::Object arg)
{
    CIMDateTime cdt; 
	Py::Object dtclass = g_modpywbem.getAttr("CIMDateTime");
	if (!arg.isInstanceOf(dtclass))
	{
		OW_THROW(PyConversionException,
			"Unknown python type converting to OW CIMDateTime");
        }
	Py::Object dt = arg.getAttr("datetime");
	Py::Object td = arg.getAttr("timedelta");
	if (!dt.isNone())
        {
		cdt.setInterval(false); 

		int year = Py::Int(dt.getAttr("year")).asLong();
		int month = Py::Int(dt.getAttr("month")).asLong();
		int day = Py::Int(dt.getAttr("day")).asLong();
		int hour = Py::Int(dt.getAttr("hour")).asLong();
		int minute = Py::Int(dt.getAttr("minute")).asLong();
		int second = Py::Int(dt.getAttr("second")).asLong();
		int microsecond = Py::Int(dt.getAttr("microsecond")).asLong();
                int offset = Py::Int(arg.getAttr("minutes_from_utc")).asLong();
		//int offset = Py::Int(PyObject_CallMethod(dt.ptr(), 
		//			"minutes_from_utc", NULL)).asLong();
		cdt.setYear(year); 
		cdt.setMonth(month); 
		cdt.setDay(day); 
		cdt.setHours(hour); 
		cdt.setMinutes(minute); 
		cdt.setSeconds(second); 
		cdt.setMicroSeconds(microsecond); 
		cdt.setUtc(offset); 
	}
	else if (!td.isNone())
	{
		cdt.setInterval(true); 
		int days = Py::Int(td.getAttr("days")).asLong();
		int seconds = Py::Int(td.getAttr("seconds")).asLong();
		int microseconds = Py::Int(td.getAttr("microsecond")).asLong();
		int hours = seconds / 3600; 
		int minutes = (seconds - hours * 3600) / 60; 
		seconds = seconds - hours * 3600 - minutes * 60; 
		cdt.setYear(0); 
		cdt.setMonth(0); 
		cdt.setDays(days); 
		cdt.setSeconds(seconds); 
		cdt.setMicroSeconds(microseconds); 
		cdt.setMinutes(minutes); 
		cdt.setHours(hours); 
	}
        else
        {
		OW_THROW(PyConversionException,
			"Invalid CIMDateTime object");
        }
    return cdt; 
}

//////////////////////////////////////////////////////////////////////////////
template <typename T>
Py::Object
numericOW2Py(
	const char* format,
	const char* func, 
	const CIMValue& owval)
{
	Py::Callable pyfunc = g_modpywbem.getAttr(func);
    if (owval.isArray())
    {
		Array<T> val;
		owval.get(val); 
		Py::List vlist;

		for (size_t i = 0; i < val.size(); ++i)
		{
			PyObject* pVal = PyObject_CallFunction(pyfunc.ptr(),
				const_cast<char*>(format), val[i]);
			if (!pVal)
			{
				throw Py::Exception(Format("Calling function %1", func).c_str());
			}
			vlist.append(Py::Object(pVal, true));
		}

		return vlist; 
    }

	T val;
	owval.get(val); 
	PyObject* pVal = PyObject_CallFunction(pyfunc.ptr(),
		const_cast<char*>(format), val);
	if (!pVal)
	{
		throw Py::Exception(Format("Calling function %1", func).c_str());
	}
	return Py::Object(pVal, true);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
convertOWDateTime(const CIMDateTime& dt)
{
	if (dt.isInterval())
	{
		try
		{
			Py::Callable pyfunc = g_modpywbem.getAttr("timedelta");
			Py::Tuple pyarg = Py::Tuple(7);
			pyarg[0] = Py::Int(int(dt.getDays()));
			pyarg[1] = Py::Int(int(dt.getSeconds()));
			pyarg[2] = Py::Int(int(dt.getMicroSeconds()));
			pyarg[3] = Py::Int(0);
			pyarg[4] = Py::Int(int(dt.getMinutes()));
			pyarg[5] = Py::Int(int(dt.getHours()));
			pyarg[6] = Py::Int(0);
                        Py::Object td(pyfunc.apply(pyarg)); 
                        pyfunc = g_modpywbem.getAttr("CIMDateTime"); 
                        Py::Tuple dtarg = Py::Tuple(1);
                        dtarg[0] = td; 
			return pyfunc.apply(dtarg);
		}
		catch(Py::Exception& e)
		{
			PY2CONVEXC(e);
		}
	}

	try
	{
		//Py::Callable pyfunc = g_modpywbem.getAttr("CIMDateTime");
                Py::Callable func = g_modpywbem.getAttr("MinutesFromUTC"); 
                Py::Tuple utc_pyarg(1);
                utc_pyarg[0] = Py::Int(int(dt.getUtc())); 
                Py::Object utc(func.apply(utc_pyarg)); 
		func = g_modpywbem.getAttr("datetime");
		Py::Tuple pyarg(8);
		pyarg[0] = Py::Int(int(dt.getYear()));
		pyarg[1] = Py::Int(int(dt.getMonth()));
		pyarg[2] = Py::Int(int(dt.getDay()));
		pyarg[3] = Py::Int(int(dt.getHours()));
		pyarg[4] = Py::Int(int(dt.getMinutes()));
		pyarg[5] = Py::Int(int(dt.getSeconds()));
		pyarg[6] = Py::Int(int(dt.getMicroSeconds()));
		pyarg[7] = utc; 
                Py::Object dt(func.apply(pyarg));
		func = g_modpywbem.getAttr("CIMDateTime");
                Py::Tuple cdtarg(1);
                cdtarg[0] = dt; 
                return func.apply(cdtarg); 
	}
	catch(Py::Exception& e)
	{
		PY2CONVEXC(e);
	}

	return Py::None();
}

//////////////////////////////////////////////////////////////////////////////
Py::Dict
makeQualDict(const CIMQualifierArray& quals)
{
	Py::Dict pyquals;
	for(CIMQualifierArray::size_type i = 0; i < quals.size(); i++)
	{
		pyquals[quals[i].getName()] = OWPyConv::OWQual2Py(quals[i]);
	}
	return pyquals;
}

//////////////////////////////////////////////////////////////////////////////
CIMQualifierArray
getQuals(const Py::Mapping& pyquals)
{
	CIMQualifierArray rv;
	Py::List items = pyquals.values();
	int len = int(items.length());
	for(int i = 0; i < len; i++)
	{
		rv.append(OWPyConv::PyQual2OW(items[i]));
	}
	return rv;
}

//////////////////////////////////////////////////////////////////////////////
CIMPropertyArray
getProps(const Py::Mapping& pyprops)
{
	CIMPropertyArray rv;
	Py::List propList = pyprops.values();
	int len = int(propList.length());
	for(int i = 0; i < len; i++)
	{
		rv.append(OWPyConv::PyProperty2OW(propList[i]));
	}

	return rv;
}

//////////////////////////////////////////////////////////////////////////////
Py::Dict
makePropDict(const CIMPropertyArray& pra)
{
	Py::Dict props;
	for(CIMPropertyArray::size_type i = 0; i < pra.size(); i++)
	{
		props[pra[i].getName()] = OWPyConv::OWProperty2Py(pra[i]);
	}
	return props;
}

//////////////////////////////////////////////////////////////////////////////
Py::Dict
makeMethDict(const CIMMethodArray& mra)
{
	Py::Dict meths;
	for(CIMMethodArray::size_type i = 0; i < mra.size(); i++)
	{
		meths[mra[i].getName()] = OWPyConv::OWMeth2Py(mra[i]);
	}
	return meths;
}

}	// End of unnamed namespace

//////////////////////////////////////////////////////////////////////////////
// STATIC
void
OWPyConv::setPyWbemMod(
	const Py::Module& mod)
{
	g_modpywbem = mod;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
String
OWPyConv::OWDataType2Py(CIMDataType::Type dt)
{
	String strtype;
	switch (dt)
	{
		// Not implemented
		case CIMDataType::CHAR16:	// Not handled by pywbem
			strtype = "char16";
			break;
		case CIMDataType::REFERENCE:
			strtype = "reference";
			break;
		case CIMDataType::DATETIME:
			strtype = "datetime";
			break;
		case CIMDataType::BOOLEAN:
			strtype = "boolean";
			break;
		case CIMDataType::REAL32:
			strtype = "real32";
			break;
		case CIMDataType::REAL64:
			strtype = "real64";
			break;
		case CIMDataType::SINT8:
			strtype = "sint8";
			break;
		case CIMDataType::SINT16:
			strtype = "sint16";
			break;
		case CIMDataType::SINT32:
			strtype = "sint32";
			break;
		case CIMDataType::SINT64:
			strtype = "sint64";
			break;
		case CIMDataType::UINT8:
			strtype = "uint8";
			break;
		case CIMDataType::UINT16:
			strtype = "uint16";
			break;
		case CIMDataType::UINT32:
			strtype = "uint32";
			break;
		case CIMDataType::UINT64:
			strtype = "uint64";
			break;
		case CIMDataType::STRING:
			strtype = "string";
			break;
		case CIMDataType::EMBEDDEDCLASS:
			strtype = "class";
			break;
		case CIMDataType::EMBEDDEDINSTANCE:
			strtype = "instance";
			break;
		default:
			break;
    }
	return strtype;
}


//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMDataType::Type
OWPyConv::PyDataType2OW(
	const String& strt)
{
	CIMDataType::Type dtype;
	if (strt == "char16")
		dtype = CIMDataType::CHAR16;
	else if (strt == "reference")
		dtype =  CIMDataType::REFERENCE;
	else if (strt == "datetime")
		dtype =  CIMDataType::DATETIME;
	else if (strt == "boolean")
		dtype =  CIMDataType::BOOLEAN;
	else if (strt == "real32")
		dtype =  CIMDataType::REAL32;
	else if (strt == "real64")
		dtype =  CIMDataType::REAL64;
	else if (strt == "sint8")
		dtype =  CIMDataType::SINT8;
	else if (strt == "sint16")
		dtype =  CIMDataType::SINT16;
	else if (strt == "sint32")
		dtype =  CIMDataType::SINT32;
	else if (strt == "sint64")
		dtype =  CIMDataType::SINT64;
	else if (strt == "uint8")
		dtype =  CIMDataType::UINT8;
	else if (strt == "uint16")
		dtype =  CIMDataType::UINT16;
	else if (strt == "uint32")
		dtype =  CIMDataType::UINT32;
	else if (strt == "uint64")
		dtype =  CIMDataType::UINT64;
	else if (strt == "string")
		dtype =  CIMDataType::STRING;
	else if (strt == "class")
		dtype =  CIMDataType::EMBEDDEDCLASS;
	else if (strt == "instance")
		dtype =  CIMDataType::EMBEDDEDINSTANCE;
	else
		OW_THROW(PyConversionException,
			Format("Unknown python type encountered in PyDataType2OW: %1",strt).c_str());
	return dtype;
}


//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::OWVal2Py(const CIMValue& owval)
{
	// Assume owval is NOT NULL

	switch (owval.getCIMDataType().getType())
	{
		// Not implemented
		case CIMDataType::CHAR16:	// Not handled by pywbem
			OW_THROW(PyConversionException,
				"Conversion from OW Char16 to python not supported");
			break;
		case CIMDataType::EMBEDDEDCLASS:
		{
			if (owval.isArray())
			{
				Array<CIMClass> val;
				owval.get(val);
				Py::List blist;
				for (size_t i = 0; i < val.size(); ++i)
				{
					blist.append(OWClass2Py(val[i]));
				}
				return blist;
			}
			CIMClass cc;
			owval.get(cc);
			return OWClass2Py(cc);
		}

		case CIMDataType::EMBEDDEDINSTANCE:
		{

			if (owval.isArray())
			{
				Array<CIMInstance> val;
				owval.get(val);
				Py::List blist;
				for (size_t i = 0; i < val.size(); ++i)
				{
					blist.append(OWInst2Py(val[i]));
				}
				return blist;
			}
			CIMInstance ci;
			owval.get(ci);
			return OWInst2Py(ci);
		}

		case CIMDataType::REFERENCE:
			return RefValOW2Py(owval);
		case CIMDataType::DATETIME:
			return DateTimeValOW2Py(owval);
		case CIMDataType::BOOLEAN:
		{
			if (owval.isArray())
			{
				Array<Bool> val;
				owval.get(val);
				Py::List blist;
				for (size_t i = 0; i < val.size(); ++i)
				{
					PyObject* bv = (val[i]) ? Py_True : Py_False;
					blist.append(Py::Object(bv));
				}
				return blist;
			}

			Bool b;
			owval.get(b);
			return (b) ?  Py::Object(Py_True) : Py::Object(Py_False);
		}

		case CIMDataType::REAL32:
			return numericOW2Py<Real32>("(d)", "Real32", owval);
		case CIMDataType::REAL64:
			return numericOW2Py<Real64>("(d)", "Real64", owval);
		case CIMDataType::SINT8:
			return numericOW2Py<Int8>("(b)", "Sint8", owval);
		case CIMDataType::SINT16:
			return numericOW2Py<Int16>("(h)", "Sint16", owval);
		case CIMDataType::SINT32:
			return numericOW2Py<Int32>("(i)", "Sint32", owval);
		case CIMDataType::SINT64:
			return numericOW2Py<Int64>("(L)", "Sint64", owval);
		case CIMDataType::UINT8:
			return numericOW2Py<UInt8>("(k)", "Uint8", owval);
		case CIMDataType::UINT16:
			return numericOW2Py<UInt16>("(k)", "Uint16", owval);
		case CIMDataType::UINT32:
			return numericOW2Py<UInt32>("(k)", "Uint32", owval);
		case CIMDataType::UINT64:
			return numericOW2Py<UInt64>("(K)", "Uint64", owval);
		case CIMDataType::STRING:
		{
			if (owval.isArray())
			{
				StringArray sa;
				owval.get(sa);
				Py::List vlist;
				for (size_t i = 0; i < sa.size(); ++i)
				{
					vlist.append(Py::String(sa[i].c_str()));
				}
				return vlist;
			}

			String s;
			owval.get(s);
			return Py::String(s.c_str());
		}
		default:
			break;
    }

	OW_THROW(PyConversionException,
		"Unknown numeric data type while converting from OW to python");

	// Shouldn't get here
	return Py::Object();
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::OWRef2Py(const CIMObjectPath& cop)
{
	if (cop.isClassPath())
	{
		Py::Callable pyfunc = g_modpywbem.getAttr("CIMClassName");
		Py::Tuple args(3);
		args[0] = Py::String(cop.getClassName());
		args[1] = Py::String(cop.getHost());
		args[2] = Py::String(cop.getNameSpace());
		return pyfunc.apply(args);
	}

	Py::Callable pyfunc = g_modpywbem.getAttr("CIMInstanceName");
	Py::Dict dict;

    CIMPropertyArray cpa = cop.getKeys();
	for (CIMPropertyArray::size_type i = 0; i < cpa.size(); i++)
	{
		const CIMProperty& prop = cpa[i];
		CIMValue cv = prop.getValue();
		if (cv)
		{
			Py::Object pVal = OWVal2Py(prop.getValue());
			dict[prop.getName().c_str()] = pVal;
		}
	}
	Py::Tuple fargs(4);
	fargs[0] = Py::String(cop.getClassName());
	fargs[1] = dict;
	fargs[2] = Py::String(cop.getHost());
	fargs[3] = Py::String(cop.getNameSpace());

	// Return results of CIMInstanceName call.
	// Should be a pywbem.CIMInstance object
	return pyfunc.apply(fargs);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::OWInst2Py(const CIMInstance& ci, const String& nsArg)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMInstance");
	Py::Tuple pyarg(4);
	pyarg[0] = Py::String(ci.getClassName());
	pyarg[1] = makePropDict(ci.getProperties());
	pyarg[2] = makeQualDict(ci.getQualifiers());
	String ns = ci.getNameSpace();
	if (ns.empty())
	{
		ns = nsArg;
	}

	CIMObjectPath icop(ns, ci);

	// If icop is a class path, then that means we didn't get
	// the key properties or their 'key' qualifiers because
	// of the type of instance that was given. This could
	// be the result of having the 'ci' parameter built
	// with includeQualifiers=false or localOnly=true or
	// a restrictive propertyList or any/all of the above.
	if (icop.isClassPath())
		pyarg[3] = Py::None();	// No way to determine keys
	else
		pyarg[3] = OWRef2Py(icop);
	return pyfunc.apply(pyarg);
}


//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::OWQual2Py(const CIMQualifier& qual)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMQualifier");
	Py::Tuple pyarg(8);
	pyarg[0] = Py::String(qual.getName());
	Py::Object qval;
	CIMValue cv = qual.getValue();
	if (cv)
	{
		pyarg[1] = OWVal2Py(cv);	// value
		pyarg[2] = Py::String(OWDataType2Py(cv.getType()));
	}
	else
	{
		pyarg[1] = Py::Object();	// value
		CIMQualifierType qt = qual.getDefaults();
		pyarg[2] = Py::String(OWDataType2Py(qt.getDataType().getType()));	// type
	}
	pyarg[3] = bool2Py(qual.getPropagated());						// propagated
	pyarg[4] = bool2Py(qual.hasFlavor(CIMFlavor::ENABLEOVERRIDE));	// overridable
	pyarg[5] = bool2Py(qual.hasFlavor(CIMFlavor::TOSUBCLASS));		// tosubclass
	pyarg[6] = Py::Object();										// toinstance
	pyarg[7] = bool2Py(qual.hasFlavor(CIMFlavor::TRANSLATE));		// translatable
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::OWCIMParam2Py(const CIMParameter& param)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMParameter");
	Py::Tuple pyarg(6);
	pyarg[0] = Py::String(param.getName());	// name
	CIMDataType dt = param.getType();
	pyarg[1] = Py::String(OWDataType2Py(dt.getType()));
	if (dt.isReferenceType())
		pyarg[2] = Py::String(dt.getRefClassName());	// reference_class
	else
		pyarg[2] = Py::Object();						// reference_class
	pyarg[3] = bool2Py(dt.isArrayType());				// is_array
	pyarg[4] = Py::Int(dt.getSize());
	pyarg[5] = makeQualDict(param.getQualifiers());		// Qualifiers
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::OWMeth2Py(const CIMMethod& meth)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMMethod");
	Py::Tuple pyarg(6);
	pyarg[0] = Py::String(meth.getName());
	pyarg[1] = Py::String(OWDataType2Py(meth.getReturnType().getType()));

	CIMParameterArray pra = meth.getParameters();
	Py::Dict params;
	for(CIMParameterArray::size_type i = 0; i < pra.size(); i++)
	{
		params[pra[i].getName()] = OWCIMParam2Py(pra[i]);
	}
	pyarg[2] = params;
	pyarg[3] = Py::String(meth.getOriginClass());
	pyarg[4] = bool2Py(meth.getPropagated());
	pyarg[5] = makeQualDict(meth.getQualifiers());
	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
Py::Object
OWPyConv::OWProperty2Py(const CIMProperty& prop)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMProperty");
	Py::Tuple pyarg(10);
	pyarg[0] = Py::String(prop.getName());	// name

	CIMDataType dt = prop.getDataType();

	CIMValue cv = prop.getValue();
	if (cv)
	{
		pyarg[1] = OWVal2Py(cv);	// value
		pyarg[2] = Py::String(OWDataType2Py(cv.getType()));	// type
	}
	else
	{
		pyarg[1] = Py::Object();	// value
		pyarg[2] = Py::String(OWDataType2Py(dt.getType()));	// type
	}

	if (prop.getOriginClass().empty())
	{
		pyarg[3] = Py::Object();	// origin
	}
	else
	{
		pyarg[3] = Py::String(prop.getOriginClass());	// class_origin
	}

	pyarg[4] = Py::Int(dt.getSize());			// array_size
	pyarg[5] = bool2Py(prop.getPropagated());	// propagated
	pyarg[6] = bool2Py(dt.isArrayType());		// is_array
	if (dt.isReferenceType())
	{
		pyarg[7] = Py::String(dt.getRefClassName());	// reference_class
	}
	else
	{
		pyarg[7] = Py::Object();					// reference_class
	}

	pyarg[8] = makeQualDict(prop.getQualifiers());
	if (dt.getType() == CIMDataType::EMBEDDEDCLASS)
	{
		pyarg[9] = Py::String("object");
	}
	else if (dt.getType() == CIMDataType::EMBEDDEDINSTANCE)
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
OWPyConv::OWClass2Py(const CIMClass& cls)
{
	Py::Callable pyfunc = g_modpywbem.getAttr("CIMClass");
	Py::Tuple pyarg(5);
	pyarg[0] = Py::String(cls.getName());
	pyarg[1] = makePropDict(cls.getProperties());
	pyarg[2] = makeMethDict(cls.getMethods());

	String superClass = cls.getSuperClass();
	if(superClass.empty())
	{
		pyarg[3] = Py::Object();
	}
	else
	{
		pyarg[3] = Py::String(superClass);
	}

	pyarg[4] = makeQualDict(cls.getQualifiers());

	return pyfunc.apply(pyarg);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMInstance
OWPyConv::PyInst2OW(const Py::Object& pyci, const String& nsArg)
{
	String ns;
    CIMInstance inst(stringAttr(pyci, "classname"));

	CIMObjectPath cop(CIMNULL);
	if (pyci.hasAttr("path"))
	{
		Py::Object pyref = pyci.getAttr("path");
		if (!pyref.isNone())
		{
			cop = PyRef2OW(pyref, nsArg);
			ns = cop.getNameSpace();
		}
	}

	inst.setNameSpace(ns);
	Py::Mapping props = pyci.getAttr("properties");
	inst.setProperties(getProps(props));
	if (cop)
	{
		CIMPropertyArray pra = cop.getKeys();
		if (pra.size())
			inst.setKeys(pra);
	}

    return inst; 
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMObjectPath
OWPyConv::PyRef2OW(
	const Py::Object& pycop,
	const String& nsArg)
{
	String className = stringAttr(pycop, "classname");
	String ns = stringAttr(pycop, "namespace");
	if (ns.empty())
	{
		ns = nsArg;
	}
	CIMObjectPath cop(className, ns);
	Py::Mapping kb = pycop.getAttr("keybindings");
	Py::List items = kb.items();
	Py::Object pciName = g_modpywbem.getAttr("CIMInstanceName");
	Py::Object pciClassName = g_modpywbem.getAttr("CIMClassName");
	
	for (int i = 0; i < int(items.length()); i++)
	{
		Py::Tuple tup(items[i]);
		if (tup.length() != 2)
		{
			OW_THROW(PyConversionException, Format("Py Ref Conversion: "
				"keybinding tuple as size of %1", tup.length()).c_str());
		}

		String kname(Py::String(tup[0]).as_ow_string());
		Py::Object pkval = tup[1];
		CIMValue cv(CIMNULL); 

		if (pkval.isBool())
		{
			cv = PyVal2OW("boolean", pkval);
		}
		else if (pkval.isString())
		{
			cv = PyVal2OW("string", pkval);
		}
		else if (pkval.isInt())
		{
			try
			{
				cv = CIMValue(Int64(Py::Int(pkval).asLongLong()));
			}
			catch(Py::OverflowError& err)
			{
				err.clear();
				// This can throw
				cv = CIMValue(Int64(Py::Int(pkval).asUnsignedLongLong()));
			}
		}
		else if (pkval.isLong())
		{
			try
			{
				cv = CIMValue(Int64(Py::Long(pkval).asLongLong()));
			}
			catch(Py::OverflowError& err)
			{
				err.clear();
				// This can throw
				cv = CIMValue(Int64(Py::Long(pkval).asUnsignedLongLong()));
			}
		}
		else if (pkval.isFloat())
		{
			cv = PyVal2OW("real64", pkval); 
		}
		else if (pkval.isInstanceOf(pciClassName)
			|| pkval.isInstanceOf(pciName))
		{
			cv = PyVal2OW("reference", pkval); 
		}
		else
		{
			OW_THROW(PyConversionException, Format("Py Ref Conversion: "
				"unhandle value for key: %1, type: %2",
				kname, pkval.type().as_string()).c_str());
		}
		cop.setKeyValue(CIMName(kname), cv); 
	}
    return cop; 
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMValue
OWPyConv::PyVal2OW(
	const String& type,
	const Py::Object& pyval)
{
	if (type == "boolean")
	{
		if (pyval.isList())
		{
			Py::List bl(pyval);
			size_t sz = bl.length();
			Array<Bool> bra(sz);
			for(size_t i = 0; i < sz; i++)
			{
					bra[i] = bl[i].isTrue();
			}
			return CIMValue(bra);
		}
		return CIMValue(Bool(pyval.isTrue()));
	}
	else if (type == "string")
	{
		if (pyval.isList())
		{
			Py::List sl(pyval);
			size_t sz = sl.length();
			StringArray sra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				sra[i] = String(Py::String(sl[i]));
			}
			return CIMValue(sra);
		}
		return CIMValue(String(Py::String(pyval)));
	}
	else if (type == "uint8")
	{
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			UInt8Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = UInt8(Py::Int(il[i]).asUnsignedLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(UInt8(Py::Int(pyval).asUnsignedLong()));
	}
	else if (type == "sint8")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			Int8Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = Int8(Py::Int(il[i]).asLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Int8(Py::Int(pyval).asLong()));
	}
	else if (type == "uint16")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			UInt16Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = UInt16(Py::Int(il[i]).asUnsignedLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(UInt16(Py::Int(pyval).asUnsignedLong()));
	}
	else if (type == "sint16")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			Int16Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = Int16(Py::Int(il[i]).asLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Int16(Py::Int(pyval).asLong()));
	}
	else if (type == "uint32")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			UInt32Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = UInt32(Py::Int(il[i]).asUnsignedLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(UInt32(Py::Int(pyval).asUnsignedLong()));
	}
	else if (type == "sint32")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			Int32Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = Int32(Py::Int(il[i]).asLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Int32(Py::Int(pyval).asLong()));
	}
	else if (type == "uint64")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			UInt64Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = UInt64(Py::LongLong(il[i]).asUnsignedLongLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(UInt64(Py::LongLong(pyval).asUnsignedLongLong()));
	}
	else if (type == "sint64")
    {
		if (pyval.isList())
		{
			Py::List il(pyval);
			size_t sz = il.length();
			Int64Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = Int64(Py::LongLong(il[i]).asLongLong());
			}
			return CIMValue(nra);
		}
		return CIMValue(Int64(Py::LongLong(pyval).asLongLong()));
	}
	else if (type == "real32")
	{
		if (pyval.isList())
		{
			Py::List fl(pyval);
			size_t sz = fl.length();
			Real32Array nra(sz);
			for(size_t i = 0; i < sz; i++)
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
			size_t sz = fl.length();
			Real64Array nra(sz);
			for(size_t i = 0; i < sz; i++)
			{
				nra[i] = Real64(Py::Float(fl[i]).as_double());
			}
			return CIMValue(nra);
		}
		return CIMValue(Real64(Py::Float(pyval).as_double()));
	}
	else if (type == "char16")
    {
		OW_THROW(PyConversionException,
			"Unable to convert to OW from python char16");
    }
	else if (type == "datetime")
    {
		if (pyval.isList())
		{
			Py::List dl(pyval);
			size_t sz = dl.length();
			Array<CIMDateTime> ra(sz); 
			for (size_t i = 0; i < sz; ++i)
			{
				ra[i] = convertPyDateTime(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(convertPyDateTime(pyval)); 
    }
	else if (type == "reference")
    {
		if (pyval.isList())
		{
			Py::List dl(pyval);
			size_t sz = dl.length();
			Array<CIMObjectPath> ra(sz); 
			for (size_t i = 0; i < sz; ++i)
			{
				ra[i] = PyRef2OW(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(PyRef2OW(pyval)); 
    }
	else if (type == "instance")
	{
		if (pyval.isList())
		{
			Py::List dl(pyval);
			size_t sz = dl.length();
			Array<CIMInstance> ra(sz); 
			for (size_t i = 0; i < sz; ++i)
			{
				ra[i] = PyInst2OW(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(PyInst2OW(pyval));
	}
	else if (type == "class")
	{
		if (pyval.isList())
		{
			Py::List dl(pyval);
			size_t sz = dl.length();
			Array<CIMClass> ra(sz); 
			for (size_t i = 0; i < sz; ++i)
			{
				ra[i] = PyClass2OW(dl[i]);
			}
			return CIMValue(ra);
		}
		return CIMValue(PyClass2OW(pyval));
	}

	OW_THROW(PyConversionException,
		Format("Unknown python data type for conversion: %1", type).c_str());

	// Shouldn't hit here
    return CIMValue(CIMNULL); 
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMValue OWPyConv::PyVal2OW(
	const Py::Tuple& tuple)
{
	if (tuple.length() != 2)
	{
		OW_THROW(PyConversionException, Format("Invalid python tuple given for "
			"data type conversion. len: %1", tuple.length()).c_str());
	}
	Py::Object wko = tuple[0];
	if (!wko.isString())
	{
		OW_THROW(PyConversionException, "Invalid python tuple given for "
			"data type conversion. first element is not a string");
	}
	String dtname = Py::String(wko).as_ow_string();
	return PyVal2OW(dtname, tuple[1]);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMClass
OWPyConv::PyClass2OW(const Py::Object& pycls)
{
	String theName = Py::String(pycls.getAttr("classname")).as_ow_string();
	CIMClass theClass(theName);
	Py::Object wko = pycls.getAttr("superclass");
	if (!wko.isNone())
	{
		theClass.setSuperClass(Py::String(wko).as_ow_string());
	}

	Py::Mapping pymap = pycls.getAttr("properties");
	theClass.setProperties(getProps(pymap));

	pymap = pycls.getAttr("qualifiers");
	theClass.setQualifiers(getQuals(pymap));

	pymap = pycls.getAttr("methods");
	CIMMethodArray methra;
	Py::List methList = pymap.values();
	int len = int(methList.length());
	for(int i = 0; i < len; i++)
	{
		methra.append(PyMeth2OW(methList[i]));
	}
	theClass.setMethods(methra);
	return theClass;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMProperty
OWPyConv::PyProperty2OW(const Py::Object& pyprop)
{
	String theName = Py::String(pyprop.getAttr("name")).as_ow_string();
	CIMProperty theProp(theName);

	// Convert data type
	String strtype = Py::String(pyprop.getAttr("type")).as_ow_string();
	CIMDataType theDataType(PyDataType2OW(strtype));
	Py::Object wko = pyprop.getAttr("is_array");
	if (wko.isTrue())
	{
		theDataType.setToArrayType(0);
	}

	theProp.setDataType(theDataType);

	// Convert value
	wko = pyprop.getAttr("embedded_object");
	if (!wko.isNone())
	{
		String stremb = Py::String(wko).as_ow_string();
		if (stremb.equalsIgnoreCase("instance"))
			strtype = "instance";
		else
			strtype = "class";
	}
	wko = pyprop.getAttr("value");
	if (!wko.isNone())
	{
		theProp.setValue(PyVal2OW(strtype, wko));
	}

	if (pyprop.getAttr("propagated").isTrue())
	{
		theProp.setPropagated(true);
	}

	wko = pyprop.getAttr("class_origin");
	if(wko.isString())
	{
		theProp.setOriginClass(Py::String(wko).as_ow_string());
	}

	Py::Mapping pyqualDict(pyprop.getAttr("qualifiers"));
	theProp.setQualifiers(getQuals(pyqualDict));
	return theProp;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMParameter
OWPyConv::PyCIMParam2OW(const Py::Object& pyparam)
{
	Py::Object wko;
	String theName = Py::String(pyparam.getAttr("name")).as_ow_string();
	CIMParameter theParam(theName);

	// Convert data type
	String strtype = Py::String(pyparam.getAttr("type")).as_ow_string();
	CIMDataType dt(PyDataType2OW(strtype));
	if (pyparam.getAttr("is_array").isTrue())
	{
		Int32 raSize = 0;
		wko = pyparam.getAttr("array_size");
		if (!wko.isNone())
		{
			raSize = int(Py::Int(wko));
		}
		dt.setToArrayType(raSize);
	}
	theParam.setDataType(dt);

	// Set the qualifiers for the parameter
	Py::Mapping pyqualDict(pyparam.getAttr("qualifiers"));
	theParam.setQualifiers(getQuals(pyqualDict));
	return theParam;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMQualifier
OWPyConv::PyQual2OW(const Py::Object& pyqual)
{
	String theName = Py::String(pyqual.getAttr("name")).as_ow_string();
	String strtype = Py::String(pyqual.getAttr("type")).as_ow_string();
	CIMDataType::Type theDataType = PyDataType2OW(strtype);
	CIMValue theValue(CIMNULL);
	Py::Object qv = pyqual.getAttr("value");
	if (!qv.isNone())
	{
		theValue = PyVal2OW(strtype, qv);
	}
	CIMQualifierType cqt(theName);
	cqt.setDataType(theDataType);
	CIMQualifier theQual(cqt);
	theQual.setValue(theValue);
	Py::Object wko = pyqual.getAttr("propagated");
	theQual.setPropagated(wko.isTrue());
	wko = pyqual.getAttr("tosubclass");
	bool tosubclass = (wko.isNone()) ? true : wko.isTrue();
	if (tosubclass)
		theQual.addFlavor(CIMFlavor(CIMFlavor::TOSUBCLASS));
	wko = pyqual.getAttr("overridable");
	bool overridable = (wko.isNone()) ? true : wko.isTrue();
	if (overridable)
		theQual.addFlavor(CIMFlavor(CIMFlavor::ENABLEOVERRIDE));
	else
		theQual.addFlavor(CIMFlavor(CIMFlavor::DISABLEOVERRIDE));
	if (pyqual.getAttr("translatable").isTrue())
		theQual.addFlavor(CIMFlavor(CIMFlavor::TRANSLATE));
	return theQual;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
CIMMethod
OWPyConv::PyMeth2OW(const Py::Object& pymeth)
{
	String theName = Py::String(pymeth.getAttr("name")).as_ow_string();
	CIMMethod theMethod(theName);
	String strtype = Py::String(pymeth.getAttr("return_type")).as_ow_string();
	theMethod.setReturnType(CIMDataType(PyDataType2OW(strtype)));

	Py::Object wko = pymeth.getAttr("class_origin");
	if(wko.isString())
	{
		theMethod.setOriginClass(Py::String(wko).as_ow_string());
	}
	if (pymeth.getAttr("propagated").isTrue())
	{
		theMethod.setPropagated(true);
	}
	wko = pymeth.getAttr("parameters");
	if (!wko.isNone())
	{
		Py::Mapping parmDict(wko);

		CIMParameterArray pra;
		Py::List items(parmDict.items());
		for (int i = 0; i < int(items.length()); i++)
		{
			pra.append(PyCIMParam2OW(items[i]));
		}
		theMethod.setParameters(pra);
	}
	Py::Mapping pyqualDict(pymeth.getAttr("qualifiers"));
	theMethod.setQualifiers(getQuals(pyqualDict));
	return theMethod;
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::RefValOW2Py(const CIMValue& owval)
{
	CIMObjectPath cop;
	owval.get(cop);
	return OWRef2Py(cop);
}

//////////////////////////////////////////////////////////////////////////////
// STATIC
Py::Object
OWPyConv::DateTimeValOW2Py(const CIMValue& owval)
{
	if (!owval.isArray())
	{
		CIMDateTime dt;
		owval.get(dt);
		return convertOWDateTime(dt);
	}

	CIMDateTimeArray val;
	owval.get(val);
	Py::List dtlist;
	for (size_t i = 0; i < val.size(); ++i)
	{
		dtlist.append(convertOWDateTime(val[i]));
	}

	return dtlist;
}

}	// End of namespace PythonProvIFC
