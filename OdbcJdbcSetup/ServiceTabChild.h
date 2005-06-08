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

// ServiceTabChild.h interface for the Service Backup class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabChild_h_)
#define _ServiceTabChild_h_

namespace OdbcJdbcSetupLibrary {

using namespace classJString;

/////////////////////////////////////////////////////////////////////////////
// CServiceTabChild dialog

class CServiceTabChild
{
public:
	CServiceTabChild();
	virtual ~CServiceTabChild();

public:
	void SetDisabledDlgItem( HWND hWnd, int ID, BOOL bDisabled = TRUE );
	bool IsLocalhost( char *fullPathFileName, int &offset );
	void CheckRemotehost( char *fullPathFileName );
	bool OnFindFileDatabase( void );
	bool OnFindFile( char *szCaption, char *szOpenFilter, char *szDefExt, JString &pathFile );
	virtual void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	virtual bool onCommand( HWND hWnd, int nCommand );
	virtual bool createDialogIndirect( CServiceTabCtrl *parentTabCtrl );
	virtual bool buildDlgChild( HWND hWndParent );
	CServiceTabChild* getObject();

public:
	CServiceTabCtrl *tabCtrl;
	HWND            parent;
	LPDLGTEMPLATE   resource;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabChild_h_)
