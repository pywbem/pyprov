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
#include "OW_PyProviderIFC.hpp"
#include "OW_PyConverter.hpp"
#include "OW_PyProviderModule.hpp"
#include "OW_PyProxyProvider.hpp"

#include <openwbem/OW_Format.hpp>
#include <openwbem/OW_ConfigOpts.hpp>
#include <openwbem/OW_FileSystem.hpp>
#include <openwbem/OW_NoSuchProviderException.hpp>
#include <openwbem/OW_Assertion.hpp>
#include <openwbem/OW_IntrusiveCountableBase.hpp>
#include <openwbem/OW_NonRecursiveMutex.hpp>
#include <openwbem/OW_NonRecursiveMutexLock.hpp>
#include <openwbem/OW_Condition.hpp>
#include <openwbem/OW_ProviderFwd.hpp>
#include <openwbem/OW_RepositoryIFC.hpp>
#include <openwbem/OW_ResultHandlers.hpp>
#include <openwbem/OW_ConfigOpts.hpp>
#include <openwbem/OW_CIMException.hpp>

#include <cstdlib>
#include <iostream>
using std::cout;
using std::endl;

#define OW_DEFAULT_PYPROVIFC_PROV_LOCATION OW_DEFAULT_OWLIBDIR"/pythonproviders"
#define OW_DEFAULT_PYPROVIFC_PROV_TTL "5"
static const char* const PYPROVIFC_PROV_LOCATION_opt = "pyprovifc.prov_location";
static const char* const PYPROVIFC_PROV_TTL_opt = "pyprovifc.prov_TTL";

using namespace OW_NAMESPACE;
using namespace WBEMFlags;

