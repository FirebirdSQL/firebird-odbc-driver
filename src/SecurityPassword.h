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
 *  Copyright (c) 2004 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// CSecurityPassword.h: interface for the CSecurityPassword class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CSecurityPassword_H_)
#define _CSecurityPassword_H_

namespace classSecurityPassword {

class CShift
{
	char segment[6];
	int off;
public:
	CShift()
	{
		memset ( segment, 0, sizeof(segment) );
		off = 0;
	}

	void operator= ( char *&seg )
	{
		memcpy ( segment, seg, sizeof(segment) );
		++seg += sizeof(segment);
	}

	char operator++ ( int pos )
	{
		return segment[off++ % 6];
	}

};

class CSecurityPassword
{
	char	securityKey[40];
	CShift	shift[4];
	int		rep;
	char	chKey;
public:
	CSecurityPassword()
	{
		memset ( securityKey, 0, sizeof(securityKey) );
		rep = sizeof(shift)/sizeof(shift[0]);
		chKey = 0;
	}

	void buildKey( char * pass )
	{
		int len = (int)strlen(pass);

		if ( !len )
			return;

		unsigned char p=0;
		char * beg = securityKey;
		char * end = beg + sizeof(securityKey);
		int off = 0;

		while ( beg < end ) 
		{
			char * ch1 = pass + off % len;
			char * ch2 = pass + off++ % len;
			*beg++ = (char)(*ch1 * (p + (0x11 * (*ch1 + off))) + *ch2);
		}
	}

	void initShifts()
	{
		char * beg = securityKey + 3;
		int i = 0;

		while ( i < rep )
			shift[i++] = beg;
	}

	char get()
	{
		char ch = 0;
		int i = 0;
		while ( i < rep )
			ch ^= shift[i++]++;
		return ch;
	}

	void make ( char *buf, int len )
	{
		for ( int i = 0 ; i < len ; i++ ) 
			*buf++ ^= get();
	}
	
	void encode ( char *password, char *passkey )
	{
		char *pt = passkey;
		int len = (int)strlen(password);
		buildKey( password );
		initShifts();
		memcpy ( pt, securityKey, sizeof(securityKey) );
		pt += sizeof(securityKey);
		memcpy ( pt, password, len );
		make ( pt, len );
		pt[len]=0;

		// convert to hex string
		len += sizeof(securityKey);
		char *address = passkey + (len << 1);
		char *end = (char *)passkey + len - 1;
		*address-- = 0;
		while( len-- )
			*address-- = (((*end >> 4) & 0x0F) + 'A'),
			*address-- = ((*end-- & 0x0F) + 'A');
	}

	void decode ( char *passkey, char *password )
	{
		if ( !*passkey )
			return;

		int lenkey = (int)strlen ( passkey );
		if ( lenkey % 2 )
			return;

		lenkey /=2;

		int len = lenkey - sizeof(securityKey);
		char *beg = passkey;
		char *next = passkey;

		while( lenkey-- )
			*beg = *next++ - 'A',*beg++ += (*next++ - 'A') << 4;

		char *pt = passkey;
		memcpy ( securityKey, passkey, sizeof(securityKey) );
		pt += sizeof(securityKey);
		initShifts();
		make ( pt, len );
		memcpy ( password, pt, len );
		password[len]=0;
	}
};

}; // end namespace classSecurityPassword

#endif // !defined(_CSecurityPassword_H_)
