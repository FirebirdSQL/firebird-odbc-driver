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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */


#ifndef __JString_H
#define __JString_H

namespace classJString {

#define ALLOC_FUDGE		100


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

}; // end namespace classJString

#endif
