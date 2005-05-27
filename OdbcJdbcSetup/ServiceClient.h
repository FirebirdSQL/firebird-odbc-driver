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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// ServiceClient.h interface for the ServiceClient class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceClient_H_)
#define _ServiceClient_H_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;
using namespace IscDbcLibrary;

class CServiceClient
{
	HINSTANCE libraryHandle;

public:

	bool initServices( const char *sharedLibrary );
	bool checkVersion( void );
	bool createDatabase( void );
	void putParameterValue( const char * name, const char * value );

public:

	CServiceClient( void );
	~CServiceClient( void );

	ServiceManager *services;
	Properties *properties;
};
  
}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceClient_H_)
