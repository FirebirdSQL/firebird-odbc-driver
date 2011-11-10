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

// IscSqlType.h: interface for the IscSqlType class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCSQLTYPE_H_)
#define _ISCSQLTYPE_H_

#include <string.h>

namespace IscDbcLibrary {

class IscSqlType  
{
public:
	IscSqlType() { memset ( this, 0, sizeof ( *this ) ); }

	void buildType ();

public:		// In
	int		blrType;
	int		subType;
	int		lengthCharIn;
	int		lengthIn;
	int		dialect;
	int		precisionIn;
	int		scale;
	int		collationId;
	int		characterId;
	int		appOdbcVersion;

public:		// Out
	int		type;
	const char	*typeName;
	int		length;
	int		bufferLength;
	int		precision;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCSQLTYPE_H_)
