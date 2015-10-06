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
#include "pycxx_iter.hpp"
#include "Objects.hpp"

void IterT::init_type()
{
    behaviors().name("IterT");
    behaviors().doc("IterT(ini_count)");
    // you must have overwritten the virtual functions
    // Py::Object iter() and Py::Object iternext()
    behaviors().supportIter();    // set entries in the Type Table
    behaviors().supportRepr();
    add_varargs_method("reversed",&IterT::reversed,"reversed()");
}

class MyIterModule : public Py::ExtensionModule<MyIterModule>
{

public:
    MyIterModule() : Py::ExtensionModule<MyIterModule>("pycxx_iter")
    {
        IterT::init_type();
        add_varargs_method("IterT",&MyIterModule::new_IterT,"IterT(from,last)");
        initialize("MyIterModule documentation"); // register with Python
    }
    
    virtual ~MyIterModule() {}

private:
    Py::Object new_IterT(const Py::Tuple& args)
    {
        if (args.length() != 2)
        {
            throw Py::RuntimeError("Incorrect # of args to IterT(from,to).");
        }
        return Py::asObject(new IterT(Py::Int(args[0]),Py::Int(args[1])));
    }
};

extern "C" void initpycxx_iter()
{
    // the following constructor call registers our extension module
    // with the Python runtime system
    static MyIterModule* IterTest = new MyIterModule;
}
