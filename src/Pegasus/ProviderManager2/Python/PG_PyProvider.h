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
#ifndef PG_PYPROVIDER_HPP_GUARD_
#define PG_PYPROVIDER_HPP_GUARD_

#include "PyCxxObjects.h"
#include "PG_PyProviderEnvironment.h"
#include <Pegasus/Common/OperationContext.h>
#include <Pegasus/Provider/CIMOMHandle.h>
/*
#include <openwbem/OW_config.h>
#include <openwbem/OW_ProviderEnvironmentIFC.hpp>
#include <openwbem/OW_DateTime.hpp>
#include <openwbem/OW_IntrusiveCountableBase.hpp>
#include <openwbem/OW_IntrusiveReference.hpp>
#include <openwbem/OW_WQLSelectStatement.hpp>
*/
extern "C"
{
#include <time.h>
}

using namespace Pegasus;

namespace PythonProvIFC
{

typedef Array<String> StringArray;

class PyProvider
{
public:
	PyProvider(const String& name, 
			   const OperationContext& ctx,
			   CIMOMHandle& ch, 
			   bool unloadableType=true);

	virtual ~PyProvider();

	void enumInstanceNames(
		const String& ns,
		const String& className);

	void enumInstances(
		const String& ns,
		const String& className,
		const Boolean localOnly,
		const Boolean deepInheritance,
		const Boolean includeQualifiers,
		const Boolean includeClassOrigin,
		const CIMPropertyList propertyList);

	void getInstance(
		const String& ns,
		const String& className,
		const CIMObjectPath& cop,
		const Boolean includeQualifiers,
		const Boolean includeClassOrigin,
		const CIMPropertyList propertyList);

	void createInstance(
		const String& ns,
		const String& className,
		const CIMInstance& inst);

	void modifyInstance(
		const String& ns,
		const String& className,
		const CIMInstance& inst,
		const Boolean includeQualifiers,
		const CIMPropertyList propertyList);

	void deleteInstance(
		const String& ns,
		const String& className,
		const CIMObjectPath& cop);

	/*
	// Associator provider
	void associators(
		const OperationContext& ctx,
		CIMInstanceResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& assocClass,
		const String& resultClass,
		const String& role,
		const String& resultRole,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
		WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
		const StringArray* propertyList);

	// Associator provider
	void associatorNames(
		const OperationContext& ctx,
		CIMObjectPathResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& assocClass,
		const String& resultClass,
		const String& role,
		const String& resultRole);

	// Associator provider
	void references(
		const OperationContext& ctx,
		CIMInstanceResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& resultClass,
		const String& role,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
		WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
		const StringArray* propertyList);

	// Associator provider
	void referenceNames(
		const OperationContext& ctx,
		CIMObjectPathResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& resultClass,
		const String& role);

	// Method provider
	CIMValue invokeMethod(
		const OperationContext& ctx,
		const String& ns,
		const CIMObjectPath& path,
		const String& methodName,
		const CIMParamValueArray& in,
		CIMParamValueArray& out);

	// Indication provider
	void activateFilter(
		const OperationContext& ctx,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes
#if OW_OPENWBEM_MAJOR_VERSION >= 4
		, bool firstActivation
#endif
		);

	// Indication provider
	void authorizeFilter(
		const OperationContext& ctx,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes,
		const String& owner);

	// Indication provider
	void deActivateFilter(
		const OperationContext& ctx,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes
#if OW_OPENWBEM_MAJOR_VERSION >= 4
		, bool lastActivation
#endif
		);

	// Indication export provider
	StringArray getHandlerClassNames();

	// Indication export provider
	void exportIndication(
		const OperationContext& ctx, 
		const String& ns,
		const CIMInstance& indHandlerInst,
		const CIMInstance& indicationInst);

	Int32 poll(const OperationContext& ctx);

	Int32 getInitialPollingInterval(
		const OperationContext& ctx);
	*/
	void shutDown(
		const OperationContext& ctx);

	bool canShutDown(
		const OperationContext& ctx) const;

	String getName() const { return m_path; }

	String getFileName() const;

	void updateAccessTime();
	CIMDateTime getLastAccessTime() const;

	bool isUnloadableType() const { return m_unloadableType; }
	void setUnloadableType(bool arg)
	{
		m_unloadableType = arg;
	}

	void setHandlerClassNames(const StringArray& cnames)
	{
		m_handlerClassNames = cnames;
	}

	time_t getFileModTime() const { return m_fileModTime; }
	bool providerChanged() const;

	static void setPyWbemMod(const Py::Module& pywbemMod);

private:
	PyProvider() {}
	PyProvider(const PyProvider& arg) {}
	PyProvider& operator= (const PyProvider& arg);

	String processPyException(
		Py::Exception& thrownEx,
		int lineno,
		//LoggerRef& lgr,
		bool doThrow=true) const;

	String m_path;
	Py::Object m_pyprov;
	CIMDateTime m_dt;
	time_t m_fileModTime;
#if OW_OPENWBEM_MAJOR_VERSION == 3
	int m_activationCount;
#endif
	bool m_unloadableType;
	StringArray m_handlerClassNames;
};

//typedef IntrusiveReference<PyProvider> PyProviderRef;

} // end namespace PythonProvIFC

#endif
