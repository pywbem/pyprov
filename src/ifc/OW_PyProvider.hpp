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
#ifndef OW_PYPROVIDER_HPP_GUARD_
#define OW_PYPROVIDER_HPP_GUARD_

#include "PyCxxObjects.hpp"

#include <openwbem/OW_config.h>
#include <openwbem/OW_ProviderEnvironmentIFC.hpp>
#include <openwbem/OW_DateTime.hpp>
#include <openwbem/OW_IntrusiveCountableBase.hpp>
#include <openwbem/OW_IntrusiveReference.hpp>
#include <openwbem/OW_WQLSelectStatement.hpp>

extern "C"
{
#include <time.h>
}

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

class PyProvider : public IntrusiveCountableBase
{
public:
	PyProvider(const String& name, const ProviderEnvironmentIFCRef& env,
		bool unloadableType=true);

	virtual ~PyProvider();

	// Instance provider
	void enumInstanceNames(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const String& className,
		CIMObjectPathResultHandlerIFC& result,
		const CIMClass& cimClass);

	// Instance provider
	void enumInstances(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const String& className,
		CIMInstanceResultHandlerIFC& result,
		WBEMFlags::ELocalOnlyFlag localOnly, 
		WBEMFlags::EDeepFlag deep, 
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers, 
		WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
		const StringArray* propertyList,
		const CIMClass& requestedClass,
		const CIMClass& cimClass);

	// Instance provider
	CIMInstance getInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMObjectPath& instanceName,
		WBEMFlags::ELocalOnlyFlag localOnly,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers, 
		WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
		const StringArray* propertyList, 
		const CIMClass& cimClass);

	// Instance provider
	CIMObjectPath createInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMInstance& cimInstance);

	// Instance provider
	void modifyInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMInstance& modifiedInstance,
		const CIMInstance& previousInstance,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
		const StringArray* propertyList,
		const CIMClass& theClass);

	// Instance provider
	void deleteInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMObjectPath& cop);

	// Associator provider
	void associators(
		const ProviderEnvironmentIFCRef& env,
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
		const ProviderEnvironmentIFCRef& env,
		CIMObjectPathResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& assocClass,
		const String& resultClass,
		const String& role,
		const String& resultRole);

	// Associator provider
	void references(
		const ProviderEnvironmentIFCRef& env,
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
		const ProviderEnvironmentIFCRef& env,
		CIMObjectPathResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& resultClass,
		const String& role);

	// Method provider
	CIMValue invokeMethod(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMObjectPath& path,
		const String& methodName,
		const CIMParamValueArray& in,
		CIMParamValueArray& out);

	// Indication provider
	void activateFilter(
		const ProviderEnvironmentIFCRef& env,
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
		const ProviderEnvironmentIFCRef& env,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes,
		const String& owner);

	// Indication provider
	void deActivateFilter(
		const ProviderEnvironmentIFCRef& env,
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
		const ProviderEnvironmentIFCRef& env, 
		const String& ns,
		const CIMInstance& indHandlerInst,
		const CIMInstance& indicationInst);

	Int32 poll(const ProviderEnvironmentIFCRef& env);

	Int32 getInitialPollingInterval(
		const ProviderEnvironmentIFCRef& env);

	void shutDown(
		const ProviderEnvironmentIFCRef& env);

	bool canShutDown(
		const ProviderEnvironmentIFCRef& env) const;

	String getName() const { return m_path; }

	String getFileName() const;

	void updateAccessTime();
	DateTime getLastAccessTime() const;

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
		LoggerRef& lgr,
		bool doThrow=true) const;

	String m_path;
	Py::Object m_pyprov;
	DateTime m_dt;
	time_t m_fileModTime;
#if OW_OPENWBEM_MAJOR_VERSION == 3
	int m_activationCount;
#endif
	bool m_unloadableType;
	StringArray m_handlerClassNames;
};

typedef IntrusiveReference<PyProvider> PyProviderRef;

} // end namespace PythonProvIFC

#endif
