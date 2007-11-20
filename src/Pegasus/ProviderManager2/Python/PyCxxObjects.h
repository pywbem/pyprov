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
//-----------------------------------------------------------------------------
//
// Copyright (c) 1998 - 2007, The Regents of the University of California
// Produced at the Lawrence Livermore National Laboratory
// All rights reserved.
//
// This file is part of PyCXX. For details,see http://cxx.sourceforge.net/. The
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
#ifndef __PYCXXOBJECTS_HPP_GUARD
#define __PYCXXOBJECTS_HPP_GUARD

#include <Python.h>
#include "PyCxxConfig.h"
#include "PyCxxException.h"

#include <Pegasus/Common/String.h>

#include <iostream>
#include <string>
#include <cstring>
#include <iterator>
#include <utility>
#include <typeinfo>

namespace Py
{

class ThreadSaver
{
public:
	ThreadSaver()
	{
		m_tstate = PyEval_SaveThread();
	}
	~ThreadSaver()
	{
		PyEval_RestoreThread(m_tstate);
	}
private:
	PyThreadState* m_tstate;
};
#define PYCXX_ALLOW_THREADS { Py::ThreadSaver ts;
#define PYCXX_END_ALLOW_THREADS }

// The GILGuard class is used to acquire python global interpreter lock.
// It acquires the lock within its constructor and releases it in its
// destructor. Do use it just declare an instance of the GILGuard when
// you want to acquire the lock. When the instance goes out of scope,
// the lock will be released.
class GILGuard
{
public:
	GILGuard();
	~GILGuard();
	void release();
	void acquire();
private:
	PyGILState_STATE m_gstate;
	bool m_acquired;
};


typedef int sequence_index_type;    // type of an index into a sequence

// Forward declarations
class Object;
class Type;
template<typename T> class SeqBase;
class String;
class List;
template<typename T> class MapBase;

// new_reference_to also overloaded below on Object
inline PyObject* new_reference_to(PyObject* p)
{
	Py::_XINCREF(p);
	return p;
}

// returning Null() from an extension method triggers a
// Python exception
inline PyObject* Null()
{
	return (static_cast<PyObject*>(0));
}

//===========================================================================//
// class Object
// The purpose of this class is to serve as the most general kind of
// Python object, for the purpose of writing C++ extensions in Python
// Objects hold a PyObject* which they own. This pointer is always a
// valid pointer to a Python object. In children we must maintain this behavior.
//
// Instructions on how to make your own class MyType descended from Object:
// (0) Pick a base class, either Object or perhaps SeqBase<T> or MapBase<T>.
//     This example assumes Object.

// (1) Write a routine int MyType_Check (PyObject *) modeled after PyInt_Check,
//     PyFloat_Check, etc.

// (2) Add method accepts:
//     virtual bool accepts (PyObject *pyob) const {
//         return pyob && MyType_Check (pyob);
// }

// (3) Include the following constructor and copy constructor
//
/*
explicit MyType (PyObject *pyob): Object(pyob) {
validate();
}

MyType(const Object& other): Object(other.ptr()) {
validate();
}
*/

// Alernate version for the constructor to allow for construction from owned pointers:
/*
explicit MyType (PyObject *pyob): Object(pyob) {
validate();
}
*/

// You may wish to add other constructors; see the classes below for examples.
// Each constructor must use "set" to set the pointer
// and end by validating the pointer you have created.

// (4) Each class needs at least these two assignment operators:
/*
MyType& operator= (const Object& rhs) {
return (*this = *rhs);
}

Mytype& operator= (PyObject* rhsp) {
if(ptr() == rhsp) return *this;
set(rhsp);
return *this;
}
*/
// Note on accepts: constructors call the base class
// version of a virtual when calling the base class constructor,
// so the test has to be done explicitly in a descendent.

// If you are inheriting from PythonExtension<T> to define an object
// note that it contains PythonExtension<T>::check
// which you can use in accepts when writing a wrapper class.
// See Demo/range.h and Demo/range.cxx for an example.

class Object
{
private:
	// the pointer to the Python object
	// Only Object sets this directly.
	// The default constructor for Object sets it to Py_None and
	// child classes must use "set" to set it
	//
	PyObject* p;

protected:
	void set (PyObject* pyob, bool owned = false);
	void validate();
public:
	// Constructor acquires new ownership of pointer unless explicitly told not to.
	explicit Object(PyObject* pyob=Py::_None(), bool owned = false);
	// Copy constructor acquires new ownership of pointer
	Object(const Object& ob);
	// Destructor
	virtual ~Object();
	void release();
	// Assignment acquires new ownership of pointer
	Object& operator= (const Object& rhs);
	Object& operator= (PyObject* rhsp);
	// Loaning the pointer to others, retain ownership
	PyObject* operator* () const;
	// Explicit reference_counting changes
	void increment_reference_count();
	void decrement_reference_count();
	// Would like to call this pointer() but messes up STL in SeqBase<T>
	PyObject* ptr () const;
	// Can pyob be used in this object's constructor?
	virtual bool accepts (PyObject *pyob) const;
	Py_ssize_t reference_count () const;
	Type type () const; // the type object associated with this one
	String str () const; // the str() representation
	Pegasus::String as_string() const;
	String repr () const; // the repr () representation
	List dir () const; // the dir() list
	bool hasAttr (const char* s) const;
	bool hasAttr(const Pegasus::String& s) const;
	Object getAttr(const char* s) const;
	Object getAttr (const Pegasus::String& s) const;
	Object getItem (const Object& key) const;
	long hashValue () const;
	bool is(PyObject *pother) const;
	bool is(const Object& other) const;
	bool isNone() const;
	bool isCallable () const;
	bool isInstanceOf(const Object& classObj) const;
	bool isInstance () const;
	bool isBool() const;
	bool isLong() const;
	bool isInt() const;
	bool isFloat() const;
	bool isDict () const;
	bool isList () const;
	bool isMapping () const;
	bool isNumeric () const;
	bool isSequence () const;
	bool isTrue () const;
	bool isType (const Type& t) const;
	bool isTuple() const;
	bool isString() const;
	bool isUnicode() const;
	// Commands
	void setAttr (const char* s, const Object& value);
	void setAttr (const Pegasus::String& s, const Object& value);
	void delAttr(const char* s);
	void delAttr (const Pegasus::String& s);
	void delItem (const Object& key);
	// Equality and comparison use PyObject_Compare
	bool operator==(const Object& o2) const;
	bool operator!=(const Object& o2) const;
	bool operator>=(const Object& o2) const;
	bool operator<=(const Object& o2) const;
	bool operator<(const Object& o2) const;
	bool operator>(const Object& o2) const;
};

// Convert a Python pointer to a CXX Object assuming
// the reference is already owned.
inline Object asObject(PyObject *p)
{
	return Object(p, true);
}

// End of class Object
PyObject* new_reference_to(const Object& g);
// Nothing() is what an extension method returns if
// there is no other return value.
Object Nothing();
// Python special None value
Object None();
std::ostream& operator<< (std::ostream& os, const Object& ob);

// Class Type
class Type: public Object
{
public:
	explicit Type (PyObject* pyob, bool owned = false);
	Type (const Object& ob);
	Type(const Type& t);
	Type& operator= (const Object& rhs);
	Type& operator= (PyObject* rhsp);
	virtual bool accepts (PyObject *pyob) const;
};

// ===============================================
// class Bool
class Bool: public Object
{
public:
	// Constructor
	Bool (PyObject *pyob, bool owned = false);
	Bool (const Bool& ob);
	// create from long
	Bool (long v = 0L);
	// create from int
	Bool (int v);
	// create from bool
	Bool (bool v);
	explicit Bool (const Object& ob);
#ifdef HAVE_LONG_LONG
	// create from long long
	Bool (PY_LONG_LONG v);
	// create from unsigned long long
	Bool (unsigned PY_LONG_LONG v);
	// convert to long long
	long long asLongLong() const;
	// convert to unsigned long long
	unsigned long long asUnsignedLongLong() const;
	// assign from long long
	Bool& operator= (PY_LONG_LONG v);
	// assign from unsigned long long
	Bool& operator= (unsigned PY_LONG_LONG v);
#endif
	// Assignment acquires new ownership of pointer
	Bool& operator= (const Object& rhs);
	Bool& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	long asLong() const;
	unsigned long asUnsignedLong() const;
	// convert to long
	operator long() const;
	// assign from an int
	Bool& operator= (int v);
	// assign from long
	Bool& operator= (long v);
};

// ===============================================
// class Int
class Int: public Object
{
public:
	// Constructor
	Int (PyObject *pyob, bool owned = false);
	Int (const Int& ob);
	// create from long
	Int (long v = 0L);
	// create from int
	Int (int v);
	// create from bool
	Int (bool v);
	explicit Int (const Object& ob);
#ifdef HAVE_LONG_LONG
	// create from long long
	Int (PY_LONG_LONG v);
	// create from unsigned long long
	Int (unsigned PY_LONG_LONG v);
	// convert to long long
	long long asLongLong() const;
	// convert to unsigned long long
	unsigned long long asUnsignedLongLong() const;
	// assign from long long
	Int& operator= (PY_LONG_LONG v);
	// assign from unsigned long long
	Int& operator= (unsigned PY_LONG_LONG v);
#endif
	// Assignment acquires new ownership of pointer
	Int& operator= (const Object& rhs);
	Int& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	long asLong() const;
	unsigned long asUnsignedLong() const;
	// convert to long
	operator long() const;
	// assign from an int
	Int& operator= (int v);
	// assign from long
	Int& operator= (long v);
};

// ===============================================
// class Long
class Long: public Object
{
public:
	// Constructor
	explicit Long (PyObject *pyob, bool owned = false);
	Long (const Long& ob);
	// create from long
	explicit Long (long v = 0L);
	// create from unsigned long
	explicit Long (unsigned long v);
	// create from int
	explicit Long (int v);
	// try to create from any object
	Long (const Object& ob);
	// Assignment acquires new ownership of pointer
	Long& operator= (const Object& rhs);
	Long& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// convert to long
	operator long() const;
	long asLong() const;

