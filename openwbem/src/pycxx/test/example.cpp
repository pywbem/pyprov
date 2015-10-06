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
//#define Py_TRACE_REFS 1

//-----------------------------------------------------------------------------
//
// Copyright (c) 1998 - 2007, The Regents of the University of California
// Produced at the Lawrence Livermore National Laboratory
// All rights reserved.
//
// This file is part of PyCXX. For details, see http://cxx.sourceforge.net. The
// full copyright notice is contained in the file COPYRIGHT located at the root
// of the PyCXX distribution.
//
// Redistribution  and  use  in  source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
//  - Redistributions of  source code must  retain the above  copyright notice,
//    this list of conditions and the disclaimer below.
//  - Redistributions in binary form must reproduce the above copyright notice,
//    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
//    documentation and/or materials provided with the distribution.
//  - Neither the name of the UC/LLNL nor  the names of its contributors may be
//    used to  endorse or  promote products derived from  this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
// ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
// CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
// ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
// SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
// CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
// LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
// OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
//-----------------------------------------------------------------------------

#include "Objects.hpp"
#include "Extensions.hpp"

#include <assert.h>

#include "range.hpp"  // Extension object
extern std::string test_extension_object();

#include <algorithm>

static std::string
test_String()
{
    Py::String s("hello");
    Py::Char blank = ' ';
    Py::String r1("world in brief", 5);
    s = s + blank + r1;
    s = s * 2;
    if(std::string(s) != "hello worldhello world")
    {
        return "failed (1) '" + std::string(s) + "'";
    }
    // test conversion
    std::string w = static_cast<std::string>(s);
    std::string w2 = (std::string) s;
    if(w != w2)
    {
        return "failed (2)";
    }
    Py::String r2("12345 789");
    Py::Char c6 = r2[5];
    if(c6 != blank)
    {
        std::cout << "|" << c6 << "|" << std::endl;
        return "failed (3)";
    }
    return "ok";
}

static std::string
test_numbers()
{
    // test the basic numerical classes
    Py::Int i;
    Py::Int j(2);
    Py::Int k = Py::Int(3);

    if (! (j < k)) return "failed (1)";
    if (! (j == j)) return "failed (2)" ;
    if (! (j != k)) return "failed (3)";
    if (! (j <= k)) return "failed (4)";
    if (! (k >= j)) return "failed (5)";
    if (! (k > j)) return "failed (6)";
    if (! (j <= j)) return "failed (7)";
    if (! (j >= Py::Int(2))) return "failed (8)";

    i = 2;
    Py::Float a;
    a = 3 + i; //5.0
    Py::Float b(4.0);
    a = (1.0 + 2*a + (b*3.0)/2.0 + k)/Py::Float(5); // 4.0
    i = a - 1.0; // 3
    if(i != k)
    {
        return "failed 9";
    }

    return "ok";
}

static std::string 
test_List_iterators (const Py::List& x, Py::List& y)
{
    std::vector<Py::Object> v;
    Py::Sequence::iterator j;
    int k = 0;
    for(Py::Sequence::const_iterator i = x.begin(); i != x.end(); ++i)
    {
        if ((*i).isList())
        {
            ++k;
        }
    }
    if(k!=1)
        return "failed List iterators (1)";

    k = 0;
    for(j = y.begin(); j != y.end(); ++j)
    {
        *j = Py::Int(k++);
        v.push_back (*j);
    }

    k = 0;
    for(j = y.begin(); j != y.end(); j++)
    {
        if(*j != Py::Int(k))
            return "failed List iterators (2)";
        if(v[k] != Py::Int(k))
            return "failed List iterators (3)";
        ++k;
    }
    Py::String o1("Howdy");
    Py::Int o2(1);
    int caught_it = 0;
    try
    {
        o2 = o1;
    } 
    catch (Py::Exception& e)
    {
        caught_it = 1;
        e.clear();
    }
    if(!caught_it)
        return "failed exception catch (4).";
    return "ok";
}

static Py::List
test_List_references (Py::List& x)
{
    Py::List y;
    for(Py::List::size_type i=0; i < x.length(); ++i)
    {
        if (x[i].isList())
        {
            y = x[i];
        }
    }
    return y;
}

static std::string
test_List()
{
    // test the Py::List class
    Py::List a;
    Py::List ans, aux;
    aux.append(Py::Int(3));
    aux.append(Py::Float(6.0));

    Py::Object b;
    Py::Int i(3);
    Py::Float x(6.0);
    Py::Float c(10.0), d(20.0);
    a.append(i);
    a.append(x);
    a.append(Py::Float(0.0));
    b = a[0]; 
    a[2] = b;
    a.append(c+d);
    a.append(aux);
    // a is now [3, 6.0, 3, 30.0, aux]

    ans.append(Py::Int(3));
    ans.append(Py::Float(6.0));
    ans.append(Py::Int(3));
    ans.append(Py::Float(30.0));
    ans.append(aux);

    Py::List::iterator l1, l2;
    for(l1= a.begin(), l2 = ans.begin();
    l1 != a.end() && l2 != ans.end();
    ++l1, ++l2) 
    {
        if(*l1 != *l2) return "failed 1" + a.as_string();
    }

    if (test_List_references (a)!= aux)
    {
        return "failed 2" + test_List_references(a).as_string();
    }
    return test_List_iterators(ans, a);
}

