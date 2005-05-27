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

// ServiceManager.h: interface for the ServiceManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ServiceManager_H_)
#define _ServiceManager_H_

namespace IscDbcLibrary {

class CServiceManager : public ServiceManager
{
public:
	virtual Properties*	allocProperties();
	virtual int	getDriverBuildKey();

	void loadShareLibrary();
	virtual void addRef();
	virtual int release();

	CServiceManager();
	~CServiceManager();

	CFbDll		*GDS;
	Properties	*properties;
	int			useCount;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ServiceManager_H_)
