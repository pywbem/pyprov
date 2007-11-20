#ifndef Pegasus_PythonProviderManager_h
#define Pegasus_PythonProviderManager_h

#include "PyCxxObjects.h"
#include "PG_PyExtensions.h"
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/HashTable.h>
#include <Pegasus/ProviderManager2/ProviderName.h>
#include <Pegasus/ProviderManager2/ProviderManager.h>
#include <Pegasus/Common/OperationContextInternal.h>
#include <Pegasus/Config/ConfigManager.h>
#include <Pegasus/ProviderManager2/OperationResponseHandler.h>
#include <Pegasus/ProviderManager2/Python/Linkage.h>
#include <Pegasus/Provider/CIMOMHandleQueryContext.h>

#include "PyCxxObjects.h"
#include "PG_PyExtensions.h"

#include <ctime>
#include <map>

PEGASUS_USING_PEGASUS;
PEGASUS_USING_STD;

namespace PythonProvIFC
{

struct Python_SelectExp;


struct PyProviderRep
{
	PyProviderRep()
		: m_path()
		, m_pyprov(Py::None())
		, m_canUnload(true)
		, m_lastAccessTime(time_t(0))
		, m_fileModTime(time_t(0))
		, m_activationCount(0)
	{
	}

	PyProviderRep(const String& path, const Py::Object pyprov,
		bool canUnload=true)
		: m_path(path)
		, m_pyprov(pyprov)
		, m_canUnload(canUnload)
		, m_lastAccessTime(time_t(0))
		, m_fileModTime(time_t(0))
		, m_activationCount(0)
	{
	}

	PyProviderRep(const PyProviderRep& arg)
		: m_path(arg.m_path)
		, m_pyprov(arg.m_pyprov)
		, m_canUnload(arg.m_canUnload)
		, m_lastAccessTime(arg.m_lastAccessTime)
		, m_fileModTime(arg.m_fileModTime)
		, m_activationCount(arg.m_activationCount)
	{
	}

	PyProviderRep& operator= (const PyProviderRep& arg)
	{
		m_path = arg.m_path;
		m_pyprov = arg.m_pyprov;
		m_canUnload = arg.m_canUnload;
		m_lastAccessTime = arg.m_lastAccessTime;
		m_fileModTime = arg.m_fileModTime;
		m_activationCount = arg.m_activationCount;
		return *this;
	}

	~PyProviderRep()
	{
		m_pyprov.release();
	}

	String m_path;
	Py::Object m_pyprov;
	bool m_canUnload;
	time_t m_lastAccessTime;
	time_t m_fileModTime;
	int m_activationCount;
};

typedef std::map<String, PyProviderRep> ProviderMap;

class PEGASUS_PYTHONPM_LINKAGE PythonProviderManager : public ProviderManager
{
public:

    PythonProviderManager();
    virtual ~PythonProviderManager();

	PyExtensions* getPyExtensionsMod() const
	{
		return m_pPyExtensions;
	}
	Py::Module getWBEMMod() const
	{
		return m_pywbemMod;
	}

    virtual Message * processMessage(Message * request);

    virtual Boolean hasActiveProviders();
    virtual void unloadIdleProviders();

    virtual Boolean supportsRemoteNameSpaces()
    {
        return true;
    }

    struct indProvRecord
    {
        indProvRecord() : enabled(false), count(1), handler(NULL)
        {
        }
        Boolean enabled;
        int count;
#ifdef PEGASUS_ENABLE_REMOTE_PYTHON
        String remoteInfo; 
#endif
        EnableIndicationsResponseHandler* handler;
    };

    struct indSelectRecord
    {
        indSelectRecord() : eSelx(NULL), count(1)
        {
        }
        Python_SelectExp *eSelx;
        CIMOMHandleQueryContext *qContext;
        int count;
    };

    typedef HashTable<String,indProvRecord*, \
        EqualFunc<String>,HashFunc<String> > IndProvTab;

    typedef HashTable<CIMObjectPath,indSelectRecord*, \
        EqualFunc<CIMObjectPath>,HashFunc<CIMObjectPath> > IndSelectTab;

protected:

    CIMResponseMessage* _handleUnsupportedRequest(CIMRequestMessage * message, PyProviderRep& provrep);
    CIMResponseMessage* _handleExecQueryRequest(CIMRequestMessage * message, PyProviderRep& provrep);

    CIMResponseMessage* _handleCreateSubscriptionRequest(CIMRequestMessage * message, PyProviderRep& provrep);
//    CIMResponseMessage* handleModifySubscriptionRequest(const Message * message, PyProviderRep& provrep);
    CIMResponseMessage* _handleDeleteSubscriptionRequest(CIMRequestMessage * message, PyProviderRep& provrep);

    CIMResponseMessage* _handleExportIndicationRequest(CIMRequestMessage * message, PyProviderRep& provrep);

    CIMResponseMessage* _handleDisableModuleRequest(CIMRequestMessage * message, PyProviderRep& provrep);
    CIMResponseMessage* _handleEnableModuleRequest(CIMRequestMessage * message, PyProviderRep& provrep);
    CIMResponseMessage* _handleStopAllProvidersRequest(CIMRequestMessage * message, PyProviderRep& provrep);
//  Note: The PG_Provider AutoStart property is not yet supported
#if 0
    CIMResponseMessage* _handleInitializeProviderRequest(const Message * message, PyProviderRep& provrep);
#endif
    CIMResponseMessage* _handleSubscriptionInitCompleteRequest (CIMRequestMessage * message, PyProviderRep& provrep);

    ProviderName _resolveProviderName(const ProviderIdContainer & providerId);

	String _resolvePhysicalName(String physicalName);

	void _initPython();

private:

	Py::Object _loadProvider(const String& provPath,
		const OperationContext& opctx);

	void _shutdownProvider(const PyProviderRep& provrep,
		const OperationContext& opctx);

	PyProviderRep _path2PyProviderRep(const String& provPath,
		const OperationContext& opctx);

	void _incActivationCount(PyProviderRep& provrep);
	void _decActivationCount(PyProviderRep& provrep);

	Py::Module m_pywbemMod;
	Py::Object m_cimexobj;
	PyExtensions* m_pPyExtensions;
	ProviderMap m_provs;
	PyThreadState* m_mainPyThreadState;

	friend class InstanceProviderHandler;
	friend class MethodProviderHandler;
	friend class AssociatorProviderHandler;
	friend class IndicationConsumerProviderHandler;
	friend class IndicationProviderHandler;
};

bool strEndsWith(const String& src, const String& tok);
String getPyFile(const String& fname);
Py::Object getPyPropertyList(const CIMPropertyList& pgpropList);
time_t getModTime(const String& fileName);
String getFunctionName(const String& fnameArg);
Py::Callable getFunction(const Py::Object& obj, const String& fnameArg);
String processPyException(Py::Exception& thrownEx, int lineno, 
	const String& provPath, OperationResponseHandler* pHandler=0);

}	// End of namespace PythonProvIFC

#endif

