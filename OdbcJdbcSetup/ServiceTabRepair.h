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

// ServiceTabRepair.h interface for the Service Repair class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_ServiceTabRepair_h_)
#define _ServiceTabRepair_h_

namespace OdbcJdbcSetupLibrary {

/////////////////////////////////////////////////////////////////////////////
// CServiceTabRepair dialog

class CServiceTabRepair : public CServiceTabChild
{
	enum
	{ 	
		enValidateDb        = 0x0001,
		enSweepDb           = 0x0002,
		enMendDb            = 0x0004,
		enListLimboTrans    = 0x0008,
		enCheckDb           = 0x0010,
		enIgnoreChecksum    = 0x0020,
		enKillShadows       = 0x0040,
		enFull              = 0x0080,

		enFixListLimboTrans = 0x1000
	};

	enum enumRepairLimboTransactions
	{ 	
		enCommitTrans       = 0x01,
		enRollbackTrans     = 0x02,
		enRecoverTwoPhase   = 0x04
	};

public:
	CServiceTabRepair();
	~CServiceTabRepair();

public:
	void updateData( HWND hDlg, BOOL bSaveAndValidate = TRUE );
	bool onCommand( HWND hWnd, int nCommand );
	void hideValidateOptions( bool hide );
	void addParameters( CServiceClient &services );
	void startRepairDatabase();
	bool createDialogIndirect( CServiceTabCtrl *parentTabCtrl );
	bool buildDlgChild( HWND hWndParent );

public:
	bool    isVisibleValidateOptions;
	ULONG   repairParameters;
	ULONG   validateParameters;
};

}; // end namespace OdbcJdbcSetupLibrary

#endif // !defined(_ServiceTabRepair_h_)