	long long asLongLong() const;
	unsigned long long asUnsignedLongLong() const;

	// convert to unsigned
	unsigned long asUnsignedLong() const;
	operator unsigned long() const;
	// convert to double
	operator double() const;
	// assign from an int
	Long& operator= (int v);
	// assign from long
	Long& operator= (long v);
	// assign from unsigned long
	Long& operator= (unsigned long v);
};

#ifdef HAVE_LONG_LONG
// ===============================================
// class LongLong
class LongLong: public Object
{
public:
	// Constructor
	explicit LongLong (PyObject *pyob, bool owned = false);
	LongLong (const LongLong& ob);
	// create from long long
	explicit LongLong (long long v = 0L);
	// create from unsigned long long
	explicit LongLong (unsigned long long v);
	// create from long
	explicit LongLong (long v);
	// create from unsigned long
	explicit LongLong (unsigned long v);
	// create from int
	explicit LongLong (int v);
	// try to create from any object
	LongLong (const Object& ob);
	// Assignment acquires new ownership of pointer
	LongLong& operator= (const Object& rhs);
	LongLong& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// convert to long long
	long long asLongLong() const;
	operator long long() const;
	// convert to unsigned long long
	unsigned long long asUnsignedLongLong() const;
	operator unsigned long long() const;
	// convert to long
	operator long() const;
	// convert to unsigned
	operator unsigned long() const;
	operator double() const;
	// assign from an int
	LongLong& operator= (int v);
	// assign from long long
	LongLong& operator= (long long v);
	// assign from unsigned long long
	LongLong& operator= (unsigned long long v);
	// assign from long
	LongLong& operator= (long v);
	// assign from unsigned long
	LongLong& operator= (unsigned long v);
};
#endif

// ===============================================
// class Float
//
class Float: public Object
{
public:
	// Constructor
	explicit Float (PyObject *pyob, bool owned = false);
	Float (const Float& f);
	// make from double
	explicit Float (double v=0.0);
	// try to make from any object
	Float (const Object& ob);
	Float& operator= (const Object& rhs);
	Float& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// convert to double
	double as_double () const;
	operator double () const;
	// assign from a double
	Float& operator= (double v);
	// assign from an int
	Float& operator= (int v);
	// assign from long
	Float& operator= (long v);
	// assign from an Int
	Float& operator= (const Int& iob);
};

// ===============================================
// class Complex
class Complex: public Object
{
public:
	// Constructor
	explicit Complex (PyObject *pyob, bool owned = false);
	Complex (const Complex& f);
	// make from double
	explicit Complex (double v=0.0, double w=0.0);
	Complex& operator= (const Object& rhs);
	Complex& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// convert to Py_complex
	operator Py_complex () const;
	// assign from a Py_complex
	Complex& operator= (const Py_complex& v);
	// assign from a double
	Complex& operator= (double v);
	// assign from an int
	Complex& operator= (int v);
	// assign from long
	Complex& operator= (long v);
	// assign from an Int
	Complex& operator= (const Int& iob);
	double real() const;
	double imag() const;
};

// Sequences
// Sequences are here represented as sequences of items of type T.
// The base class SeqBase<T> represents that.
// In basic Python T is always "Object".

// seqref<T> is what you get if you get elements from a non-const SeqBase<T>.
// Note: seqref<T> could probably be a nested class in SeqBase<T> but that might stress
// some compilers needlessly. Simlarly for mapref later.

// While this class is not intended for enduser use, it needs some public
// constructors for the benefit of the STL.

// See Scott Meyer's More Essential C++ for a description of proxies.
// This application is even more complicated. We are doing an unusual thing
// in having a double proxy. If we want the STL to work
// properly we have to compromise by storing the rvalue inside. The
// entire Object API is repeated so that things like s[i].isList() will
// work properly.

// Still, once in a while a weird compiler message may occur using expressions like x[i]
// Changing them to Object(x[i]) helps the compiler to understand that the
// conversion of a seqref to an Object is wanted.

template<typename T>
class seqref
{
protected:
	SeqBase<T>& s; // the sequence
	int offset; // item number
	T the_item; // lvalue
public:

