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
 *
 *
 *	ChangeLog
 *
 *	2003-03-24	Value.cpp
 *				Contributed by Norbert Meyer
 *				Improve memory cleanup Value::getString()
 *
 *	2002-11-25	Values.cpp
 *				Contributed by C. G. Alvarez
 *				Minor adjustment to improve handling of 
 *				NUMERIC and DECIMAL
 *
 *	2002-11-24	Values.cpp
 *				Contributed by C. G. Alvarez
 *				Improved handling of TIME datatype
 *
 *	2002-05-20	Value.cpp
 *				Contributed by Bernhard Schulte
 *				o Updated setValue() to support changes 
 *				  in timestamp conversion.	
 *
 */

// Value.cpp: implementation of the Value class.
//
//////////////////////////////////////////////////////////////////////


// copyright (c) 1999 - 2000 by James A. Starkey

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "IscDbc.h"
#include "Value.h"
#include "SQLError.h"
#include "BinaryBlob.h"

#define DECIMAL_POINT		'.'
#define DIGIT_SEPARATOR		','

#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif

#undef NOT_YET_IMPLEMENTED
#define NOT_YET_IMPLEMENTED throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "conversion is not implemented");

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Value::Value()
{
	type = Null;
}

Value::~Value()
{
	clear();
}


void Value::setValue (int number, int scl)
{
	clear();
	type = Long;
	scale = scl;
	data.integer = number;
}

void Value::setString(const char * string, bool copy)
{
	clear();

	if (!string)
		return;

	type = String;
	copyFlag = copy;
	data.string.length = (int)strlen (string);

	if (copy)
		{
		data.string.string = new char [data.string.length + 1];
		strcpy (data.string.string, string);
		}
	else
		data.string.string = (char*) string;
}

Value::Value(const char * string)
{
	type = Null;
	setString (string, true);
}

