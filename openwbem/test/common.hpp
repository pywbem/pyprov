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
#ifndef OWPYCOMMON_HPP_GUARD
#define OWPYCOMMON_HPP_GUARD

#include <Python.h>
#include <openwbem/OW_Array.hpp>

typedef OpenWBEM::Array<PyObject*> PyObjectPtrArray;

class DecRefs
{
public:
	DecRefs() : m_objs() {}
	~DecRefs()
	{
		for (PyObjectPtrArray::size_type i = 0; i < m_objs.size(); i++)
		{
			Py_DECREF(m_objs[i]);
		}
	}

	DecRefs& operator += (PyObject* pobj)
	{
		if (pobj)
		{
			m_objs.append(pobj);
		}
		return *this; 
	}

private:
	PyObjectPtrArray m_objs;
};

#endif	// OWPYCOMMON_HPP_GUARD
