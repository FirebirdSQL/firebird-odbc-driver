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

// ServiceTabRestore.h interface for the Service Restore class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabRestore_h_)
#define _ServiceTabRestore_h_

namespace OdbcJdbcSetupLibrary {

/////////////////////////////////////////////////////////////////////////////
// CServiceTabRestore dialog

class CServiceTabRestore : public CServiceTabChild
{
	enum enumRestoreParameters
	{ 	
		enMetadataOnly       = 0x0004,
		enDeactivateIndexes  = 0x0100,
		enNoShadow           = 0x0200,
		enNoValidityCheck    = 0x0400,
		enOneRelationAtATime = 0x0800,
		enReplace            = 0x1000, // if not then enCreateNewDB = 0x2000,
		enUseAllSpace        = 0x4000
	};

	enum enumRestoreExecutedPart
	{ 	
		enDomains            = 0x0001,
		enTables             = 0x0002,
		enFunctions          = 0x0004,
		enGenerators         = 0x0008,
		enStoredProcedures   = 0x0010,
		enExceptions         = 0x0020,
		enDataForTables      = 0x0040,
		enTriggers           = 0x0080,
		enPrivileges         = 0x0100,
		enSqlRoles           = 0x0200
	};

public:
	CServiceTabRestore();
	~CServiceTabRestore();

public:
	void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool onCommand( HWND hWnd, int nCommand );
	void addParameters( CServiceClient &services );
	void onStartRestore();
	bool OnFindFileBackup( void );
	bool createDialogIndirect( CServiceTabCtrl *parentTabCtrl );
	bool buildDlgChild( HWND hWndParent );

public:
	ULONG   restoreParameters;
	JString	pageSize;
	JString	buffersSize;
	bool    noReadOnly;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabRestore_h_)
