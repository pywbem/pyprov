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
#include "common.hpp"
#include "converter.hpp"
#include <openwbem/OW_CIMInstance.hpp>
#include <openwbem/OW_CIMProperty.hpp>
#include <openwbem/OW_CIMValue.hpp>
#include <openwbem/OW_CIMDateTime.hpp>
#include <openwbem/OW_Bool.hpp>
#include <openwbem/OW_CIMObjectPath.hpp>

#include <iostream>
using namespace std; 

using namespace OpenWBEM; 

static PyObject* g_pModulePyWBEM = 0;

namespace OW_Py_Converter
{

PyObject* RefOW2Py(const CIMValue& owval);
PyObject* DateTimeOW2Py(const CIMValue& owval);
CIMValue CIMValue_Py2OW(const char* type, PyObject* pyval); 

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void setModulePyWBEM(PyObject* mod)
{
    Py_XDECREF(g_pModulePyWBEM);
    g_pModulePyWBEM = mod; 
    Py_INCREF(g_pModulePyWBEM); 
}

//////////////////////////////////////////////////////////////////////////////
template <typename T> 
PyObject* numericOW2Py(const char* format, const char* func, 
		       const CIMValue& owval)
{
    PyObject* pFunc = PyObject_GetAttrString(g_pModulePyWBEM, 
					     const_cast<char*>(func)); 
    assert(pFunc); 
    assert(PyCallable_Check(pFunc));

	DecRefs dr;
	dr += pFunc;

    if (owval.isArray())
    {
		Array<T> val;
		owval.get(val); 
		PyObject* pList = PyList_New(val.size()); 
		assert(pList);

		for (size_t i = 0; i < val.size(); ++i)
		{
			PyObject* pVal = PyObject_CallFunction(pFunc,
				const_cast<char*>(format), val[i]);

			if (!pVal)
			{
				PyErr_Print();
				assert(0);
			}

			if (PyList_SetItem(pList, i, pVal))
			{
				PyErr_Print();
				assert(0);
			}
		}

		return pList; 
    }

	T val;
	owval.get(val); 

	PyObject* pVal = PyObject_CallFunction(pFunc,
		const_cast<char*>(format), val);

	assert(pVal); 
	return pVal; 
}

//////////////////////////////////////////////////////////////////////////////
PyObject* CIMValue_OW2Py(const OpenWBEM::CIMValue& owval)
{
	if (!owval)
	{
		cout << "CIMValue_OW2Py: NULL owval parm. Returning NULL" << endl;
		return NULL;
	}

	switch (owval.getCIMDataType().getType())
	{
		case CIMDataType::CHAR16:	// Not handled by pywbem
			break;
		case CIMDataType::EMBEDDEDCLASS:
			break;
		case CIMDataType::EMBEDDEDINSTANCE:
			break;

		case CIMDataType::DATETIME:
			return DateTimeOW2Py(owval);
		case CIMDataType::BOOLEAN:
		{
			if (owval.isArray())
			{
				Array<Bool> val;
				owval.get(val);
				PyObject* pList = PyList_New(val.size());
				assert(pList);

				for (size_t i = 0; i < val.size(); ++i)
				{
					PyObject* bv = (val[i]) ? Py_True : Py_False;
					Py_INCREF(bv);
					if (PyList_SetItem(pList, i, bv))
					{
						PyErr_Print();
						assert(0);
					}
				}
				return pList; 
			}

			Bool b;
			owval.get(b);
			if (b)
			{
				Py_RETURN_TRUE;
			}
			Py_RETURN_FALSE;
		}

		case CIMDataType::REAL32:
			return numericOW2Py<Real32>("(d)", "Real32", owval);
		case CIMDataType::REAL64:
			return numericOW2Py<Real64>("(d)", "Real64", owval);
		case CIMDataType::REFERENCE:
			return RefOW2Py(owval);
		case CIMDataType::SINT8:
			//return numericOW2Py<Int8>("(l)", "Sint8", owval);
			return numericOW2Py<Int8>("(b)", "Sint8", owval);
		case CIMDataType::SINT16:
			//return numericOW2Py<Int16>("(l)", "Sint16", owval);
			return numericOW2Py<Int16>("(h)", "Sint16", owval);
		case CIMDataType::SINT32:
			//return numericOW2Py<Int32>("(l)", "Sint32", owval);
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
				PyObject* pList = PyList_New(sa.size());
				if (!pList)
				{
					PyErr_Print();
					assert(0);
				}

				for (size_t i = 0; i < sa.size(); ++i)
				{
					PyObject* pStr = PyString_FromString(sa[i].c_str());
					if (!pStr)
					{
						PyErr_Print();
						assert(0);
					}
					if (PyList_SetItem(pList, i, pStr))
					{
						PyErr_Print();
						assert(0);
					}
				}
				return pList;
			}

			String s;
			owval.get(s);
			return PyString_FromString(s.c_str());
		}
		default:
			; // TODO error

    }
	// TODO error
	return 0; 
}

