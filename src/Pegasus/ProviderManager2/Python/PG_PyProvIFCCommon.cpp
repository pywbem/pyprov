#include "PG_PyProvIFCCommon.h"
#include <Pegasus/Common/Formatter.h>
#include <Pegasus/Common/Logger.h>

using namespace Pegasus;

namespace PythonProvIFC
{

PyProviderIFCException::PyProviderIFCException()
	: Exception("PythonProviderIFCException Thrown")
{
}

PyProviderIFCException::PyProviderIFCException(const String& msg)
	: Exception(msg)
{
}

PyProviderIFCException::PyProviderIFCException(const String& file, int lineno, const String& msg)
	: Exception(Formatter::format("Msg: $0  File: $1  Line:$2", msg, file, lineno))
{
}

PyNoSuchMethodException::PyNoSuchMethodException()
	: Exception("PyNoSuchMethodException Thrown")
{
}

PyNoSuchMethodException::PyNoSuchMethodException(const String& msg)
	: Exception(msg)
{
}

PyNoSuchMethodException::PyNoSuchMethodException(const String& file, int lineno, const String& msg)
	: Exception(Formatter::format("Msg: $0  File: $1  Line:$2", msg, file, lineno))
{
}

PyNoSuchProviderException::PyNoSuchProviderException()
	: Exception("PyNoSuchProviderException Thrown")
{
}

PyNoSuchProviderException::PyNoSuchProviderException(const String& msg)
	: Exception(msg)
{
}

PyNoSuchProviderException::PyNoSuchProviderException(const String& file, int lineno, const String& msg)
	: Exception(Formatter::format("Msg: $0  File: $1  Line:$2", msg, file, lineno))
{
}

///////////////////////////////////////////////////////////////////////////////
void
throwCIMException(CIMStatusCode code, const String& msg, int lineno,
	const char* filename)
{
	throw CIMException(code, Formatter::format("$0 File: $1  Line: $2",
			msg, filename, lineno));
}

///////////////////////////////////////////////////////////////////////////////
String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno,
	Py::Object& etype,
	Py::Object& evalue,
	bool isError)
{
	String tb = Py::getCurrentErrorInfo(etype, evalue);
	if (isError)
	{
		Logger::put(Logger::STANDARD_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"Python exception value: $0", evalue.as_string());
		Logger::put(Logger::STANDARD_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"Python exception type: $0", etype.as_string());
		Logger::put(Logger::STANDARD_LOG, PYSYSTEM_ID, Logger::SEVERE,
			"Python trace : $0", tb);
	}
	else
	{
		Logger::put(Logger::DEBUG_LOG, PYSYSTEM_ID, Logger::TRACE,
			"Python exception value: $0", evalue.as_string());
		Logger::put(Logger::DEBUG_LOG, PYSYSTEM_ID, Logger::TRACE,
			"Python exception type: $0", etype.as_string());
		Logger::put(Logger::DEBUG_LOG, PYSYSTEM_ID, Logger::TRACE,
			"Python trace : $0", tb);
	}
	return tb;
}

///////////////////////////////////////////////////////////////////////////////
String
LogPyException(
	Py::Exception& thrownEx,
	const char* fileName,
	int lineno)
{
	Py::Object etype, evalue;
	return LogPyException(thrownEx, fileName, lineno, etype, evalue, true);
}


}	// End of namespace PythonProvIFC