static std::string
test_Dict()
{
    // test the Dict class
    Py::Dict a,b;
    Py::List v;
    Py::String s("two");
    a["one"] = Py::Int(1);
    a[s] = Py::Int(2);
    a["three"] = Py::Int(3);
    if(Py::Int(a["one"]) != Py::Int(1))
        return "failed 1a " + a.as_string();
    if(Py::Int(a[s]) != Py::Int(2))
        return "failed 1b " + a.as_string();

    v = a.values();
#if 0
    std::sort(v.begin(), v.end());

    for(int k = 1; k < 4; ++k)
    {
        if(v[k-1] != Py::Int(k))
            return "failed 2 " + v.as_string();
    }
#endif

    b = a;
    b.clear();
    if(b.keys().length() != 0)
    {
        return "failed 3 " + b.as_string();
    }
    return "ok";
}

static std::string
test_Tuple()
{
    // test the Tuple class
    Py::Tuple a(3);
    Py::Tuple t1;
    Py::Float f1(1.0), f2(2.0), f3(3.0);
    a[0] = f1; // should be ok since no other reference owned
    a[1] = f2;
    a[2] = f3;
    Py::Tuple b(a);
    int k = 0;
    for(Py::Tuple::iterator i = b.begin(); i != b.end(); ++i)
    {
        if(*i != Py::Float(++k)) return "failed 1 " + b.as_string();
    }

    t1 = a;
    try
    {
        t1[0] = Py::Int(1); // should fail, tuple has multiple references
        return "failed 2";
    }
    catch (Py::Exception& e)
    {
        e.clear();
    }
    return "ok";
}

static std::string 
test_STL()
{
    int ans1;
    Py::List w;
    Py::List wans;
    wans.append(Py::Int(1));
    wans.append(Py::Int(1));
    wans.append(Py::Int(2));
    wans.append(Py::Int(3));
    wans.append(Py::Int(4));
    wans.append(Py::Int(5));
    w.append(Py::Int(5));
    w.append(Py::Int(1));
    w.append(Py::Int(4));
    w.append(Py::Int(2));
    w.append(Py::Int(3));
    w.append(Py::Int(1));
    ans1 = std::count(w.begin(), w.end(), Py::Float(1.0));
    if (ans1 != 2)
    {
        return "failed count test";
    }
#if 0
    std::sort(w.begin(), w.end());
    if (w != wans)
    {
        return "failed sort test";
    }
#endif

    Py::Dict    d;
    Py::String    s1("blah");
    Py::String    s2("gorf");
    d[ "one" ] = s1;
    d[ "two" ] = s1;
    d[ "three" ] = s2;
    d[ "four" ] = s2;

    Py::Dict::iterator    it( d.begin() );
//    it = d.begin();        // this (using the assignment operator) is causing
                // a problem; if I just use the copy ctor it works fine.
    for( ; it != d.end(); ++it )
    {
        Py::Dict::value_type    vt( *it );
        Py::String rs = vt.second.repr();
        std::string ls = rs.operator std::string();
        fprintf( stderr, "%s\n", ls.c_str() );
    }

    return "ok";
}


void debug_check_ref_queue()
{
#ifdef Py_TRACE_REFS

    // create an element to find the queue
    Py::Int list_element;

    PyObject *p_slow = list_element.ptr();
    PyObject *p_fast = p_slow;

    do
    {
        assert( p_slow->_ob_next->_ob_prev == p_slow );
        assert( p_slow->_ob_prev->_ob_next == p_slow );


        p_slow = p_slow->_ob_next;
        p_fast = p_slow->_ob_next->_ob_next;

        assert( p_slow != p_fast );    
    }
    while( p_slow != list_element.ptr() );

#endif
}


class example_module : public Py::ExtensionModule<example_module>
{
public:
    example_module()
        : Py::ExtensionModule<example_module>( "example" )
    {
        range::init_type();

        add_varargs_method("string", &example_module::ex_string, "string( s ) = return string");
        add_varargs_method("sum", &example_module::ex_sum, "sum(arglist) = sum of arguments");
        add_varargs_method("test", &example_module::ex_test, "test(arglist) runs a test suite");
        add_varargs_method("range", &example_module::new_r, "range(start,stop,stride)");
        add_keyword_method("kw", &example_module::ex_keyword, "kw()");

        initialize( "documentation for the example module" );

        Py::Dict d( moduleDictionary() );

        Py::Object b(Py::asObject(new range(1,10,2)));

        d["a_constant"] = b.getAttr("c");
    }

    virtual ~example_module()
    {
	}

private:

    Py::Object
	ex_keyword(
		const Py::Tuple &args,
		const Py::Dict &kws )
    {
        std::cout << "Called with " << args.length() << " normal arguments." << std::endl;
        Py::List names( kws.keys() );
        std::cout << "and with " << names.length() << " keyword arguments:" << std::endl;
        for( Py::List::size_type i=0; i< names.length(); i++ )
        {
            Py::String name( names[i] );
            std::cout << "    " << name << std::endl;
        }

        return Py::Int(0);
    }

    Py::Object new_r (const Py::Tuple &rargs)
    {
        if (rargs.length() < 2 || rargs.length() > 3)
        {
            throw Py::RuntimeError("Incorrect # of args to range(start,stop [,step]).");
        }

        Py::Int start(rargs[0]);
        Py::Int stop(rargs[1]);
        Py::Int step(1);
        if (rargs.length() == 3)
        {
            step = rargs[2];
        }
        if (long(start) > long(stop) + 1 || long(step) == 0)
        {
            throw Py::RuntimeError("Bad arguments to range(start,stop [,step]).");
        }
        return Py::asObject(new range(start, stop, step));
    }

    Py::Object ex_string (const Py::Tuple &a)
    {
        std::cout << "ex_std::string: s1 is first arg" << std::endl;
        Py::String s1( a[0] );
        std::cout << "ex_string: s1.isString() " << s1.isString() << std::endl;
        std::cout << "ex_string: s1.isUnicode() " << s1.isUnicode() << std::endl;
        std::cout << "ex_string: s1.size() " << s1.size() << std::endl;

        if( s1.isUnicode() )
    {
            std::cout << "ex_string: s2 is s1.encode( utf-8 )" << std::endl;
            Py::String s2( s1.encode( "utf-8" ) );
            std::cout << "ex_string: s2.isString() " << s2.isString() << std::endl;
            std::cout << "ex_string: s2.isUnicode() " << s2.isUnicode() << std::endl;
            std::cout << "ex_string: s2.size() " << s2.size() << std::endl;
            return s2;
    }
        else
    {
            std::cout << "ex_string: s2 is s1.decode( utf-8 )" << std::endl;
            Py::String s2( s1.decode( "utf-8" ) );
            std::cout << "ex_string: s2.isString() " << s2.isString() << std::endl;
            std::cout << "ex_string: s2.isUnicode() " << s2.isUnicode() << std::endl;
            std::cout << "ex_string: s2.size() " << s2.size() << std::endl;
            return s2;
    }
    }

    Py::Object ex_sum (const Py::Tuple &a)
    {
        // this is just to test the function verify_length:
        try
        {
            a.verify_length(0);
            std::cout << "I see that you refuse to give me any work to do." << std::endl;
        }
        catch (Py::Exception& e)
        {
            e.clear();
            std::cout << "I will now add up your elements, oh great one." << std::endl;
        }


        Py::Float f(0.0);
        for( Py::Sequence::size_type i = 0; i < a.length(); i++ )
        {    
            Py::Float g (a[i]);
            f = f + g;
        }

        return f;
    }

    Py::Object ex_test( const Py::Tuple &a) 
    {
        debug_check_ref_queue();

        std::cout << "Example Test starting" << std::endl;
        try
        {
            Py::String s("this should fail");
            std::cout << "Trying to convert a Py::String to an Py::Int" << std::endl;
            Py::Int k(s.ptr());
        }
        catch (Py::TypeError& e)
        {
            std::cout << "Correctly caught " << Py::type(e) << std::endl;
            std::cout << "  Py::Exception value: " << Py::value(e) << std::endl;
            std::cout << "  Py::Exception traceback: " << Py::trace(e) << std::endl;
            e.clear();
        }
        debug_check_ref_queue();

        std::cout << "Numbers: " << test_numbers() << std::endl;
        debug_check_ref_queue();
        std::cout << "Py::String: " << test_String() << std::endl;
        debug_check_ref_queue();
        std::cout << "Py::List: " << test_List() << std::endl;
        debug_check_ref_queue();
        std::cout << "Py::Dict: " << test_Dict() << std::endl;
        debug_check_ref_queue();
        std::cout << "Py::Tuple: " << test_Tuple() << std::endl;
        debug_check_ref_queue();
        std::cout << "STL test: " << test_STL() << std::endl;
        debug_check_ref_queue();
        std::cout << "Extension object test: " << test_extension_object() << std::endl;
        debug_check_ref_queue();

        Py::List b(a);
        Py::Tuple c(b);
        if( c != a)
        {
            std::cout << "Py::Tuple/list conversion failed.\n";
        }

        Py::Module m("sys");
        Py::Object s = m.getAttr("stdout");
        Py::Object nun;
        nun = PyObject_CallMethod(s.ptr(), "write", "s", "Module test ok.\n");
        return Py::None();
    }
};

extern "C" void initexample()
{
    static example_module* example = new example_module;
}

// symbol required for the debug version
extern "C" void initexample_d()
{ 
	initexample(); 
}