//////////////////////////////////////////////////////////////////////////////
PyObject*
convertOWDateTime(const CIMDateTime& dt)
{
	DecRefs dr;
	PyObject* pFunc = 0;
	if (dt.isInterval())
	{
		pFunc = PyObject_GetAttrString(g_pModulePyWBEM,
			const_cast<char*>("timedelta"));
		//	const_cast<char*>("datetime.timedelta"));
		if (!pFunc)
		{
			PyErr_Print();
			assert(0);
		}
		dr += pFunc;

		PyObject* dtobj = PyObject_CallFunction(pFunc,
			"(iiiiiii)",
			(int)dt.getDays(),
			(int)dt.getSeconds(),
			(int)dt.getMicroSeconds(),
			(int)0,
			(int)dt.getMinutes(),
			(int)dt.getHours(),
			(int)0);
		assert(dtobj);
		return dtobj;
	}

	pFunc = PyObject_GetAttrString(g_pModulePyWBEM, 
				       const_cast<char*>("utcoffset")); 
	if (!pFunc)
	{
		PyErr_Print();
		assert(0);
	}
	dr += pFunc;
	PyObject* utcobj = PyObject_CallFunction(pFunc, "(i)", 
			(int)dt.getUtc()); 
	dr += utcobj; 

	pFunc = PyObject_GetAttrString(g_pModulePyWBEM,
		const_cast<char*>("datetime"));
		//const_cast<char*>("datetime.datetime"));
	if (!pFunc)
	{
		PyErr_Print();
		assert(0);
	}
	dr += pFunc;


	PyObject* dtobj = PyObject_CallFunction(pFunc,
		"(iiiiiiiO)",
		(int)dt.getYear(),
		(int)dt.getMonth(),
		(int)dt.getDay(),
		(int)dt.getHours(),
		(int)dt.getMinutes(),
		(int)dt.getSeconds(),
		(int)dt.getMicroSeconds(),
		utcobj); 

	assert(dtobj);
	return dtobj;
}

//////////////////////////////////////////////////////////////////////////////
PyObject*
DateTimeOW2Py(const CIMValue& owval)
{
	DecRefs dr;

	if (!owval.isArray())
	{
		CIMDateTime dt;
		owval.get(dt);
		return convertOWDateTime(dt);
	}

	CIMDateTimeArray val;
	owval.get(val);
	PyObject* pList = PyList_New(val.size());
	assert(pList);

	for (size_t i = 0; i < val.size(); ++i)
	{
		PyObject* pVal = convertOWDateTime(val[i]);
		if (!pVal)
		{
			PyErr_Print();
			assert(0);
		}
		if (PyList_SetItem(pList, i, pVal))
		{
			PyErr_Print();
			assert(0);
		}
	}

	return pList;
}

//////////////////////////////////////////////////////////////////////////////
PyObject*
RefOW2Py(const CIMValue& owval)
{
	CIMObjectPath cop;
	owval.get(cop);

	DecRefs dr;
    PyObject* pFunc = PyObject_GetAttrString(g_pModulePyWBEM,
		const_cast<char*>("CIMInstanceName"));
    assert(pFunc); 
	dr += pFunc;

    assert(PyCallable_Check(pFunc));

	PyObject* pdict = PyDict_New();
	assert(pdict);

    CIMPropertyArray cpa = cop.getKeys();
    for (CIMPropertyArray::const_iterator iter = cpa.begin(); iter != cpa.end();
	  ++iter)
    {
		const CIMProperty& prop = *iter;
		PyObject* pVal = CIMValue_OW2Py(prop.getValue()); 
		assert(pVal);

		if (-1 == PyDict_SetItemString(pdict,
			const_cast<char*>(prop.getName().c_str()),pVal))
		{
			PyErr_Print();
			assert(0);
		}
    }

	PyObject* piname = PyObject_CallFunction(pFunc,
		"(sOss)", cop.getClassName().c_str(), pdict,
		cop.getHost().c_str(), cop.getNameSpace().c_str());
	assert(piname);

	return piname;
}

