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
#include "PG_PyConverter.hpp"

#include <iostream>
#include <Pegasus/Common/CIMProperty.h>
#include <Pegasus/Common/CIMQualifier.h>
#include <Pegasus/Common/CIMDateTime.h>

// NOTE: Buffer.h and MofWriter.h were copies to /usr/include/Pegasus/Common
// and MofWriter.h was tweaked a bit before building this file
#include <Pegasus/Common/Buffer.h>
#include <Pegasus/Common/MofWriter.h>

using namespace PythonProvIFC;
using namespace Pegasus; 
using std::cout;
using std::cerr;
using std::endl;

//////////////////////////////////////////////////////////////////////////////
CIMInstance
makeTestInstance()
{
    CIMInstance inst("MyClass");

	CIMQualifier qual("Description", 
		CIMValue(String("This is a description for MyClass")));
	inst.addQualifier(qual);

	CIMProperty prop("one", CIMValue(String("ONE")));
	qual = CIMQualifier("Key",CIMValue(Boolean(true)));
	prop.addQualifier(qual);
    inst.addProperty(prop);

    inst.addProperty(CIMProperty("two", CIMValue(Sint32(2))));
    inst.addProperty(CIMProperty("minus_two", CIMValue(Sint16(-2))));
    inst.addProperty(CIMProperty("float", CIMValue(Real32(-2.12345))));
    inst.addProperty(CIMProperty("bool_true", CIMValue(Boolean(true))));
    inst.addProperty(CIMProperty("bool_false", CIMValue(Boolean(false))));
	inst.addProperty(CIMProperty("real64", CIMValue(Real64(-99883988883.9989989))));


	Array<CIMKeyBinding> kbs;
	kbs.append(CIMKeyBinding("Key1", "ValueForKey1", CIMKeyBinding::STRING));
	kbs.append(CIMKeyBinding("Key2", "1234", CIMKeyBinding::NUMERIC));
	kbs.append(CIMKeyBinding("Key3", "1234.56", CIMKeyBinding::NUMERIC));
	kbs.append(CIMKeyBinding("Key4", "TRUE", CIMKeyBinding::BOOLEAN));
	CIMObjectPath cop("", "root/cimv2", "MyFakeClass", kbs);

	// pywbem instanceNames are troublesome.  keyvals are expected to be 
	// integers, not pywbem.Uint32
	inst.addProperty(CIMProperty("RefProp", CIMValue(cop)));

    Array<Sint32> intArray; 
    intArray.append(Sint32(-1)); 
    intArray.append(Sint32(0)); 
    intArray.append(Sint32(1)); 
    intArray.append(Sint32(2)); 
    intArray.append(Sint32(3)); 
    inst.addProperty(CIMProperty("intArray", CIMValue(intArray)));

    Array<String> stringArray; 
    stringArray.append("one"); 
    stringArray.append("two"); 
    stringArray.append("three"); 
    inst.addProperty(CIMProperty("stringArray", CIMValue(stringArray)));

	Array<Boolean> boolArray;
	boolArray.append(true);
	boolArray.append(true);
	boolArray.append(false);
	boolArray.append(false);
    inst.addProperty(CIMProperty("boolArray", CIMValue(boolArray)));

	CIMDateTime cdt = CIMDateTime::getCurrentDateTime();
	inst.addProperty(CIMProperty("dateNow", CIMValue(cdt)));

	CIMDateTime idt(Uint64(123456789), true);
	inst.addProperty(CIMProperty("interval", CIMValue(idt)));

	Array<CIMDateTime> dtra;
	dtra.append(cdt);
	dtra.append(idt);
	inst.addProperty(CIMProperty("dateTimeArray", CIMValue(dtra)));
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
		PGPyConv::setPyWbemMod(pywbemMod);
		cout << "Imported module pywbem" << endl;

		cout << "Importing script module: " << argv[1] << " ..." << endl;
		Py::Module scriptMod(argv[1], true);
		
		cout << "Imported. Dir..." << endl;
		cout << "dir(" << argv[1] << ")" << endl;
		cout << scriptMod.dir() << endl;

		cout << "Getting function: " << argv[2] << endl;
		Py::Callable pyfunc = scriptMod.getAttr(argv[2]);

		CIMInstance ci = makeTestInstance();	// Create bogus PG Instance

		// Convert to a pywbem.CIMInstance
		cout << "Converting instance to Python object" << endl;
		Py::Object pci = PGPyConv::PGInst2Py(ci);

		// Now call script with pywbem.CIMInstance
		Py::Tuple scriptArg(1);
		scriptArg[0] = pci;
		cout << "Calling function " << argv[2] << " with instance" << endl;
		Py::Object pyInst = pyfunc.apply(scriptArg);
		cout << "Got the instance from Python!" << endl;

		cout << "Converting instance back to an PG Instance..." << endl;
		CIMInstance convci = PGPyConv::PyInst2PG(pyInst);
		cout << "Converted Instance:" << endl;
		Buffer bfr;
		MofWriter::appendInstanceElement(bfr, convci);
		cout << bfr.getData() << endl;
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
		cerr << "Usage: " << argv[0] << "<.py script> <func name>" << endl;
		return 1; 
    }

    Py_Initialize(); 

	runTest(argv);

	Py_Finalize();

    return 0; 
}
