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
#include "PyCxxObjects.hpp"

#include "OW_PyConverter.hpp"

#include <iostream>
#include <openwbem/OW_CIMProperty.hpp>
#include <openwbem/OW_CIMQualifier.hpp>
#include <openwbem/OW_Bool.hpp>
#include <openwbem/OW_DateTime.hpp>
#include <openwbem/OW_CIMDateTime.hpp>
#include <openwbem/OW_Format.hpp>

using namespace OpenWBEM; 
using namespace PythonProvIFC;
using std::cout;
using std::cerr;
using std::endl;

//////////////////////////////////////////////////////////////////////////////
CIMInstance
makeTestInstance()
{
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
	return inst;
}

//////////////////////////////////////////////////////////////////////////////
void
runTest(char** argv)
{
	try
	{
		cout << "Importing module pywbem" << endl;
		Py::Module pywbemMod("pywbem", true);
		OWPyConv::setPyWbemMod(pywbemMod);
		cout << "Imported module pywbem" << endl;

		cout << "Importing script module: " << argv[1] << " ..." << endl;
		Py::Module scriptMod(argv[1], true);
		
		cout << "Imported. Dir..." << endl;
		cout << "dir(" << argv[1] << ")" << endl;
		cout << scriptMod.dir() << endl;

		cout << "Getting function: " << argv[2] << endl;
		Py::Callable pyfunc = scriptMod.getAttr(argv[2]);

		CIMInstance ci = makeTestInstance();	// Create bogus OW Instance

		// Convert to a pywbem.CIMInstance
		cout << "Converting instance to Python object" << endl;
		Py::Object pci = OWPyConv::OWInst2Py(ci);

		// Now call script with pywbem.CIMInstance
		Py::Tuple scriptArg(1);
		scriptArg[0] = pci;
		cout << "Calling function " << argv[2] << " with instance" << endl;
		Py::Object pyInst = pyfunc.apply(scriptArg);
		cout << "Got the instance from Python!" << endl;

		cout << "Converting instance back to an OW Instance..." << endl;
		CIMInstance convci = OWPyConv::PyInst2OW(pyInst);
		cout << "Converted Instance:" << endl;
		cout << convci.toMOF() << endl;
	}
	catch(Py::Exception& e)
	{
		cout << "Caught Py::Exception" << endl;
		cout << "Value: " << Py::value(e) << endl;
		cout << "traceback: " << Py::trace(e) << endl;
		e.clear();
		return;
	}

	cout << "runTest done. returning" << endl;
}

int
main(int argc, char *argv[])
{

    if (argc != 3)
    {
		cerr << "Usage: " << argv[0] << " <.py script> <func name>" << endl;
		return 1; 
    }

    Py_Initialize(); 

	runTest(argv);

	Py_Finalize();

    return 0; 
}
