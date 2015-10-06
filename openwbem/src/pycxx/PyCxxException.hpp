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

#ifndef __PYCXXEXCEPTION_HPP_GUARD
#define __PYCXXEXCEPTION_HPP_GUARD

#include <Python.h>
#include "PyCxxConfig.hpp"
#include "PyCxxPythonWrap.hpp"

#include <openwbem/OW_String.hpp>

// This mimics the Python structure, in order to minimize confusion
namespace Py
{

// Fwd declarations
class ExtensionExceptionType;
class Object;

//////////////////////////////////////////////////////////////////////////////
class Exception
{
public:
	Exception( ExtensionExceptionType &exception, const OpenWBEM::String& reason );
	Exception( ExtensionExceptionType &exception, Object &reason );

	explicit Exception ()
	{}
	
	Exception (const OpenWBEM::String& reason)
	{
		PyErr_SetString (Py::_Exc_RuntimeError(), reason.c_str());
	}
	
	Exception (PyObject* exception, const OpenWBEM::String& reason)
	{
		PyErr_SetString (exception, reason.c_str());
	}
	
	Exception (PyObject* exception, Object& reason);        
	Exception (Object& exception, Object &excArgs);        

	void clear() // clear the error
	// technically but not philosophically const
	{
		PyErr_Clear();
	}

	static void throwKnownException(
		const OpenWBEM::String& reason=OpenWBEM::String());
};

//////////////////////////////////////////////////////////////////////////////
// Abstract
class StandardError: public Exception
{
protected: 
	explicit StandardError()
	{}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class LookupError: public StandardError
{
protected: 
	explicit LookupError()
	{}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class ArithmeticError: public StandardError
{
protected: 
	explicit ArithmeticError()
	{}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class EnvironmentError: public StandardError
{
protected: 
	explicit EnvironmentError()
	{}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
// Concrete
class TypeError: public StandardError
{
public:
	TypeError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_TypeError(),reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class IndexError: public LookupError
{
public:
	IndexError (const OpenWBEM::String& reason)
		: LookupError()
	{
		PyErr_SetString (Py::_Exc_IndexError(), reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class AttributeError: public StandardError
{
public:
	AttributeError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_AttributeError(), reason.c_str());
	}        
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class NameError: public StandardError
{
public:
	NameError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_NameError(), reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class RuntimeError: public StandardError
{
public:
	RuntimeError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_RuntimeError(), reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class SystemError: public StandardError
{
public:
	SystemError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_SystemError(),reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class KeyError: public LookupError
{
public:
	KeyError (const OpenWBEM::String& reason)
		: LookupError()
	{
		PyErr_SetString (Py::_Exc_KeyError(),reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class ValueError: public StandardError
{
public:
	ValueError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_ValueError(), reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class OverflowError: public ArithmeticError
{
public:
	OverflowError (const OpenWBEM::String& reason)
		: ArithmeticError()
	{
		PyErr_SetString (Py::_Exc_OverflowError(), reason.c_str());
	}        
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class ZeroDivisionError: public ArithmeticError
{
public:
	ZeroDivisionError (const OpenWBEM::String& reason)
		: ArithmeticError() 
	{
		PyErr_SetString (Py::_Exc_ZeroDivisionError(), reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class FloatingPointError: public ArithmeticError
{
public:
	FloatingPointError (const OpenWBEM::String& reason)
		: ArithmeticError() 
	{
		PyErr_SetString (Py::_Exc_FloatingPointError(), reason.c_str());
	}
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class MemoryError: public StandardError
{
public:
	MemoryError (const OpenWBEM::String& reason)
		: StandardError()
	{
		PyErr_SetString (Py::_Exc_MemoryError(), reason.c_str());
	}    
	friend class Exception;
};

//////////////////////////////////////////////////////////////////////////////
class SystemExit: public StandardError
{
public:
	SystemExit (const OpenWBEM::String& reason)
		: StandardError() 
	{
		PyErr_SetString (Py::_Exc_SystemExit(),reason.c_str());
	}
	friend class Exception;
};

}	// End of namepspace Py

#endif	// __PYCXXEXCEPTION_HPP_GUARD
