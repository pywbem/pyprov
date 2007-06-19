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
#ifndef OW_PYPROVIDERIFC_HPP_GUARD_
#define OW_PYPROVIDERIFC_HPP_GUARD_

#include "PyCxxObjects.hpp"
#include "OW_PyProviderModule.hpp"
#include "OW_PyProvider.hpp"
#include "OW_PyProvIFCCommon.hpp"
#include <openwbem/OW_config.h>
#include <openwbem/OW_ProviderIFCBaseIFC.hpp>
#include <openwbem/OW_Map.hpp>
#include <openwbem/OW_SharedLibrary.hpp>
//#include <openwbem/OW_CppProviderBaseIFC.hpp>
#include <openwbem/OW_MutexLock.hpp>

using namespace OW_NAMESPACE;

namespace PythonProvIFC
{

class PyProviderIFC : public ProviderIFCBaseIFC
{
public:
	PyProviderIFC();
	virtual ~PyProviderIFC();

	PyProviderModule* getPyProviderMod() const
	{
		return m_pyprovMod;
	}
	Py::Module getWBEMMod() const
	{
		return m_pywbemMod;
	}

protected:
	virtual const char* getName() const { return "python"; }

	/**
	 * The derived classes must override these functions to implement the
	 * desired functionality.
	 */
	virtual void doInit(const ProviderEnvironmentIFCRef& env,
		InstanceProviderInfoArray& i,
		SecondaryInstanceProviderInfoArray& si,
#ifndef OW_DISABLE_ASSOCIATION_TRAVERSAL
		AssociatorProviderInfoArray& a,
#endif
		MethodProviderInfoArray& m,
		IndicationProviderInfoArray& ind);

	virtual InstanceProviderIFCRef doGetInstanceProvider(
		const ProviderEnvironmentIFCRef& env, const char* provIdString);

	virtual SecondaryInstanceProviderIFCRef doGetSecondaryInstanceProvider(
		const ProviderEnvironmentIFCRef& env,
		const char* provIdString);

	virtual MethodProviderIFCRef doGetMethodProvider(
		const ProviderEnvironmentIFCRef& env,
		const char* provIdString);

#ifndef OW_DISABLE_ASSOCIATION_TRAVERSAL
	virtual AssociatorProviderIFCRef doGetAssociatorProvider(
		const ProviderEnvironmentIFCRef& env,
		const char* provIdString);
#endif

	virtual IndicationExportProviderIFCRefArray doGetIndicationExportProviders(
		const ProviderEnvironmentIFCRef& env);

	virtual PolledProviderIFCRefArray doGetPolledProviders(
		const ProviderEnvironmentIFCRef& env);

	virtual IndicationProviderIFCRef doGetIndicationProvider(
		const ProviderEnvironmentIFCRef& env, const char* provIdString);

	virtual void doUnloadProviders(const ProviderEnvironmentIFCRef& env);

	virtual void doShuttingDown(const ProviderEnvironmentIFCRef& env);

private:

	typedef Map<String, PyProviderRef> ProviderMap;
	typedef Map<String, String> ProvIdMap;

	void initPython(const ProviderEnvironmentIFCRef& env);
	void getTTLOption(const ProviderEnvironmentIFCRef& env);

	String getPath4Id(const String& provid);

	PyProviderRef getProvider(
		const ProviderEnvironmentIFCRef& env,
		const String& providerId,
		bool unloadableType=true);

	PyProviderModule* m_pyprovMod;
	Py::Module m_pywbemMod;
	Py::Module m_pycimMod;
	bool m_disabled;
	ProviderMap m_loadedProvsByPath;
	ProvIdMap m_idmap;
	PyThreadState* m_mainPyThreadState;
	Int32 m_provTTL;					// Provider TTL in minutes
	Mutex m_guard;
	bool m_pythonInitialized;
};

} // end namespace PythonProvIFC

#endif
