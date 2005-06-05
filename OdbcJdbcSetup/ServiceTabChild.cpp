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

// ServiceTabChild.cpp: Service Backup Manager class.
//
//////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "ServiceTabChild.h"

namespace OdbcJdbcSetupLibrary {

CServiceTabChild::CServiceTabChild()
{
	parent = NULL;
	resource = NULL;
}

CServiceTabChild::~CServiceTabChild()
{
	if ( resource )
		LocalFree( LocalHandle( resource ) );
}

void CServiceTabChild::SetDisabledDlgItem( HWND hDlg, int ID, BOOL bDisabled )
{
	HWND hWnd = GetDlgItem( hDlg, ID );
	int style = GetWindowLong( hWnd, GWL_STYLE );
	if ( bDisabled )style |= WS_DISABLED;
	else			style &= ~WS_DISABLED;
	SetWindowLong( hWnd, GWL_STYLE, style );
	InvalidateRect( hWnd, NULL, TRUE );
}

void CServiceTabChild::UpdateData( HWND hDlg, BOOL bSaveAndValidate )
{
}

bool CServiceTabChild::createDialogIndirect()
{
	return true;
}

bool CServiceTabChild::buildDlgChild( HWND hWndParent )
{
	parent = hWndParent;
	return true;
}

CServiceTabChild* CServiceTabChild::getObject()
{
	return this;
}

}; // end namespace OdbcJdbcSetupLibrary
