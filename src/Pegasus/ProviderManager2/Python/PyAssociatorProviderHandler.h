#ifndef PYASSOCIATORPROVIDERHANDLDER_H_GUARD
#define PYASSOCIATORPROVIDERHANDLDER_H_GUARD

#include "PythonProviderManager.h"

PEGASUS_USING_PEGASUS;

namespace PythonProvIFC
{

class AssociatorProviderHandler
{
public:
    static CIMResponseMessage* handleAssociatorsRequest(CIMRequestMessage* message, PyProviderRep& provrep, PythonProviderManager* pgmr);
    static CIMResponseMessage* handleAssociatorNamesRequest(CIMRequestMessage* message, PyProviderRep& provrep, PythonProviderManager* pgmr);
    static CIMResponseMessage* handleReferencesRequest(CIMRequestMessage* message, PyProviderRep& provrep, PythonProviderManager* pgmr);
    static CIMResponseMessage* handleReferenceNamesRequest(CIMRequestMessage* message, PyProviderRep& provrep, PythonProviderManager* pgmr);
};

}	// end of namespace PythonProvIFC

#endif	//PYASSOCIATORPROVIDERHANDLDER_H_GUARD
