// RString.h: interface for the CRString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSTRING_H__11F793EC_AF83_11D1_AB1B_0000C01D2301__INCLUDED_)
#define AFX_RSTRING_H__11F793EC_AF83_11D1_AB1B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CRString : public CString  
{
	class CRepeat
		{
		public:
			void append (const char *what, const char *separator);
			CRepeat(CRepeat *prior, const char *pattern, const char *what);
			CString pattern;
			CString string;
			CRepeat	*next;
		};
			
public:
	void append (const char *pattern, const char *string, const char *separator = "");
	const char* apply();
	CString getCRstring();
	CString getCRLFstring();
	void replace (const char* pattern, int value);
	 CRString (CString string);
	void replace (const char* pattern, const char* text);
	 CRString (const char*);
	CRString();
	virtual ~CRString();
	CRepeat	*repeat;
};

#endif // !defined(AFX_RSTRING_H__11F793EC_AF83_11D1_AB1B_0000C01D2301__INCLUDED_)
