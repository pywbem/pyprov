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
#include <iostream>
#include <cassert>
#include <openwbem/OW_CIMInstance.hpp>
#include <openwbem/OW_CIMProperty.hpp>
#include <openwbem/OW_CIMQualifier.hpp>
#include <openwbem/OW_CIMValue.hpp>
#include <openwbem/OW_Bool.hpp>
#include <openwbem/OW_CIMObjectPath.hpp>
#include <openwbem/OW_DateTime.hpp>
#include <openwbem/OW_CIMDateTime.hpp>

#include "converter.hpp"

using namespace std; 
using namespace OpenWBEM; 

typedef Array<PyObject*> PyObjectPtrArray;

void
runTest(char** argv)
{
	DecRefs dr;
	PyObject* pModuleScript = PyImport_ImportModule(argv[1]);
	if (!pModuleScript)
	{
		cerr << "Failed to import module: " << argv[1] << " pyerr: ";
		PyErr_Print();
		return;
	}
	dr += pModuleScript;

    PyObject* pModulePyWBEM = PyImport_ImportModule("pywbem");
	if (!pModulePyWBEM)
	{
		cerr << "Failed to get pywbem module. pyerr: ";
		PyErr_Print();
		return;
	}
	dr += pModulePyWBEM;
    OW_Py_Converter::setModulePyWBEM(pModulePyWBEM); 



    CIMInstance inst("MyClass");

	CIMQualifier qual(CIMQualifier::CIM_QUAL_DESCRIPTION);
	qual.setValue(CIMValue("This is a description for MyClass"));
	inst.setQualifier(qual);

	CIMProperty prop("one", CIMValue(String("ONE")));
	qual = CIMQualifier(CIMQualifier::CIM_QUAL_KEY);
	qual.setValue(CIMValue(Bool(true)));
	prop.setQualifier(qual);

    inst.setProperty(prop);

    inst.setProperty("two", CIMValue(Int32(2))); 
    inst.setProperty("minus_two", CIMValue(Int16(-2))); 
    inst.setProperty("float", CIMValue(Real32(-2.12345))); 
    inst.setProperty("bool_true", CIMValue(Bool(true))); 
    inst.setProperty("bool_false", CIMValue(Bool(false))); 
	inst.setProperty("real64", CIMValue(Real64(-99883988883.9989989)));

	CIMObjectPath cop("MyFakeClass", "root/cimv2");
	cop.setKeyValue("Key1", CIMValue("ValueForKey1"));
	cop.setKeyValue("Key2", CIMValue(UInt32(1234)));
	cop.setKeyValue("Key3", CIMValue(Real32(1234.56)));
	cop.setKeyValue("Key4", CIMValue(Bool(true)));
	// pywbem instanceNames are troublesome.  keyvals are expected to be 
	// integers, not pywbem.Uint32
	inst.setProperty("RefProp", CIMValue(cop));

    Array<Int32> intArray; 
    intArray.push_back(Int32(-1)); 
    intArray.push_back(Int32(0)); 
    intArray.push_back(Int32(1)); 
    intArray.push_back(Int32(2)); 
    intArray.push_back(Int32(3)); 
    inst.setProperty("intArray", CIMValue(intArray)); 

    StringArray stringArray; 
    stringArray.push_back("one"); 
    stringArray.push_back("two"); 
    stringArray.push_back("three"); 
    inst.setProperty("stringArray", CIMValue(stringArray)); 

	BoolArray boolArray;
	boolArray.append(true);
	boolArray.append(true);
	boolArray.append(false);
	boolArray.append(false);
    inst.setProperty("boolArray", CIMValue(boolArray)); 

	DateTime dt;
	dt.setToCurrent();
	CIMDateTime cdt(dt);
	inst.setProperty("dateNow", CIMValue(cdt));

	CIMDateTime idt(UInt64(123456789));
	inst.setProperty("interval", CIMValue(idt));

	CIMDateTimeArray dtra;
	dtra.append(cdt);
	dtra.append(idt);
	inst.setProperty("dateTimeArray", CIMValue(dtra));

	PyObject* pInst = OW_Py_Converter::CIMInstance_OW2Py(inst); 
	dr += pInst;
	CIMInstance newInst = OW_Py_Converter::CIMInstance_Py2OW(pInst); 
	cout << inst.toMOF() << endl;
	cout << newInst.toMOF() << endl;
	

	//CIMInstance newOWInst = OW_Py_Converter::CIMInstance_Py2OW(pInst); 

    PyObject_Print(pInst, stderr, Py_PRINT_RAW); 

    PyObject* pFunc = PyObject_GetAttrString(pModuleScript, argv[2]);
	if (!pFunc)
	{
		cerr << "Failed getting function "
			<< argv[2] << " in module " << argv[1] << endl;
		PyErr_Print();
		return;
	}
	dr += pFunc;

	PyObject* pValue = PyObject_CallFunction(pFunc, "(O)", pInst);
	if (!pValue)
	{
		cerr << "Failed to call function "
			<< argv[2] << " in module " << argv[1] << endl;
		PyErr_Print();
		return;
	}
	dr += pValue;

    return;
}

int
main(int argc, char *argv[])
{

    if (argc != 3)
    {
		cerr << "Usage: " << argv[0] << "<.py script> <func name>" << endl;
		return 1; 
    }

    Py_Initialize(); 

	runTest(argv);

	Py_Finalize();

    return 0; 
}
