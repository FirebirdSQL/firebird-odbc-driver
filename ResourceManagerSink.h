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
 *  Copyright (c) 2004 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// ResourceManagerSink.h interface for the ResourceManagerSink class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _WINDOWS

#if _MSC_VER > 1000

#if !defined(_ResourceManagerSink_H_)
#define _ResourceManagerSink_H_

namespace OdbcJdbcLibrary {

class ResourceManagerSink: public CComObjectRoot, public IResourceManagerSink
{
	TransactionResourceAsync * tranResourceAsync;

public:

BEGIN_COM_MAP( ResourceManagerSink )
	COM_INTERFACE_ENTRY( IResourceManagerSink )
END_COM_MAP()

	ResourceManagerSink( void );
	~ResourceManagerSink( void );

public:

	STDMETHODIMP TMDown( void );
	void setResourceAsync( TransactionResourceAsync * ptResAsync );

};
  
}; // end namespace OdbcJdbcLibrary

#endif // !defined(_ResourceManagerSink_H_)

#endif // _MSC_VER > 1000

#endif // _WINDOWS
