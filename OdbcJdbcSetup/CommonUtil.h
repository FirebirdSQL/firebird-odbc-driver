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

// CommonUtil.h interface for common defined.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_CommonUtil_h_)
#define _CommonUtil_h_

namespace OdbcJdbcSetupLibrary {

	enum enServiceTabCtrl { COUNT_PAGE = 5 };

class CServiceTabCtrl;
class CServiceTabChild;

typedef struct _TAG_DIALOG_HEADER
{ 
	CServiceTabCtrl    *tabCtrl;
	HWND               hWndTab;
	HWND               hWndChildTab;
	CServiceTabChild*  childTab[COUNT_PAGE];

} TAG_DIALOG_HEADER, *PTAG_DIALOG_HEADER;

}; // end namespace OdbcJdbcSetupLibrary

int nCopyAnsiToWideChar( LPWORD lpWCStr, LPCSTR lpAnsiIn );
LPWORD lpwAlign( LPWORD lpIn);

#define TMP_COMTROL(CONTROL,STRTEXT,CTRL_ID,X,Y,CX,CY,STYLE)	\
	p = lpwAlign( p );											\
																\
	lStyle = STYLE;												\
	*p++ = LOWORD( lStyle );									\
	*p++ = HIWORD( lStyle );									\
	*p++ = 0;			/* LOWORD (lExtendedStyle) */			\
	*p++ = 0;			/* HIWORD (lExtendedStyle) */			\
	*p++ = X;			/* x  */								\
	*p++ = Y;			/* y  */								\
	*p++ = CX;			/* cx */								\
	*p++ = CY;			/* cy */								\
	*p++ = CTRL_ID;		/* ID */								\
																\
	*p++ = (WORD)0xffff;										\
	*p++ = (WORD)CONTROL;										\
																\
	/* copy the text of the item */								\
	nchar = nCopyAnsiToWideChar( p, TEXT( STRTEXT ) );	        \
	p += nchar;													\
																\
	*p++ = 0;  /* advance pointer over nExtraStuff WORD	*/		\


//    PUSHBUTTON      "Browse",IDC_FIND_FILE,189,42,36,14
#define TMP_PUSHBUTTON(STRTEXT,ID,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, (BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP) )

#define TMP_DEFPUSHBUTTON(STRTEXT,ID,X,Y,CX,CY)	\
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, (BS_DEFPUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP) )

//    EDITTEXT        IDC_NAME,7,17,102,12,ES_AUTOHSCROLL
#define TMP_EDITTEXT(ID,X,Y,CX,CY,STYLE) \
	TMP_COMTROL(0x0081,"",ID,X,Y,CX,CY, 0x50810080|STYLE )

//    COMBOBOX        IDC_DRIVER,123,17,102,47,CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP
#define TMP_COMBOBOX(ID,X,Y,CX,CY,STYLE) \
	TMP_COMTROL(0x0085,"",ID,X,Y,CX,CY, (STYLE | WS_VISIBLE | WS_CHILD ) )

//    LTEXT           "Data Source Name (DSN)",IDC_STATIC,7,7,83,8
#define TMP_LTEXT(STRTEXT,ID,X,Y,CX,CY)	\
	TMP_COMTROL(0x0082,STRTEXT,(short)ID,X,Y,CX,CY, ( WS_VISIBLE | WS_CHILD ) )

//    GROUPBOX        "Options",IDC_STATIC,7,111,223,49
#define TMP_GROUPBOX(STRTEXT,ID,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,(short)ID,X,Y,CX,CY, (BS_GROUPBOX | WS_VISIBLE | WS_CHILD | WS_TABSTOP) )

//    CONTROL         "read (default write)",IDC_CHECK_READ,"Button", BS_AUTOCHECKBOX | WS_TABSTOP,18,129,69,10
#define TMP_BUTTONCONTROL(STRTEXT,ID,NAME_CTRL,STYLE,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, 0x50010003 )

//    CONTROL         "3",IDC_DIALECT3,"Button",BS_AUTORADIOBUTTON,104,154,16,10
#define TMP_RADIOCONTROL(STRTEXT,ID,NAME_CTRL,STYLE,X,Y,CX,CY) \
	TMP_COMTROL(0x0080,STRTEXT,ID,X,Y,CX,CY, 0x50000009 )

#define TMP_NAMECTRL(STRTEXT,CTRL_ID,NAME_CTRL,X,Y,CX,CY,STYLE)	\
	p = lpwAlign( p );											\
																\
	lStyle = STYLE;												\
	*p++ = LOWORD( lStyle );									\
	*p++ = HIWORD( lStyle );									\
	*p++ = 0;			/* LOWORD (lExtendedStyle) */			\
	*p++ = 0;			/* HIWORD (lExtendedStyle) */			\
	*p++ = X;			/* x  */								\
	*p++ = Y;			/* y  */								\
	*p++ = CX;			/* cx */								\
	*p++ = CY;			/* cy */								\
	*p++ = CTRL_ID;		/* ID */								\
																\
	/* copy the text of the item */								\
	nchar = nCopyAnsiToWideChar( p, TEXT( NAME_CTRL ) );        \
	p += nchar;													\
																\
	/* copy the text of the item */								\
	nchar = nCopyAnsiToWideChar( p, TEXT( STRTEXT ) );	        \
	p += nchar;													\
																\
	*p++ = 0;  /* advance pointer over nExtraStuff WORD	*/		\

//    CONTROL         "Tab1",IDC_TAB1,"SysTabControl32",0x0,7,7,367,198
#define TMP_NAMECONTROL(STRTEXT,ID,NAME_CTRL,STYLE,X,Y,CX,CY) \
	TMP_NAMECTRL(STRTEXT,ID,NAME_CTRL,X,Y,CX,CY, 0x50000000 | STYLE )

#ifdef _WIN64
#define GW_USERDATA GWLP_USERDATA
#else // if _WIN32
#define GW_USERDATA GWL_USERDATA
#endif

#endif // !defined(_CommonUtil_h_)