//////////////////////////////////////////////////////////////////////////////
PyObject*
CIMInstance_OW2Py(const OpenWBEM::CIMInstance& owinst)
{
	DecRefs dr;

    PyObject* pFunc = PyObject_GetAttrString(g_pModulePyWBEM, "CIMInstance");
	if (!pFunc)
	{
		return 0;
	}
	dr += pFunc;

    if (!PyCallable_Check(pFunc))
    {
		//cerr << "Can't get pywbem.CIMInstance callable object" << endl;
		return 0; 
    }

	PyObject* pInst = PyObject_CallFunction(pFunc,
		"(s)", owinst.getClassName().c_str());

    assert(PyMapping_Check(pInst)); 

    CIMPropertyArray cpa = owinst.getProperties(); 
    for (CIMPropertyArray::const_iterator iter = cpa.begin(); iter != cpa.end(); 
	  ++iter)
    {
		const CIMProperty& prop = *iter;

		PyObject* pVal = CIMValue_OW2Py(prop.getValue());
		assert(pVal);
		// cerr << "pVal: " << PyString_AsString(pVal) << endl;

		if(-1 == PyMapping_SetItemString(pInst,
			const_cast<char*>(prop.getName().c_str()), pVal))
		{
			PyErr_Print();
			assert(0);
		}

    }
    return pInst; 
}

template <typename OWT, typename PYT>
OWT buildOWType(PYT (*pyfunc)(PyObject* p), PyObject* pyval)
{
    OWT rv = (*pyfunc)(pyval); 
    return rv; 
}

template <typename OWT, typename PYT>
Array<OWT> buildOWArrayType(PYT (*pyfunc)(PyObject* p), PyObject* pyval)
{

    size_t sz = PyList_Size(pyval); 
    Array<OWT> rv(sz); 
    for (size_t i = 0; i < sz; ++i)
    {
	PyObject* pv = PyList_GetItem(pyval, i); 
	rv[i] = buildOWType<OWT, PYT>(pyfunc, pv); 
    }
    return rv; 
}

//////////////////////////////////////////////////////////////////////////////
CIMDateTime convertPyDateTime(PyObject* pydt)
{
    CIMDateTime cdt; 
    DecRefs decRefs; 
    PyObject* pval = 0; 

    PyObject* dtclass = PyObject_GetAttrString(g_pModulePyWBEM, "datetime"); 
    decRefs += dtclass; 
    PyObject* tdeltaclass = PyObject_GetAttrString(g_pModulePyWBEM, "timedelta"); 
    decRefs += tdeltaclass; 


    if (PyObject_IsInstance(pydt, dtclass))
    {
	cdt.setInterval(false); 
	pval = PyObject_GetAttrString(pydt, "year"); 
	decRefs += pval; 
	int year = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "month"); 
	decRefs += pval; 
	int month = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "day"); 
	decRefs += pval; 
	int day = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "hour"); 
	decRefs += pval; 
	int hour = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "minute"); 
	decRefs += pval; 
	int minute = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "second"); 
	decRefs += pval; 
	int second = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "microsecond"); 
	decRefs += pval; 
	int microsecond = PyInt_AsLong(pval); 
	PyObject* poffset = PyObject_CallMethod(pydt, "utcoffset", NULL); 
	decRefs += poffset; 
	int offset = 0; 
	if (poffset)
	{
	    pval = PyObject_GetAttrString(poffset, "seconds"); 
	    decRefs += pval; 
	    offset = PyInt_AsLong(pval) / 60; 
	    pval = PyObject_GetAttrString(poffset, "days"); 
	    decRefs += pval; 
	    if (PyInt_AsLong(pval) == -1)
	    {
		offset = -(24*60 - offset); 
	    }
	}

	cdt.setYear(year); 
	cdt.setMonth(month); 
	cdt.setDay(day); 
	cdt.setHours(hour); 
	cdt.setMinutes(minute); 
	cdt.setSeconds(second); 
	cdt.setMicroSeconds(microsecond); 
	cdt.setUtc(offset); 
    }
    else if (PyObject_IsInstance(pydt, tdeltaclass))
    {
	cdt.setInterval(true); 
	pval = PyObject_GetAttrString(pydt, "days"); 
	decRefs += pval; 
	int days = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "seconds"); 
	decRefs += pval; 
	int seconds = PyInt_AsLong(pval); 
	pval = PyObject_GetAttrString(pydt, "microseconds"); 
	decRefs += pval; 
	int microseconds = PyInt_AsLong(pval); 

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
	// ERROR
    }
    return cdt; 
}

