/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			SQLException.h
 *	DESCRIPTION:	SQL Exception object
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __SQLEXCEPTION_H
#define __SQLEXCEPTION_H

#include <exception>

#ifdef _WIN32xxx
#define DllExport	__declspec( dllexport )
#else
#define DllExport
#endif

namespace IscDbcLibrary {

enum SqlCode {
    SYNTAX_ERROR = -1,
	FEATURE_NOT_YET_IMPLEMENTED = -2,
	BUG_CHECK = -3,
	COMPILE_ERROR = -4,
	RUNTIME_ERROR = -5,
	OCS_ERROR = -6,
	NETWORK_ERROR = -7,
	CONVERSION_ERROR = -8,
	TRUNCATION_ERROR = -9,
	CONNECTION_ERROR = -10,
	DDL_ERROR = -11,
	APPLICATION_ERROR = -12,
	SECURITY_ERROR = -13,
	UNSUPPORTED_DATATYPE = -14,
	NO_RECORDS_FOR_FETCH = -508 // No current record for fetch operation 335544348L
	};

class DllExport SQLException : public std::exception
{
public:
	//virtual void		addRef() = 0;
	//virtual int		release() = 0;
	virtual int			getFbcode () = 0;
	virtual int			getSqlcode () = 0;
	virtual const char	*getText() = 0;
	virtual const char	*getTrace() = 0;
};

}; // end namespace IscDbcLibrary

#endif
