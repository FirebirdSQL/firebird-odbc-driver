
// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#ifndef __ENGINE_H
#define __ENGINE_H

/***
#ifdef ENGINE
typedef unsigned short	WCHAR;
#else
#include <wchar.h>
typedef wchar_t			WCHAR;
#endif
***/

#ifdef _LEAKS
#include <AFX.h>
//static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#undef THIS_FILE
#endif


#ifndef NULL
#define NULL		0
#endif

#define OFFSET(type,fld)	(int)&(((type)0)->fld)
#define MAX(a,b)			((a > b) ? a : b)
#define MIN(a,b)			((a < b) ? a : b)
#define ABS(n)				(((n) >= 0) ? (n) : -(n))
#define MASK(n)				(1 << (n))
#define ISLOWER(c)			(c >= 'a' && c <= 'z')
#define ISUPPER(c)			(c >= 'A' && c <= 'Z')
#define ISDIGIT(c)			(c >= '0' && c <= '9')
#define UPPER(c)			((ISLOWER (c)) ? c - 'a' + 'A' : c)
#define ROUNDUP(n,b)		((n + b - 1) & ~(b - 1))
#define SQLEXCEPTION		SQLError
#ifdef _WIN32

#define strcasecmp		stricmp
#define strncasecmp		strnicmp

#else

#define __int64			long long
#define _stdcall
#endif

typedef unsigned char	UCHAR;
typedef unsigned long	ULONG;

enum LockType {
	None,
    Exclusive,
	Shared
	};

#ifdef ENGINE
#include "Error.h"
#endif

#include "JString.h"


#endif
