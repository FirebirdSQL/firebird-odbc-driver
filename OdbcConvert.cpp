/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// OdbcConvert.cpp: OdbcConvert class.
//
//////////////////////////////////////////////////////////////////////
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "OdbcJdbc.h"
#include "OdbcDesc.h"
#include "OdbcStatement.h"
#include "OdbcConnection.h"
#include "OdbcError.h"
#include "IscDbc/Connection.h"
#include "DescRecord.h"
#include "IscDbc/SQLException.h"

#define MAKEQUAD(a, b)      ((QUAD)(((long)(a)) | ((UQUAD)((long)(b))) << 32))
#define LO_LONG(l)           ((long)(l))
#define HI_LONG(l)           ((long)(((UQUAD)(l) >> 32) & 0xFFFFFFFF))

namespace OdbcJdbcLibrary {

#ifdef __GNUWIN32__
double listScale[] =
{
	1.0,
	10.0,
	100.0,
	1000.0,
	10000.0,
	100000.0,
	1000000.0,
	10000000.0,
	100000000.0,
	1000000000.0,
	10000000000.0,
	100000000000.0,
	1000000000000.0,
	10000000000000.0,
	100000000000000.0,
	1000000000000000.0,
	10000000000000000.0,
	100000000000000000.0,
	1000000000000000000.0
};
#elif defined __FreeBSD__ || defined __linux__
unsigned __int64 listScale[] =
{
	0x0000000000000001LL,
	0x000000000000000aLL,
	0x0000000000000064LL,
	0x00000000000003e8LL,
	0x0000000000002710LL,
	0x00000000000186a0LL,
	0x00000000000f4240LL,
	0x0000000000989680LL,
	0x0000000005f5e100LL,
	0x000000003b9aca00LL,
	0x00000002540be400LL,
	0x000000174876e800LL,
	0x000000e8d4a51000LL,
	0x000009184e72a000LL,
	0x00005af3107a4000LL,
	0x00038d7ea4c68000LL,
	0x002386f26fc10000LL,
	0x016345785d8a0000LL,
	0x0de0b6b3a7640000LL,
	0x8ac7230489e80000LL
};
#else
unsigned __int64 listScale[] =
{
	0x0000000000000001,
	0x000000000000000a,
	0x0000000000000064,
	0x00000000000003e8,
	0x0000000000002710,
	0x00000000000186a0,
	0x00000000000f4240,
	0x0000000000989680,
	0x0000000005f5e100,
	0x000000003b9aca00,
	0x00000002540be400,
	0x000000174876e800,
	0x000000e8d4a51000,
	0x000009184e72a000,
	0x00005af3107a4000,
	0x00038d7ea4c68000,
	0x002386f26fc10000,
	0x016345785d8a0000,
	0x0de0b6b3a7640000,
	0x8ac7230489e80000
};
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcConvert::OdbcConvert(OdbcStatement * parent)
{
	parentStmt = parent;
	bIdentity = false;
	tempBindOffsetPtr = 0;
	bindOffsetPtrTo = &tempBindOffsetPtr;
	bindOffsetPtrIndTo = &tempBindOffsetPtr;
	bindOffsetPtrFrom = &tempBindOffsetPtr;
}

void OdbcConvert::setParent(OdbcStatement *parent)
{ 
	parentStmt = parent;
}

void OdbcConvert::setZeroColumn(DescRecord * to, long rowNumber)
{
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	*(long*)pointer = rowNumber + 1;
	if ( indicatorPointer )
		*indicatorPointer = sizeof(long);
}

void OdbcConvert::setBindOffsetPtrTo(SQLINTEGER	*bindOffsetPtr, SQLINTEGER	*bindOffsetPtrInd)
{
	if( bindOffsetPtr )
		bindOffsetPtrTo = bindOffsetPtr;
	else
		bindOffsetPtrTo = &tempBindOffsetPtr;

	if( bindOffsetPtrInd )
		bindOffsetPtrIndTo = bindOffsetPtrInd;
	else
		bindOffsetPtrIndTo = &tempBindOffsetPtr;
}

void OdbcConvert::setBindOffsetPtrFrom(SQLINTEGER *bindOffsetPtr)
{
	if( bindOffsetPtr )
		bindOffsetPtrFrom = bindOffsetPtr;
	else
		bindOffsetPtrFrom = &tempBindOffsetPtr;
}

ADRESS_FUNCTION OdbcConvert::getAdresFunction(DescRecord * from, DescRecord * to)
{
	bIdentity = false;

	switch(from->conciseType)
	{
	case SQL_C_SHORT:
	case SQL_C_USHORT:
	case SQL_C_SSHORT:
		switch(to->conciseType)
		{
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			bIdentity = true;
			return &OdbcConvert::convShortToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convShortToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convShortToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convShortToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convShortToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convShortToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convShortToTagNumeric;
		case SQL_C_BINARY:
		case SQL_C_TYPE_DATE:
		case SQL_C_DATE:
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIME:
		case SQL_C_TIME:
			break;
		}
		break;

	case SQL_C_LONG:
	case SQL_C_ULONG:
	case SQL_C_SLONG:
		switch(to->conciseType)
		{
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			return &OdbcConvert::convLongToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			bIdentity = true;
			return &OdbcConvert::convLongToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convLongToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convLongToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convLongToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convLongToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convLongToTagNumeric;
		}
		break;

	case SQL_C_FLOAT:
		switch(to->conciseType)
		{
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			return &OdbcConvert::convFloatToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convFloatToLong;
		case SQL_C_FLOAT:
			bIdentity = true;
			return &OdbcConvert::convFloatToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convFloatToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convFloatToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convFloatToString;
		}
		break;

	case SQL_C_DOUBLE:
		switch(to->conciseType)
		{
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			return &OdbcConvert::convDoubleToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convDoubleToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convDoubleToFloat;
		case SQL_C_DOUBLE:
			bIdentity = true;
			return &OdbcConvert::convDoubleToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convDoubleToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convDoubleToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convDoubleToTagNumeric;
		}
		break;

	case SQL_C_SBIGINT:
	case SQL_C_UBIGINT:
		switch(to->conciseType)
		{
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			return &OdbcConvert::convBigintToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convBigintToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convBigintToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convBigintToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			bIdentity = true;
			return &OdbcConvert::convBigintToBigint;
		case SQL_C_BINARY:
			return &OdbcConvert::convBigintToBinary;
		case SQL_C_CHAR:
			return &OdbcConvert::convBigintToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convBigintToTagNumeric;
		}
		break;

	case SQL_DECIMAL:
	case SQL_C_NUMERIC:
		switch(to->conciseType)
		{
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			return &OdbcConvert::convNumericToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convNumericToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convNumericToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convNumericToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convNumericToBigint;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			bIdentity = true;
			return &OdbcConvert::convNumericToTagNumeric;
		}
		break;

	case SQL_C_DATE:
	case SQL_C_TYPE_DATE:
		switch(to->conciseType)
		{
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convDateToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convDateToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convDateToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convDateToBigint;
		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
			bIdentity = true;
			return &OdbcConvert::convDateToTagDate;
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			return &OdbcConvert::convDateToTagTimestamp;
		case SQL_C_CHAR:
			return &OdbcConvert::convDateToString;
		}
		break;

	case SQL_C_TIME:
	case SQL_C_TYPE_TIME:
		switch(to->conciseType)
		{
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convTimeToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convTimeToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convTimeToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convTimeToBigint;
		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
			bIdentity = true;
			return &OdbcConvert::convTimeToTagTime;
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			return &OdbcConvert::convTimeToTagTimestamp;
		case SQL_C_CHAR:
			return &OdbcConvert::convTimeToString;
		}
		break;

	case SQL_C_TYPE_TIMESTAMP:
	case SQL_C_TIMESTAMP:
		switch(to->conciseType)
		{
		case SQL_C_DOUBLE:
			return &OdbcConvert::convDateTimeToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convDateTimeToBigint;
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			bIdentity = true;
			return &OdbcConvert::convDateTimeToTagDateTime;
		case SQL_C_CHAR:
			return &OdbcConvert::convDateTimeToString;
		}
		break;

	case SQL_C_BINARY:
		switch(to->conciseType)
		{
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convBlobToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convBlobToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convBlobToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convBlobToBigint;
		case SQL_C_BINARY:
			bIdentity = true;
			return &OdbcConvert::convBlobToBlob;
		case SQL_C_CHAR:
			return &OdbcConvert::convBlobToString;
		}
		break;

	case SQL_C_CHAR:
		if ( from->type == JDBC_VARCHAR )
			switch(to->conciseType) // Varying
			{
			case SQL_C_LONG:
			case SQL_C_ULONG:
			case SQL_C_SLONG:
				return &OdbcConvert::convVarStringToLong;
			case SQL_C_SHORT:
			case SQL_C_USHORT:
			case SQL_C_SSHORT:
				return &OdbcConvert::convVarStringToShort;
			case SQL_C_FLOAT:
				return &OdbcConvert::convVarStringToFloat;
			case SQL_C_DOUBLE:
				return &OdbcConvert::convVarStringToDouble;
			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
				return &OdbcConvert::convVarStringToBigint;
			case SQL_C_CHAR:
				bIdentity = true;
				return &OdbcConvert::convVarStringToString;
			}
		else 
			switch(to->conciseType) // Text
			{
			case SQL_C_LONG:
			case SQL_C_ULONG:
			case SQL_C_SLONG:
				return &OdbcConvert::convStringToLong;
			case SQL_C_SHORT:
			case SQL_C_USHORT:
			case SQL_C_SSHORT:
				return &OdbcConvert::convStringToShort;
			case SQL_C_FLOAT:
				return &OdbcConvert::convStringToFloat;
			case SQL_C_DOUBLE:
				return &OdbcConvert::convStringToDouble;
			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
				return &OdbcConvert::convStringToBigint;
			case SQL_C_CHAR:
				bIdentity = true;
				return &OdbcConvert::convStringToString;
			}
		break;
	}
	return NULL;
}

inline 
SQLPOINTER OdbcConvert::getAdressBindDataFrom(char * pointer)
{
	return (SQLPOINTER)(pointer + *bindOffsetPtrFrom);
}

inline 
SQLPOINTER OdbcConvert::getAdressBindDataTo(char * pointer)
{
	return (SQLPOINTER)(pointer + *bindOffsetPtrTo);
}

inline
SQLPOINTER OdbcConvert::getAdressBindIndTo(char * pointer)
{
	return (SQLPOINTER)(pointer + *bindOffsetPtrIndTo);
}

#define ODBCCONVERT_CHECKNULL					\
	if( *(short*)from->indicatorPtr == -1 )		\
	{											\
		if ( indicatorPointer )					\
			*indicatorPointer = -1;				\
		return SQL_SUCCESS;						\
	}											\

#define ODBCCONVERT_CONV(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)								\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	*(C_TYPE_TO*)pointer=(C_TYPE_TO)*(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);	\
	if ( indicatorPointer )																		\
		*indicatorPointer = sizeof(C_TYPE_TO);													\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONVTAGNUMERIC(TYPE_FROM,C_TYPE_FROM)										\
int OdbcConvert::conv##TYPE_FROM##ToTagNumeric(DescRecord * from, DescRecord * to)				\
{																								\
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	*(QUAD*)(pointer+3) = (QUAD)*(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);		\
	QUAD &number = *(QUAD*)(pointer+3);															\
	*pointer++=(char)from->precision;															\
	*pointer++=(char)from->scale;																\
																								\
	if ( number < 0 )																			\
		number = -number,																		\
		*pointer++=0;																			\
	else																						\
		*pointer++=1;																			\
																								\
/*	if ( from->scale )		*/																	\
/*		number *= (QUAD)listScale[from->scale];	*/												\
																								\
	if ( indicatorPointer )																		\
		*indicatorPointer = sizeof(tagSQL_NUMERIC_STRUCT);										\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONVROUND(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)							\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	C_TYPE_FROM valFrom = *(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);			\
	if ( valFrom < 0 )valFrom -= 0.5;															\
	else valFrom += 0.5;																		\
																								\
	*(C_TYPE_TO*)pointer = (C_TYPE_TO)valFrom;													\
	if ( indicatorPointer )																		\
		*indicatorPointer = sizeof(C_TYPE_TO);													\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONV_TO_STRING(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)								\
int OdbcConvert::conv##TYPE_FROM##ToString(DescRecord * from, DescRecord * to)					\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	int len = to->length;																		\
																								\
	if ( !len && to->dataPtr)																	\
		*(char*)to->dataPtr = 0;																\
	else																						\
	{	/* Original source from IscDbc/Value.cpp */												\
		C_TYPE_FROM number = *(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);		\
		char *string = (char*)pointer;															\
		int scale = -from->scale;																\
																								\
		if (number == 0)																		\
		{																						\
			len = 1;																			\
			strcpy (string, "0");																\
		}																						\
		else if (scale < -18)																	\
		{																						\
			len = 3;																			\
			strcpy (string, "***");																\
		}																						\
		else																					\
		{																						\
			bool negative = false;																\
																								\
			if (number < 0)																		\
			{																					\
				number = -number;																\
				negative = true;																\
			}																					\
																								\
			char temp [100], *p = temp;															\
			int n;																				\
			for (n = 0; number; number /= 10, --n)												\
			{																					\
				if (scale && scale == n)														\
					*p++ = '.';																	\
				*p++ = '0' + (char) (number % 10);												\
			}																					\
																								\
			if (scale <= n)																		\
			{																					\
				for (; n > scale; --n)															\
					*p++ = '0';																	\
				*p++ = '.';																		\
			}																					\
																								\
			char *q = string;																	\
			int l=0;																			\
																								\
			if (negative)																		\
				*q++ = '-',++l;																	\
																								\
			if ( p - temp > len - l )															\
				p = temp + len - l;																\
																								\
			while (p > temp)																	\
				*q++ = *--p;																	\
																								\
			*q = 0;																				\
			len = q - string;																	\
		}																						\
	}																							\
																								\
	if ( indicatorPointer )																		\
		*indicatorPointer = len;																\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONV_TO_BINARY(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)								\
int OdbcConvert::conv##TYPE_FROM##ToBinary(DescRecord * from, DescRecord * to)					\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	int len = to->length;																		\
																								\
	if ( !len && to->dataPtr)																	\
		*(short*)to->dataPtr = '0';																\
	else																						\
	{																							\
		C_TYPE_FROM number = *(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);		\
		short *strbin = (short*)pointer;														\
		int scale = -from->scale;																\
																								\
		if (number == 0)																		\
		{																						\
			len = 1;																			\
			*strbin = '0';																		\
		}																						\
		else if (scale < -18)																	\
		{																						\
			len = 3;																			\
			*strbin++ = '*';																	\
			*strbin++ = '*';																	\
			*strbin++ = '*';																	\
		}																						\
		else																					\
		{																						\
			bool negative = false;																\
																								\
			if (number < 0)																		\
			{																					\
				number = -number;																\
				negative = true;																\
			}																					\
																								\
			char temp [100], *p = temp;															\
			int n;																				\
			for (n = 0; number; number /= 10, --n)												\
			{																					\
				if (scale && scale == n)														\
					*p++ = '.';																	\
				*p++ = '0' + (char) (number % 10);												\
			}																					\
																								\
			if (scale <= n)																		\
			{																					\
				for (; n > scale; --n)															\
					*p++ = '0';																	\
				*p++ = '.';																		\
			}																					\
																								\
			short *q = strbin;																	\
			int l=0;																			\
																								\
			if (negative)																		\
				*q++ = '-',++l;																	\
																								\
			if ( p - temp > len - l )															\
				p = temp + len - l;																\
																								\
			while (p > temp)																	\
				*q++ = *--p;																	\
																								\
			len = q - strbin;																	\
		}																						\
	}																							\
																								\
	if ( indicatorPointer )																		\
		*indicatorPointer = len << 1;															\
																								\
	return SQL_SUCCESS;																			\
}																								\

////////////////////////////////////////////////////////////////////////
// Short
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Short,short,Short,short);
ODBCCONVERT_CONV(Short,short,Long,long);
ODBCCONVERT_CONV(Short,short,Float,float);
ODBCCONVERT_CONV(Short,short,Double,double);
ODBCCONVERT_CONV(Short,short,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(Short,short,5);
ODBCCONVERT_CONVTAGNUMERIC(Short,short);

////////////////////////////////////////////////////////////////////////
// Long
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Long,long,Short,short);
ODBCCONVERT_CONV(Long,long,Long,long);
ODBCCONVERT_CONV(Long,long,Float,float);
ODBCCONVERT_CONV(Long,long,Double,double);
ODBCCONVERT_CONV(Long,long,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(Long,long,10);
ODBCCONVERT_CONVTAGNUMERIC(Long,long);

////////////////////////////////////////////////////////////////////////
// Float
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONVROUND(Float,float,Short,short);
ODBCCONVERT_CONVROUND(Float,float,Long,long);
ODBCCONVERT_CONV(Float,float,Float,float);
ODBCCONVERT_CONV(Float,float,Double,double);
ODBCCONVERT_CONVROUND(Float,float,Bigint,QUAD);

int OdbcConvert::convFloatToString(DescRecord * from, DescRecord * to)
{
	char * pointerTo = (char *)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	int len = to->length;

	if ( len )	// MAX_FLOAT_DIGIT_LENGTH = 7
		convertFloatToString(*(float*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len-1, &len, 7);

	if ( indicatorPointer )
		*indicatorPointer = len;

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Double
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONVROUND(Double,double,Short,short);
ODBCCONVERT_CONVROUND(Double,double,Long,long);
ODBCCONVERT_CONV(Double,double,Float,float);
ODBCCONVERT_CONV(Double,double,Double,double);
ODBCCONVERT_CONVROUND(Double,double,Bigint,QUAD);
ODBCCONVERT_CONVTAGNUMERIC(Double,double);

int OdbcConvert::convDoubleToString(DescRecord * from, DescRecord * to)
{
	char * pointerTo = (char *)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	int len = to->length;

	if ( len )	// MAX_DOUBLE_DIGIT_LENGTH = 15
		convertFloatToString(*(double*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len-1, &len);

	if ( indicatorPointer )
		*indicatorPointer = len;

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Bigint
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Bigint,QUAD,Short,short);
ODBCCONVERT_CONV(Bigint,QUAD,Long,long);
ODBCCONVERT_CONV(Bigint,QUAD,Float,float);
ODBCCONVERT_CONV(Bigint,QUAD,Double,double);
ODBCCONVERT_CONV(Bigint,QUAD,Bigint,QUAD);
ODBCCONVERT_CONV_TO_BINARY(Bigint,QUAD,18);
ODBCCONVERT_CONV_TO_STRING(Bigint,QUAD,18);

int OdbcConvert::convBigintToTagNumeric(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)(pointer+3) = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	*pointer++=(char)from->precision;
	*pointer++=(char)from->scale;

	if ( number < 0 )
		number = -number,
		*pointer++=0;
	else
		*pointer++=1;

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagSQL_NUMERIC_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Numeric,Decimal
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Numeric,QUAD,Short,short);
ODBCCONVERT_CONV(Numeric,QUAD,Long,long);
ODBCCONVERT_CONV(Numeric,QUAD,Float,float);
ODBCCONVERT_CONV(Numeric,QUAD,Double,double);
ODBCCONVERT_CONV(Numeric,QUAD,Bigint,QUAD);
//ODBCCONVERT_CONV(Numeric,QUAD,Numeric,QUAD);

int OdbcConvert::convNumericToTagNumeric(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)(pointer+3) = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	*pointer++=(char)from->precision;
	*pointer++=(char)from->scale;

	if ( number < 0 )
		number = -number,
		*pointer++=0;
	else
		*pointer++=1;

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagSQL_NUMERIC_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
#define ODBCCONVERT_TEMP_CONV(TYPE_FROM,TYPE_TO,C_TYPE_TO)										\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	if ( indicatorPointer )																		\
		*indicatorPointer = 0;																	\
																								\
	return SQL_SUCCESS;																			\
}																								\
																								\
////////////////////////////////////////////////////////////////////////
// Date
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Date,long,Long,long);
ODBCCONVERT_CONV(Date,long,Float,float);
ODBCCONVERT_CONV(Date,long,Double,double);
ODBCCONVERT_CONV(Date,long,Bigint,QUAD);

int OdbcConvert::convDateToString(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	SQLUSMALLINT mday, month;
	SQLSMALLINT year;

	decode_sql_date(*(long*)getAdressBindDataFrom((char*)from->dataPtr), mday, month, year);
	int len, outlen = to->length;

	len = snprintf(pointer, outlen, "%04d-%02d-%02d",year,month,mday);

	if ( indicatorPointer )
	{
		if ( len == -1 )
			*indicatorPointer = outlen;
		else
			*indicatorPointer = len;
	}

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToTagDate(DescRecord * from, DescRecord * to)
{
	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_date(*(long*)getAdressBindDataFrom((char*)from->dataPtr), tagDt->day, tagDt->month, tagDt->year);

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagDATE_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToTagTimestamp(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_date(*(long*)getAdressBindDataFrom((char*)from->dataPtr), tagTs->day, tagTs->month, tagTs->year);
	tagTs->hour = tagTs->minute = tagTs->second = 0;
	tagTs->fraction = 0;

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Time,long,Long,long);
ODBCCONVERT_CONV(Time,long,Float,float);
ODBCCONVERT_CONV(Time,long,Double,double);
ODBCCONVERT_CONV(Time,long,Bigint,QUAD);

int OdbcConvert::convTimeToString(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	SQLUSMALLINT hour, minute, second;
	long ntime = *(long*)getAdressBindDataFrom((char*)from->dataPtr);
	long nnano = ntime % ISC_TIME_SECONDS_PRECISION;

	decode_sql_time(ntime, hour, minute, second);

	int len, outlen = to->length;

	if ( nnano )
		len = snprintf(pointer, outlen, "%02d:%02d:%02d.%lu",hour, minute, second, nnano);
	else
		len = snprintf(pointer, outlen, "%02d:%02d:%02d",hour, minute, second);

	if ( indicatorPointer )
	{
		if ( len == -1 )
			*indicatorPointer = outlen;
		else
			*indicatorPointer = len;
	}

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToTagTime(DescRecord * from, DescRecord * to)
{
	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_time(*(long*)getAdressBindDataFrom((char*)from->dataPtr), tagTm->hour, tagTm->minute, tagTm->second);

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagTIME_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToTagTimestamp(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	long ntime = *(long*)getAdressBindDataFrom((char*)from->dataPtr);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->day = tagTs->month = tagTs->year = 0;
	tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// DateTime
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(DateTime,QUAD,Double,double);
ODBCCONVERT_CONV(DateTime,QUAD,Bigint,QUAD);

int OdbcConvert::convDateTimeToString(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD pointerFrom = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	long ndate = LO_LONG(pointerFrom);
	long ntime = HI_LONG(pointerFrom);
	long nnano = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;
	SQLUSMALLINT mday, month;
	SQLSMALLINT year;
	SQLUSMALLINT hour, minute, second;

	decode_sql_date(ndate, mday, month, year);
	decode_sql_time(ntime, hour, minute, second);
	int len, outlen = to->length;

	if ( nnano )
		len = snprintf(pointer, outlen, "%04d-%02d-%02d %02d:%02d:%02d.%lu",year,month,mday,hour, minute, second, nnano);
	else
		len = snprintf(pointer, outlen, "%04d-%02d-%02d %02d:%02d:%02d",year,month,mday,hour, minute, second);

	if ( indicatorPointer )
	{
		if ( len == -1 )
			*indicatorPointer = outlen;
		else
			*indicatorPointer = len;
	}

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagDateTime(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);

	long nday = LO_LONG(number);
	long ntime = HI_LONG(number);

	decode_sql_date(nday, tagTs->day, tagTs->month, tagTs->year);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;

	if ( indicatorPointer )
		*indicatorPointer = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Blob
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_TEMP_CONV(Blob,Long,long);
ODBCCONVERT_TEMP_CONV(Blob,Float,float);
ODBCCONVERT_TEMP_CONV(Blob,Double,double);
ODBCCONVERT_TEMP_CONV(Blob,Bigint,QUAD);
//ODBCCONVERT_TEMP_CONV(Blob,Blob,short);
//ODBCCONVERT_TEMP_CONV(Blob,String,char);
/*
int OdbcConvert::convBlobToBlob(DescRecord * from, DescRecord * to)
{
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

//	ODBCCONVERT_CHECKNULL;
	if( *(short*)getAdressBindDataFrom((char*)from->indicatorPtr) == -1 )
	{
		if ( indicatorPointer )
			*indicatorPointer = -1;
		return SQL_SUCCESS;
	}

	if ( indicatorPointer )
		*indicatorPointer = 0;

	return SQL_SUCCESS;
}
*/

int OdbcConvert::convBlobToBlob(DescRecord * from, DescRecord * to)
{
	RETCODE ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	Blob *& blob = from->dataBlobPtr;
	long length = 0;

	if ( blob )
	{
		if ( from->currentFetched != parentStmt->getCurrentFetched() )
		{ // attach new blob
			from->dataOffset = 0;
			if ( parentStmt->isStaticCursor() )
				blob->attach((char*)from->dataPtr,parentStmt->isStaticCursor(),false);
			else
				blob->bind(parentStmt->connection->connection,(char*)from->dataPtr);
			from->currentFetched = parentStmt->getCurrentFetched();
		}

		length = blob->length();
		
		int dataRemaining = length - from->dataOffset;

		if ( !to->length )
			length = dataRemaining;
		else if (!dataRemaining && from->dataOffset)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else
		{
			int len = MIN(dataRemaining, MAX(0, (long)to->length));
			 
			if ( !pointer )
				length = dataRemaining;
			else
			{
				if ( len > 0 ) 
					blob->getBytes (from->dataOffset, len, pointer);

				from->dataOffset += len;

				if ( len && len < dataRemaining )
				{
					OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
					ret = SQL_SUCCESS_WITH_INFO;
				}
				length = dataRemaining;
			}
		}
	}

	if ( indicatorPointer )
		*indicatorPointer = length;

	return ret;
}

int OdbcConvert::convBlobToString(DescRecord * from, DescRecord * to)
{
	RETCODE ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	Blob *& blob = from->dataBlobPtr;
	long length = 0;

	if ( blob )
	{
		if ( from->currentFetched != parentStmt->getCurrentFetched() )
		{ // attach new blob
			from->dataOffset = 0;
			if ( parentStmt->isStaticCursor() )
				blob->attach((char*)from->dataPtr,parentStmt->isStaticCursor(),false);
			else
				blob->bind(parentStmt->connection->connection,(char*)from->dataPtr);

			from->currentFetched = parentStmt->getCurrentFetched();
		}

		length = blob->length();
		
		bool isBlob = blob->isBlob();

		if ( isBlob )
			length *= 2;

		int dataRemaining = length - from->dataOffset;

		if ( !to->length )
			length = dataRemaining;
		else if (!dataRemaining && from->dataOffset)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else
		{
			int len = MIN(dataRemaining, MAX(0, (long)to->length-1));
			 
			if ( !pointer )
				length = dataRemaining;
			else
			{
				if ( len > 0 ) 
				{
					if ( isBlob )
						blob->getHexString (from->dataOffset/2, len/2, pointer);
					else
						blob->getBytes (from->dataOffset, len, pointer);

					((char*) (pointer)) [len] = 0;
				}

				from->dataOffset += len;

				if ( len && len < dataRemaining )
				{
					OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
					ret = SQL_SUCCESS_WITH_INFO;
				}
					
				length = dataRemaining;
			}
		}
	}

	if ( indicatorPointer )
		*indicatorPointer = length;

	return ret;
}


////////////////////////////////////////////////////////////////////////
// String
////////////////////////////////////////////////////////////////////////

#define OFFSET_STRING  0
#define CALC_LEN_STRING  strlen((char*)p)

#define ODBCCONVERT_CONV_STRING_TO(TYPE_FROM,TYPE_TO,C_TYPE_TO)									\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);	\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	/* Original source from IscDbc/Value.cpp */													\
	QUAD number = 0;																			\
	double divisor = 1;																			\
	bool decimal = false;																		\
	bool negative = false;																		\
	char	*p = (char*)getAdressBindDataFrom((char*)from->dataPtr + OFFSET_STRING),			\
			*end = p + CALC_LEN_STRING;															\
	for (;p < end;)																				\
	{																							\
		char c = *p++;																			\
		if (c >= '0' && c <= '9')																\
		{																						\
			number = number * 10 + c - '0';														\
			if (decimal)																		\
				divisor *= 10;																	\
		}																						\
		else if (c == '-')																		\
			negative = true;																	\
																								\
		else if (c == '.')																		\
			decimal = true;																		\
		else if (c == '+' || c == ',' || c == '\'' || c == '`')									\
			;																					\
		else if (c != ' ' && c != '\t' && c != '\n')											\
			break;																				\
	}																							\
																								\
	if( negative )number = -number;																\
																								\
	int scale = to->scale;																		\
																								\
	if ( scale )																				\
	{																							\
		if (scale < 0)																			\
			for (; scale; ++scale)																\
				divisor /= 10;																	\
		else if (scale > 0)																		\
			for (; scale; --scale)																\
				divisor *= 10;																	\
	}																							\
																								\
	if (divisor == 1)																			\
		*(C_TYPE_TO*)pointer = (C_TYPE_TO)number;												\
	else																						\
		*(C_TYPE_TO*)pointer = (C_TYPE_TO)(number / divisor);									\
																								\
	if ( indicatorPointer )																		\
		*indicatorPointer = sizeof(C_TYPE_TO);													\
																								\
	return SQL_SUCCESS;																			\
}																								\

ODBCCONVERT_CONV_STRING_TO(String,Short,short);
ODBCCONVERT_CONV_STRING_TO(String,Long,long);
ODBCCONVERT_CONV_STRING_TO(String,Float,float);
ODBCCONVERT_CONV_STRING_TO(String,Double,double);
ODBCCONVERT_CONV_STRING_TO(String,Bigint,QUAD);

int OdbcConvert::convStringToString(DescRecord * from, DescRecord * to)
{
	SQLPOINTER pointerFrom = getAdressBindDataFrom((char*)from->dataPtr);
	SQLPOINTER pointerTo = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	RETCODE ret = SQL_SUCCESS;
	int len = MIN((int)from->length, (int)MAX(0, (int)to->length-1));
#pragma FB_COMPILER_MESSAGE("Dispute on a theme \"Whether it is necessary to carry out trimBlanks on a type CHAR?\" FIXME!")
	len = MIN(len, (int)strlen((char*)pointerFrom));
	
	if( len )
		memcpy (pointerTo, pointerFrom, len);

	((char*) (pointerTo)) [len] = 0;
	
	if (from->length && (long)from->length > (long)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( indicatorPointer )
		*indicatorPointer = len;

	return ret;
}

#undef OFFSET_STRING
#undef CALC_LEN_STRING
#define OFFSET_STRING  sizeof(short)
#define CALC_LEN_STRING  *(short*)p

ODBCCONVERT_CONV_STRING_TO(VarString,Short,short);
ODBCCONVERT_CONV_STRING_TO(VarString,Long,long);
ODBCCONVERT_CONV_STRING_TO(VarString,Float,float);
ODBCCONVERT_CONV_STRING_TO(VarString,Double,double);
ODBCCONVERT_CONV_STRING_TO(VarString,Bigint,QUAD);

int OdbcConvert::convVarStringToString(DescRecord * from, DescRecord * to)
{
	SQLPOINTER pointerFrom = getAdressBindDataFrom((char*)from->dataPtr);
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	RETCODE ret = SQL_SUCCESS;
	unsigned short lenVar = *(unsigned short*)pointerFrom;
	int len = MIN(lenVar, MAX(0,(int)to->length-1));
	
	if( len > 0 )
		memcpy (pointer, (char*)pointerFrom + 2, len);
	
	((char*) (pointer)) [len] = 0;

	if (lenVar && (long)lenVar > (long)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( indicatorPointer )
		*indicatorPointer = len;

	return ret;
}

signed long OdbcConvert::encode_sql_date(SQLUSMALLINT day, SQLUSMALLINT month, SQLSMALLINT year)
{
/**************************************
 *
 *	n d a y
 *
 **************************************
 *
 * Functional description
 *	Convert a calendar date to a numeric day
 *	(the number of days since the base date).
 *
 **************************************/
	signed long	c, ya;

	if (month > 2)
		month -= 3;
	else
	{
		month += 9;
		year -= 1;
	}

	c = year / 100;
	ya = year - 100 * c;

	return (signed long) (((QUAD) 146097 * c) / 4 + 
		(1461 * ya) / 4 + 
		(153 * month + 2) / 5 + 
		day - 678882); //	day + 1721119 - 2400001);
}

void OdbcConvert::decode_sql_date(signed long nday, SQLUSMALLINT &mday, SQLUSMALLINT &month, SQLSMALLINT &year)
{
/**************************************
 *
 *	n d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert a numeric day to [day, month, year].
 *
 * Calenders are divided into 4 year cycles.
 * 3 Non-Leap years, and 1 leap year.
 * Each cycle takes 365*4 + 1 == 1461 days.
 * There is a further cycle of 100 4 year cycles.
 * Every 100 years, the normally expected leap year
 * is not present.  Every 400 years it is.
 * This cycle takes 100 * 1461 - 3 == 146097 days
 * The origin of the constant 2400001 is unknown.
 * The origin of the constant 1721119 is unknown.
 * The difference between 2400001 and 1721119 is the
 * number of days From 0/0/0000 to our base date of
 * 11/xx/1858. (678882)
 * The origin of the constant 153 is unknown.
 *
 * This whole routine has problems with ndates
 * less than -678882 (Approx 2/1/0000).
 *
 **************************************/
	signed long	day;
	signed long	century;

//	nday -= 1721119 - 2400001;
	nday += 678882;

	century = (4 * nday - 1) / 146097;
	nday = 4 * nday - 1 - 146097 * century;
	day = nday / 4;

	nday = (4 * day + 3) / 1461;
	day = 4 * day + 3 - 1461 * nday;
	day = (day + 4) / 4;

	month = (SQLUSMALLINT)((5 * day - 3) / 153);
	day = 5 * day - 3 - 153 * month;
	mday = (SQLUSMALLINT)((day + 5) / 5);

	year = (short)(100 * century + nday);

	if (month < 10)
		month += 3;
	else
	{
		month -= 9;
		year += 1;
	}
}

signed long OdbcConvert::encode_sql_time(SQLUSMALLINT hour, SQLUSMALLINT minute, SQLUSMALLINT second)
{
	return ((hour * 60 + minute) * 60 +
				 second) * ISC_TIME_SECONDS_PRECISION;
}

void OdbcConvert::decode_sql_time(signed long ntime, SQLUSMALLINT &hour, SQLUSMALLINT &minute, SQLUSMALLINT &second)
{
	long minutes;

	minutes = ntime / (ISC_TIME_SECONDS_PRECISION * 60);
	hour = (SQLUSMALLINT)(minutes / 60);
	minute = (SQLUSMALLINT)(minutes % 60);
	second = (SQLUSMALLINT)((ntime / ISC_TIME_SECONDS_PRECISION) % 60);
}

void OdbcConvert::convertFloatToString(double value, char *string, int size, int *length, int digit, char POINT_DIV)
{
	char temp[64];
	char * dst = temp;
	int  decimal, sign;
	bool copy = false;
	char * strCvt = fcvt( value, digit, &decimal, &sign );
	int len = strlen( strCvt );

	if ( !size )
		return;

	if ( size >= 24 )
		dst = string;
	else
		copy = true;

	if ( !*strCvt )
	{
		len = strlen( gcvt( value, digit, dst) );
		char * end = dst + len - 1;
		if ( *end == '.' )
			*end = '\0',--len;
	}
	else if ( !len )
	{
		*dst++ = '0';
		*dst = '\0';
		len = 1;
	}
	else
	{
		char strF[20];
		char * src = strF, * end, * begin = dst;

		if ( sign )
			*dst++ = '-';

		if ( len < digit + 1 )
		{
			char * ch = strCvt;
			end = strF;
			while ( (*end++ = *ch++) );
			end -= 2;
		}
		else
		{
			char * ch = strCvt;
			char * chEnd = strCvt + digit;
			end = strF;

			if ( *(strCvt + digit) < '5' )
			{
				while ( ch < chEnd )
					*end++ = *ch++;
				*end-- = '\0';
			}
			else
			{
				*strF = '0';
				end++;
				while ( ch < chEnd )
					*end++ = *ch++;
				*end-- = '\0';
				chEnd = end;

				while ( chEnd > strF && *chEnd + 1 > '9' )
					*chEnd-- = '0';

				++(*chEnd);

				if ( chEnd > strF )
					src++;
			}
		}

		if ( decimal <= 0 )
		{
			int dec = decimal;

			while ( end > src && *end == '0' )
				*end-- = '\0';

			if ( end >= src )
			{
				*dst++ = '0';
				*dst++ = POINT_DIV;

				while ( dec++ )
					*dst++ = '0';

				while ( (*dst++ = *src++) );
				--dst;
			}
			else // if ( *end == '0' )
			{
				dst = begin;
				*dst++ = '0';
				*dst = '\0';
			}
		}			
		else if ( decimal > 0 )
		{
			int dec = decimal;
			while ( *src )
			{
				*dst++ = *src++;
				if (--dec == 0)
				{
					if ( *src && decimal < digit )
						*dst++ = POINT_DIV;
					else
						break;
				}
			}

			if ( dec > 0 )
				while ( dec-- )
					*dst++ = '0';
			else
			{
				--dst;
				while ( *dst == '0' )
					*dst-- = '\0';

				if ( *dst == POINT_DIV )
					*dst-- = '\0';
				++dst;
			}
			*dst = '\0';
		}
		len = dst - begin;
	}
	if ( copy )
	{
		len = MIN( len, size - 1 );
		memcpy( string, temp, len );
		string[len] = '\0';
	}
	*length = len;
}

}; // end namespace OdbcJdbcLibrary