	seqref (SeqBase<T>& seq, sequence_index_type j)
		: s(seq), offset(j), the_item (s.getItem(j))
	{}

	seqref (const seqref<T>& range)
		: s(range.s), offset(range.offset), the_item(range.the_item)
	{}

	// TMM: added this seqref ctor for use with STL algorithms
	seqref (Object& obj)
		: s(dynamic_cast< SeqBase<T>&>(obj))
		, offset( NULL )
		, the_item(s.getItem(offset))
	{}
	~seqref()
	{}

	operator T() const
	{ // rvalue
		return the_item;
	}

	seqref<T>& operator=(const seqref<T>& rhs)
	{ //used as lvalue
		the_item = rhs.the_item;
		s.setItem(offset, the_item);
		return *this;
	}

	seqref<T>& operator=(const T& ob)
	{ // used as lvalue
		the_item = ob;
		s.setItem(offset, ob);
		return *this;
	}

	// forward everything else to the item
	PyObject* ptr () const
	{
		return the_item.ptr();
	}

	int reference_count () const
	{ // the reference count
		return the_item.reference_count();
	}

	Type type () const
	{
		return the_item.type();
	}

	String str () const;

	String repr () const;

	bool hasAttr(const char* attr_name) const
	{
		return the_item.hasAttr(attr_name);
	}
	bool hasAttr(const Pegasus::String& attr_name) const
	{
		return hasAttr((const char*)attr_name.getCString());
	}

	Object getAttr(const char* attr_name) const
	{
		return the_item.getAttr(attr_name);
	}
	Object getAttr(const Pegasus::String& attr_name) const
	{
		return getAttr((const char*)attr_name.getCString());
	}

	Object getItem (const Object& key) const
	{
		return the_item.getItem(key);
	}

	long hashValue () const
	{
		return the_item.hashValue();
	}

	bool isCallable () const
	{
		return the_item.isCallable();
	}

	bool isInstance () const
	{
		return the_item.isInstance();
	}

	bool isDict () const
	{
		return the_item.isDict();
	}

	bool isList () const
	{
		return the_item.isList();
	}

	bool isMapping () const
	{
		return the_item.isMapping();
	}

	bool isNumeric () const
	{
		return the_item.isNumeric();
	}

	bool isSequence () const
	{
		return the_item.isSequence();
	}

	bool isTrue () const
	{
		return the_item.isTrue();
	}

	bool isType (const Type& t) const
	{
		return the_item.isType (t);
	}

	bool isTuple() const
	{
		return the_item.isTuple();
	}

	bool isString() const
	{
		return the_item.isString();
	}
	// Commands
	void setAttr (const char* attr_name, const Object& value)
	{
		the_item.setAttr(attr_name, value);
	}
	void setAttr (const Pegasus::String& attr_name, const Object& value)
	{
		setAttr((const char*)attr_name.getCString(), value);
	}

	void delAttr (const char* attr_name)
	{
		the_item.delAttr(attr_name);
	}
	void delAttr (const Pegasus::String& attr_name)
	{
		delAttr((const char*)attr_name.getCString());
	}

	void delItem (const Object& key)
	{
		the_item.delItem(key);
	}

	bool operator==(const Object& o2) const
	{
		return the_item == o2;
	}

	bool operator!=(const Object& o2) const
	{
		return the_item != o2;
	}

	bool operator>=(const Object& o2) const
	{
		return the_item >= o2;
	}

	bool operator<=(const Object& o2) const
	{
		return the_item <= o2;
	}

	bool operator<(const Object& o2) const
	{
		return the_item < o2;
	}

	bool operator>(const Object& o2) const
	{
		return the_item > o2;
	}
}; // end of seqref


// class SeqBase<T>
// ...the base class for all sequence types

template<typename T>
class SeqBase: public Object
{
public:
	// STL definitions
	typedef size_t size_type;
	typedef seqref<T> reference;
	typedef T const_reference;
	typedef seqref<T>* pointer;
	typedef int difference_type;
	typedef T value_type;        // TMM: 26Jun'01

	virtual size_type max_size() const
	{
		return std::string::npos; // ?
	}

	virtual size_type capacity() const
	{
		return size();
	}

	virtual void swap(SeqBase<T>& c)
	{
		SeqBase<T> temp = c;
		c = ptr();
		set(temp.ptr());
	}

	virtual size_type size () const
	{
		return PySequence_Length (ptr());
	}

	explicit SeqBase<T> ()
		:Object(PyTuple_New(0), true)
	{
		validate();
	}

	explicit SeqBase<T> (PyObject* pyob, bool owned=false)
		: Object(pyob, owned)
	{
		validate();
	}

	SeqBase<T> (const Object& ob): Object(ob)
	{
		validate();
	}

	// Assignment acquires new ownership of pointer

	SeqBase<T>& operator= (const Object& rhs)
	{
		return (*this = *rhs);
	}

	SeqBase<T>& operator= (PyObject* rhsp)
	{
		if(ptr() == rhsp) return *this;
		set (rhsp);
		return *this;
	}

	virtual bool accepts (PyObject *pyob) const
	{
		return pyob && PySequence_Check (pyob);
	}