//////////////////////////////////////////////////////////////////////////////
CIMObjectPath RefPy2OW(PyObject* pyref)
{
    DecRefs dr; 
    PyObject* val = 0; 
    val = PyObject_GetAttrString(pyref, "classname"); 
    dr += val; 
    String className(PyString_AsString(val)); 
    val = PyObject_GetAttrString(pyref, "namespace"); 
    dr += val; 
    String ns(PyString_AsString(val)); 
    CIMObjectPath cop(className, ns); 
    val = PyObject_GetAttrString(pyref, "keybindings"); 
    dr += val; 
    assert(PyMapping_Check(val)); 
    val = PyMapping_Items(val); 
    dr += val; 

    PyObject* pInstanceName = PyObject_GetAttrString(g_pModulePyWBEM, 
						     "CIMInstanceName"); 
    dr += pInstanceName; 
    PyObject* pClassName = PyObject_GetAttrString(g_pModulePyWBEM, 
						  "CIMClassName"); 
    dr += pClassName; 

    assert(PyList_Check(val)); 
    for (int i = 0; i < PyList_Size(val); ++i)
    {
	PyObject* tup = PyList_GetItem(val, i); 
	// tup is borrowed ref. 
	assert(PyTuple_Check(tup)); 
	assert(PyTuple_Size(tup) == 2); 
	PyObject* pkname = PyTuple_GetItem(tup, 0); 
	// pkname is borrowed. 
	PyObject* pkval = PyTuple_GetItem(tup, 1); 
	// pkname is borrowed. 
	String kname(PyString_AsString(pkname)); 
	CIMValue cv(CIMNULL); 
	if (PyBool_Check(pkval))
	{
	    cv = CIMValue_Py2OW("boolean", pkval); 
	}
	else if (PyString_Check(pkval))
	{
	    cv = CIMValue_Py2OW("string", pkval); 
	}
	else if (PyInt_Check(pkval) || PyLong_Check(pkval))
	{
	    // TODO what if it's a uint64, that won't fit in an int64. 
	    // test for negative? 
	    cv = CIMValue_Py2OW("sint64", pkval); 
	}
	else if (PyFloat_Check(pkval))
	{
	    cv = CIMValue_Py2OW("real64", pkval); 
	}
	else if (PyObject_IsInstance(pkval, pClassName) 
		 || PyObject_IsInstance(pkval, pInstanceName))
	{
	    cv = CIMValue_Py2OW("reference", pkval); 
	}
	else
	{
	    assert(0); 
	}
	cop.setKeyValue(kname, cv); 
    }
    return cop; 
}

