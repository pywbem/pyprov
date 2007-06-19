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
#include "OW_PyProxyProvider.hpp"

#include <openwbem/OW_CIMClass.hpp>
#include <openwbem/OW_CIMInstance.hpp>
#include <openwbem/OW_CIMObjectPath.hpp>
#include <openwbem/OW_CIMValue.hpp>

using namespace OW_NAMESPACE;
using namespace WBEMFlags;

namespace PythonProvIFC
{
//////////////////////////////////////////////////////////////////////////////
PyProxyInstanceProvider::PyProxyInstanceProvider(PyProviderRef pProv)
	: InstanceProviderIFC()
	, m_pProv(pProv)
{
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyInstanceProvider::enumInstanceNames(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const String& className,
	CIMObjectPathResultHandlerIFC& result,
	const CIMClass& cimClass)
{
	m_pProv->updateAccessTime();
	m_pProv->enumInstanceNames(env, ns, className, result, cimClass);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyInstanceProvider::enumInstances(
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
	const CIMClass& cimClass)
{
	m_pProv->updateAccessTime();
	m_pProv->enumInstances(env, ns, className, result, localOnly, deep,
		includeQualifiers, includeClassOrigin, propertyList,
		requestedClass, cimClass);
}

//////////////////////////////////////////////////////////////////////////////
CIMInstance
PyProxyInstanceProvider::getInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMObjectPath& instanceName,
	WBEMFlags::ELocalOnlyFlag localOnly,
	WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
	WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
	const StringArray* propertyList,
	const CIMClass& cimClass)
{
	m_pProv->updateAccessTime();
	return m_pProv->getInstance(env, ns, instanceName, localOnly,
		includeQualifiers, includeClassOrigin, propertyList,
		cimClass);
}

//////////////////////////////////////////////////////////////////////////////
CIMObjectPath
PyProxyInstanceProvider::createInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMInstance& cimInstance)
{
	m_pProv->updateAccessTime();
	return m_pProv->createInstance(env, ns, cimInstance);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyInstanceProvider::modifyInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMInstance& modifiedInstance,
	const CIMInstance& previousInstance,
	WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
	const StringArray* propertyList,
	const CIMClass& theClass)
{
	m_pProv->updateAccessTime();
	m_pProv->modifyInstance(env, ns, modifiedInstance, previousInstance,
		includeQualifiers, propertyList, theClass);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyInstanceProvider::deleteInstance(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMObjectPath& cop)
{
	m_pProv->updateAccessTime();
	m_pProv->deleteInstance(env, ns, cop);
}

//////////////////////////////////////////////////////////////////////////////
PyProxyAssociatorProvider::PyProxyAssociatorProvider(PyProviderRef pProv)
	: AssociatorProviderIFC()
	, m_pProv(pProv)
{
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyAssociatorProvider::associators(
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
	const StringArray* propertyList)
{
	m_pProv->updateAccessTime();
	m_pProv->associators(env, result, ns, objectName, assocClass, resultClass,
		role, resultRole, includeQualifiers, includeClassOrigin,
		propertyList);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyAssociatorProvider::associatorNames(
	const ProviderEnvironmentIFCRef& env,
	CIMObjectPathResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& assocClass,
	const String& resultClass,
	const String& role,
	const String& resultRole)
{
	m_pProv->updateAccessTime();
	m_pProv->associatorNames(env, result, ns, objectName, assocClass,
		resultClass, role, resultRole);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyAssociatorProvider::references(
	const ProviderEnvironmentIFCRef& env,
	CIMInstanceResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& resultClass,
	const String& role,
	WBEMFlags::EIncludeQualifiersFlag includeQualifiers,
	WBEMFlags::EIncludeClassOriginFlag includeClassOrigin,
	const StringArray* propertyList)
{
	m_pProv->updateAccessTime();
	m_pProv->references(env, result, ns, objectName, resultClass, role,
		includeQualifiers, includeClassOrigin, propertyList);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyAssociatorProvider::referenceNames(
	const ProviderEnvironmentIFCRef& env,
	CIMObjectPathResultHandlerIFC& result,
	const String& ns,
	const CIMObjectPath& objectName,
	const String& resultClass,
	const String& role)
{
	m_pProv->updateAccessTime();
	m_pProv->referenceNames(env, result, ns, objectName, resultClass, role);
}

//////////////////////////////////////////////////////////////////////////////
PyProxyMethodProvider::PyProxyMethodProvider(PyProviderRef pProv)
	: MethodProviderIFC()
	, m_pProv(pProv)
{
}

//////////////////////////////////////////////////////////////////////////////
CIMValue
PyProxyMethodProvider::invokeMethod(
	const ProviderEnvironmentIFCRef& env,
	const String& ns,
	const CIMObjectPath& path,
	const String& methodName,
	const CIMParamValueArray& in,
	CIMParamValueArray& out)
{
	m_pProv->updateAccessTime();
	return m_pProv->invokeMethod(env, ns, path, methodName, in, out);
}

//////////////////////////////////////////////////////////////////////////////
PyProxyIndicationProvider::PyProxyIndicationProvider(
	PyProviderRef pProv)
	: IndicationProviderIFC()
	, m_pProv(pProv)
{
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyIndicationProvider::activateFilter(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes)
{
	m_pProv->activateFilter(env, filter, eventType, nameSpace, classes);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyIndicationProvider::authorizeFilter(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes,
	const String& owner)
{
	m_pProv->authorizeFilter(env, filter, eventType, nameSpace, classes,
		owner);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyIndicationProvider::deActivateFilter(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes)
{
	m_pProv->deActivateFilter(env, filter, eventType, nameSpace, classes);
}

//////////////////////////////////////////////////////////////////////////////
int
PyProxyIndicationProvider::mustPoll(
	const ProviderEnvironmentIFCRef& env,
	const WQLSelectStatement& filter,
	const String& eventType,
	const String& nameSpace,
	const StringArray& classes)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
PyProxyIndicationExportProvider::PyProxyIndicationExportProvider(
	PyProviderRef pProv)
	: IndicationExportProviderIFC()
	, m_pProv(pProv)
{
}

//////////////////////////////////////////////////////////////////////////////
StringArray
PyProxyIndicationExportProvider::getHandlerClassNames()
{
	return m_pProv->getHandlerClassNames();
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyIndicationExportProvider::exportIndication(
	const ProviderEnvironmentIFCRef& env, 
	const String& ns,
	const CIMInstance& indHandlerInst,
	const CIMInstance& indicationInst)
{
	m_pProv->exportIndication(env, ns, indHandlerInst, indicationInst);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyIndicationExportProvider::doCooperativeCancel()
{
	// Do nothing
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyIndicationExportProvider::doDefinitiveCancel()
{
	// Do nothing
}

//////////////////////////////////////////////////////////////////////////////
PyProxyPolledProvider::PyProxyPolledProvider(PyProviderRef pProv)
	: PolledProviderIFC()
	, m_pProv(pProv)
{
}

//////////////////////////////////////////////////////////////////////////////
Int32
PyProxyPolledProvider::poll(const ProviderEnvironmentIFCRef& env)
{
	return m_pProv->poll(env);
}

//////////////////////////////////////////////////////////////////////////////
Int32
PyProxyPolledProvider::getInitialPollingInterval(
	const ProviderEnvironmentIFCRef& env)
{
	return m_pProv->getInitialPollingInterval(env);
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyPolledProvider::doCooperativeCancel()
{
	// Do nothing
}

//////////////////////////////////////////////////////////////////////////////
void
PyProxyPolledProvider::doDefinitiveCancel()
{
	// Do nothing
}

}	// End of namespace PythonProvIFC

