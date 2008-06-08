/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef __SQLERROR_H
#define __SQLERROR_H

#include "SQLException.h"
#include "JString.h"

namespace IscDbcLibrary {

class Stream;

class SQLError : public SQLException
{
public:
	virtual int release();
	virtual void addRef();
	virtual const char* getTrace();
	SQLError (int sqlcode, int fbcode, const char *text, ...);
	SQLError (SqlCode sqlcode, const char *text, ...);
	SQLError (Stream *trace, SqlCode code, const char *txt,...);
	~SQLError() throw();

	virtual int			getFbcode ();
	virtual int			getSqlcode ();
	virtual const char	*getText();

	//void		Delete();
	operator	const char*();

	int		fbcode;
	int		sqlcode;
	JString	text;
	JString	stackTrace;
	int		useCount;
    };

}; // end namespace IscDbcLibrary

#endif
