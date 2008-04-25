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
#ifndef PG_PYCONVERTER_HPP_GUARD
#define PG_PYCONVERTER_HPP_GUARD

#include "PyCxxObjects.h"
#include <Pegasus/Common/CIMValue.h>
#include <Pegasus/Common/CIMInstance.h>
#include <Pegasus/Common/CIMObjectPath.h>
#include <Pegasus/Common/CIMQualifier.h>
#include <Pegasus/Common/CIMQualifierDecl.h>
#include <Pegasus/Common/CIMClass.h>
#include <Pegasus/Common/CIMMethod.h>
#include <Pegasus/Common/CIMParameter.h>

#include <Pegasus/Common/Exception.h>

using namespace Pegasus;

namespace PythonProvIFC
{

class PyConversionException : public Exception
{
public:
	PyConversionException();
	PyConversionException(const String& msg);
	PyConversionException(const String& file, int lineno, const String& msg);
};

class PGPyConv
{
public:
	static Py::Object PGInst2Py(const CIMConstInstance& ci, const String& ns=String());
	static Py::Object PGRef2Py(const CIMObjectPath& cop);
	static Py::Object PGVal2Py(const CIMValue& val);	

	static Py::Object PGClass2Py(const CIMConstClass& cls);
	static Py::Object PGProperty2Py(const CIMConstProperty& prop);
	static Py::Object PGQual2Py(const CIMConstQualifier& qual);
	static Py::Object PGQualType2Py(const CIMConstQualifierDecl& qualt);

	static Py::Object PGCIMParam2Py(const CIMConstParameter& param);
	static Py::Object PGMeth2Py(const CIMConstMethod& meth);
	static String PGDataType2Py(CIMType dt);


	static CIMInstance PyInst2PG(const Py::Object& pyci, const String& ns=String());
	static CIMObjectPath PyRef2PG(const Py::Object& pycop, const String& ns=String());
	static CIMValue PyVal2PG(const String& type, const Py::Object& pyval);
	static CIMValue PyVal2PG(const Py::Tuple& tuple);

	static CIMClass PyClass2PG(const Py::Object& pycls);
	static CIMProperty PyProperty2PG(const Py::Object& pyprop);
	static CIMQualifier PyQual2PG(const Py::Object& pyqual);
	static CIMQualifierDecl PyQualType2PG(const Py::Object& pyqualt);
	static CIMParameter PyCIMParam2PG(const Py::Object& pyparam);
	static CIMMethod PyMeth2PG(const Py::Object& pymeth);
	static CIMType PyDataType2PG(const String& strt);

	static void setPyWbemMod(const Py::Module& mod);

private:
	static Py::Object RefValPG2Py(const CIMValue& owval);
	static Py::Object DateTimeValPG2Py(const CIMValue& owval);
};

}	// End of namespace PythonProvIFC

#endif	// PG_PYCONVERTER_HPP_GUARD