	size_type length () const
	{
		return PySequence_Length (ptr());
	}

	// Element access
	const T operator[](sequence_index_type index) const
	{
		return getItem(index);
	}

	seqref<T> operator[](sequence_index_type index)
	{
		return seqref<T>(*this, index);
	}

	virtual T getItem (sequence_index_type i) const
	{
		return T(asObject(PySequence_GetItem (ptr(), i)));
	}

	virtual void setItem (sequence_index_type i, const T& ob)
	{
		if (PySequence_SetItem (ptr(), i, *ob) == -1)
		{
			throw Exception();
		}
	}

	SeqBase<T> repeat (int count) const
	{
		return SeqBase<T> (PySequence_Repeat (ptr(), count), true);
	}

	SeqBase<T> concat (const SeqBase<T>& other) const
	{
		return SeqBase<T> (PySequence_Concat(ptr(), *other), true);
	}

	// more STL compatability
	const T front () const
	{
		return getItem(0);
	}

	seqref<T> front()
	{
		return seqref<T>(this, 0);
	}

	const T back () const
	{
		return getItem(size()-1);
	}

	seqref<T> back()
	{
		return seqref<T>(this, size()-1);
	}

	void verify_length(size_type required_size) const
	{
		if (size() != required_size)
		throw IndexError ("Unexpected SeqBase<T> length.");
	}

	void verify_length(size_type min_size, size_type max_size) const
	{
		size_type n = size();
		if (n < min_size || n > max_size)
		throw IndexError ("Unexpected SeqBase<T> length.");
	}

	class iterator
		: public random_access_iterator_parent(seqref<T>)
	{
	protected:
		friend class SeqBase<T>;
		SeqBase<T>* seq;
		int count;

	public:
		~iterator ()
		{}

		iterator ()
			: seq( 0 )
			, count( 0 )
		{}

		iterator (SeqBase<T>* s, int where)
			: seq( s )
			, count( where )
		{}

		iterator (const iterator& other)
			: seq( other.seq )
			, count( other.count )
		{}

		bool eql (const iterator& other) const
		{
			return (*seq == *other.seq) && (count == other.count);
		}

		bool neq (const iterator& other) const
		{
			return (*seq != *other.seq) || (count != other.count);
		}

		bool lss (const iterator& other) const
		{
			return (count < other.count);
		}

		bool gtr (const iterator& other) const
		{
			return (count > other.count);
		}

		bool leq (const iterator& other) const
		{
			return (count <= other.count);
		}

		bool geq (const iterator& other) const
		{
			return (count >= other.count);
		}

		seqref<T> operator*()
		{
			return seqref<T>(*seq, count);
		}

		seqref<T> operator[] (sequence_index_type i)
		{
			return seqref<T>(*seq, count + i);
		}

		iterator& operator=(const iterator& other)
		{
			if (this == &other) return *this;
			seq = other.seq;
			count = other.count;
			return *this;
		}

		iterator operator+(int n) const
		{
			return iterator(seq, count + n);
		}

		iterator operator-(int n) const
		{
			return iterator(seq, count - n);
		}

		iterator& operator+=(int n)
		{
			count = count + n;
			return *this;
		}

		iterator& operator-=(int n)
		{
			count = count - n;
			return *this;
		}

		int operator-(const iterator& other) const
		{
			if (*seq != *other.seq)
			throw RuntimeError ("SeqBase<T>::iterator comparison error");
			return count - other.count;
		}

		// prefix ++
		iterator& operator++ ()
		{ count++; return *this;}
		// postfix ++
		iterator operator++ (int)
		{ return iterator(seq, count++);}
		// prefix --
		iterator& operator-- ()
		{ count--; return *this;}
		// postfix --
		iterator operator-- (int)
			{ return iterator(seq, count--);}

		Pegasus::String diagnose() const
		{
			std::iostream ios;
			ios << "iterator diagnosis " << seq << ", " << count;
			return Pegasus::String((char *)ios);
		}
	};    // end of class SeqBase<T>::iterator

	iterator begin ()
	{
		return iterator(this, 0);
	}

	iterator end ()
	{
		return iterator(this, length());
	}

	class const_iterator
		: public random_access_iterator_parent(const Object)
	{
	protected:
		friend class SeqBase<T>;
		const SeqBase<T>* seq;
		sequence_index_type count;

	public:
		~const_iterator ()
		{}

		const_iterator ()
			: seq( 0 )
			, count( 0 )
		{}

		const_iterator (const SeqBase<T>* s, int where)
			: seq( s )
			, count( where )
		{}

		const_iterator(const const_iterator& other)
			: seq( other.seq )
			, count( other.count )
		{}

		const T operator*() const
		{
			return seq->getItem(count);
		}

		const T operator[] (sequence_index_type i) const
		{
			return seq->getItem(count + i);
		}

		const_iterator& operator=(const const_iterator& other)
		{
			if (this == &other) return *this;
			seq = other.seq;
			count = other.count;
			return *this;
		}

		const_iterator operator+(int n) const
		{
			return const_iterator(seq, count + n);
		}

		bool eql (const const_iterator& other) const
		{
			return (*seq == *other.seq) && (count == other.count);
		}

		bool neq (const const_iterator& other) const
		{
			return (*seq != *other.seq) || (count != other.count);
		}

		bool lss (const const_iterator& other) const
		{
			return (count < other.count);
		}

		bool gtr (const const_iterator& other) const
		{
			return (count > other.count);
		}

		bool leq (const const_iterator& other) const
		{
			return (count <= other.count);
		}

		bool geq (const const_iterator& other) const
		{
			return (count >= other.count);
		}

		const_iterator operator-(int n)
		{
			return const_iterator(seq, count - n);
		}

		const_iterator& operator+=(int n)
		{
			count = count + n;
			return *this;
		}

		const_iterator& operator-=(int n)
		{
			count = count - n;
			return *this;
		}

		int operator-(const const_iterator& other) const
		{
			if (*seq != *other.seq)
			throw RuntimeError ("SeqBase<T>::const_iterator::- error");
			return count - other.count;
		}
		// prefix ++
		const_iterator& operator++ ()
		{ count++; return *this;}
		// postfix ++
		const_iterator operator++ (int)
		{ return const_iterator(seq, count++);}
		// prefix --
		const_iterator& operator-- ()
		{ count--; return *this;}
		// postfix --
		const_iterator operator-- (int)
		{ return const_iterator(seq, count--);}
	};    // end of class SeqBase<T>::const_iterator

