/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			JString.h
 *	DESCRIPTION:	Transportable flexible string
 *
 * copyright (c) 1997 - 2000 by James A. Starkey for IBPhoenix.
 */

#ifndef __JString_H
#define __JString_H

#define ALLOC_FUDGE		100

/***
#ifndef __ENGINE_H
#ifdef ENGINE
typedef unsigned short	WCHAR;
#else
#include <wchar.h>
typedef wchar_t			WCHAR;
#endif
#endif
***/

class JString 
{
public:
	void releaseBuffer ();
	char* getBuffer (int length);
	 JString (const char *source, int length);
	int length();
	bool equalsNoCase (const char *string2);
	static JString upcase (const char *source);
	static int findSubstring (const char *string, const char *sub);
	//JString& operator = (const WCHAR *wString);
	// JString (const WCHAR *wString);
	int hash (int tableSize);
	static int hash (const char *string, int tableSize);
	bool IsEmpty();
	const char* after (char c);
	JString before (char c);
	//void set (int length, const char *stuff);
	bool operator == (const char *string);
	bool operator != (const char *stuff);

	//JString (const WCHAR *wString, int len);
	JString();
	JString (const char *string);
	JString(const JString& stringSrc);
	~JString();
	
	void		append (const char*);
	void		setString (const char*);
	//void		setString (const WCHAR *wString, int len);
	void		setString (const char *source, int length);

	void		Format (const char*, ...);
	const char	*getString();
	operator const char*();
	JString& operator = (const char *string);
	JString& operator = (const JString& string);
	JString& operator+=(const char *string);
	JString& operator+=(const JString& string);

	friend JString operator + (const JString& string1, const char* string2);

protected:
	void	alloc (int length);
	void	release();

	char	*string;
};


#endif