void Value::setValue(Value * value)
{
	switch (value->type)
		{
		case String:
			setString (value->data.string.string, true);
			return;

		case Char:
		case Varchar:
			setString (value->data.string.length, value->data.string.string, true);
			return;
		}

	clear();
	type = value->type;
					
	switch (value->type)
		{
		case Null:
			break;

		case Short:
			scale = value->scale;
			data.integer = value->data.smallInt;
			break;

		case Long:
			scale = value->scale;
			data.integer = value->data.integer;
			break;

		case Quad:
			scale = value->scale;
			data.quad = value->data.quad;
			break;

		case Float:
			data.flt = value->data.flt;
			break;

		case Double:
			data.dbl = value->data.dbl;
			break;

		case BlobPtr:
			data.blob = value->data.blob;
			data.blob->addRef();
			break;

		case Date:
			data.date = value->data.date;
			break;
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

}

void Value::setString(int length, const char * string, bool copy)
{
	clear();
	type = String;
	copyFlag = copy;
	data.string.length = length;

	if (copy)
		{
		data.string.string = new char [length + 1];
		memcpy (data.string.string, string, length);
		data.string.string [length] = 0;
		}
	else
		data.string.string = (char*) string;
}

void Value::convertStringData()
{
	char * pt, * ptTmp = data.string.string;

	if (!ptTmp || !*ptTmp)
		return;

	while(*ptTmp==' ')ptTmp++;

	if(*ptTmp!='{')
		return;

	pt = data.string.string;

	while(*ptTmp && *ptTmp!='\'')ptTmp++;

	if(*ptTmp!='\'')
		return;

	ptTmp++; // ch \'

	while(*ptTmp && *ptTmp!='\'')
		*pt++=*ptTmp++;

	data.string.length = pt-data.string.string;
	*pt = '\0';
	// validate end string check Server
}

const char* Value::getString()
{
	switch (type)
		{
		case Null:
			return "";

		case String:
		case Char:
			return data.string.string;

		default:
			NOT_YET_IMPLEMENTED;
		}

	return "";
}


int Value::getString(int bufferSize, char * buffer)
{
	switch (type)
		{
		case Null:
			buffer [0] = 0;
			return 1;
					
		case String:
		case Char:
		case Varchar:
			if (data.string.length > bufferSize)
				throw SQLEXCEPTION (TRUNCATION_ERROR, "string truncation");
			memcpy (buffer, data.string.string, data.string.length);
			return data.string.length;

		case Date:
			return data.date.getString (bufferSize, buffer);
		case TimeType:
			return data.time.getString (bufferSize, buffer);

		default:
			NOT_YET_IMPLEMENTED;
		}

	return -1;
}

double Value::getDouble()
{
	switch (type)
		{
		case Null:
			return 0;

		case Float:
			return (double)data.flt;

		case Double:
			return data.dbl;

		case Short:
		case Long:
		case Quad:
			return (double) getQuad();

		case Char:
		case Varchar:
		case String:
			break;

		case Date:
			return (double) data.date.date;

		default:
			NOT_YET_IMPLEMENTED;
		}

	double divisor;
	QUAD number = convertToQuad (divisor);
	return number / divisor;
}

float Value::getFloat()
{
	switch (type)
		{
		case Null:
			return 0;

		case Float:
			return data.flt;

		case Double:
			return (float)data.dbl;

		case Short:
		case Long:
		case Quad:
			return (float) getQuad();

		case Char:
		case Varchar:
		case String:
			break;

		case Date:
			return (float) data.date.date;

		default:
			NOT_YET_IMPLEMENTED;
		}

	double divisor;
	QUAD number = convertToQuad (divisor);
	return (float)(number / divisor);
}

int Value::compare(Value * value)
{
	if (type == value->type)
		switch (type)
			{
			case Short:
				return data.smallInt - value->data.smallInt;

			case Long:
				return data.integer - value->data.integer;

			case Float:
				return (int) (data.flt - value->data.flt);

			case Double:
				return (int) (data.dbl - value->data.dbl);

			case Quad:
				return (int) (data.quad - value->data.quad);
			}

	switch (MAX (type, value->type))
		{
		case Null:
			return 0;

		case String:
		case Char:
		case Varchar:
			{
			char *p = data.string.string;
			char *q = value->data.string.string;
			int l1 = data.string.length;
			int l2 = value->data.string.length;
			int l = MIN (l1, l2);
			int n;
			for (n = 0; n < l; ++n)
				{
				int c = *p++ - *q++;
				if (c)
					return c;
				}
			int c;
			if (n < l1)
				{
				for (; n < l1; ++n)
					if (c = *p++ - ' ')
						return c;
				return 0;
				}
			if (n < l2)
				{
				for (; n < l2; ++n)
					if (c = ' ' - *q++)
						return c;
				}
			return 0;
			}

		case Double:
		case Float:
			return (int) (getDouble() - value->getDouble());

		case Quad:
			return (int) (getQuad() - value->getQuad());

		case Short:
		case Long:
			return (int) (getLong() - value->getLong());

		case Date:
			return getDate().date - value->getDate().date;
		}

	NOT_YET_IMPLEMENTED;

	return 0;						
}

void Value::setValue(float value)
{
	clear();
	type = Float;
	data.flt = value;
}

void Value::setValue(double value)
{
	clear();
	type = Double;
	data.dbl = value;
}

QUAD Value::getQuad(int scale)
{
	switch (type)
		{
		case Null:
			return 0;

		case Short:
			return data.smallInt;

		case Long:
			return data.integer;

		case Quad:
			return data.quad;

		case Float:
			return (QUAD) data.flt;

		case Double:
			return (QUAD) data.dbl;

		default:
			{
			double divisor;
			QUAD quad = convertToQuad (divisor);
			if (scale < 0)
				for (; scale; ++scale)
					divisor /= 10;
			else if (scale > 0)
				for (; scale; --scale)
					divisor *= 10;
			if (divisor == 1)
				return (int) quad;
			return (int) (quad / divisor);
			}
		}

	return 0;
}

int Value::getLong(int scale)
{
	switch (type)
		{
		case Null:
			return 0;

		case Short:
			return data.smallInt;

		case Long:
			return data.integer;

		case Quad:
			return (int) getQuad();

		case Char:
		case Varchar:
		case String:
			{
			double divisor;
			QUAD quad = convertToQuad (divisor);
			if (divisor == 1)
				return (int) quad;
			return (int) (quad / divisor);
			}

		default:
			NOT_YET_IMPLEMENTED;
		}

	return 0;
}

bool Value::getBoolean()
{
	switch (type)
	{
	case Null:
		return false;

	case Short:
		return !!data.smallInt;

	case Long:
		return !!data.integer;

	case Char:
	case Varchar:
	case String:
		{
		double divisor;
		QUAD quad = convertToQuad (divisor);
		if (divisor == 1)
			return !!quad;
		return !!(quad / divisor);
		}

	case Quad:
	default:
		return !!getQuad();
	}

	return 0;
}

short Value::getShort(int scale)
{
	switch (type)
		{
		case Null:
			return 0;

		case Short:
			return data.smallInt;

		case Long:
			return (short) data.integer;

		case Char:
		case Varchar:
		case String:
			{
			double divisor;
			QUAD quad = convertToQuad (divisor);
			if (divisor == 1)
				return (short) quad;
			return (short) (quad / divisor);
			}

		case Quad:
		default:
			return (short) getQuad();
		}

	return 0;
}

const char* Value::getString(char **tempPtr)
{
	char	temp [64];
	int		length;

	switch (type)
		{
		case Null:
			return "";

		case String:
		case Char:
			return data.string.string;

		case Short:
			convert (data.smallInt, scale, temp);
			break;

		case Long:
			convert (data.integer, scale, temp);
			break;

		case Quad:
			convert (data.quad, scale, temp);
			break;

		case Float:
			convertFloatToString(data.flt, temp, sizeof(temp), &length, 7);
			break;

		case Double:
			convertFloatToString(data.dbl, temp, sizeof(temp), &length);
			break;

		case Date:
			data.date.getString (sizeof (temp), temp);
			break;

		case Timestamp:
			data.timestamp.getTimeString (sizeof (temp), temp);
			break;
			
		case TimeType:
			data.time.getString (sizeof (temp), temp);
			break;

		case BlobPtr:
			if (!tempPtr)	// NOMEY +
				throw SQLEXCEPTION (BUG_CHECK, "NULL-Pointer in Value::getString, case BlobPtr:"); // NOMEY +

 			if (*tempPtr)
				delete [] *tempPtr;

			if(data.blob->enType == enTypeBlob)
			{
				int length = data.blob->length();
				*tempPtr = new char [length*2 + 1];
				if( length > 0 )
					data.blob->getHexString (0, length, *tempPtr);
				(*tempPtr) [length*2] = 0;
			}
			else if(data.blob->enType == enTypeClob)
			{
				int length = data.blob->length();
				*tempPtr = new char [length + 1];
				if( length > 0 )
					data.blob->getBytes (0, length, *tempPtr);
				(*tempPtr) [length] = 0;
			}
			else // if(data.blob->enType == enTypeArray)
			{
				BinaryBlob * ptArr = (BinaryBlob *)data.blob;
				int length = ptArr->getLength();
				*tempPtr = new char [length + 1];
				if( length > 0 )
					ptArr->getSegment ((int)0, length,(void*)*tempPtr);
				(*tempPtr) [length] = 0;
			}
			return *tempPtr;

		default:
			NOT_YET_IMPLEMENTED;
		}

	length = (int)strlen (temp);

	if (!tempPtr)	//NOMEY +
		throw SQLEXCEPTION (BUG_CHECK, "NULL-Pointer in Value::getString"); //NOMEY +

 	if (*tempPtr)
//	delete *tempPtr;   // NOMEY -
	delete[] *tempPtr; // NOMEY +


	*tempPtr = new char [length + 1];
	strcpy (*tempPtr, temp);

	return *tempPtr;
}

Blob* Value::getBlob()
{
	BinaryBlob *blob;

	switch (type)
		{
		case Null:
			return new BinaryBlob;

		case BlobPtr:
			data.blob->addRef();
			return data.blob;

		case String:
			blob = new BinaryBlob;
			blob->putSegment (data.string.length, data.string.string, false);	
			return blob;

		}

	NOT_YET_IMPLEMENTED;

	return NULL;
}

void Value::getStream(Stream * stream, bool copyFlag)
{
	switch (type)
		{
		case Null:
			break;

		case Char:
		case Varchar:
		case String:
			stream->putSegment (data.string.length, data.string.string, copyFlag);
			break;

		default:
			{
			char temp [128];
			int length = getString (sizeof (temp), temp);
			stream->putSegment (length, temp, true);
			}
		}
}

void Value::setValue(Blob * blb)
{
	clear();
	type = BlobPtr;
	data.blob = blb;
	data.blob->addRef();
}

QUAD Value::convertToQuad(double & divisor)
{
	QUAD number = 0;
	divisor = 1;
	bool decimal = false;
	bool negative = false;

	for (char *p = data.string.string, *end = p + data.string.length; p < end;)
		{
		char c = *p++;
		if (c >= '0' && c <= '9')
			{
			number = number * 10 + c - '0';
			if (decimal)
				divisor *= 10;
			}
		else if (c == '-')
			negative = true;
		else if (c == '+' || c == DIGIT_SEPARATOR)
			;
		else if (c == DECIMAL_POINT)
			decimal = true;
		else if (c != ' ' && c != '\t' && c != '\n')
			throw SQLEXCEPTION (CONVERSION_ERROR, "error converting to numeric from '%*s'",
									data.string.length, data.string.string);
		}

	return (negative) ? -number : number;
}

DateTime Value::getDate()
{
	switch (type)
		{
		case Null:
			{
			DateTime date;
			date.date = (int) 0;
			return date;
			}

		case Char:
		case String:
		case Varchar:
			break;


		case Long:
			{
			DateTime date;
			date.date = data.integer;
			return date;
			}

		case Date:
			return data.date;

		case TimeType:
		case Timestamp:
			return data.timestamp;

		default:
			NOT_YET_IMPLEMENTED;
		}

	return DateTime::convert (data.string.string, data.string.length);
}


TimeStamp Value::getTimestamp()
{
	switch (type)
	{
	case Null:
		{
		TimeStamp timestamp;
		timestamp.date = 0;
		timestamp.nanos = 0;
		return timestamp;
		}

	case Char:
	case String:
	case Varchar:
		break;

	case Date:
		{
		TimeStamp timestamp;
		timestamp.date = getDate().date;
		timestamp.nanos = 0;
		return timestamp;
		}

	case TimeType:
		{
		TimeStamp timestamp;
		timestamp.date = 0;
		timestamp.nanos = getTime().timeValue;
		return timestamp;
		}

	case Timestamp:
		return data.timestamp;

	default:
		NOT_YET_IMPLEMENTED;
	}

	return TimeStamp::convert (data.string.string, data.string.length);
}

void Value::setValue(DateTime value)
{
	clear();
	type = Date;
	data.date = value;
}

void Value::setDate(int value)
{
	clear();
	type = Date;
	data.date.date = value;
}

void Value::allocString(Type typ, int length)
{
	clear();
	type = typ;
	data.string.length = length;
	data.string.string = new char [length + 1];
	data.string.string [length] = 0;
}

void Value::setValue(short value, int scl)
{
	clear();
	type = Short;
	scale = scl;
	data.smallInt = value;
}

void Value::setValue(QUAD value, int scl)
{
	clear();
	type = Quad;
	scale = scl;
	data.quad = value;
}

bool Value::isNull()
{
	return type == Null;
}

void Value::setNull()
{
	clear();
}


bool Value::isNull(Type conversionType)
{
	if (type == Null)
		return true;

	if (conversionType == Date)
		switch (type)
			{
			case Char:
			case String:
			case Varchar:
				if (data.string.length == 0)
					return true;
				break;
			}

	return false;
}

void Value::add(Value * value)
{
	Type maxType = MAX (type, value->type);

	if (Null || value->type == Null)
		{
		clear();
		return;
		}

	switch (maxType)
		{
		case Short:
		case Long:
			setValue (getLong() + value->getLong());
			break;

		case Float:
		case Double:
			setValue (getDouble() + value->getDouble());
			break;

		default:
			NOT_YET_IMPLEMENTED;
		}
}

void Value::add(int value)
{
	setValue ((int) (getLong() + value));
}

void Value::divide(Value * value)
{

	if (Null || value->type == Null)
		{
		clear();
		return;
		}

	switch (type)
		{
		case Short:
		case Long:
			{
			int divisor = value->getLong();
			if (divisor == 0)
				throw SQLEXCEPTION (RUNTIME_ERROR, "integer divide by zero");
			setValue (getLong() / divisor);
			}
			break;

		case Float:
		case Double:
			{
			double divisor = value->getLong();
			if (divisor == 0)
				throw SQLEXCEPTION (RUNTIME_ERROR, "integer divide by zero");
			setValue (getDouble() / divisor);
			}
			break;

		default:
			NOT_YET_IMPLEMENTED;
		}
}

char Value::getByte(int scale)
{
	switch (type)
		{
		case Null:
			return 0;

		case Short:
			return (char) data.smallInt;

		case Long:
			return (char) data.integer;

		case Quad:
		default:
			return (char) getQuad();
		}

	return 0;
}

void Value::setValue(TimeStamp value)
{
	clear();
	type = Timestamp;
    data.timestamp.date = value.date;
    data.timestamp.nanos = value.nanos;
}

SqlTime Value::getTime()
{
	switch (type)
		{
		case Null:
			{
			SqlTime time ;
			time.timeValue = 0;
			return time;
			}

		case Char:
		case String:
		case Varchar:
			break;

		case TimeType:
			return data.time;

		case Long:
			{
			SqlTime time;
			time.timeValue = data.integer;
			return time;
			}

		default:
			NOT_YET_IMPLEMENTED;
		}

	return SqlTime::convert (data.string.string, data.string.length);
}

void Value::convert(QUAD value, int scale, char *string)
{
	QUAD number = value;

	if (number == 0)
		{
		strcpy (string, "0");
		return;
		}

	if (scale < -18)
		{
		strcpy (string, "***");
		return;
		}

	bool negative = false;

	if (number < 0)
		{
		number = -number;
		negative = true;
		}

	char temp [100], *p = temp;
	int n;
	for (n = 0; number; number /= 10, --n)
		{
		if (scale && scale == n)
			*p++ = '.';
		*p++ = '0' + (char) (number % 10);
		}

	if (scale <= n)
		{
		for (; n > scale; --n)
			*p++ = '0';
		*p++ = '.';
		}

	char *q = string;

	if (negative)
		*q++ = '-';

	while (p > temp)
		*q++ = *--p;

	*q = 0;
}

void Value::roundStringNumber ( char *& strNumber, int numDigits, int &realDigits )
{
	char * &chBeg = strNumber;
	char * chEnd = chBeg + numDigits;

	if ( *chEnd >= '5' ) 
	{
		++*--chEnd;

		while ( *chEnd > '9' ) 
		{
			*chEnd = '0';
			if ( chEnd > chBeg )
				++*--chEnd;
			else
			{
				*--chBeg = '1';
				++realDigits;
			}
		}
	}
}

void Value::convertFloatToString(double value, char *string, int size, int *length, int digit, char POINT_DIV)
{
#define MAXDIGITS 512
	const int maxDecimalExponent = 308;
	char temp[64];
	char *dst = temp;
	int numDigits = digit - 1;
	int realDigits;
	double valInt, valFract;
	char *pt, *pt1;
	char buf[MAXDIGITS];
	int sign;
	bool copy = false;
	int &len = *length;

	len = 0;

	if ( !size )
		return;

	if ( size >= 24 )
		dst = string;
	else
		copy = true;

	realDigits = 0;
	sign = 0;

	if ( value < 0 )
	{
		sign = 1;
		value = -value;
	}

	value = modf ( value, &valInt );

	if ( valInt != 0 )
	{
		pt = pt1 = &buf[MAXDIGITS - numDigits - 1];
		char * end = buf + 1;

		while ( valInt != 0 )
		{
			valFract = modf ( valInt / 10, &valInt );
			*--pt1 = (int)( ( valFract + 0.03 ) * 10 ) + '0';
			realDigits++;

			if ( realDigits > maxDecimalExponent )
			{
				*pt1 = '1';
				break;
			}
		}

		if ( realDigits > numDigits ) // big number
		{
			roundStringNumber ( pt1, numDigits, realDigits );

			int ndig = numDigits;

			pt = dst;

			if ( sign )
				*pt++ = '-';
			
			*pt++ = *pt1++;
			*pt++ = POINT_DIV;

			while ( --ndig )
				*pt++ = *pt1++;

			end = pt - 1;
			while ( *end == '0' ) --end;
			
			if ( *end == POINT_DIV )
				pt = end;
			else
				pt = end + 1;

			*pt++ = 'e';
			*pt = '+';

			ndig = realDigits - 1;

			int n;
			for ( n = 3, pt += n; ndig; ndig /= 10, --n)
				*pt-- = '0' + (char) (ndig % 10);

			while ( n-- )
				*pt-- = '0';

			pt += 4;
			*pt = '\0';

			len = pt - dst;
			return;
		}

		// normal number
		end = pt1 + numDigits;
		
		while ( pt <= end )
		{
			value *= 10;
			value = modf ( value, &valFract );
			*pt++ = (int)valFract + '0';
		}

		*pt = '\0';

		roundStringNumber ( pt1, numDigits, realDigits );

		*(pt-1) = '\0';
		pt = dst;

		if ( sign )
			*pt++ = '-';

		int n = realDigits;

		while ( n-- )
			*pt++ = *pt1++;

		n = numDigits - realDigits;
		end = pt1 + (n - 1);

		while ( n > 0 && *end == '0' ) --end, --n;

		if ( !n )
			*pt = '\0';
		else
		{
			*(end + 1) = '\0';
			*pt++ = POINT_DIV;
			while ( (*pt++ = *pt1++) );
			--pt;
		}
	} 
	else if ( value > 0 ) 
	{   // shift to left number 0.0000122 to 0.122
		while ( ( valFract = value * 10 ) < 1 ) 
		{
			value = valFract;
			realDigits--;
		}

		char * beg = buf + 1;
		pt1 = buf + numDigits + 1;
		pt = buf + 1;
		
		while ( pt <= pt1 )
		{
			value *= 10;
			value = modf ( value, &valFract );
			*pt++ = (int)valFract + '0';
		}

		*pt = '\0';

		roundStringNumber ( beg, numDigits, realDigits );

		*--pt = '\0';
		--pt;

		while ( pt > beg && *pt == '0' ) 
			*pt-- = '\0';

		pt = dst;

		if ( sign )
			*pt++ = '-';

		if ( realDigits == 1 )
		{
			while ( (*pt++ = *beg++) );
		}
		else if ( realDigits >= -3 )
		{
			*pt++ = '0';

			if ( *beg > '0' )
			{
				int n = realDigits;

				*pt++ = POINT_DIV;

				while ( n++ )
					*pt++ = '0';

				while ( (*pt++ = *beg++) );
				--pt;
			}
			else
				*pt = '\0';
		}
		else
		{
			*pt++ = *beg++;

			if ( *beg )
			{
				*pt++ = POINT_DIV;

				while ( *beg )
					*pt++ = *beg++;
			}

			*pt++ = 'e';
			*pt = '-';

			int ndig = -realDigits + 1;
			int n;

			for ( n = 3, pt += n; ndig; ndig /= 10, --n)
				*pt-- = '0' + (char) (ndig % 10);

			while ( n-- )
				*pt-- = '0';

			pt += 4;
			*pt = '\0';
		}
	}
	else
	{
		pt = dst;
		*pt++ = '0';
		*pt = '\0';
	}

	len = pt - dst;

	if ( copy )
	{
		len = MIN ( len, size - 1 );
		memcpy ( string, temp, len );
		string[len] = '\0';
	}
}

void Value::setValue(SqlTime value)
{
	clear();
	type = TimeType;
	data.time = value;
}

}; // end namespace IscDbcLibrary