	const_iterator begin () const
	{
		return const_iterator(this, 0);
	}

	const_iterator end () const
	{
		return const_iterator(this, length());
	}
};

// Here's an important typedef you might miss if reading too fast...
typedef SeqBase<Object> Sequence;

template <typename T> bool operator==(const typename SeqBase<T>::iterator& left,
		const typename SeqBase<T>::iterator& right);
template <typename T> bool operator!=(const typename SeqBase<T>::iterator& left,
		const typename SeqBase<T>::iterator& right);
template <typename T> bool operator< (const typename SeqBase<T>::iterator& left,
		const typename SeqBase<T>::iterator& right);
template <typename T> bool operator> (const typename SeqBase<T>::iterator& left,
		const typename SeqBase<T>::iterator& right);
template <typename T> bool operator<=(const typename SeqBase<T>::iterator& left,
		const typename SeqBase<T>::iterator& right);
template <typename T> bool operator>=(const typename SeqBase<T>::iterator& left,
		const typename SeqBase<T>::iterator& right);

template <typename T> bool operator==(const typename SeqBase<T>::const_iterator& left,
		const typename SeqBase<T>::const_iterator& right);
template <typename T> bool operator!=(const typename SeqBase<T>::const_iterator& left,
		const typename SeqBase<T>::const_iterator& right);
template <typename T> bool operator< (const typename SeqBase<T>::const_iterator& left,
		const typename SeqBase<T>::const_iterator& right);
template <typename T> bool operator> (const typename SeqBase<T>::const_iterator& left,
		const typename SeqBase<T>::const_iterator& right);
template <typename T> bool operator<=(const typename SeqBase<T>::const_iterator& left,
		const typename SeqBase<T>::const_iterator& right);
template <typename T> bool operator>=(const typename SeqBase<T>::const_iterator& left,
		const typename SeqBase<T>::const_iterator& right); 

extern bool operator==(const Sequence::iterator& left, const Sequence::iterator& right);
extern bool operator!=(const Sequence::iterator& left, const Sequence::iterator& right);
extern bool operator< (const Sequence::iterator& left, const Sequence::iterator& right);
extern bool operator> (const Sequence::iterator& left, const Sequence::iterator& right);
extern bool operator<=(const Sequence::iterator& left, const Sequence::iterator& right);
extern bool operator>=(const Sequence::iterator& left, const Sequence::iterator& right);

extern bool operator==(const Sequence::const_iterator& left, const Sequence::const_iterator& right);
extern bool operator!=(const Sequence::const_iterator& left, const Sequence::const_iterator& right);
extern bool operator< (const Sequence::const_iterator& left, const Sequence::const_iterator& right);
extern bool operator> (const Sequence::const_iterator& left, const Sequence::const_iterator& right);
extern bool operator<=(const Sequence::const_iterator& left, const Sequence::const_iterator& right);
extern bool operator>=(const Sequence::const_iterator& left, const Sequence::const_iterator& right); 

// ==================================================
// class Char
// Python strings return strings as individual elements.
// I'll try having a class Char which is a String of length 1
//
typedef std::basic_string<Py_UNICODE> unicodestring;
extern Py_UNICODE unicode_null_string[1];

class Char: public Object
{
public:
	explicit Char (PyObject *pyob, bool owned = false);
	Char (const Object& ob);
	Char (const char* v = "");
	Char (const Pegasus::String& v);
	Char (char v);
	Char (Py_UNICODE v);
	// Assignment acquires new ownership of pointer
	Char& operator= (const Object& rhs);
	Char& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// Assignment from C string
	Char& operator= (const char* v);
	Char& operator= (const Pegasus::String& v);
	Char& operator= (char v);
	Char& operator= (const unicodestring& v);
	Char& operator= (Py_UNICODE v);
	// Conversion
	operator String() const;
	operator Pegasus::String () const;
};

class String: public SeqBase<Char>
{
public:
	explicit String (PyObject *pyob, bool owned = false);
	String (const Object& ob);
	String();
	String(const Pegasus::String& v);
	String( const char *s, const char *encoding, const char *error="strict" );
	String( const char *s, int len, const char *encoding, const char *error="strict" );
	String( const Pegasus::String &s, const char *encoding, const char *error="strict" );
	String( const Pegasus::String& v, size_t vsize );
	String( const char *v, int vsize );
	String( const char* v );
	virtual size_type capacity() const;
	// Assignment acquires new ownership of pointer
	String& operator= ( const Object& rhs );
	String& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// Assignment from C string
	String& operator= (const char* v);
	String& operator= (const Pegasus::String& v);
	String& operator= (const unicodestring& v);
	// Encode
	String encode( const char *encoding, const char *error="strict" );
	String decode( const char *encoding, const char *error="strict" );
	// Queries
	virtual size_type size () const;
	Pegasus::String as_peg_string() const;
	operator Pegasus::String () const;
	unicodestring as_unicodestring() const;
};

// ==================================================
// class Tuple
class Tuple: public Sequence
{
public:
	virtual void setItem (sequence_index_type offset, const Object&ob);
	// Constructor
	explicit Tuple (PyObject *pyob, bool owned = false);
	Tuple (const Object& ob);
	// New tuple of a given size
	explicit Tuple (int size = 0);
	// Tuple from any sequence
	explicit Tuple (const Sequence& s);
	// Assignment acquires new ownership of pointer
	Tuple& operator= (const Object& rhs);
	Tuple& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	Tuple getSlice (int i, int j) const;
};

// ==================================================
// class List

class List: public Sequence
{
public:
	// Constructor
	explicit List (PyObject *pyob, bool owned = false);
	List (const Object& ob);
	// Creation at a fixed size
	List (int size = 0);
	// List from a sequence
	List (const Sequence& s);
	virtual size_type capacity() const;
	// Assignment acquires new ownership of pointer
	List& operator= (const Object& rhs);
	List& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	List getSlice (int i, int j) const;
	void setSlice (int i, int j, const Object& v);
	void append (const Object& ob);
	void insert (int i, const Object& ob);
	void sort ();
	void reverse ();
};


// Mappings
// ==================================================
template<typename T>
class mapref
{
protected:
	MapBase<T>& s; // the map
	Object key; // item key
	T the_item;

public:
	mapref<T> (MapBase<T>& map, const char* k)
		: s(map), the_item()
	{
		key = String(k);
		if(map.hasKey(key))
			the_item = map.getItem(key);
	}

