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
#include <stdio.h>
#include <stdlib.h>
#include "OdbcJdbc.h"
#include "OdbcDesc.h"
#include "OdbcConnection.h"
#include "IscDbc/Connection.h"
#include "DescRecord.h"
#include "IscDbc/SQLException.h"

#define MAKEQUAD(a, b)      ((QUAD)(((long)(a)) | ((UQUAD)((long)(b))) << 32))
#define LO_LONG(l)           ((long)(l))
#define HI_LONG(l)           ((long)(((UQUAD)(l) >> 32) & 0xFFFFFFFF))

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
#else
unsigned __int64 listScale[] =
{
	0x0000000000000001,
	0x000000000000000a,
	0x0000000000000064,
	0x00000000000003e8,
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

OdbcConvert::OdbcConvert()
{
	bIdentity = false;
	bindOffsetPtr = NULL;
}

void OdbcConvert::setBindOffsetPtr(SQLINTEGER	**ptBindOffsetPtr)
{
	bindOffsetPtr = ptBindOffsetPtr;
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
		case SQL_C_CHAR:
			return &OdbcConvert::convBigintToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convBigintToTagNumeric;
		}

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
		case SQL_C_CHAR:
			return &OdbcConvert::convDateToString;
		}

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
		case SQL_C_CHAR:
			return &OdbcConvert::convTimeToString;
		}

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

	case SQL_C_CHAR:
		switch(to->conciseType)
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
			if ( from->type == JDBC_VARCHAR )
				return &OdbcConvert::convVarStringToString;
			bIdentity = true;
			return &OdbcConvert::convStringToString;
		}
	}
	return NULL;
}

inline 
SQLPOINTER OdbcConvert::getAdressData(char * pointer)
{
	return (SQLPOINTER)(pointer + **bindOffsetPtr);
}

#define ODBCCONVERT_CHECKNULL					\
	if( *(short*)from->indicatorPtr == -1 )		\
	{											\
		if ( indicatorPointer )					\
			*indicatorPointer = -1;				\
		return 0;								\
	}											\

#define ODBCCONVERT_CONV(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)							\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)			\
{																							\
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);									\
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);	\
																							\
	ODBCCONVERT_CHECKNULL;																	\
																							\
	*(C_TYPE_TO*)pointer = (C_TYPE_TO)*(C_TYPE_FROM*)from->dataPtr;							\
	if ( indicatorPointer )																	\
		*indicatorPointer = sizeof(C_TYPE_TO);												\
																							\
	return 0;																				\
}																							\

#define ODBCCONVERT_CONVTAGNUMERIC(TYPE_FROM,C_TYPE_FROM)									\
int OdbcConvert::conv##TYPE_FROM##ToTagNumeric(DescRecord * from, DescRecord * to)			\
{																							\
	char* pointer = (char*)getAdressData((char*)to->dataPtr);								\
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);	\
																							\
	ODBCCONVERT_CHECKNULL;																	\
																							\
	QUAD &number = *(QUAD*)(pointer+3) = (QUAD)*(C_TYPE_FROM*)from->dataPtr;				\
	*pointer++=(char)from->precision;														\
	*pointer++=(char)from->scale;															\
																							\
	if ( number < 0 )																		\
		number = -number,																	\
		*pointer++=0;																		\
	else																					\
		*pointer++=1;																		\
																							\
	if ( from->scale )																		\
		number *= (QUAD)listScale[from->scale];												\
																							\
	if ( indicatorPointer )																	\
		*indicatorPointer = 0;																\
																							\
	return 0;																				\
}																							\

#define ODBCCONVERT_CONVROUND(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)						\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)			\
{																							\
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);									\
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);	\
																							\
	ODBCCONVERT_CHECKNULL;																	\
																							\
	C_TYPE_FROM valFrom = *(C_TYPE_FROM*)from->dataPtr;										\
	if ( valFrom < 0 )valFrom -= 0.5;														\
	else valFrom += 0.5;																	\
																							\
	*(C_TYPE_TO*)pointer = (C_TYPE_TO)valFrom;												\
	if ( indicatorPointer )																	\
		*indicatorPointer = sizeof(C_TYPE_TO);												\
																							\
	return 0;																				\
}																							\

