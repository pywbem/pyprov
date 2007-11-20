#ifndef PYMETHODPROVIDERHANDLDER_H_GUARD
#define PYMETHODPROVIDERHANDLDER_H_GUARD

#include "PythonProviderManager.h"

PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

class MethodProviderHandler
{
public:
    static CIMResponseMessage* handleInvokeMethodRequest(CIMRequestMessage* message, PyProviderRep& provrep, PythonProviderManager* pmgr);
};

}	// end of namespace PythonProvIFC

#endif	//PYMETHODPROVIDERHANDLDER_H_GUARD
