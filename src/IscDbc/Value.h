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
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Value.h: interface for the Value class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(_VALUE_H_INCLUDED_)
#define _VALUE_H_INCLUDED_

#include "Types.h"
#include "TimeStamp.h"
#include "Stream.h"

#include "Blob.h"
#include "DateTime.h"	// Added by ClassView

namespace IscDbcLibrary {

class Blob;

class Value  
{
public:
	void setValue (Blob *blb);
	Blob* getBlob();
	const char* getString (char **tempPtr);
	Value (const char *string);
	int compare (Value *value);

	bool	getBoolean();
	short	getShort(int scale = 0);
	int		getLong(int scale = 0);
	QUAD	getQuad(int scale = 0);
	float	getFloat();
	double	getDouble();
	const char	*getString();
	int		getString (int bufferSize, char *buffer);

	void	setValue (float value);
	void	setValue (double value);
	void	setValue (int value, int scale = 0);
	void	setValue (Value *value);

//protected:
	void	setString (int length, const char *string, bool copy);
	void	setString (const char *value, bool copy);
	void	convertStringData();
	inline void roundStringNumber ( char *& strNumber, int numDigits, int &realDigits );
	void	convertFloatToString(double value, char *string, int size, int *length, int precision = 15, char POINT_DIV = '.');

public:	
	void setValue (SqlTime value);
	void convert (QUAD number, int scale, char *string);
	TimeStamp getTimestamp();
	SqlTime getTime();
	void setValue (TimeStamp value);
	char getByte (int scale = 0);
	void divide (Value *value);
	void add (int value);
	void add (Value *value);
	bool isNull (Type type);
	void setDate (int value);
	void setNull();
	bool isNull();
	void setValue (QUAD value, int scale = 0);
	void setValue (short value, int scale = 0);
	void allocString (Type typ, int length);
	void getStream (Stream *stream, bool copyFlag);
	void setValue (DateTime value);

	DateTime getDate();
	QUAD convertToQuad (double& divisor);
	inline void clear()
	{
		if (type == String && copyFlag && data.string.string)
		{
			delete [] data.string.string;
			data.string.string = NULL;
		}
		else if (type == BlobPtr)
			data.blob->release();

		type = Null;
	}

	Value();
	~Value();

	Type		type;
	bool		copyFlag;
	char		scale;

	union
		{
		struct
			{
			char	*string;
			int		length;
			}	string;
		short		smallInt;
		int			integer;
		float		flt;
		double		dbl;
		QUAD		quad;
		Blob		*blob;
		DateTime	date;
		TimeStamp	timestamp;
		SqlTime		time;
		} data;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_VALUE_H_INCLUDED_)