#define ODBCCONVERT_CONV_TO_STRING(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)							\
int OdbcConvert::conv##TYPE_FROM##ToString(DescRecord * from, DescRecord * to)				\
{																							\
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);									\
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);	\
																							\
	ODBCCONVERT_CHECKNULL;																	\
																							\
	int len = to->length;																	\
																							\
	if ( !len )																				\
		*(char*)to->dataPtr = 0;															\
	else																					\
	{	/* Original source from IscDbc/Value.cpp */											\
		C_TYPE_FROM number = *(C_TYPE_FROM*)from->dataPtr;									\
		char *string = (char*)pointer;														\
		int scale = from->scale;															\
																							\
		if (number == 0)																	\
			strcpy (string, "0");															\
		else if (scale < -DEF_SCALE)														\
			strcpy (string, "***");															\
		else																				\
		{																					\
			bool negative = false;															\
																							\
			if (number < 0)																	\
			{																				\
				number = -number;															\
				negative = true;															\
			}																				\
																							\
			char temp [100], *p = temp;														\
			int n;																			\
			for (n = 0; number; number /= 10, --n)											\
			{																				\
				if (scale && scale == n)													\
					*p++ = '.';																\
				*p++ = '0' + (char) (number % 10);											\
			}																				\
																							\
			if (scale <= n)																	\
			{																				\
				for (; n > scale; --n)														\
					*p++ = '0';																\
				*p++ = '.';																	\
			}																				\
																							\
			char *q = string;																\
			int l=0;																		\
																							\
			if (negative)																	\
				*q++ = '-',++l;																\
																							\
			if ( p - temp > len - l )														\
				p = temp + len - l;															\
																							\
			while (p > temp)																\
				*q++ = *--p;																\
																							\
			*q = 0;																			\
		}																					\
	}																						\
																							\
	if ( indicatorPointer )																	\
		*indicatorPointer = len;															\
																							\
	return len;																				\
}																							\

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
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	int len = snprintf((char*)to->dataPtr,to->octetLength,"%f",*(float*)from->dataPtr);
//	int len = snprintf((char*)to->dataPtr,to->octetLength,"%.*f",from->scale,*(float*)from->dataPtr);

	if ( indicatorPointer )
		*indicatorPointer = len;

	return len;
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
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	int len = snprintf((char*)to->dataPtr,to->octetLength,"%f",*(double*)from->dataPtr);
//	int len = snprintf((char*)to->dataPtr,to->octetLength,"%.*f",from->scale,*(double*)from->dataPtr);

	if ( indicatorPointer )
		*indicatorPointer = len;

	return len;
}

////////////////////////////////////////////////////////////////////////
// Bigint
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Bigint,QUAD,Short,short);
ODBCCONVERT_CONV(Bigint,QUAD,Long,long);
ODBCCONVERT_CONV(Bigint,QUAD,Float,float);
ODBCCONVERT_CONV(Bigint,QUAD,Double,double);
ODBCCONVERT_CONV(Bigint,QUAD,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(Bigint,QUAD,18);

int OdbcConvert::convBigintToTagNumeric(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)(pointer+3) = *(QUAD*)from->dataPtr;
	*pointer++=(char)from->precision;
	*pointer++=(char)from->scale;

	if ( number < 0 )
		number = -number,
		*pointer++=0;
	else
		*pointer++=1;

	if ( indicatorPointer )
		*indicatorPointer = 0;

	return 0;
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
	char* pointer = (char*)getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)(pointer+3) = *(QUAD*)from->dataPtr;
	*pointer++=(char)from->precision;
	*pointer++=(char)from->scale;

	if ( number < 0 )
		number = -number,
		*pointer++=0;
	else
		*pointer++=1;

	if ( indicatorPointer )
		*indicatorPointer = 0;

	return 0;
}

////////////////////////////////////////////////////////////////////////
#define ODBCCONVERT_TEMP_CONV(TYPE_FROM,TYPE_TO,C_TYPE_TO)									\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)			\
{																							\
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);									\
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);	\
																							\
	ODBCCONVERT_CHECKNULL;																	\
																							\
	if ( indicatorPointer )																	\
		*indicatorPointer = 0;																\
																							\
	return 0;																				\
}																							\
																							\
////////////////////////////////////////////////////////////////////////
// Date
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_TEMP_CONV(Date,Long,long);
ODBCCONVERT_TEMP_CONV(Date,Float,float);
ODBCCONVERT_TEMP_CONV(Date,Double,double);
ODBCCONVERT_TEMP_CONV(Date,Bigint,QUAD);
//ODBCCONVERT_TEMP_CONV(Date,TagDate,short);
ODBCCONVERT_TEMP_CONV(Date,String,char);

