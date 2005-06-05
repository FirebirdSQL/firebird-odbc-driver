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

// ServiceTabBackup.h interface for the Service Backup class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabBackup_h_)
#define _ServiceTabBackup_h_

namespace OdbcJdbcSetupLibrary {

/////////////////////////////////////////////////////////////////////////////
// CServiceTabBackup dialog

class CServiceTabBackup : public CServiceTabChild
{
public:
	CServiceTabBackup();
	~CServiceTabBackup();

public:
	void UpdateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool createDialogIndirect( void );
	bool buildDlgChild( HWND hWndParent );
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabBackup_h_)