	mapref<T> (MapBase<T>& map, const Pegasus::String& k)
		: s(map), the_item()
	{
		key = String(k);
		if(map.hasKey(key)) the_item = map.getItem(key);
	}

	mapref<T> (MapBase<T>& map, const Object& k)
		: s(map), key(k), the_item()
	{
		if(map.hasKey(key)) the_item = map.getItem(key);
	}

	virtual ~mapref<T>()
	{}

	// MapBase<T> stuff
	// lvalue
	mapref<T>& operator=(const mapref<T>& other)
	{
		if(this == &other) return *this;
		the_item = other.the_item;
		s.setItem(key, other.the_item);
		return *this;
	}

	mapref<T>& operator= (const T& ob)
	{
		the_item = ob;
		s.setItem (key, ob);
		return *this;
	}

	// rvalue
	operator T() const
	{
		return the_item;
	}

	// forward everything else to the_item
	PyObject* ptr () const
	{
		return the_item.ptr();
	}

	int reference_count () const
	{ // the mapref count
		return the_item.reference_count();
	}

	Type type () const
	{
		return the_item.type();
	}

	String str () const
	{
		return the_item.str();
	}

	String repr () const
	{
		return the_item.repr();
	}

	bool hasAttr (const char* attr_name) const
	{
		return the_item.hasAttr(attr_name);
	}

	bool hasAttr (const Pegasus::String& attr_name) const
	{
		return hasAttr((const char*)attr_name.getCString());
	}

	Object getAttr (const char* attr_name) const
	{
		return the_item.getAttr(attr_name);
	}

	Object getAttr (const Pegasus::String& attr_name) const
	{
		return getAttr((const char*)attr_name.getCString());
	}

	Object getItem (const Object& k) const
	{
		return the_item.getItem(k);
	}

	long hashValue () const
	{
		return the_item.hashValue();
	}

	bool isCallable () const
	{
		return the_item.isCallable();
	}

	bool isInstance () const
	{
		return the_item.isInstance();
	}

	bool isList () const
	{
		return the_item.isList();
	}

	bool isMapping () const
	{
		return the_item.isMapping();
	}

	bool isNumeric () const
	{
		return the_item.isNumeric();
	}

	bool isSequence () const
	{
		return the_item.isSequence();
	}

	bool isTrue () const
	{
		return the_item.isTrue();
	}

	bool isType (const Type& t) const
	{
		return the_item.isType (t);
	}

	bool isTuple() const
	{
		return the_item.isTuple();
	}

	bool isString() const
	{
		return the_item.isString();
	}

	// Commands
	void setAttr (const char* attr_name, const Object& value)
	{
		the_item.setAttr(attr_name, value);
	}
	void setAttr (const Pegasus::String& attr_name, const Object& value)
	{
		setAttr((const char*)attr_name.getCString(), value);
	}

	void delAttr (const char* attr_name)
	{
		the_item.delAttr(attr_name);
	}
	void delAttr (const Pegasus::String& attr_name)
	{
		delAttr((const char*)attr_name.getCString());
	}

	void delItem (const Object& k)
	{
		the_item.delItem(k);
	}
}; // end of mapref

// TMM: now for mapref<T>
template< class T >
bool operator==(const mapref<T>& left, const mapref<T>& right)
{
	return true;    // NOT completed.
}

template< class T >
bool operator!=(const mapref<T>& left, const mapref<T>& right)
{
	return true;    // not completed.
}

template<typename T>
class MapBase: public Object
{
protected:
	explicit MapBase<T>() {}

public:

	// reference: proxy class for implementing []
	// TMM: 26Jun'01 - the types
	// If you assume that Python mapping is a hash_map...
	// hash_map::value_type is not assignable, but
	// (*it).second = data must be a valid expression
	typedef size_t size_type;
	typedef Object key_type;
	typedef mapref<T> data_type;
	typedef std::pair< const T, T > value_type;
	typedef std::pair< const T, mapref<T> > reference;
	typedef const std::pair< const T, const T > const_reference;
	typedef std::pair< const T, mapref<T> > pointer;

	// Constructor
	explicit MapBase<T> (PyObject *pyob, bool owned = false): Object(pyob, owned)
	{
		validate();
	}

	// TMM: 02Jul'01 - changed MapBase<T> to Object in next line
	MapBase<T> (const Object& ob): Object(ob)
	{
		validate();
	}

	// Assignment acquires new ownership of pointer
	MapBase<T>& operator= (const Object& rhs)
	{
		return (*this = *rhs);
	}

	MapBase<T>& operator= (PyObject* rhsp)
	{
		if (ptr() != rhsp)
			set (rhsp);
		return *this;
	}
	// Membership
	virtual bool accepts (PyObject *pyob) const
	{
		return pyob && PyMapping_Check(pyob);
	}

	// Clear -- PyMapping Clear is missing
	//

	void clear ()
	{
		List k = keys();
		for(List::iterator i = k.begin(); i != k.end(); i++)
		{
			delItem(*i);
		}
	}

	virtual size_type size() const
	{
		return PyMapping_Length (ptr());
	}

	// Element Access
	T operator[](const char* key) const
	{
		return getItem(key);
	}
	T operator[](const Pegasus::String& key) const
	{
		return getItem(key);
	}

	T operator[](const Object& key) const
	{
		return getItem(key);
	}

	mapref<T> operator[](const char* key)
	{
		return mapref<T>(*this, key);
	}
	mapref<T> operator[](const Pegasus::String& key)
	{
		return mapref<T>(*this, key);
	}

	mapref<T> operator[](const Object& key)
	{
		return mapref<T>(*this, key);
	}

	int length () const
	{
		return PyMapping_Length (ptr());
	}

	bool hasKey (const char* s) const
	{
		return PyMapping_HasKeyString (ptr(),const_cast<char*>(s)) != 0;
	}
	bool hasKey (const Pegasus::String& s) const
	{
		return hasKey((const char*)s.getCString());
	}

	bool hasKey (const Object& s) const
	{
		return PyMapping_HasKey (ptr(), s.ptr()) != 0;
	}

	T getItem (const char* s) const
	{
		return T(asObject(PyMapping_GetItemString (ptr(),
			const_cast<char*>(s))));
	}
	T getItem (const Pegasus::String& s) const
	{
		return getItem((const char*)s.getCString());
	}

	T getItem (const Object& s) const
	{
		return T(asObject(PyObject_GetItem (ptr(), s.ptr())));
	}

