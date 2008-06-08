#include "stdafx.h"
#include <odbcinst.h>

extern BOOL INSTAPI ConfigDSN(HWND		hWnd,
			   WORD		fRequest,
			   LPCSTR	lpszDriver,
			   LPCSTR	lpszAttributes);

int main (int argc, char **argv)
{
	ConfigDSN (NULL, ODBC_CONFIG_DSN, "Firebird/InterBase(r) driver", "");
	
	return 0;
}