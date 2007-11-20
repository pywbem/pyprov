#ifndef PYINDCONSUMERPROVIDERHANDLDER_H_GUARD
#define PYINDCONSUMERPROVIDERHANDLDER_H_GUARD

#include "PythonProviderManager.h"

PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

class IndicationConsumerProviderHandler
{
public:
    static CIMResponseMessage* handleExportIndicationRequest(
		CIMRequestMessage* message, 
		PyProviderRep& provrep,
		PythonProviderManager* pmgr);
};

}	// end of namespace PythonProvIFC

#endif	//PYINDCONSUMERPROVIDERHANDLDER_H_GUARD
