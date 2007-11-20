#ifndef PYINDICATIONPROVIDERHANDLDER_H_GUARD
#define PYINDICATIONPROVIDERHANDLDER_H_GUARD

#include "PythonProviderManager.h"

PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

class IndicationProviderHandler
{
public:
    static CIMResponseMessage* handleCreateSubscriptionRequest(CIMRequestMessage* message, 
															   PyProviderRep& provrep, 
															   PythonProviderManager* pmgr);
	static CIMResponseMessage* handleModifySubscriptionRequest(CIMRequestMessage* message, 
															   PyProviderRep& provrep, 
															   PythonProviderManager* pmgr);
	static CIMResponseMessage* handleDeleteSubscriptionRequest(CIMRequestMessage* message, 
															   PyProviderRep& provrep, 
															   PythonProviderManager* pmgr);
};

}	// end of namespace PythonProvIFC

#endif	//PYINDICATIONPROVIDERHANDLDER_H_GUARD