//////////////////////////////////////////////////////////////////////////////
CIMValue CIMValue_Py2OW(const char* type, PyObject* pyval)
{
    if (strcmp(type, "boolean") == 0)
    {
	if (PyList_Check(pyval))
	{
	    size_t sz = PyList_Size(pyval); 
	    Array<Bool> bra(sz); 
	    for (size_t i = 0; i < sz; ++i)
	    {
		bra[i] = Bool(PyList_GetItem(pyval, i) == Py_True); 
	    }
	    return CIMValue(bra); 
	}
	else
	{
	    return CIMValue(Bool(pyval == Py_True)); 
	}
    }
    else if (strcmp(type, "string") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<String, char*>(PyString_AsString, pyval)); 
	else
	    return CIMValue(buildOWType<String, char*>(PyString_AsString, pyval)); 
    }
    else if (strcmp(type, "uint8") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<UInt8, unsigned long>(PyInt_AsUnsignedLongMask, pyval)); 
	else
	    return CIMValue(buildOWType<UInt8, unsigned long>(PyInt_AsUnsignedLongMask, pyval)); 
    }
    else if (strcmp(type, "sint8") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<Int8, long>(PyInt_AsLong, pyval)); 
	else
	    return CIMValue(buildOWType<Int8, long>(PyInt_AsLong, pyval)); 
    }
    else if (strcmp(type, "uint16") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<UInt16, unsigned long>(PyInt_AsUnsignedLongMask, pyval)); 
	else
	    return CIMValue(buildOWType<UInt16, unsigned long>(PyInt_AsUnsignedLongMask, pyval)); 
    }
    else if (strcmp(type, "sint16") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<Int16, long>(PyInt_AsLong, pyval)); 
	else
	    return CIMValue(buildOWType<Int16, long>(PyInt_AsLong, pyval)); 
    }
    else if (strcmp(type, "uint32") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<UInt32, unsigned long>(PyInt_AsUnsignedLongMask, pyval)); 
	else
	    return CIMValue(buildOWType<UInt32, unsigned long>(PyInt_AsUnsignedLongMask, pyval)); 
    }
    else if (strcmp(type, "sint32") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<Int32, long>(PyInt_AsLong, pyval)); 
	else
	    return CIMValue(buildOWType<Int32, long>(PyInt_AsLong, pyval)); 
    }
    else if (strcmp(type, "uint64") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<UInt64, unsigned long long>(PyLong_AsUnsignedLongLong, pyval)); 
	else
	    return CIMValue(buildOWType<UInt64, unsigned long long>(PyLong_AsUnsignedLongLong, pyval)); 
    }
    else if (strcmp(type, "sint64") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<Int64, long long>(PyLong_AsLongLong, pyval)); 
	else
	    return CIMValue(buildOWType<Int64, long long>(PyLong_AsLongLong, pyval)); 
    }
    else if (strcmp(type, "real32") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<Real32, double>(PyFloat_AsDouble, pyval)); 
	else
	    return CIMValue(buildOWType<Real32, double>(PyFloat_AsDouble, pyval)); 
    }
    else if (strcmp(type, "real64") == 0)
    {
	if (PyList_Check(pyval))
	    return CIMValue(buildOWArrayType<Real64, double>(PyFloat_AsDouble, pyval)); 
	else
	    return CIMValue(buildOWType<Real64, double>(PyFloat_AsDouble, pyval)); 
    }
    //else if (strcmp(type, "char16") == 0)
    //{
    //}
    else if (strcmp(type, "datetime") == 0)
    {
	if (PyList_Check(pyval))
	{
	    size_t sz = PyList_Size(pyval); 
	    Array<CIMDateTime> ra(sz); 
	    for (size_t i = 0; i < sz; ++i)
	    {
		ra[i] = convertPyDateTime(PyList_GetItem(pyval, i)); 
	    }
	    return CIMValue(ra); 
	}
	else
	{
	    return CIMValue(convertPyDateTime(pyval)); 
	}
    }
    else if (strcmp(type, "reference") == 0)
    {
	return CIMValue(RefPy2OW(pyval)); 
    }
    else
    {
	// ERROR
	cerr << "** Unknown type: " << type << endl;
	assert(0 == "unknown type"); 
    }

    return CIMValue(CIMNULL); 
}

//////////////////////////////////////////////////////////////////////////////
CIMInstance CIMInstance_Py2OW(PyObject* pyInst)
{
    DecRefs decRefs; 
    PyObject* val = PyObject_GetAttrString(pyInst, "classname"); 
    decRefs += val; 
    const char* cstr = PyString_AsString(val); 
    CIMInstance inst(cstr); 
    PyObject* pProps = PyObject_GetAttrString(pyInst, "properties"); 
    assert(pProps); 
    assert(PyMapping_Check(pProps)); 
    decRefs += pProps; 
    pProps = PyMapping_Values(pProps); 
    assert(pProps); 
    decRefs += pProps; 
    for (int i = 0; i < PyMapping_Length(pProps); ++i)
    {
	DecRefs decRefs; 
	PyObject* pProp = PySequence_GetItem(pProps, i); 
	assert(pProp); 
	decRefs += pProp; 
	val = PyObject_GetAttrString(pProp, "name"); 
	decRefs += val; 
	cstr = PyString_AsString(val); 
	String name(cstr); 
	val = PyObject_GetAttrString(pProp, "type"); 
	decRefs += val; 
	cstr = PyString_AsString(val); 
	val = PyObject_GetAttrString(pProp, "value"); 
	decRefs += val; 
	CIMValue owval = CIMValue_Py2OW(cstr, val); 
	inst.setProperty(name, owval); 
    }

    return inst; 


}

}	// End of namespace OW_Py_Converter
