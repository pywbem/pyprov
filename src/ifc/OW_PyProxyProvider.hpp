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
#ifndef OW_PYPROXYPROVIDER_HPP_GUARD
#define OW_PYPROXYPROVIDER_HPP_GUARD

#include <openwbem/OW_InstanceProviderIFC.hpp>
#include <openwbem/OW_MethodProviderIFC.hpp>
#include <openwbem/OW_AssociatorProviderIFC.hpp>
#include <openwbem/OW_IndicationProviderIFC.hpp>
#include <openwbem/OW_IndicationExportProviderIFC.hpp>
#include <openwbem/OW_PolledProviderIFC.hpp>
#include "OW_PyProvider.hpp"

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

class PyProxyInstanceProvider : public InstanceProviderIFC
{
public:
	PyProxyInstanceProvider(PyProviderRef pProv);
	virtual void enumInstanceNames(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const String& className,
		CIMObjectPathResultHandlerIFC& result,
		const CIMClass& cimClass);
	virtual void enumInstances(
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
	virtual CIMInstance getInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMObjectPath& instanceName,
		WBEMFlags::ELocalOnlyFlag localOnly,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
		WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
		const StringArray* propertyList,
		const CIMClass& cimClass);
	virtual CIMObjectPath createInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMInstance& cimInstance);
	virtual void modifyInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMInstance& modifiedInstance,
		const CIMInstance& previousInstance,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
		const StringArray* propertyList,
		const CIMClass& theClass);
	virtual void deleteInstance(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMObjectPath& cop);
private:
	PyProviderRef m_pProv;
};

class PyProxyAssociatorProvider : public AssociatorProviderIFC
{
public:
	PyProxyAssociatorProvider(PyProviderRef pProv);

	virtual void associators(
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
	virtual void associatorNames(
		const ProviderEnvironmentIFCRef& env,
		CIMObjectPathResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& assocClass,
		const String& resultClass,
		const String& role,
		const String& resultRole);
	virtual void references(
		const ProviderEnvironmentIFCRef& env,
		CIMInstanceResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& resultClass,
		const String& role,
		WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
		WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
		const StringArray* propertyList);
	virtual void referenceNames(
		const ProviderEnvironmentIFCRef& env,
		CIMObjectPathResultHandlerIFC& result,
		const String& ns,
		const CIMObjectPath& objectName,
		const String& resultClass,
		const String& role);

private:
	PyProviderRef m_pProv;
};

class PyProxyMethodProvider : public MethodProviderIFC
{
public:
	PyProxyMethodProvider(PyProviderRef pProv);
	virtual CIMValue invokeMethod(
		const ProviderEnvironmentIFCRef& env,
		const String& ns,
		const CIMObjectPath& path,
		const String& methodName,
		const CIMParamValueArray& in,
		CIMParamValueArray& out);

private:
	PyProviderRef m_pProv;
};

class PyProxyIndicationProvider : public IndicationProviderIFC
{
public:
	PyProxyIndicationProvider(PyProviderRef pProv);
	virtual void activateFilter(
		const ProviderEnvironmentIFCRef& env,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes);
	virtual void authorizeFilter(
		const ProviderEnvironmentIFCRef& env,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes,
		const String& owner);
	virtual void deActivateFilter(
		const ProviderEnvironmentIFCRef& env,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes);
	virtual int mustPoll(
		const ProviderEnvironmentIFCRef& env,
		const WQLSelectStatement& filter,
		const String& eventType,
		const String& nameSpace,
		const StringArray& classes);
private:
	PyProviderRef m_pProv;
};

class PyProxyIndicationExportProvider : public IndicationExportProviderIFC
{
public:
	PyProxyIndicationExportProvider(PyProviderRef pProv);
	virtual StringArray getHandlerClassNames();
	virtual void exportIndication(const ProviderEnvironmentIFCRef& env, 
		const String& ns, const CIMInstance& indHandlerInst,
		const CIMInstance& indicationInst);
	virtual void doCooperativeCancel();
	virtual void doDefinitiveCancel();
private:
	PyProviderRef m_pProv;
};

class PyProxyPolledProvider : public PolledProviderIFC
{
public:
	PyProxyPolledProvider(PyProviderRef pProv);
	virtual Int32 poll(const ProviderEnvironmentIFCRef& env);
	virtual Int32 getInitialPollingInterval(
		const ProviderEnvironmentIFCRef& env);
	virtual void doCooperativeCancel();
	virtual void doDefinitiveCancel();
private:
	PyProviderRef m_pProv;
};

}	// End of namespace PythonProvIFC

#endif	// OW_PYPROXYPROVIDER_HPP_GUARD
