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