namespace PythonProvIFC
{

OW_DEFINE_EXCEPTION(PyProviderIFC);

namespace
{

PyProviderReg
getProviderReg(
	const ProviderEnvironmentIFCRef& env,
	const String& provid)
{
	// Get instance of provider registration
    String interopNs = env->getConfigItem(ConfigOpts::INTEROP_SCHEMA_NAMESPACE_opt,
		OW_DEFAULT_INTEROP_SCHEMA_NAMESPACE);
	CIMInstance ci(CIMNULL);
	try
	{
		CIMObjectPath cop("OpenWBEM_PyProviderRegistration", interopNs);
		cop.setKeyValue("InstanceID", CIMValue(provid));
		ci = env->getCIMOMHandle()->getInstance(interopNs, cop);
		return PyProviderReg(ci);
	}
	catch (const CIMException& e)
	{
		// Ignore
	}
	return PyProviderReg();
}

}	// End of unnamed namespace


//////////////////////////////////////////////////////////////////////////////
PyProviderIFC::PyProviderIFC()
	: ProviderIFCBaseIFC()
	, m_pywbemMod()
	, m_pycimMod()
	, m_disabled(false)
	, m_loadedProvsByPath()
	, m_idmap()
	, m_mainPyThreadState(0)
	, m_provTTL(String(OW_DEFAULT_PYPROVIFC_PROV_TTL).toInt32())
	, m_guard()
	, m_pythonInitialized(false)
{
}

//////////////////////////////////////////////////////////////////////////////
PyProviderIFC::~PyProviderIFC()
{
	// We have to shutdown python here, because openwbem shuts down the
	// polling manager after it shuts down the provider interfaces.
	// If python is shutdown in the doShuttingDown method, there will
	// be a segfault when the python polled provider destructor gets
	// called.
	if (m_pythonInitialized)
	{
		PyEval_AcquireLock();
		PyThreadState_Swap(m_mainPyThreadState);
		Py_Finalize();
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProviderIFC::doInit(
	const ProviderEnvironmentIFCRef& env,
	InstanceProviderInfoArray& ipia,
	SecondaryInstanceProviderInfoArray& si,
	AssociatorProviderInfoArray& apia,
	MethodProviderInfoArray& mpia,
	IndicationProviderInfoArray& indpia)
{
	LoggerRef logger = myLogger(env);
	OW_LOG_DEBUG(logger, "PyProviderIFC::doInit called..");

	getTTLOption(env);
	initPython(env);
	if (m_disabled)
	{
		// Python must have failed to initialize. No use registering
		// any providers
		return;
	}

	// Now enumerate instances of OpenWBEM_PyProvider in the interop namespace
	// to get provider registration information
    String interopNs = env->getConfigItem(ConfigOpts::INTEROP_SCHEMA_NAMESPACE_opt,
		OW_DEFAULT_INTEROP_SCHEMA_NAMESPACE);
	CIMInstanceArray regs;
	try
	{
		regs = env->getCIMOMHandle()->enumInstancesA(interopNs, "OpenWBEM_PyProviderRegistration");

	}
	catch (const CIMException& e)
	{
		OW_LOG_INFO(logger, Format("PyProviderIFC::doInit() caught exception "
			"(%1) while enumerating instances of OpenWBEM_PyProvider in "
			"in namespace %2", e, interopNs));
		return;
	}

	if (!regs.size())
	{
		OW_LOG_INFO(logger, Format("PyProviderIFC::doInit() did not find any "
			"provider registrations in namespace %1", interopNs));
		return;
	}
	int instsize = int(regs.size());
	for (int i = 0; i < instsize; i++)
	{
		PyProviderReg reg(regs[i]);
		String provid = reg.getInstanceId();
		UInt16Array providerTypes = reg.getProviderTypes();
		if (providerTypes.empty())
		{
			OW_LOG_ERROR(logger, Format("PyProviderIFC no provider types in "
				"registration for provider %1", provid));
			continue;
		}
		StringArray namespaces = reg.getNameSpaceNames();
		String className = reg.getClassName();
		for (size_t pti = 0; pti < providerTypes.size(); pti++)
		{
			switch(providerTypes[pti])
			{
				case PyProviderReg::E_INSTANCE:
				{
					InstanceProviderInfo ipi;
					ipi.setProviderName(provid);
					InstanceProviderInfo::ClassInfo classInfo(className, namespaces);
					ipi.addInstrumentedClass(classInfo);
					ipia.append(ipi);
					break;
				}
				case PyProviderReg::E_ASSOCIATION:
				{
					AssociatorProviderInfo api;
					api.setProviderName(provid);
					AssociatorProviderInfo::ClassInfo classInfo(className, namespaces);
					api.addInstrumentedClass(classInfo);
					apia.append(api);
					break;
				}
				case PyProviderReg::E_LIFECYCLE_INDICATION:
				{
					IndicationProviderInfo ipi;
					ipi.setProviderName(provid);
					const char* instanceLifeCycleIndicationClassNames[] =
						{ "CIM_InstCreation", "CIM_InstModification",
						  "CIM_InstDeletion", "CIM_InstIndication",
						  "CIM_Indication", 0 };
					for (const char** pIndicationClassName = instanceLifeCycleIndicationClassNames;
						*pIndicationClassName != 0; ++pIndicationClassName)
					{
						const char* indicationClassName = *pIndicationClassName;
						IndicationProviderInfoEntry e(indicationClassName, namespaces,
							StringArray(1, className));
						ipi.addInstrumentedClass(e);
					}
					indpia.append(ipi);
					break;
				}
				case PyProviderReg::E_ALERT_INDICATION:
				{
					OW_LOG_DEBUG(logger, Format("Python Provider IFC "
						"registering Alert indication provider %1 for class %2",
						provid, className));

					IndicationProviderInfo ipi;
					ipi.setProviderName(provid);
					IndicationProviderInfo::ClassInfo classInfo(className, namespaces);
					ipi.addInstrumentedClass(classInfo);
					indpia.append(ipi);
					break;
				}
				case PyProviderReg::E_METHOD:
				{
					MethodProviderInfo mpi;
					mpi.setProviderName(provid);
					StringArray methods = reg.getMethodNames();
					MethodProviderInfo::ClassInfo classInfo(className, namespaces, methods);
					mpi.addInstrumentedClass(classInfo);
					mpia.append(mpi);
					break;
				}
				case PyProviderReg::E_INDICATION_HANDLER:	// Not reported here
				case PyProviderReg::E_POLLED:				// Not reported here
					break;
				case PyProviderReg::E_SECONDARY_INSTANCE:	// Not supported
				default:
					OW_LOG_ERROR(logger, Format("PyProviderIFC encountered "
						"unsupported provider type %1 in registration for "
						"provider %2", providerTypes[pti], provid));
					break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProviderIFC::getTTLOption(
	const ProviderEnvironmentIFCRef& env)
{
	LoggerRef logger = myLogger(env);
	String ttlOpt = env->getConfigItem(PYPROVIFC_PROV_TTL_opt, "");
	if (ttlOpt.empty())
	{
		// m_provTTL was set in CTOR of PyProviderIFC
		OW_LOG_INFO(logger, Format("Python provider TTL not specified "
			"in options file. Set to default of %1", m_provTTL));
	}
	else
	{
		try
		{
			m_provTTL = ttlOpt.toInt32();
			if (m_provTTL < 1)
			{
				OW_LOG_INFO(logger, Format("Python provider TTL set to %1 "
					"through configuration file. Python provider will stay "
					"loaded", m_provTTL));
			}
			else
			{
				OW_LOG_INFO(logger, Format("Python provider TTL set to %1 "
					"minutes through configuration file", m_provTTL));
			}
		}
		catch(const StringConversionException&)
		{
			// m_provTTL was set in CTOR of PyProviderIFC

			OW_LOG_ERROR(logger, Format("Invalid Python provider TTL in "
				"options file: %1 Defaulting to %2 minutes",
				ttlOpt, m_provTTL));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProviderIFC::initPython(
	const ProviderEnvironmentIFCRef& env)
{
	LoggerRef logger = myLogger(env);
	String confpath = env->getConfigItem(PYPROVIFC_PROV_LOCATION_opt,
		OW_DEFAULT_PYPROVIFC_PROV_LOCATION);

	OW_LOG_DEBUG(logger, "Python provider ifc initializing python...");

	// Initialize embedded python interpreter
	Py_Initialize();
	PyEval_InitThreads();
	// Save thread state for shutdown
	m_mainPyThreadState = PyThreadState_Get();
	// Release the lock acquired by PyEval_InitThreads
	PyEval_ReleaseLock();
	m_pythonInitialized = true;

	GILGuard gg;	// Acquire python's GIL

	try
	{	
		// Load the pywbem module for use in interacting
		// with python providers
		OW_LOG_DEBUG(logger, "Python provider ifc loading pywbem module...");
		m_pywbemMod = Py::Module("pywbem", true);	// load pywbem module
		OWPyConv::setPyWbemMod(m_pywbemMod);
		PyProvider::setPyWbemMod(m_pywbemMod);
	}
	catch (Py::Exception& e)
	{
		m_disabled = true;
		String msg = "Python provider ifc caught exception "
			"loading pywbem module:";
		OW_LOG_ERROR(logger, msg);
		String tb = LogPyException(e, __FILE__, __LINE__, logger);
		e.clear();
		msg += tb;
		OW_THROW(PyProviderIFCException, msg.c_str());
	}

	try
	{
		// Initialize the pycimmb module
		// (python provider support module)
		OW_LOG_DEBUG(logger, "Python provider ifc initializing the "
			"pycimmb module...");
		PyProviderModule::doInit(m_pywbemMod);
		m_pycimMod = Py::Module("pycim", true);
		PyProvider::setCIMProvMod(m_pycimMod);
	}
	catch (Py::Exception& e)
	{
		m_disabled = true;
		String msg = "Python provider ifc caught exception "
			"initializing pycim module:";
		OW_LOG_ERROR(logger, msg);
		String tb = LogPyException(e, __FILE__, __LINE__, logger);
		e.clear();
		msg += tb;
		OW_THROW(PyProviderIFCException, msg.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////////
InstanceProviderIFCRef
PyProviderIFC::doGetInstanceProvider(
	const ProviderEnvironmentIFCRef& env,
	const char* provIdString)
{
	if (m_disabled)
	{
		OW_THROW(NoSuchProviderException, provIdString);
	}

	OW_LOG_DEBUG(myLogger(env),
		Format("PyProviderIFC::doGetInstanceProvider called with "
			"provIdString: %1", provIdString));

	PyProviderRef pref = getProvider(env, provIdString);
	return InstanceProviderIFCRef(new PyProxyInstanceProvider(pref));
}

//////////////////////////////////////////////////////////////////////////////
SecondaryInstanceProviderIFCRef
PyProviderIFC::doGetSecondaryInstanceProvider(
	const ProviderEnvironmentIFCRef& env,
	const char* provIdString)
{
	if (m_disabled)
	{
		OW_THROW(NoSuchProviderException, provIdString);
	}
	OW_LOG_DEBUG(myLogger(env),
		Format("PyProviderIFC::doGetSecondaryInstanceProvider called with "
			"provIdString: %1 -- Not Currently Supported", provIdString));

	return SecondaryInstanceProviderIFCRef(0);
}

//////////////////////////////////////////////////////////////////////////////
MethodProviderIFCRef
PyProviderIFC::doGetMethodProvider(
	const ProviderEnvironmentIFCRef& env,
	const char* provIdString)
{
	if (m_disabled)
	{
		OW_THROW(NoSuchProviderException, provIdString);
	}

	OW_LOG_DEBUG(myLogger(env),
		Format("PyProviderIFC::doGetMethodProvider called with "
			"provIdString: %1", provIdString));

	PyProviderRef pref = getProvider(env, provIdString);
	return MethodProviderIFCRef(new PyProxyMethodProvider(pref));
}

//////////////////////////////////////////////////////////////////////////////
AssociatorProviderIFCRef
PyProviderIFC::doGetAssociatorProvider(
	const ProviderEnvironmentIFCRef& env,
	const char* provIdString)
{
	if (m_disabled)
	{
		OW_THROW(NoSuchProviderException, provIdString);
	}

	OW_LOG_DEBUG(myLogger(env),
		Format("PyProviderIFC::doGetAssociatorProvider called with "
			"provIdString: %1", provIdString));

	PyProviderRef pref = getProvider(env, provIdString);
	return AssociatorProviderIFCRef(new PyProxyAssociatorProvider(pref));
}

//////////////////////////////////////////////////////////////////////////////
IndicationExportProviderIFCRefArray
PyProviderIFC::doGetIndicationExportProviders(
	const ProviderEnvironmentIFCRef& env)
{
	LoggerRef logger = myLogger(env);
	OW_LOG_DEBUG(logger,
		"PyProviderIFC::doGetIndicationExportProviders called...");

	if (m_disabled)
	{
		OW_LOG_INFO(logger, 
			"PyProviderIFC is disabled. Return NO indication "
			"export providers");
		return IndicationExportProviderIFCRefArray();
	}

	IndicationExportProviderIFCRefArray provra;
    String interopNs = env->getConfigItem(ConfigOpts::INTEROP_SCHEMA_NAMESPACE_opt,
		OW_DEFAULT_INTEROP_SCHEMA_NAMESPACE);
	CIMInstanceArray regs;
	try
	{
		regs = env->getCIMOMHandle()->enumInstancesA(interopNs, "OpenWBEM_PyProviderRegistration");
	}
	catch (const CIMException& e)
	{
		OW_LOG_INFO(logger,
			Format("PyProviderIFC::doGetIndicationExportProviders() caught "
			"exception (%1) while enumerating instances of "
			"OpenWBEM_PyProvider in in namespace %2", e, interopNs));
		return provra;
	}

	if (!regs.size())
	{
		OW_LOG_INFO(logger,
			Format("PyProviderIFC::doGetIndicationExportProviders() "
			"did not find any provider registrations in namespace %1",
				interopNs));
		return provra;
	}
	int instsize = int(regs.size());
	for (int i = 0; i < instsize; i++)
	{
		PyProviderReg reg(regs[i]);
		String provid = reg.getInstanceId();
		UInt16Array providerTypes = reg.getProviderTypes();
		if (providerTypes.empty())
		{
			OW_LOG_ERROR(logger, Format("PyProviderIFC no provider types in "
				"registration for provider %1", provid));
			continue;
		}

		for (size_t pti = 0; pti < providerTypes.size(); pti++)
		{
			if (providerTypes[pti] == PyProviderReg::E_INDICATION_HANDLER)
			{
				StringArray handlerClassNames = reg.getExportHandlerClassNames();
				if (handlerClassNames.empty())
				{
					OW_LOG_ERROR(logger, Format("PyProviderIFC no handler class "
						"names in registration for IndicationHandlerProvider %1 "
						"not registering", provid));
				}

				try
				{
					PyProviderRef pref = getProvider(env, provid, false);
					pref->setHandlerClassNames(handlerClassNames);
					provra.append(IndicationExportProviderIFCRef(new PyProxyIndicationExportProvider(pref)));
				}
				catch(const Exception& e)
				{
					OW_LOG_INFO(logger,
						Format("PyProviderIFC::doGetIndicationExportProviders() caught "
						"exception (%1) while loading provider %2", e, provid));
				}
				break;
			}
		}
	}
	OW_LOG_DEBUG(logger,
		Format("PyProviderIFC::doGetIndicationExportProviders "
			"return %1 providers", provra.size()));
	return provra;
}

//////////////////////////////////////////////////////////////////////////////
PolledProviderIFCRefArray
PyProviderIFC::doGetPolledProviders(
	const ProviderEnvironmentIFCRef& env)
{
	LoggerRef logger = myLogger(env);
	OW_LOG_DEBUG(logger,
		"PyProviderIFC::doGetPolledProviders called...");

	if (m_disabled)
	{
		OW_LOG_INFO(logger, 
			"PyProviderIFC is disabled. Return NO polled providers");
		return PolledProviderIFCRefArray();
	}

	PolledProviderIFCRefArray provra;
    String interopNs = env->getConfigItem(ConfigOpts::INTEROP_SCHEMA_NAMESPACE_opt,
		OW_DEFAULT_INTEROP_SCHEMA_NAMESPACE);
	CIMInstanceArray regs;
	try
	{
		regs = env->getCIMOMHandle()->enumInstancesA(interopNs, "OpenWBEM_PyProviderRegistration");
	}
	catch (const CIMException& e)
	{
		OW_LOG_INFO(logger,
			Format("PyProviderIFC::doGetPolledProviders() caught "
			"exception (%1) while enumerating instances of "
			"OpenWBEM_PyProvider in in namespace %2", e, interopNs));
		return provra;
	}

	if (!regs.size())
	{
		OW_LOG_INFO(logger,
			Format("PyProviderIFC::doGetPolledProviders() "
			"did not find any provider registrations in namespace %1",
				interopNs));
		return provra;
	}
	int instsize = int(regs.size());
	for (int i = 0; i < instsize; i++)
	{
		PyProviderReg reg(regs[i]);
		String provid = reg.getInstanceId();
		UInt16Array providerTypes = reg.getProviderTypes();
		if (providerTypes.empty())
		{
			OW_LOG_ERROR(logger, Format("PyProviderIFC no provider types in "
				"registration for provider %1", provid));
			continue;
		}

		for (size_t pti = 0; pti < providerTypes.size(); pti++)
		{
			if (providerTypes[pti] == PyProviderReg::E_POLLED)
			{
				try
				{
					PyProviderRef pref = getProvider(env, provid, false);
					provra.append(PolledProviderIFCRef(new PyProxyPolledProvider(pref)));
				}
				catch(const Exception& e)
				{
					OW_LOG_INFO(logger,
						Format("PyProviderIFC::doGetPolledProviders() caught "
						"exception (%1) while loading provider %2", e, provid));
				}
				break;
			}
		}
	}

	OW_LOG_DEBUG(logger, Format("PyProviderIFC::doGetPolledProviders "
		"returning %1 providers", provra.size()));
	return provra;
}

//////////////////////////////////////////////////////////////////////////////
IndicationProviderIFCRef
PyProviderIFC::doGetIndicationProvider(
	const ProviderEnvironmentIFCRef& env,
	const char* provIdString)
{
	if (m_disabled)
	{
		OW_THROW(NoSuchProviderException, provIdString);
	}

	OW_LOG_DEBUG(myLogger(env),
		Format("PyProviderIFC::doGetIndicationProvider called with "
			"provIdString: %1", provIdString));

	PyProviderRef pref = getProvider(env, provIdString, false);
	return IndicationProviderIFCRef(new PyProxyIndicationProvider(pref));
}

//////////////////////////////////////////////////////////////////////////////
String
PyProviderIFC::getPath4Id(
	const String& provid)
{
	// Assumes m_guard is acquired
	String pypath;
	ProvIdMap::iterator idit = m_idmap.find(provid);
	if (idit != m_idmap.end())
	{
		pypath = idit->second;
	}
	return pypath;
}

//////////////////////////////////////////////////////////////////////////////
PyProviderRef
PyProviderIFC::getProvider(
	const ProviderEnvironmentIFCRef& env,
	const String& providerId,
	bool unloadableType)
{
	LoggerRef logger = myLogger(env);

	OW_LOG_DEBUG(logger,
		Format("PyProviderIFC getProvider called with provider ID %1",
			providerId));

	MutexLock ml(m_guard);
	PyProviderReg reg;
	// See if we already know about this provider id
	String pypath = getPath4Id(providerId);
	if (pypath.empty())
	{
		// No. lets get the instance so we can determine the python module
		reg = getProviderReg(env, providerId);
		if (reg.isNull())
		{
			OW_THROW(NoSuchProviderException, providerId.c_str());
		}

		pypath = reg.getModPath();
		if (pypath.empty())
		{
			OW_THROW(NoSuchProviderException, Format("Python provider registration "
				"%1 has not ModulePath property", providerId).c_str());
		}
	}

	// See if we have the python module loaded
	ProviderMap::iterator it = m_loadedProvsByPath.find(pypath);
	if (it != m_loadedProvsByPath.end())
	{
		// Provider is loaded
		PyProviderRef pref = it->second;
		if (pref->providerChanged() == false
			|| pref->isUnloadableType() == false)
		{
			// Provider has changed. force reload
			OW_LOG_DEBUG(logger,
				Format("PyProviderIFC getProvider. provider ID %1 already "
					"loaded. returning", providerId));
			if (pref->isUnloadableType() && !unloadableType)
			{
				pref->setUnloadableType(false);
			}
			// Associate this module to this provider id
			m_idmap[providerId] = pypath;
			return pref;
		}

		String fname = pref->getFileName();
		OW_LOG_DEBUG(logger, Format("PyProviderIFC detected change in "
			"provider %1  File: %2. Reloading...", 
				pref->getName(), fname));
		pref->shutDown(env);
		m_loadedProvsByPath.erase(it);
		m_idmap.erase(fname);
	}

	// OK. At this point we have the python module path and
	// we know it needs to be loaded

	OW_LOG_DEBUG(logger,
		Format("PyProviderIFC loading provider %1 from %2",
			providerId, pypath));

	PyProviderRef pref = new PyProvider(pypath, env, unloadableType);
	m_loadedProvsByPath[pypath] = pref;
	m_idmap[providerId] = pypath;

	OW_LOG_DEBUG(logger,
		Format("PyProviderIFC loaded provider %1 from file %2",
			providerId, pypath));

	return pref;
}

//////////////////////////////////////////////////////////////////////////////
void
PyProviderIFC::doUnloadProviders(
	const ProviderEnvironmentIFCRef& env)
{
	if (m_disabled || m_provTTL < 1)
	{
		return;
	}

	LoggerRef logger = myLogger(env);

	MutexLock ml(m_guard);
	DateTime dt;
	dt.setToCurrent();
	ProviderMap::iterator it = m_loadedProvsByPath.begin();
	while (it != m_loadedProvsByPath.end())
	{
		PyProviderRef pref = it->second;
		// Only do an unload here if it is not an 
		// indication/indicationexport/polled provider	
		if (pref->isUnloadableType())
		{
			String pname = pref->getName();
			DateTime provDt = pref->getLastAccessTime();
			provDt.addMinutes(m_provTTL);
			if (provDt < dt)
			{
				try
				{
					if (pref->canShutDown(env))
					{
						OW_LOG_DEBUG(logger, Format("PyProviderIFC unloading "
							"provider %1 because it has been inactive for "
							"more than %2 minutes", pname, m_provTTL));
						pref->shutDown(env);
						String fname = it->second->getFileName();
						m_idmap.erase(fname);
						m_loadedProvsByPath.erase(it++);
						OW_LOG_DEBUG(logger, Format("PyProviderIFC. provider "
							"%1 unloaded", pname));
						continue;
					}
				}
				catch(...)
				{
					// Ignore?
				}
			}
		}
		++it;
	}
}

//////////////////////////////////////////////////////////////////////////////
void
PyProviderIFC::doShuttingDown(
	const ProviderEnvironmentIFCRef& env)
{
	LoggerRef logger = myLogger(env);
	OW_LOG_DEBUG(logger,
		"PyProviderIFC::doShuttingDown called. Shutting down providers...");

	MutexLock ml(m_guard);

	ProviderMap::iterator it = m_loadedProvsByPath.begin();
	while (it != m_loadedProvsByPath.end())
	{
		PyProviderRef pref = it->second;
		try
		{
			pref->shutDown(env);
		}
		catch(Py::Exception& e)
		{
			OW_LOG_ERROR(logger, Format("Python provider ifc caught "
				"exception shutting down provider %1", pref->getName()));
			String tb = LogPyException(e, __FILE__, __LINE__, logger);
			e.clear();
		}
		catch(...)
		{
			OW_LOG_ERROR(logger, Format("Python provider ifc caught "
				"UNKNOWN exception shutting down provider %1",
				pref->getName()));
			// Ignore?
		}
		it++;
	}
	m_loadedProvsByPath.clear();
	m_idmap.clear();

	// Note: Python gets shutdown in the PyProviderIFC destructor
}

} // end namespace PythonProvIFC

//////////////////////////////////////////////////////////////////////////////
OW_PROVIDERIFCFACTORY(PythonProvIFC::PyProviderIFC, python)



