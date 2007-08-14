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

namespace Py
{

// Constructor acquires new ownership of pointer unless explicitly told not to.
Object::Object(PyObject* pyob, bool owned)
	: p(pyob)
{
	if(!owned)
		_XINCREF (p);
	validate();
}

// Copy constructor acquires new ownership of pointer
Object::Object(const Object& ob)
	: p(ob.p)
{
	_XINCREF (p);
	validate();
}

void Object::set(PyObject* pyob, bool owned)
{
	release();
	p = pyob;
	if (!owned)
	{
		_XINCREF (p);
	}
	validate();
}

void Object::validate()
{
	// release pointer if not the right type
	if (! accepts (p))
	{
		release ();
		if(PyErr_Occurred())
		{ // Error message already set
			throw Exception();
		}
		// Better error message if RTTI available
#if defined( _CPPRTTI ) || defined(__GNUG__)
		OpenWBEM::String s("CXX : Error creating object of type ");
		s += (typeid (*this)).name();
#else
		const char *s = "CXX: type error.";
#endif
		throw TypeError(s);
	}
}

void Object::release()
{
	_XDECREF (p);
	p = 0;
}

// Assignment acquires new ownership of pointer
Object& Object::operator= (const Object& rhs)
{
	set(rhs.p);
	return *this;
}

Object& Object::operator= (PyObject* rhsp)
{
	if(ptr() == rhsp) return *this;
	set (rhsp);
	return *this;
}

// Destructor
Object::~Object ()
{
	release ();
}

// Loaning the pointer to others, retain ownership
PyObject* Object::operator* () const
{
	return p;
}

// Explicit reference_counting changes
void Object::increment_reference_count()
{
	_XINCREF(p);
}

void Object::decrement_reference_count()
{
	// not allowed to commit suicide, however
	if(reference_count() == 1)
		throw RuntimeError("Object::decrement_reference_count error.");
	_XDECREF(p);
}
// Would like to call this pointer() but messes up STL in SeqBase<T>
PyObject* Object::ptr() const
{
	return p;
}

Type Object::type() const
{ 
    return Type(PyObject_Type (p), true);
}

String Object::str() const
{
    return String(PyObject_Str (p), true);
}

String Object::repr() const
{ 
    return String(PyObject_Repr (p), true);
}

OpenWBEM::String Object::as_string() const
{
    return static_cast<OpenWBEM::String>(str());
}

List Object::dir() const
{
	return List(PyObject_Dir(p), true);
}

bool Object::isType(const Type& t) const
{ 
    return type().ptr() == t.ptr();
}

// Can pyob be used in this object's constructor?
bool Object::accepts(PyObject *pyob) const
{
	return (pyob != 0);
}

Py_ssize_t Object::reference_count() const
{ // the reference count
	return p ? p->ob_refcnt : 0;
}

bool Object::hasAttr(const char* s) const
{
	return PyObject_HasAttrString (p, const_cast<char*>(s)) ? true: false;
}

bool Object::hasAttr(const OpenWBEM::String& s) const
{
	return hasAttr(s.c_str());
}

Object Object::getAttr(const char* s) const
{
	return Object(PyObject_GetAttrString(p, const_cast<char*>(s)), true);
}

Object Object::getAttr(const OpenWBEM::String& s) const
{
	return getAttr(s.c_str());
}

Object Object::getItem(const Object& key) const
{
	return Object(PyObject_GetItem(p, *key), true);
}

long Object::hashValue() const
{
	return PyObject_Hash(p);
}

bool Object::is(PyObject *pother) const
{  // identity test
	return p == pother;
}

bool Object::is(const Object& other) const
{ // identity test
	return p == other.p;
}

bool Object::isNone() const
{
	return p == Py_None;
}

bool Object::isCallable() const
{
	return PyCallable_Check(p) != 0;
}

bool Object::isInstanceOf(const Object& classObj) const
{
	int cc = PyObject_IsInstance(p, classObj.p);
	if (cc < 0)
		throw Exception();
	return (cc == 1) ? true : false;
}

bool Object::isInstance() const
{
	return PyInstance_Check(p) != 0;
}

bool Object::isBool() const
{
	return PyBool_Check(p);
}

bool Object::isLong() const
{
	return PyLong_Check(p);
}

bool Object::isInt() const
{
	return PyInt_Check(p);
}

bool Object::isFloat() const
{
	return PyFloat_Check(p);
}

bool Object::isDict() const
{
	return _Dict_Check(p);
}

bool Object::isList() const
{
	return _List_Check(p);
}

bool Object::isMapping() const
{
	return PyMapping_Check(p) != 0;
}

bool Object::isNumeric() const
{
	return PyNumber_Check(p) != 0;
}

bool Object::isSequence() const
{
	return PySequence_Check(p) != 0;
}

bool Object::isTrue() const
{
	return PyObject_IsTrue(p) != 0;
}

bool Object::isTuple() const
{
	return _Tuple_Check(p);
}

bool Object::isString() const
{
	return _String_Check(p) || _Unicode_Check(p);
}

bool Object::isUnicode() const
{
	return _Unicode_Check(p);
}

// Commands
void Object::setAttr(const char* s, const Object& value)
{
	if(PyObject_SetAttrString(p, const_cast<char*>(s), *value) == -1)
		throw AttributeError("getAttr failed.");
}
void Object::setAttr(const OpenWBEM::String& s, const Object& value)
{
	setAttr(s.c_str(), value);
}

void Object::delAttr(const char* s)
{
	if(PyObject_DelAttrString(p, const_cast<char*>(s)) == -1)
		throw AttributeError("delAttr failed.");
}
void Object::delAttr(const OpenWBEM::String& s)
{
	delAttr(s.c_str());
}

// PyObject_SetItem is too weird to be using from C++
// so it is intentionally omitted.
void Object::delItem(const Object& key)
{
	if(PyObject_DelItem(p, *key) == -1)
		throw KeyError("delItem failed.");
}

bool Object::operator==(const Object& o2) const
{
	int k = PyObject_Compare(p, *o2);
	if (PyErr_Occurred())
		throw Exception();
	return k == 0;
}

bool Object::operator!=(const Object& o2) const
{
	int k = PyObject_Compare(p, *o2);
	if (PyErr_Occurred())
		throw Exception();
	return k != 0;

}

bool Object::operator>=(const Object& o2) const
{
	int k = PyObject_Compare(p, *o2);
	if (PyErr_Occurred())
		throw Exception();
	return k >= 0;
}

bool Object::operator<=(const Object& o2) const
{
	int k = PyObject_Compare(p, *o2);
	if (PyErr_Occurred()) 
		throw Exception();
	return k <= 0;
}

bool Object::operator<(const Object& o2) const
{
	int k = PyObject_Compare(p, *o2);
	if (PyErr_Occurred()) 
		throw Exception();
	return k < 0;
}

bool Object::operator>(const Object& o2) const
{
	int k = PyObject_Compare (p, *o2);
	if (PyErr_Occurred()) 
		throw Exception();
	return k > 0;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Type
Type::Type(PyObject* pyob, bool owned)
	: Object(pyob, owned)
{
	validate();
}

Type::Type(const Object& ob)
	: Object(*ob)
{
	validate();
}

Type::Type(const Type& t)
	: Object(t)
{
	validate();
}

Type& Type::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Type& Type::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set(rhsp);
	return *this;
}

bool Type::accepts (PyObject *pyob) const
{
	return pyob && Py::_Type_Check (pyob);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Bool

// Constructor
Bool::Bool(PyObject *pyob, bool owned)
	: Object (pyob, owned)
{
	validate();
}

Bool::Bool(const Bool& ob)
	: Object(*ob)
{
	validate();
}

// create from long
Bool::Bool(long v)
	: Object(PyBool_FromLong(v), true)
{
	validate();
}

// create from int
Bool::Bool(int v)
	: Object(PyBool_FromLong(long(v)), true)
{
	validate();
}

// create from bool
Bool::Bool(bool v)
	: Object(PyBool_FromLong(long(v)), true)
{
	validate();
}

Bool::Bool(const Object& ob)
	: Object(PyBool_FromLong(long(ob.isTrue())), true)
{
	validate();
}

#ifdef HAVE_LONG_LONG
// create from long long
Bool::Bool(PY_LONG_LONG v)
	: Object(PyBool_FromLong(long(v ? 1 : 0)), true)
{
	validate();
}

// create from unsigned long long
Bool::Bool(unsigned PY_LONG_LONG v)
	: Object(PyBool_FromLong(long(v ? 1 : 0)), true)
{
	validate();
}
// convert to long long
long long Bool::asLongLong() const
{
	long long rc = (isTrue()) ? 1 : 0;
	return rc;
}
// convert to unsigned long long
unsigned long long Bool::asUnsignedLongLong() const
{
	unsigned long long rc = (isTrue()) ? 1 : 0;
	return rc;
}
// assign from long long
Bool& Bool::operator= (PY_LONG_LONG v)
{
	set(PyBool_FromLong(long(v)), true);
	return *this;
}
// assign from unsigned long long
Bool& Bool::operator= (unsigned PY_LONG_LONG v)
{
	set(PyBool_FromLong(long(v)), true);
	return *this;
}
#endif

Bool& Bool::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Bool& Bool::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
	{
		set(PyBool_FromLong(long(PyObject_IsTrue(rhsp))), true);
	}
	return *this;
}

// Membership
bool Bool::accepts(PyObject *pyob) const
{
	return pyob && PyBool_Check(pyob);
}

long Bool::asLong() const
{
	return long(isTrue());
}

unsigned long Bool::asUnsignedLong() const
{
	return (unsigned long) isTrue();
}

// convert to long
Bool::operator long() const
{
	return long(isTrue());
}

// assign from an int
Bool& Bool::operator= (int v)
{
	set(PyBool_FromLong(long(v)), true);
	return *this;
}

// assign from long
Bool& Bool::operator= (long v)
{
	set(PyBool_FromLong (v), true);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Int

// Constructor
Int::Int(PyObject *pyob, bool owned)
	: Object (pyob, owned)
{
	validate();
}

Int::Int(const Int& ob)
	: Object(*ob)
{
	validate();
}

// create from long
Int::Int(long v)
	: Object(PyInt_FromLong(v), true)
{
	validate();
}

// create from int
Int::Int(int v)
	: Object(PyInt_FromLong(long(v)), true)
{
	validate();
}

// create from bool
Int::Int(bool v)
	: Object(PyInt_FromLong(long(v ? 1 : 0)), true)
{
	validate();
}

Int::Int(const Object& ob)
	: Object(PyNumber_Int(*ob), true)
{
	validate();
}


#ifdef HAVE_LONG_LONG
// create from long long
Int::Int(PY_LONG_LONG v)
	: Object(PyLong_FromLongLong(v), true)
{
	validate();
}

// create from unsigned long long
Int::Int(unsigned PY_LONG_LONG v)
	: Object(PyLong_FromUnsignedLongLong(v), true)
{
	validate();
}
// convert to long long
long long Int::asLongLong() const
{
	long long rc = (long long) PyInt_AsLong(ptr());
	Exception::throwKnownException();
	return rc;
}
// convert to unsigned long long
unsigned long long Int::asUnsignedLongLong() const
{
	unsigned long long rc = (unsigned long long) PyInt_AsLong(ptr());
	Exception::throwKnownException();
	return rc;
}
// assign from long long
Int& Int::operator= (PY_LONG_LONG v)
{
	set(PyLong_FromLongLong (v), true);
	return *this;
}
// assign from unsigned long long
Int& Int::operator= (unsigned PY_LONG_LONG v)
{
	set(PyLong_FromUnsignedLongLong (v), true);
	return *this;
}
#endif

Int& Int::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Int& Int::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set(PyNumber_Int(rhsp), true);
	return *this;
}

// Membership
bool Int::accepts(PyObject *pyob) const
{
	return pyob && Py::_Int_Check (pyob);
}

long Int::asLong() const
{
	return PyInt_AsLong(ptr());
}

unsigned long Int::asUnsignedLong() const
{
	return PyInt_AsUnsignedLongMask(ptr());
}

// convert to long
Int::operator long() const
{
	return PyInt_AsLong (ptr());
}

// assign from an int
Int& Int::operator= (int v)
{
	set(PyInt_FromLong (long(v)), true);
	return *this;
}

// assign from long
Int& Int::operator= (long v)
{
	set(PyInt_FromLong (v), true);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Long

// Constructor
Long::Long(PyObject *pyob, bool owned)
	: Object (pyob, owned)
{
	validate();
}

Long::Long (const Long& ob)
	: Object(ob.ptr())
{
	validate();
}

// create from long
Long::Long (long v)
	: Object(PyLong_FromLong(v), true)
{
	validate();
}

// create from unsigned long
Long::Long (unsigned long v)
	: Object(PyLong_FromUnsignedLong(v), true)
{
	validate();
}
// create from int
Long::Long (int v)
	: Object(PyLong_FromLong(static_cast<long>(v)), true)
{
	validate();
}

// try to create from any object
Long::Long (const Object& ob)
	: Object(PyNumber_Long(*ob), true)
{
	validate();
}

// Assignment acquires new ownership of pointer
Long& Long::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Long& Long::operator= (PyObject* rhsp)
{
	if(ptr() != rhsp)
		set (PyNumber_Long(rhsp), true);
	return *this;
}
// Membership
bool Long::accepts (PyObject *pyob) const
{
	return pyob && Py::_Long_Check (pyob);
}
// convert to long
Long::operator long() const
{
	return PyLong_AsLong (ptr());
}
long Long::asLong() const
{
	return PyLong_AsLong (ptr());
}
// convert to unsigned
Long::operator unsigned long() const
{
	return PyLong_AsUnsignedLong (ptr());
}
unsigned long Long::asUnsignedLong() const
{
	return PyLong_AsUnsignedLong (ptr());
}

long long Long::asLongLong() const
{
	return (long long) PyLong_AsLong (ptr());
}
unsigned long long Long::asUnsignedLongLong() const
{
	return (unsigned long long) PyLong_AsUnsignedLong (ptr());
}

Long::operator double() const
{
	return PyLong_AsDouble (ptr());
}
// assign from an int
Long& Long::operator= (int v)
{
	set(PyLong_FromLong (long(v)), true);
	return *this;
}
// assign from long
Long& Long::operator= (long v)
{
	set(PyLong_FromLong (v), true);
	return *this;
}
// assign from unsigned long
Long& Long::operator= (unsigned long v)
{
	set(PyLong_FromUnsignedLong (v), true);
	return *this;
}


#ifdef HAVE_LONG_LONG
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class LongLong
LongLong::LongLong (PyObject *pyob, bool owned)
	: Object (pyob, owned)
{
	validate();
}

LongLong::LongLong (const LongLong& ob)
	: Object(ob.ptr())
{
	validate();
}
// create from long long
LongLong::LongLong (long long v)
	: Object(PyLong_FromLongLong(v), true)
{
	validate();
}
// create from unsigned long long
LongLong::LongLong (unsigned long long v)
	: Object(PyLong_FromUnsignedLongLong(v), true)
{
	validate();
}
// create from long
LongLong::LongLong (long v)
	: Object(PyLong_FromLongLong(v), true)
{
	validate();
}
// create from unsigned long
LongLong::LongLong (unsigned long v)
	: Object(PyLong_FromUnsignedLongLong(v), true)
{
	validate();
}
// create from int
LongLong::LongLong (int v)
	: Object(PyLong_FromLongLong(static_cast<long long>(v)), true)
{
	validate();
}

// try to create from any object
LongLong::LongLong (const Object& ob)
	: Object(PyNumber_Long(*ob), true)
{
	validate();
}

// Assignment acquires new ownership of pointer

LongLong& LongLong::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

LongLong& LongLong::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (PyNumber_Long(rhsp), true);
	return *this;
}

long long
LongLong::asLongLong() const
{
	long long rc = PyLong_AsLongLong(ptr());
	Exception::throwKnownException();
	return rc;
}

unsigned long long
LongLong::asUnsignedLongLong() const
{
	unsigned long long rc = PyLong_AsUnsignedLongLong (ptr());
	Exception::throwKnownException();
	return rc;
}

// Membership
bool LongLong::accepts (PyObject *pyob) const
{
	return pyob && Py::_Long_Check (pyob);
}
// convert to long long
LongLong::operator long long() const
{
	return PyLong_AsLongLong (ptr());
}
// convert to unsigned long
LongLong::operator unsigned long long() const
{
	return PyLong_AsUnsignedLongLong (ptr());
}
// convert to long
LongLong::operator long() const
{
	return PyLong_AsLong (ptr());
}
// convert to unsigned
LongLong::operator unsigned long() const
{
	return PyLong_AsUnsignedLong (ptr());
}
LongLong::operator double() const
{
	return PyLong_AsDouble (ptr());
}
// assign from an int
LongLong& LongLong::operator= (int v)
{
	set(PyLong_FromLongLong (long(v)), true);
	return *this;
}
// assign from long long
LongLong& LongLong::operator= (long long v)
{
	set(PyLong_FromLongLong (v), true);
	return *this;
}
// assign from unsigned long long
LongLong& LongLong::operator= (unsigned long long v)
{
	set(PyLong_FromUnsignedLongLong (v), true);
	return *this;
}
// assign from long
LongLong& LongLong::operator= (long v)
{
	set(PyLong_FromLongLong (v), true);
	return *this;
}
// assign from unsigned long
LongLong& LongLong::operator= (unsigned long v)
{
	set(PyLong_FromUnsignedLongLong (v), true);
	return *this;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Float
// Constructor
Float::Float(PyObject *pyob, bool owned)
	: Object(pyob, owned)
{
	validate();
}

Float::Float(const Float& f)
	: Object(f)
{
	validate();
}

// make from double
Float::Float(double v)
	: Object(PyFloat_FromDouble (v), true)
{
	validate();
}

// try to make from any object
Float::Float(const Object& ob)
	: Object(PyNumber_Float(*ob), true)
{
	validate();
}

Float& Float::operator=(const Object& rhs)
{
	return (*this = *rhs);
}

Float& Float::operator=(PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (PyNumber_Float(rhsp), true);
	return *this;
}
// Membership
bool Float::accepts (PyObject *pyob) const
{
	return pyob && Py::_Float_Check (pyob);
}
// convert to double
double Float::as_double () const
{
	return PyFloat_AsDouble (ptr());
}
Float::operator double () const
{
	return PyFloat_AsDouble (ptr());
}
// assign from a double
Float& Float::operator= (double v)
{
	set(PyFloat_FromDouble (v), true);
	return *this;
}
// assign from an int
Float& Float::operator= (int v)
{
	set(PyFloat_FromDouble (double(v)), true);
	return *this;
}
// assign from long
Float& Float::operator= (long v)
{
	set(PyFloat_FromDouble (double(v)), true);
	return *this;
}
// assign from an Int
Float& Float::operator= (const Int& iob)
{
	set(PyFloat_FromDouble (double(long(iob))), true);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Complex
Complex::Complex (PyObject *pyob, bool owned)
	: Object(pyob, owned)
{
	validate();
}

Complex::Complex (const Complex& f)
	: Object(f)
{
	validate();
}

// make from double
Complex::Complex(double v, double w)
	:Object(PyComplex_FromDoubles (v, w), true)
{
	validate();
}

Complex& Complex::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Complex& Complex::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (rhsp);
	return *this;
}
// Membership
bool Complex::accepts (PyObject *pyob) const
{
	return pyob && Py::_Complex_Check (pyob);
}
// convert to Py_complex
Complex::operator Py_complex () const
{
	return PyComplex_AsCComplex (ptr());
}
// assign from a Py_complex
Complex& Complex::operator= (const Py_complex& v)
{
	set(PyComplex_FromCComplex (v), true);
	return *this;
}
// assign from a double
Complex& Complex::operator= (double v)
{
	set(PyComplex_FromDoubles (v, 0.0), true);
	return *this;
}
// assign from an int
Complex& Complex::operator= (int v)
{
	set(PyComplex_FromDoubles (double(v), 0.0), true);
	return *this;
}
// assign from long
Complex& Complex::operator= (long v)
{
	set(PyComplex_FromDoubles (double(v), 0.0), true);
	return *this;
}
// assign from an Int
Complex& Complex::operator= (const Int& iob)
{
	set(PyComplex_FromDoubles (double(long(iob)), 0.0), true);
	return *this;
}

double Complex::real() const
{
	return PyComplex_RealAsDouble(ptr());
}

double Complex::imag() const
{
	return PyComplex_ImagAsDouble(ptr());
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Char

Char::Char(PyObject *pyob, bool owned)
	: Object(pyob, owned)
{
	validate();
}

Char::Char(const Object& ob)
	: Object(ob)
{
	validate();
}

Char::Char(const char* v)
	:Object(PyString_FromStringAndSize (const_cast<char*>(v),1), true)
{
	validate();
}

Char::Char(const OpenWBEM::String& v)
	:Object(PyString_FromStringAndSize (const_cast<char*>(v.c_str()),1), true)
{
	validate();
}

Char::Char(char v)
	: Object(PyString_FromStringAndSize (&v, 1), true)
{
	validate();
}

Char::Char(Py_UNICODE v)
	: Object(PyUnicode_FromUnicode (&v, 1), true)
{
	validate();
}

// Assignment acquires new ownership of pointer
Char& Char::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Char& Char::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (rhsp);
	return *this;
}

// Membership
bool Char::accepts (PyObject *pyob) const
{
	return pyob && (Py::_String_Check(pyob) || Py::_Unicode_Check(pyob)) && PySequence_Length (pyob) == 1;
}

// Assignment from C string
Char& Char::operator= (const char* v)
{
	set(PyString_FromStringAndSize (const_cast<char*>(v),1), true);
	return *this;
}

Char& Char::operator= (const OpenWBEM::String& v)
{
	set(PyString_FromStringAndSize (const_cast<char*>(v.c_str()),1), true);
	return *this;
}

Char& Char::operator= (char v)
{
	set(PyString_FromStringAndSize (&v, 1), true);
	return *this;
}

Char& Char::operator= (const unicodestring& v)
{
	set(PyUnicode_FromUnicode (const_cast<Py_UNICODE*>(v.data()),1), true);
	return *this;
}

Char& Char::operator= (Py_UNICODE v)
{
	set(PyUnicode_FromUnicode (&v, 1), true);
	return *this;
}

// Conversion
Char::operator String() const
{
    return String(ptr());
}

Char::operator OpenWBEM::String () const
{
	return OpenWBEM::String(PyString_AsString (ptr()));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class String
String::String (PyObject *pyob, bool owned)
	: SeqBase<Char>(pyob, owned)
{
	validate();
}

String::String (const Object& ob)
	: SeqBase<Char>(ob)
{
	validate();
}

String::String()
	: SeqBase<Char>( PyString_FromStringAndSize( "", 0 ), true )
{
	validate();
}

String::String(const OpenWBEM::String& v)
	: SeqBase<Char>(PyString_FromString(const_cast<char*>(v.c_str())), true )
{
	validate();
}

String::String(const char *s, const char *encoding, const char *error)
	: SeqBase<Char>(PyUnicode_Decode( s, strlen( s ), encoding, error), true )
{
	validate();
}

String::String(const char *s, int len, const char *encoding, const char *error)
	: SeqBase<Char>( PyUnicode_Decode( s, len, encoding, error ), true )
{
	validate();
}

String::String( const OpenWBEM::String &s, const char *encoding, const char *error)
	: SeqBase<Char>(PyUnicode_Decode( s.c_str(), s.length(), encoding, error ), true)
{
	validate();
}

String::String( const OpenWBEM::String& v, size_t vsize )
	: SeqBase<Char>(PyString_FromStringAndSize( const_cast<char*>(v.c_str()),
			static_cast<int>(vsize)), true)
{
	validate();
}

String::String( const char *v, int vsize )
	: SeqBase<Char>(PyString_FromStringAndSize( const_cast<char*>(v), vsize ), true )
{
	validate();
}

String::String( const char* v )
	: SeqBase<Char>( PyString_FromString( v ), true )
{
	validate();
}

String::size_type String::capacity() const
{
	return max_size();
}

// Assignment acquires new ownership of pointer
String& String::operator= ( const Object& rhs )
{
	return *this = *rhs;
}

String& String::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (rhsp);
	return *this;
}
// Membership
bool String::accepts (PyObject *pyob) const
{
	return pyob && (Py::_String_Check(pyob) || Py::_Unicode_Check(pyob));
}

// Assignment from C string
String& String::operator= (const char* v)
{
	set(PyString_FromString(const_cast<char*>(v)), true);
	return *this;
}
String& String::operator= (const OpenWBEM::String& v)
{
	set(PyString_FromString(const_cast<char*>(v.c_str())), true);
	return *this;
}
String& String::operator= (const unicodestring& v)
{
	set( PyUnicode_FromUnicode( const_cast<Py_UNICODE*>( v.data() ),
			static_cast<int>( v.length() ) ), true );
	return *this;
}

// Encode
String String::encode(const char *encoding, const char *error)
{
	if(isUnicode())
	{
		return String( PyUnicode_AsEncodedString( ptr(), encoding, error ) );
	}
	else
	{
		return String( PyString_AsEncodedObject( ptr(), encoding, error ) );
	}
}

String String::decode( const char *encoding, const char *error)
{
	return Object( PyString_AsDecodedObject( ptr(), encoding, error ) );
}

// Queries
String::size_type String::size () const
{
	if( isUnicode() )
	{
		return static_cast<size_type>( PyUnicode_GET_SIZE (ptr()) );
	}
	else
	{
		return static_cast<size_type>( PyString_Size (ptr()) );
	}
}

OpenWBEM::String String::as_ow_string() const
{
	if(isUnicode())
	{
		Py::String s(PyUnicode_AsUTF8String(ptr()), true);
		return OpenWBEM::String(PyString_AsString(s.ptr()));
		//throw TypeError("cannot return OpenWBEM::String from Unicode object");
	}
	else
	{
		return OpenWBEM::String(PyString_AsString(ptr()));
	}
}

String::operator OpenWBEM::String () const
{
	return as_ow_string();
}

unicodestring String::as_unicodestring() const
{
	if( isUnicode() )
	{
		return unicodestring(PyUnicode_AS_UNICODE( ptr()),
			static_cast<size_type>( PyUnicode_GET_SIZE( ptr())));
	}
	else
	{
		throw TypeError("can only return unicodestring from Unicode object");
	}
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Tuple
Tuple::Tuple (PyObject *pyob, bool owned)
	: Sequence (pyob, owned)
{
	validate();
}

Tuple::Tuple (const Object& ob)
	: Sequence(ob)
{
	validate();
}

// New tuple of a given size
Tuple::Tuple (int size)
{
	set(PyTuple_New (size), true);
	validate ();
	for (sequence_index_type i=0; i < size; i++)
	{
		if(PyTuple_SetItem (ptr(), i, new_reference_to(Py::_None())) == -1)
		{
			throw Exception();
		}
	}
}
// Tuple from any sequence
Tuple::Tuple (const Sequence& s)
{
	sequence_index_type limit( sequence_index_type( s.length() ) );

	set(PyTuple_New (limit), true);
	validate();
	
	for(sequence_index_type i=0; i < limit; i++)
	{
		if(PyTuple_SetItem (ptr(), i, new_reference_to(s[i])) == -1)
		{
			throw Exception();
		}
	}
}

void Tuple::setItem (sequence_index_type offset, const Object&ob)
{
	// note PyTuple_SetItem is a thief...
	if(PyTuple_SetItem (ptr(), offset, new_reference_to(ob)) == -1)
	{
		throw Exception();
	}
}

// Assignment acquires new ownership of pointer

Tuple& Tuple::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Tuple& Tuple::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (rhsp);
	return *this;
}
// Membership
bool Tuple::accepts (PyObject *pyob) const
{
	return pyob && Py::_Tuple_Check (pyob);
}

Tuple Tuple::getSlice (int i, int j) const
{
	return Tuple (PySequence_GetSlice (ptr(), i, j), true);
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class List
List::List (PyObject *pyob, bool owned)
	: Sequence(pyob, owned)
{
	validate();
}

List::List (const Object& ob)
	: Sequence(ob)
{
	validate();
}

// Creation at a fixed size
List::List (int size)
{
	set(PyList_New (size), true);
	validate();
	for (sequence_index_type i=0; i < size; i++)
	{
		if(PyList_SetItem (ptr(), i, new_reference_to(Py::_None())) == -1)
		{
			throw Exception();
		}
	}
}

// List from a sequence
List::List (const Sequence& s)
	: Sequence()
{
	int n = (int)s.length();
	set(PyList_New (n), true);
	validate();
	for (sequence_index_type i=0; i < n; i++)
	{
		if(PyList_SetItem (ptr(), i, new_reference_to(s[i])) == -1)
		{
			throw Exception();
		}
	}
}

List::size_type List::capacity() const
{
	return max_size();
}

// Assignment acquires new ownership of pointer
List& List::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

List& List::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (rhsp);
	return *this;
}
// Membership
bool List::accepts (PyObject *pyob) const
{
	return pyob && Py::_List_Check (pyob);
}

List List::getSlice (int i, int j) const
{
	return List (PyList_GetSlice (ptr(), i, j), true);
}

void List::setSlice (int i, int j, const Object& v)
{
	if(PyList_SetSlice (ptr(), i, j, *v) == -1)
	{
		throw Exception();
	}
}

void List::append (const Object& ob)
{
	if(PyList_Append (ptr(), *ob) == -1)
	{
		throw Exception();
	}
}

void List::insert (int i, const Object& ob)
{
	if(PyList_Insert (ptr(), i, *ob) == -1)
	{
		throw Exception();
	}
}

void List::sort ()
{
	if(PyList_Sort(ptr()) == -1)
	{
		throw Exception();
	}
}

void List::reverse ()
{
	if(PyList_Reverse(ptr()) == -1)
	{
		throw Exception();
	}
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Dict
Dict::Dict(PyObject *pyob, bool owned)
	: Mapping(pyob, owned)
{
	validate();
}


Dict::Dict(const Object& ob)
	: Mapping(ob)
{
	validate();
}

Dict::Dict(const Dict& ob)
	: Mapping(ob)
{
	validate();
}
// Creation
Dict::Dict()
{
	set(PyDict_New (), true);
	validate();
}
// Assignment acquires new ownership of pointer
Dict& Dict::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Dict& Dict::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set(rhsp);
	return *this;
}
// Membership
bool Dict::accepts (PyObject *pyob) const
{
	return pyob && Py::_Dict_Check (pyob);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Callable
Callable::Callable()
	: Object()
{
}

Callable::Callable(PyObject *pyob, bool owned)
	: Object (pyob, owned)
{
	validate();
}

Callable::Callable(const Object& ob)
	: Object(ob)
{
	validate();
}

// Assignment acquires new ownership of pointer
Callable& Callable::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Callable& Callable::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set (rhsp);
	return *this;
}

// Membership
bool Callable::accepts (PyObject *pyob) const
{
	return pyob && PyCallable_Check (pyob);
}

// Call
Object Callable::apply(const Tuple& args) const
{
	Object ro = asObject(PyObject_CallObject(ptr(), args.ptr()));
	if(PyErr_Occurred())
	{ // Error message already set
		throw Exception();
	}
	return ro;
}

// Call with keywords
Object Callable::apply(const Tuple& args, const Dict& kw) const
{
	Object ro = asObject(PyEval_CallObjectWithKeywords(ptr(), args.ptr(), kw.ptr()));
	if(PyErr_Occurred())
	{ // Error message already set
		throw Exception();
	}
	return ro;
}

Object Callable::apply(PyObject* pargs) const
{
	return apply(Tuple(pargs));
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Class Module
void Module::loadOrAdd(const char* s, bool doLoad)
{
	if(doLoad)
	{
		PyObject *m = PyImport_ImportModule(const_cast<char*>(s));
		set(m, true);
	}
	else
	{
		PyObject *m = PyImport_AddModule(const_cast<char *>(s));
		set(m, false);
	}
	validate ();
}

Module::Module (PyObject* pyob, bool owned)
	: Object (pyob, owned)
{
	validate();
}
// Construct from module name
Module::Module (const char* s, bool doLoad)
	: Object()
{
	loadOrAdd(s, doLoad);
}
Module::Module (const OpenWBEM::String& s, bool doLoad)
	: Object()
{
	loadOrAdd(s.c_str(), doLoad);
}

// Copy constructor acquires new ownership of pointer
Module::Module (const Module& ob)
	: Object(*ob)
{
	validate();
}

Module& Module::operator= (const Object& rhs)
{
	return (*this = *rhs);
}

Module& Module::operator= (PyObject* rhsp)
{
	if (ptr() != rhsp)
		set(rhsp);
	return *this;
}

OpenWBEM::String Module::getFileName() const
{
	char *fname = PyModule_GetFilename(ptr());
	if (PyErr_Occurred())
	{
		PyErr_Clear();
		return OpenWBEM::String();
	}
	return OpenWBEM::String(fname);
}

Dict Module::getDict()
{
	// Caution -- PyModule_GetDict returns borrowed reference!
	return Dict(PyModule_GetDict(ptr()));
}

//////////////////////////////////////////////////////////////////////////////
// Non-class methods
//////////////////////////////////////////////////////////////////////////////
PyObject* new_reference_to(const Object& g)
{
	PyObject* p = g.ptr();
	Py::_XINCREF(p);
	return p;
}

// Nothing() is what an extension method returns if
// there is no other return value.
Object Nothing()
{
	return Object(Py::_None());
}

// Python special None value
Object None()
{
	return Object(Py::_None());
}

std::ostream& operator<< (std::ostream& os, const Object& ob)
{
    return (os << static_cast<OpenWBEM::String>(ob.str()));
}  

std::pair<Object,Object> coerce(const Object& a, const Object& b)
{
	PyObject *p1, *p2;
	p1 = *a;
	p2 = *b;
	if(PyNumber_Coerce(&p1,&p2) == -1)
	{
		throw Exception();
	}
	return std::pair<Object,Object>(asObject(p1), asObject(p2));
}

Object type(const Exception&) // return the type of the error
{
	PyObject *ptype, *pvalue, *ptrace;
	PyErr_Fetch(&ptype, &pvalue, &ptrace);
	PyErr_NormalizeException(&ptype, &pvalue, &ptrace);
	Object result;
	if (ptype)
		result = ptype;
	PyErr_Restore(ptype, pvalue, ptrace);
	return result;
}

Object value(const Exception&) // return the value of the error
{
	PyObject *ptype, *pvalue, *ptrace;
	PyErr_Fetch(&ptype, &pvalue, &ptrace);
	PyErr_NormalizeException(&ptype, &pvalue, &ptrace);
	Object result;
	if (pvalue)
		result = pvalue;
	PyErr_Restore(ptype, pvalue, ptrace);
	return result;
}

Object trace(const Exception&) // return the traceback of the error
{
	PyObject *ptype, *pvalue, *ptrace;
	PyErr_Fetch(&ptype, &pvalue, &ptrace);
	PyErr_NormalizeException(&ptype, &pvalue, &ptrace);
	Object result;
	if (ptrace)
		result = ptrace;
	PyErr_Restore(ptype, pvalue, ptrace);
	return result;
}

}	// End of namespace Py