int OdbcConvert::convDateToTagDate(DescRecord * from, DescRecord * to)
{
	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_date(*(long*)from->dataPtr, tagDt->day, tagDt->month, tagDt->year);

	if ( indicatorPointer )
		*indicatorPointer = 0;

	return 0;
}

////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_TEMP_CONV(Time,Long,long);
ODBCCONVERT_TEMP_CONV(Time,Float,float);
ODBCCONVERT_TEMP_CONV(Time,Double,double);
ODBCCONVERT_TEMP_CONV(Time,Bigint,QUAD);
//ODBCCONVERT_TEMP_CONV(Time,TagTime,short);
ODBCCONVERT_TEMP_CONV(Time,String,char);

int OdbcConvert::convTimeToTagTime(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTm = (tagTIMESTAMP_STRUCT*)getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_time(*(long*)from->dataPtr, tagTm->hour, tagTm->minute, tagTm->second);

	if ( indicatorPointer )
		*indicatorPointer = 0;

	return 0;
}


////////////////////////////////////////////////////////////////////////
// DateTime
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_TEMP_CONV(DateTime,Double,double);
ODBCCONVERT_TEMP_CONV(DateTime,Bigint,QUAD);
//ODBCCONVERT_TEMP_CONV(DateTime,TagDateTime,short);
ODBCCONVERT_TEMP_CONV(DateTime,String,char);

int OdbcConvert::convDateTimeToTagDateTime(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)from->dataPtr;

	long nday = LO_LONG(number);
	long ntime = HI_LONG(number);

	decode_sql_date(nday, tagTs->day, tagTs->month, tagTs->year);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->fraction = ntime % ISC_TIME_SECONDS_PRECISION;

	if ( indicatorPointer )
		*indicatorPointer = 0;

	return 0;
}

////////////////////////////////////////////////////////////////////////
// Blob
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_TEMP_CONV(Blob,Long,long);
ODBCCONVERT_TEMP_CONV(Blob,Float,float);
ODBCCONVERT_TEMP_CONV(Blob,Double,double);
ODBCCONVERT_TEMP_CONV(Blob,Bigint,QUAD);
ODBCCONVERT_TEMP_CONV(Blob,Blob,short);
ODBCCONVERT_TEMP_CONV(Blob,String,char);

////////////////////////////////////////////////////////////////////////
// String
////////////////////////////////////////////////////////////////////////

#define ODBCCONVERT_CONV_STRING_TO(TYPE_TO,C_TYPE_TO)											\
int OdbcConvert::convStringTo##TYPE_TO(DescRecord * from, DescRecord * to)						\
{																								\
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);										\
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);		\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	/* Original source from IscDbc/Value.cpp */													\
	QUAD number = 0;																			\
	double divisor = 1;																			\
	bool decimal = false;																		\
	bool negative = false;																		\
																								\
	for (char *p = (char*)from->dataPtr, *end = p + strlen((char*)from->dataPtr); p < end;)		\
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
	return 0;																					\
}																								\

ODBCCONVERT_CONV_STRING_TO(Short,short);
ODBCCONVERT_CONV_STRING_TO(Long,long);
ODBCCONVERT_CONV_STRING_TO(Float,float);
ODBCCONVERT_CONV_STRING_TO(Double,double);
ODBCCONVERT_CONV_STRING_TO(Bigint,QUAD);

int OdbcConvert::convStringToString(DescRecord * from, DescRecord * to)
{
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	int len = MIN(from->length, (int)to->length);
	
	if( len )
		memcpy (pointer, from->dataPtr, len);

//	!!! 99 tip 
	
	((char*) (pointer)) [len] = 0;
	
	if ( indicatorPointer )
		*indicatorPointer = len;

	return len;
}

int OdbcConvert::convVarStringToString(DescRecord * from, DescRecord * to)
{
	SQLPOINTER pointer = getAdressData((char*)to->dataPtr);
	SQLINTEGER * indicatorPointer = (SQLINTEGER *)getAdressData((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	int len = MIN(*(short*)from->dataPtr, (int)to->length);
	
	if( len > 0 )
		memcpy (pointer, (char*)from->dataPtr + 2, len);
	
	((char*) (pointer)) [len] = 0;

	if ( indicatorPointer )
		*indicatorPointer = len;

	return len;
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

	month = (5 * day - 3) / 153;
	day = 5 * day - 3 - 153 * month;
	mday = (day + 5) / 5;

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
	hour = minutes / 60;
	minute = minutes % 60;
	second = (ntime / ISC_TIME_SECONDS_PRECISION) % 60;
}
