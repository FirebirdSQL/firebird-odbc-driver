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

// ServiceTabStatistics.h interface for the Service Statistics class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabStatistics_h_)
#define _ServiceTabStatistics_h_

namespace OdbcJdbcSetupLibrary {

/////////////////////////////////////////////////////////////////////////////
// CServiceTabStatistics dialog

class CServiceTabStatistics : public CServiceTabChild
{
	enum
	{ 	
		enDataPages         = 0x01,
		enDbLog             = 0x02,
		enHdrPages          = 0x04,
		enIdxPages          = 0x08,
		enSysRelations      = 0x10,
		enRecordVersions    = 0x20,
		enTable             = 0x40,
		enShowDbLog         = 0x80
	};						

public:
	CServiceTabStatistics();
	~CServiceTabStatistics();

public:
	void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool onCommand( HWND hWnd, int nCommand );
	void addParameters( CServiceClient &services );
	void onStartStatistics( void );
	bool createDialogIndirect( CServiceTabCtrl *parentTabCtrl );
	bool buildDlgChild( HWND hWndParent );

public:
	ULONG   statisticParameters;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabStatistics_h_)