	virtual void setItem (const char *s, const Object& ob)
	{
		if(PyMapping_SetItemString (ptr(), const_cast<char*>(s), *ob) == -1)
		{
			throw Exception();
		}
	}
	virtual void setItem (const Pegasus::String& s, const Object& ob)
	{
		setItem((const char*)s.getCString(), ob);
	}

	virtual void setItem (const Object& s, const Object& ob)
	{
		if(PyObject_SetItem (ptr(), s.ptr(), ob.ptr()) == -1)
		{
			throw Exception();
		}
	}

	void delItem (const char* s)
	{
		if(PyMapping_DelItemString(ptr(), const_cast<char*>(s)) == -1)
		{
			throw Exception();
		}
	}
	void delItem (const Pegasus::String& s)
	{
		delItem((const char*)s.getCString());
	}

	void delItem (const Object& s)
	{
		if (PyMapping_DelItem (ptr(), *s) == -1)
		{
			throw Exception();
		}
	}
	// Queries
	List keys () const
	{
		return List(PyMapping_Keys(ptr()), true);
	}

	List values () const
	{ // each returned item is a (key, value) pair
		return List(PyMapping_Values(ptr()), true);
	}

	List items () const
	{
		return List(PyMapping_Items(ptr()), true);
	}

	// iterators for MapBase<T>
	// Added by TMM: 2Jul'01 - NOT COMPLETED
	// There is still a bug.  I decided to stop, before fixing the bug, because
	// this can't be halfway efficient until Python gets built-in iterators.
	// My current solution is to iterate over the map by getting a copy of its keys
	// and iterating over that.  Not a good solution.

	// The iterator holds a MapBase<T>* rather than a MapBase<T> because that's
	// how the sequence iterator is implemented and it works.  But it does seem
	// odd to me - we are iterating over the map object, not the reference.

#if 0    // here is the test code with which I found the (still existing) bug
	typedef cxx::Dict    d_t;
	d_t    d;
	cxx::String    s1("blah");
	cxx::String    s2("gorf");
	d[ "one" ] = s1;
	d[ "two" ] = s1;
	d[ "three" ] = s2;
	d[ "four" ] = s2;

	d_t::iterator    it;
	it = d.begin();        // this (using the assignment operator) is causing
	// a problem; if I just use the copy ctor it works fine.
	for( ; it != d.end(); ++it )
	{
		d_t::value_type    vt( *it );
		cxx::String rs = vt.second.repr();
		std::string ls = rs.operator std::string();
		fprintf( stderr, "%s\n", ls );
	}
#endif // 0

	class iterator
	{
		// : public forward_iterator_parent( std::pair<const T,T> ) {
	protected:
		typedef std::forward_iterator_tag iterator_category;
		typedef std::pair< const T, T > value_type;
		typedef int difference_type;
		typedef std::pair< const T, mapref<T> >    pointer;
		typedef std::pair< const T, mapref<T> >    reference;

		friend class MapBase<T>;
		//
		MapBase<T>* map;
		List    keys;            // for iterating over the map
		List::iterator    pos;        // index into the keys

	public:
		~iterator () {}

		iterator ()
			: map( 0 )
			, keys()
			, pos() {}

		iterator (MapBase<T>* m, bool end = false )
			: map( m )
			, keys( m->keys() )
			, pos( end ? keys.end() : keys.begin() ) {}

		iterator (const iterator& other)
			: map( other.map )
			, keys( other.keys )
			, pos( other.pos ) {}

		reference operator*()
		{
			Object key = *pos;
			return std::make_pair(key, mapref<T>(*map,key));
		}

		iterator& operator=(const iterator& other)
		{
			if (this != &other)
			{
				map = other.map;
				keys = other.keys;
				pos = other.pos;
			}
			return *this;
		}

		bool eql(const iterator& right) const
		{
			return *map == *right.map && pos == right.pos;
		}
		bool neq( const iterator& right ) const
		{
			return *map != *right.map || pos != right.pos;
		}

		// prefix ++
		iterator& operator++ () { pos++; return *this;}
		// postfix ++
		iterator operator++ (int) { return iterator(map, keys, pos++);}
		// prefix --
		iterator& operator-- () { pos--; return *this;}
		// postfix --
		iterator operator-- (int) { return iterator(map, keys, pos--);}

		Pegasus::String diagnose() const
		{
			std::iostream ios;
			ios << "iterator diagnosis " << map << ", " << pos;
			return String((char *)ios);
		}
	};    // end of class MapBase<T>::iterator

	iterator begin ()
	{
		return iterator(this);
	}

	iterator end ()
	{
		return iterator(this, true);
	}

	class const_iterator
	{
	protected:
		typedef std::forward_iterator_tag iterator_category;
		typedef const std::pair< const T, T > value_type;
		typedef int difference_type;
		typedef const std::pair< const T, T > pointer;
		typedef const std::pair< const T, T > reference;

		friend class MapBase<T>;
		const MapBase<T>* map;
		List    keys;    // for iterating over the map
		List::iterator    pos;        // index into the keys

	public:
		~const_iterator ()
		{}

		const_iterator ()
			: map( 0 )
			, keys()
			, pos()
		{}

		const_iterator (const MapBase<T>* m, List k, List::iterator p )
			: map( m )
			, keys( k )
			, pos( p )
		{}

		const_iterator(const const_iterator& other)
			: map( other.map )
			, keys( other.keys )
			, pos( other.pos )
		{}

		bool eql(const const_iterator& right) const
		{
			return *map == *right.map && pos == right.pos;
		}
		bool neq( const const_iterator& right ) const
		{
			return *map != *right.map || pos != right.pos;
		}


		//            const_reference    operator*() {
		//                Object key = *pos;
		//                return std::make_pair( key, map->[key] );
		// GCC < 3 barfes on this line at the '['.
		//         }

		const_iterator& operator=(const const_iterator& other)
		{
			if (this == &other) return *this;
			map = other.map;
			keys = other.keys;
			pos = other.pos;
			return *this;
		}

		// prefix ++
		const_iterator& operator++ ()
		{ pos++; return *this;}
		// postfix ++
		const_iterator operator++ (int)
		{ return const_iterator(map, keys, pos++);}
		// prefix --
		const_iterator& operator-- ()
		{ pos--; return *this;}
		// postfix --
		const_iterator operator-- (int)
		{ return const_iterator(map, keys, pos--);}
	};    // end of class MapBase<T>::const_iterator

	const_iterator begin () const
	{
		return const_iterator(this, 0);
	}

