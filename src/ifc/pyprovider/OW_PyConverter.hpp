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
#ifndef OW_PYCONVERTER_HPP_GUARD
#define OW_PYCONVERTER_HPP_GUARD

#include "PyCxxObjects.hpp"
#include <openwbem/OW_CIMValue.hpp>
#include <openwbem/OW_CIMInstance.hpp>
#include <openwbem/OW_CIMObjectPath.hpp>
#include <openwbem/OW_CIMQualifier.hpp>
#include <openwbem/OW_CIMQualifierType.hpp>
#include <openwbem/OW_CIMClass.hpp>
#include <openwbem/OW_CIMMethod.hpp>
#include <openwbem/OW_CIMParameter.hpp>

#include <openwbem/OW_Exception.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

OW_DECLARE_EXCEPTION(PyConversion);

class OWPyConv
{
public:
	static Py::Object OWInst2Py(const CIMInstance& ci, const String& ns=String());
	static Py::Object OWRef2Py(const CIMObjectPath& cop);
	static Py::Object OWVal2Py(const CIMValue& val);	

	static Py::Object OWClass2Py(const CIMClass& cls);
	static Py::Object OWProperty2Py(const CIMProperty& prop);
	static Py::Object OWQual2Py(const CIMQualifier& qual);
	static Py::Object OWQualType2Py(const CIMQualifierType& qualt);

	static Py::Object OWCIMParam2Py(const CIMParameter& param);
	static Py::Object OWMeth2Py(const CIMMethod& meth);
	static String OWDataType2Py(CIMDataType::Type dt);

	static CIMInstance PyInst2OW(const Py::Object& pyci, const String& ns=String());
	static CIMObjectPath PyRef2OW(const Py::Object& pycop, const String& ns=String());
	static CIMValue PyVal2OW(const String& type, const Py::Object& pyval);
	static CIMValue PyVal2OW(const Py::Tuple& tuple);

	static CIMClass PyClass2OW(const Py::Object& pycls);
	static CIMProperty PyProperty2OW(const Py::Object& pyprop);
	static CIMQualifier PyQual2OW(const Py::Object& pyqual);
	static CIMQualifierType PyQualType2OW(const Py::Object& pyqualt);
	static CIMParameter PyCIMParam2OW(const Py::Object& pyparam);
	static CIMMethod PyMeth2OW(const Py::Object& pymeth);
	static CIMDataType::Type PyDataType2OW(const String& strt);

	static void setPyWbemMod(const Py::Module& mod);

private:
	static Py::Object RefValOW2Py(const CIMValue& owval);
	static Py::Object DateTimeValOW2Py(const CIMValue& owval);
};

}	// End of namespace PythonProvIFC

#endif	// OW_PYCONVERTER_HPP_GUARD

