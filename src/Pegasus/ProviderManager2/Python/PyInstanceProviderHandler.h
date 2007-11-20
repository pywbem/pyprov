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
#ifndef PYINSTANCEPROVIDERHANDLDER_H_GUARD
#define PYINSTANCEPROVIDERHANDLDER_H_GUARD

#include "PythonProviderManager.h"

PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

class InstanceProviderHandler
{
public:
    static CIMResponseMessage* handleGetInstanceRequest(
		CIMRequestMessage* message, 
		PyProviderRep& provrep,
		PythonProviderManager* pmgr);

    static CIMResponseMessage* handleEnumerateInstancesRequest(
		CIMRequestMessage* message,
		PyProviderRep& provrep,
		PythonProviderManager* pmgr);

    static CIMResponseMessage* handleEnumerateInstanceNamesRequest(
		CIMRequestMessage* message, 
		PyProviderRep& provrep, 
		PythonProviderManager* pmgr);

    static CIMResponseMessage* handleCreateInstanceRequest(
		CIMRequestMessage* message,
		PyProviderRep& provrep, 
		PythonProviderManager* pmgr);

    static CIMResponseMessage* handleModifyInstanceRequest(
		CIMRequestMessage* message,
		PyProviderRep& provrep,
		PythonProviderManager* pmgr);

    static CIMResponseMessage* handleDeleteInstanceRequest(
		CIMRequestMessage* message,
		PyProviderRep& provrep, 
		PythonProviderManager* pmgr);

	static CIMResponseMessage* handleGetPropertyRequest(
		CIMRequestMessage* message,
		PyProviderRep& provrep, 
		PythonProviderManager* pmgr);

	static CIMResponseMessage* handleSetPropertyRequest(
		CIMRequestMessage* message,
		PyProviderRep& provrep, 
		PythonProviderManager* pmgr);
};

}	// end of namespace PythonProvIFC

#endif	//PYINSTANCEPROVIDERHANDLDER_H_GUARD