	const_iterator end () const
	{
		return const_iterator(this, length());
	}

};    // end of MapBase<T>

typedef MapBase<Object> Mapping;

template <typename T> bool operator==(const typename MapBase<T>::iterator& left,
	const typename MapBase<T>::iterator& right);
template <typename T> bool operator!=(const typename MapBase<T>::iterator& left,
	const typename MapBase<T>::iterator& right);
template <typename T> bool operator==(const typename MapBase<T>::const_iterator& left,
	const typename MapBase<T>::const_iterator& right);
template <typename T> bool operator!=(const typename MapBase<T>::const_iterator& left,
	const typename MapBase<T>::const_iterator& right);

extern bool operator==(const Mapping::iterator& left, const Mapping::iterator& right);
extern bool operator!=(const Mapping::iterator& left, const Mapping::iterator& right);
extern bool operator==(const Mapping::const_iterator& left, const Mapping::const_iterator& right);
extern bool operator!=(const Mapping::const_iterator& left, const Mapping::const_iterator& right);


// ==================================================
// class Dict
class Dict: public Mapping
{
public:
	// Constructor
	explicit Dict (PyObject *pyob, bool owned=false);
	explicit Dict (const Dict& ob);
	Dict (const Object& ob);

	// Creation
	Dict ();
	// Assignment acquires new ownership of pointer
	Dict& operator= (const Object& rhs);
	Dict& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
};

class Callable: public Object
{
public:
	// Constructor
	explicit Callable ();
	explicit Callable (PyObject *pyob, bool owned = false);
	Callable (const Object& ob);
	// Assignment acquires new ownership of pointer
	Callable& operator= (const Object& rhs);
	Callable& operator= (PyObject* rhsp);
	// Membership
	virtual bool accepts (PyObject *pyob) const;
	// Call
	Object apply(const Tuple& args) const;
	// Call with keywords
	Object apply(const Tuple& args, const Dict& kw) const;
	Object apply(PyObject* pargs = 0) const;
};

class Module: public Object
{
private:
	void loadOrAdd(const char* s, bool doLoad);
public:
	explicit Module (PyObject* pyob=Py::_None(), bool owned = false);
	// Construct from module name
	explicit Module (const char* s, bool doLoad=false);
	explicit Module (const Pegasus::String& s, bool doLoad=false);
	// Copy constructor acquires new ownership of pointer
	Module (const Module& ob);
	Module& operator= (const Object& rhs);
	Module& operator= (PyObject* rhsp);
	Pegasus::String getFileName() const;
	Dict getDict();
};

// Numeric interface
inline Object operator+ (const Object& a)
{
	return asObject(PyNumber_Positive(*a));
}
inline Object operator- (const Object& a)
{
	return asObject(PyNumber_Negative(*a));
}

inline Object abs(const Object& a)
{
	return asObject(PyNumber_Absolute(*a));
}

std::pair<Object,Object> coerce(const Object& a, const Object& b);

inline Object operator+ (const Object& a, const Object& b)
{
	return asObject(PyNumber_Add(*a, *b));
}
inline Object operator+ (const Object& a, int j)
{
	return asObject(PyNumber_Add(*a, *Int(j)));
}
inline Object operator+ (const Object& a, double v)
{
	return asObject(PyNumber_Add(*a, *Float(v)));
}
inline Object operator+ (int j, const Object& b)
{
	return asObject(PyNumber_Add(*Int(j), *b));
}
inline Object operator+ (double v, const Object& b)
{
	return asObject(PyNumber_Add(*Float(v), *b));
}

inline Object operator- (const Object& a, const Object& b)
{
	return asObject(PyNumber_Subtract(*a, *b));
}
inline Object operator- (const Object& a, int j)
{
	return asObject(PyNumber_Subtract(*a, *Int(j)));
}
inline Object operator- (const Object& a, double v)
{
	return asObject(PyNumber_Subtract(*a, *Float(v)));
}
inline Object operator- (int j, const Object& b)
{
	return asObject(PyNumber_Subtract(*Int(j), *b));
}
inline Object operator- (double v, const Object& b)
{
	return asObject(PyNumber_Subtract(*Float(v), *b));
}

inline Object operator* (const Object& a, const Object& b)
{
	return asObject(PyNumber_Multiply(*a, *b));
}
inline Object operator* (const Object& a, int j)
{
	return asObject(PyNumber_Multiply(*a, *Int(j)));
}
inline Object operator* (const Object& a, double v)
{
	return asObject(PyNumber_Multiply(*a, *Float(v)));
}
inline Object operator* (int j, const Object& b)
{
	return asObject(PyNumber_Multiply(*Int(j), *b));
}
inline Object operator* (double v, const Object& b)
{
	return asObject(PyNumber_Multiply(*Float(v), *b));
}

inline Object operator/ (const Object& a, const Object& b)
{
	return asObject(PyNumber_Divide(*a, *b));
}
inline Object operator/ (const Object& a, int j)
{
	return asObject(PyNumber_Divide(*a, *Int(j)));
}
inline Object operator/ (const Object& a, double v)
{
	return asObject(PyNumber_Divide(*a, *Float(v)));
}
inline Object operator/ (int j, const Object& b)
{
	return asObject(PyNumber_Divide(*Int(j), *b));
}
inline Object operator/ (double v, const Object& b)
{
	return asObject(PyNumber_Divide(*Float(v), *b));
}

inline Object operator% (const Object& a, const Object& b)
{
	return asObject(PyNumber_Remainder(*a, *b));
}
inline Object operator% (const Object& a, int j)
{
	return asObject(PyNumber_Remainder(*a, *Int(j)));
}
inline Object operator% (const Object& a, double v)
{
	return asObject(PyNumber_Remainder(*a, *Float(v)));
}
inline Object operator% (int j, const Object& b)
{
	return asObject(PyNumber_Remainder(*Int(j), *b));
}
inline Object operator% (double v, const Object& b)
{
	return asObject(PyNumber_Remainder(*Float(v), *b));
}

Pegasus::String getCurrentErrorInfo(Object& etype, Object& evalue);

Object type(const Exception&);// return the type of the error
Object value(const Exception&);// return the value of the error
Object trace(const Exception&);// return the traceback of the error

template<typename T>
String seqref<T>::str () const
{
	return the_item.str();
}

template<typename T>
String seqref<T>::repr () const
{
	return the_item.repr();
}

}	// End of namespace Py

#endif	// __PYCXXOBJECTS_HPP_GUARD
