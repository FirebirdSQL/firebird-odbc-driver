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
#include "OdbcEnv.h"
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
	bindOffsetPtrIndFrom = &tempBindOffsetPtr;
}

void OdbcConvert::setZeroColumn(DescRecord * to, long rowNumber)
{
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	*(long*)pointer = rowNumber + 1;
	if ( indicatorTo )
		*indicatorTo = sizeof(long);
}

void OdbcConvert::setBindOffsetPtrTo(SQLINTEGER	*bindOffsetPtr, SQLINTEGER *bindOffsetPtrInd)
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

void OdbcConvert::setBindOffsetPtrFrom(SQLINTEGER *bindOffsetPtr, SQLINTEGER *bindOffsetPtrInd)
{
	if( bindOffsetPtr )
		bindOffsetPtrFrom = bindOffsetPtr;
	else
		bindOffsetPtrFrom = &tempBindOffsetPtr;

	if( bindOffsetPtrInd )
		bindOffsetPtrIndFrom = bindOffsetPtrInd;
	else
		bindOffsetPtrIndFrom = &tempBindOffsetPtr;
}

ADRESS_FUNCTION OdbcConvert::getAdresFunction(DescRecord * from, DescRecord * to)
{
	bIdentity = false;

	if ( to->isIndicatorSqlDa )
		setHeadSqlVar ( to );

	switch(from->conciseType)
	{
	case SQL_C_TINYINT:
	case SQL_C_UTINYINT:
	case SQL_C_STINYINT:
		switch(to->conciseType)
		{
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			bIdentity = true;
			return &OdbcConvert::convTinyIntToTinyInt;
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			bIdentity = true;
			return &OdbcConvert::convTinyIntToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			return &OdbcConvert::convTinyIntToLong;
		case SQL_C_FLOAT:
			return &OdbcConvert::convTinyIntToFloat;
		case SQL_C_DOUBLE:
			return &OdbcConvert::convTinyIntToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convTinyIntToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convTinyIntToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convTinyIntToTagNumeric;
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

	case SQL_C_SHORT:
	case SQL_C_USHORT:
	case SQL_C_SSHORT:
		switch(to->conciseType)
		{
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			return &OdbcConvert::convShortToTinyInt;
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
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			return &OdbcConvert::convLongToTinyInt;
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
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			return &OdbcConvert::convFloatToTinyInt;
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
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			return &OdbcConvert::convDoubleToTinyInt;
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
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			return &OdbcConvert::convBigintToTinyInt;
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
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToTinyInt;
			return &OdbcConvert::convNumericToTinyInt;
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToShort;
			return &OdbcConvert::convNumericToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToLong;
			return &OdbcConvert::convNumericToLong;
		case SQL_C_FLOAT:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToFloat;
			return &OdbcConvert::convNumericToFloat;
		case SQL_C_DOUBLE:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToDouble;
			return &OdbcConvert::convNumericToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToBigint;
			return &OdbcConvert::convNumericToBigint;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convTagNumericToBigint;
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
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagDateToDate;
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
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagTimeToTime;
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
		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagDateTimeToDate;
			return &OdbcConvert::convDateTimeToTagDate;
		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagDateTimeToTime;
			return &OdbcConvert::convDateTimeToTagTime;
		case SQL_C_TYPE_TIMESTAMP:
		case SQL_C_TIMESTAMP:
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagDateTimeToDateTime;
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
			if ( to->isIndicatorSqlDa )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferBinaryStringToAllowedType;
			}
			return &OdbcConvert::convBlobToLong;
		case SQL_C_FLOAT:
			if ( to->isIndicatorSqlDa )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferBinaryStringToAllowedType;
			}
			return &OdbcConvert::convBlobToFloat;
		case SQL_C_DOUBLE:
			if ( to->isIndicatorSqlDa )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferBinaryStringToAllowedType;
			}
			return &OdbcConvert::convBlobToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			if ( to->isIndicatorSqlDa )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferBinaryStringToAllowedType;
			}
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
			case SQL_C_TINYINT:
			case SQL_C_UTINYINT:
			case SQL_C_STINYINT:
				return &OdbcConvert::convVarStringToTinyInt;
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
				if ( from->isIndicatorSqlDa && from->dataBlobPtr && from->dataBlobPtr->isArray() )
					return &OdbcConvert::convBlobToString;
				bIdentity = true;
				return &OdbcConvert::convVarStringToString;
			case SQL_C_BINARY:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr && from->dataBlobPtr->isArray() )
					return &OdbcConvert::convBlobToString;
			}
		else 
			switch(to->conciseType) // Text
			{
			case SQL_C_LONG:
			case SQL_C_ULONG:
			case SQL_C_SLONG:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToAllowedType;
				}
				return &OdbcConvert::convStringToLong;
			case SQL_C_TINYINT:
			case SQL_C_UTINYINT:
			case SQL_C_STINYINT:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToTinyInt;
				}
				return &OdbcConvert::convStringToTinyInt;
			case SQL_C_SHORT:
			case SQL_C_USHORT:
			case SQL_C_SSHORT:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToAllowedType;
				}
				return &OdbcConvert::convStringToShort;
			case SQL_C_FLOAT:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToAllowedType;
				}
				return &OdbcConvert::convStringToFloat;
			case SQL_C_DOUBLE:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToAllowedType;
				}
				return &OdbcConvert::convStringToDouble;
			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToAllowedType;
				}
				return &OdbcConvert::convStringToBigint;
			case SQL_C_CHAR:
				if ( to->type == SQL_VARCHAR )
				{
					if ( to->isIndicatorSqlDa )
					{
						if ( to->isBlobOrArray )
							return &OdbcConvert::convStringToBlob;
						to->headSqlVarPtr->setTypeText();
						return &OdbcConvert::transferStringToAllowedType;
					}
					return &OdbcConvert::convStringToVarString;
				}
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToAllowedType;
				}
				bIdentity = true;
				return &OdbcConvert::convStringToString;
			case SQL_C_BINARY:
				if ( to->isIndicatorSqlDa )
					return &OdbcConvert::convStringToBlob;
				return &OdbcConvert::convStringToString;
			case SQL_C_DATE:
			case SQL_C_TYPE_DATE:
			case SQL_C_TIME:
			case SQL_C_TYPE_TIME:
			case SQL_C_TYPE_TIMESTAMP:
			case SQL_C_TIMESTAMP:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringToDateTime;
				}
#pragma FB_COMPILER_MESSAGE("Realized convStringToDateTime() FIXME!")
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
SQLINTEGER * OdbcConvert::getAdressBindIndFrom(char * pointer)
{
	return (SQLINTEGER *)(pointer + *bindOffsetPtrIndFrom);
}

inline 
SQLPOINTER OdbcConvert::getAdressBindDataTo(char * pointer)
{
	return (SQLPOINTER)(pointer + *bindOffsetPtrTo);
}

inline
SQLINTEGER * OdbcConvert::getAdressBindIndTo(char * pointer)
{
	return (SQLINTEGER *)(pointer + *bindOffsetPtrIndTo);
}

#define ODBCCONVERT_CHECKNULL					\
	if( *(short*)indicatorFrom == -1 )			\
	{											\
		if ( indicatorTo )						\
			*indicatorTo = -1;					\
		return SQL_SUCCESS;						\
	}											\

#define ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO)				\
	if ( from->isIndicatorSqlDa )							\
	{														\
		if( *(short*)indicatorFrom == -1 )					\
		{													\
			if ( indicatorTo )								\
				*indicatorTo = -1;							\
			return SQL_SUCCESS;								\
		}													\
		else if ( indicatorTo )								\
			*indicatorTo = sizeof(C_TYPE_TO);				\
	}														\
	else /* if ( to->isIndicatorSqlDa ) */					\
	{														\
		if( indicatorFrom && *indicatorFrom == -1 )			\
		{													\
			*(short*)indicatorTo = -1;						\
			return SQL_SUCCESS;								\
		}													\
		else												\
			*indicatorTo = 0;								\
	}														\

#define ODBCCONVERT_CHECKNULL_SQLDA					\
	if( indicatorFrom && *indicatorFrom == -1 )		\
	{												\
		*(short*)indicatorTo = -1;					\
		return SQL_SUCCESS;							\
	}												\

#define ODBCCONVERT_CONV(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)								\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	*(C_TYPE_TO*)pointer=(C_TYPE_TO)*(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);	\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERTBIGINT_CONV(TYPE_TO,C_TYPE_TO)												\
int OdbcConvert::convBigintTo##TYPE_TO(DescRecord * from, DescRecord * to)						\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	*(C_TYPE_TO*)pointer = ( (C_TYPE_TO)*(QUAD*)getAdressBindDataFrom((char*)from->dataPtr)	)	\
					/(C_TYPE_TO)(QUAD)listScale[from->scale];									\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERTTAG_NUMERIC_CONV(TYPE_TO,C_TYPE_TO)											\
int OdbcConvert::convTagNumericTo##TYPE_TO(DescRecord * from, DescRecord * to)					\
{																								\
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	C_TYPE_TO * pointer = (C_TYPE_TO*)getAdressBindDataTo((char*)to->dataPtr);					\
																								\
	QUAD val;																					\
	tagSQL_NUMERIC_STRUCT * nm =																\
					(tagSQL_NUMERIC_STRUCT *)getAdressBindDataFrom((char*)from->dataPtr);		\
																								\
	val = *(QUAD*)nm->val;																		\
																								\
	if ( to->scale != nm->scale )																\
		val = ( val * listScale[to->scale] ) / listScale[nm->scale];							\
																								\
	if ( !nm->sign )																			\
		val = -val;																				\
																								\
	*pointer = (C_TYPE_TO)val;																	\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONVTAGNUMERIC(TYPE_FROM,C_TYPE_FROM)										\
int OdbcConvert::conv##TYPE_FROM##ToTagNumeric(DescRecord * from, DescRecord * to)				\
{																								\
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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
	if ( indicatorTo )																			\
		*indicatorTo = sizeof(tagSQL_NUMERIC_STRUCT);											\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONVROUND(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)							\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	C_TYPE_FROM valFrom = *(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);			\
	if ( valFrom < 0 )valFrom -= 0.5;															\
	else valFrom += 0.5;																		\
																								\
	*(C_TYPE_TO*)pointer = (C_TYPE_TO)valFrom;													\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONV_TO_STRING(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)								\
int OdbcConvert::conv##TYPE_FROM##ToString(DescRecord * from, DescRecord * to)					\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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
		else if (scale < -DEF_SCALE)															\
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
	if ( indicatorTo )																			\
		*indicatorTo = len;																		\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONV_TO_BINARY(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)								\
int OdbcConvert::conv##TYPE_FROM##ToBinary(DescRecord * from, DescRecord * to)					\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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
		else if (scale < -DEF_SCALE)															\
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
	if ( indicatorTo )																			\
		*indicatorTo = len << 1;																\
																								\
	return SQL_SUCCESS;																			\
}																								\

////////////////////////////////////////////////////////////////////////
// TinyInt
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(TinyInt,char,TinyInt,char);
ODBCCONVERT_CONV(TinyInt,char,Short,short);
ODBCCONVERT_CONV(TinyInt,char,Long,long);
ODBCCONVERT_CONV(TinyInt,char,Float,float);
ODBCCONVERT_CONV(TinyInt,char,Double,double);
ODBCCONVERT_CONV(TinyInt,char,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(TinyInt,char,3);
ODBCCONVERT_CONVTAGNUMERIC(TinyInt,char);

////////////////////////////////////////////////////////////////////////
// Short
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Short,short,TinyInt,char);
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

ODBCCONVERT_CONV(Long,long,TinyInt,char);
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

ODBCCONVERT_CONVROUND(Float,float,TinyInt,char);
ODBCCONVERT_CONVROUND(Float,float,Short,short);
ODBCCONVERT_CONVROUND(Float,float,Long,long);
ODBCCONVERT_CONV(Float,float,Float,float);
ODBCCONVERT_CONV(Float,float,Double,double);
ODBCCONVERT_CONVROUND(Float,float,Bigint,QUAD);

int OdbcConvert::convFloatToString(DescRecord * from, DescRecord * to)
{
	char * pointerTo = (char *)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	int len = to->length;

	if ( len )	// MAX_FLOAT_DIGIT_LENGTH = 7
		convertFloatToString(*(float*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len-1, &len, 7);

	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Double
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONVROUND(Double,double,TinyInt,char);
ODBCCONVERT_CONVROUND(Double,double,Short,short);
ODBCCONVERT_CONVROUND(Double,double,Long,long);
ODBCCONVERT_CONV(Double,double,Float,float);
ODBCCONVERT_CONV(Double,double,Double,double);
ODBCCONVERT_CONVROUND(Double,double,Bigint,QUAD);
ODBCCONVERT_CONVTAGNUMERIC(Double,double);

int OdbcConvert::convDoubleToString(DescRecord * from, DescRecord * to)
{
	char * pointerTo = (char *)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	int len = to->length;

	if ( len )	// MAX_DOUBLE_DIGIT_LENGTH = 15
		convertFloatToString(*(double*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len-1, &len);

	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Bigint
////////////////////////////////////////////////////////////////////////

ODBCCONVERTBIGINT_CONV(TinyInt,char);
ODBCCONVERTBIGINT_CONV(Short,short);
ODBCCONVERTBIGINT_CONV(Long,long);
ODBCCONVERTBIGINT_CONV(Float,float);
ODBCCONVERTBIGINT_CONV(Double,double);
ODBCCONVERT_CONV(Bigint,QUAD,Bigint,QUAD);
ODBCCONVERT_CONV_TO_BINARY(Bigint,QUAD,18);
ODBCCONVERT_CONV_TO_STRING(Bigint,QUAD,18);

int OdbcConvert::convBigintToTagNumeric(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)(pointer+3) = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	*pointer++=(char)from->precision;
	*pointer++=(char)from->scale;

	if ( number < 0 )
		number = -number,
		*pointer++=0;
	else
		*pointer++=1;

	if ( indicatorTo )
		*indicatorTo = sizeof(tagSQL_NUMERIC_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Numeric,Decimal
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Numeric,QUAD,TinyInt,char);
ODBCCONVERT_CONV(Numeric,QUAD,Short,short);
ODBCCONVERT_CONV(Numeric,QUAD,Long,long);
ODBCCONVERT_CONV(Numeric,QUAD,Float,float);
ODBCCONVERT_CONV(Numeric,QUAD,Double,double);
ODBCCONVERT_CONV(Numeric,QUAD,Bigint,QUAD);

////////////////////////////////////////////////////////////////////////
// TagNumeric
////////////////////////////////////////////////////////////////////////

ODBCCONVERTTAG_NUMERIC_CONV(TinyInt,char);
ODBCCONVERTTAG_NUMERIC_CONV(Short,short);
ODBCCONVERTTAG_NUMERIC_CONV(Long,long);
ODBCCONVERTTAG_NUMERIC_CONV(Float,float);
ODBCCONVERTTAG_NUMERIC_CONV(Double,double);
ODBCCONVERTTAG_NUMERIC_CONV(Bigint,QUAD);

//ODBCCONVERT_CONV(Numeric,QUAD,Numeric,QUAD);
int OdbcConvert::convNumericToTagNumeric(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)(pointer+3) = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	*pointer++=(char)from->precision;
	*pointer++=(char)from->scale;

	if ( number < 0 )
		number = -number,
		*pointer++=0;
	else
		*pointer++=1;

	if ( indicatorTo )
		*indicatorTo = sizeof(tagSQL_NUMERIC_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
#define ODBCCONVERT_TEMP_CONV(TYPE_FROM,TYPE_TO,C_TYPE_TO)										\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL;																		\
																								\
	if ( indicatorTo )																			\
		*indicatorTo = 0;																		\
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
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	SQLUSMALLINT mday, month;
	SQLSMALLINT year;

	decode_sql_date(*(long*)getAdressBindDataFrom((char*)from->dataPtr), mday, month, year);
	int len, outlen = to->length;

	len = snprintf(pointer, outlen, "%04d-%02d-%02d",year,month,mday);

	if ( indicatorTo )
	{
		if ( len == -1 )
			*indicatorTo = outlen;
		else
			*indicatorTo = len;
	}

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToTagDate(DescRecord * from, DescRecord * to)
{
	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_date(*(long*)getAdressBindDataFrom((char*)from->dataPtr), tagDt->day, tagDt->month, tagDt->year);

	if ( indicatorTo )
		*indicatorTo = sizeof(tagDATE_STRUCT);

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateToDate(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	*(long*)pointer = encode_sql_date ( tagDt->day, tagDt->month, tagDt->year );
	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToTagTimestamp(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_date(*(long*)getAdressBindDataFrom((char*)from->dataPtr), tagTs->day, tagTs->month, tagTs->year);
	tagTs->hour = tagTs->minute = tagTs->second = 0;
	tagTs->fraction = 0;

	if ( indicatorTo )
		*indicatorTo = sizeof(tagTIMESTAMP_STRUCT);

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
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

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

	if ( indicatorTo )
	{
		if ( len == -1 )
			*indicatorTo = outlen;
		else
			*indicatorTo = len;
	}

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToTagTime(DescRecord * from, DescRecord * to)
{
	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	decode_sql_time(*(long*)getAdressBindDataFrom((char*)from->dataPtr), tagTm->hour, tagTm->minute, tagTm->second);

	if ( indicatorTo )
		*indicatorTo = sizeof(tagTIME_STRUCT);

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagTimeToTime(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	*(long*)pointer = encode_sql_time ( tagTm->hour, tagTm->minute, tagTm->second );
	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToTagTimestamp(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	long ntime = *(long*)getAdressBindDataFrom((char*)from->dataPtr);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->day = tagTs->month = tagTs->year = 0;
	tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;

	if ( indicatorTo )
		*indicatorTo = sizeof(tagTIMESTAMP_STRUCT);

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
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

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

	if ( indicatorTo )
	{
		if ( len == -1 )
			*indicatorTo = outlen;
		else
			*indicatorTo = len;
	}

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagDate(DescRecord * from, DescRecord * to)
{
	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	long nday = LO_LONG ( *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr) );

	decode_sql_date(nday, tagDt->day, tagDt->month, tagDt->year);

	if ( indicatorTo )
		*indicatorTo = sizeof(tagDATE_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagTime(DescRecord * from, DescRecord * to)
{
	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	long ntime = HI_LONG ( *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr) );

	decode_sql_time(ntime, tagTm->hour, tagTm->minute, tagTm->second);

	if ( indicatorTo )
		*indicatorTo = sizeof(tagTIME_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagDateTime(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	QUAD &number = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);

	long nday = LO_LONG(number);
	long ntime = HI_LONG(number);

	decode_sql_date(nday, tagTs->day, tagTs->month, tagTs->year);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;

	if ( indicatorTo )
		*indicatorTo = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateTimeToDate(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	*(long*)pointer = encode_sql_date ( tagTs->day, tagTs->month, tagTs->year );
	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateTimeToTime(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	long &ntime = *(long*)getAdressBindDataTo((char*)to->dataPtr);

	ntime = encode_sql_time ( tagTs->hour, tagTs->minute, tagTs->second );
	ntime += tagTs->fraction / ( STD_TIME_SECONDS_PRECISION / ISC_TIME_SECONDS_PRECISION );

	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateTimeToDateTime(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	long nday = encode_sql_date ( tagTs->day, tagTs->month, tagTs->year );
	long ntime = encode_sql_time ( tagTs->hour, tagTs->minute, tagTs->second );

	ntime += tagTs->fraction / ( STD_TIME_SECONDS_PRECISION / ISC_TIME_SECONDS_PRECISION );

	*(QUAD*)pointer = MAKEQUAD( nday, ntime );
	*(short*)indicatorTo = 0;

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

int OdbcConvert::convBlobToBlob(DescRecord * from, DescRecord * to)
{
	RETCODE ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

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
				blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );
			else
				blob->bind ( parentStmt->connection->connection, ptBlob );
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
				{
					if ( blob->isArray() )
						blob->getBinary (from->dataOffset, len, pointer);
					else
						blob->getBytes (from->dataOffset, len, pointer);

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

	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

int OdbcConvert::convBlobToBinary(DescRecord * from, DescRecord * to)
{
	RETCODE ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	Blob *& blob = from->dataBlobPtr;
	int dataRemaining = 0;

	if ( blob )
	{
		if ( from->currentFetched != parentStmt->getCurrentFetched() )
		{ // attach new blob
			from->dataOffset = 0;
			if ( parentStmt->isStaticCursor() )
				blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );
			else
				blob->bind ( parentStmt->connection->connection, ptBlob );

			from->currentFetched = parentStmt->getCurrentFetched();
		}

		dataRemaining = blob->length() - from->dataOffset;

		if ( !to->length )
			;
		else if (!dataRemaining && from->dataOffset)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else if ( pointer )
		{
			int len = MIN(dataRemaining, MAX(0, (long)to->length-1)>>1);
		 
			if ( len > 0 ) 
				blob->getBinary (from->dataOffset, len, pointer);

			from->dataOffset += len;

			if ( len && len < dataRemaining )
			{
				OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
				ret = SQL_SUCCESS_WITH_INFO;
			}
		}
	}

	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBlobToString(DescRecord * from, DescRecord * to)
{
	RETCODE ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

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
				blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );
			else
				blob->bind ( parentStmt->connection->connection, ptBlob );

			from->currentFetched = parentStmt->getCurrentFetched();
		}

		if ( blob->isArray() )
			length = ((BinaryBlob*)blob)->getLength();
		else
		{
			length = blob->length();
			
			if ( blob->isBlob() )
				length *= 2;
		}

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
					if ( blob->isBlob() )
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

	if ( indicatorTo )
		*indicatorTo = length;

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
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
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
	return SQL_SUCCESS;																			\
}																								\

ODBCCONVERT_CONV_STRING_TO(String,TinyInt,char);
ODBCCONVERT_CONV_STRING_TO(String,Short,short);
ODBCCONVERT_CONV_STRING_TO(String,Long,long);
ODBCCONVERT_CONV_STRING_TO(String,Float,float);
ODBCCONVERT_CONV_STRING_TO(String,Double,double);
ODBCCONVERT_CONV_STRING_TO(String,Bigint,QUAD);

int OdbcConvert::convStringToString(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	RETCODE ret = SQL_SUCCESS;
	int len;

	ODBCCONVERT_CHECKNULL;

	len = MIN ( (int)from->length, (int)MAX(0, (int)to->length-1));

	if( len > 0 )
		memcpy ( pointerTo, pointerFrom, len );

	pointerTo[ len ] = '\0';

	if ( indicatorTo )
		*indicatorTo = from->length;

	if (len && len < (int)from->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStringToVarString(DescRecord * from, DescRecord * to)
{	
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	RETCODE ret = SQL_SUCCESS;
	unsigned short &lenVar = *(unsigned short*)pointerTo;
	int len;

	if ( indicatorFrom && *indicatorFrom == SQL_NTS )
	{
		len = strlen ( pointerFrom );
		len = MIN( len, (int)MAX(0, (int)to->length-1));
	}
	else
		len = MIN( (int)from->length == -1 ? (int)strlen(pointerFrom) : (int)from->length, (int)MAX(0, (int)to->length-1));

	lenVar = len;

	if( lenVar > 0 )
		memcpy ( pointerTo+2, pointerFrom, lenVar);

	if (lenVar && (long)lenVar > (long)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( indicatorTo )
		*(short*)indicatorTo = lenVar;

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStringToBlob(DescRecord * from, DescRecord * to)
{	
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	RETCODE ret = SQL_SUCCESS;
	int len;

	if ( indicatorFrom && *indicatorFrom == SQL_NTS )
	{
		len = strlen ( pointerFrom );
		len = MIN( len, (int)MAX(0, (int)to->length-1));
	}
	else
		len = MIN( (int)from->length == -1 ? (int)strlen(pointerFrom) : (int)from->length, (int)MAX(0, (int)to->length-1));

	if( len >= 0 )
		to->dataBlobPtr->writeStringHexToBlob(pointerTo, pointerFrom, len);

	if ( indicatorTo )
		*(short*)indicatorTo = 0;

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStreamHexStringToBlob(DescRecord * from, DescRecord * to)
{	
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	Blob * blob = from->dataBlobPtr;
	if ( blob->isBlob() )
		from->dataBlobPtr->writeStreamHexToBlob( pointerTo );
	else
		from->dataBlobPtr->writeBlob( pointerTo );

	if ( indicatorTo )
		*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::convStreamToBlob(DescRecord * from, DescRecord * to)
{	
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	from->dataBlobPtr->writeBlob( pointerTo );

	if ( indicatorTo )
		*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferStringToTinyInt(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	to->headSqlVarPtr->setSqlLen( 1 );

	char * src = pointerFrom;
	char * org = src;

	if ( !from->data_at_exec )
		to->headSqlVarPtr->setSqlData( pointerFrom );
	else
	{
		if ( !to->isLocalDataPtr )
			to->allocateLocalDataPtr();

		to->headSqlVarPtr->setSqlData( to->localDataPtr );
		org = to->localDataPtr;
	}

	bool minus = false;
	int val = 0;

	if ( *src == '-' )
	{
		src++;
		minus = true;
	}
	
	while ( *src )
	{
		val *= 10;
		val += *src++ - '0';
	}

	if ( minus )
		*org = (char)-val;
	else
		*org = (char)val;

	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferStringToDateTime(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	int len = 0;
	convertStringDataToServerStringData ( pointerFrom, len );

	if ( !from->data_at_exec )
	{
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( pointerFrom );
	}
	else
	{
		if ( !to->isLocalDataPtr )
			to->allocateLocalDataPtr();

		len = MIN( len, to->octetLength );
		memcpy(to->localDataPtr, pointerFrom, len);
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( to->localDataPtr );
	}

	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferStringToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	if ( !from->data_at_exec )
	{
		short len;
		if ( octetLengthPtr && *octetLengthPtr != SQL_NTSL )
			len = (short)*octetLengthPtr;
		else
			len = strlen( pointerFrom );

		to->headSqlVarPtr->setSqlLen( len );
		to->headSqlVarPtr->setSqlData( pointerFrom );
	}
	else
	{
		if ( !to->isLocalDataPtr )
			to->allocateLocalDataPtr();

		int len = strlen( pointerFrom );
		len = MIN( len, to->octetLength );
		memcpy(to->localDataPtr, pointerFrom, len);
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( to->localDataPtr );
	}

	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferBinaryStringToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	short len;
	if ( octetLengthPtr && *octetLengthPtr != SQL_NTSL )
	{
		len = (short)*octetLengthPtr >> 1;
		len = MIN ( len, (short)to->octetLength );

		if ( len )
		{
			if ( !to->isLocalDataPtr )
				to->allocateLocalDataPtr();

			to->headSqlVarPtr->setSqlLen( len );

			char * dst = to->localDataPtr;
			char * src = pointerFrom;
			while ( len-- )
			{
				*dst++ = *src;
				src += 2;
			}

			to->headSqlVarPtr->setSqlData( to->localDataPtr );
		}
	}

	*(short*)indicatorTo = 0;

	return SQL_SUCCESS;
}

#undef OFFSET_STRING
#undef CALC_LEN_STRING
#define OFFSET_STRING  sizeof(short)
#define CALC_LEN_STRING  *(short*)p

ODBCCONVERT_CONV_STRING_TO(VarString,TinyInt,char);
ODBCCONVERT_CONV_STRING_TO(VarString,Short,short);
ODBCCONVERT_CONV_STRING_TO(VarString,Long,long);
ODBCCONVERT_CONV_STRING_TO(VarString,Float,float);
ODBCCONVERT_CONV_STRING_TO(VarString,Double,double);
ODBCCONVERT_CONV_STRING_TO(VarString,Bigint,QUAD);

int OdbcConvert::convVarStringToString(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	RETCODE ret = SQL_SUCCESS;
	unsigned short lenVar = *(unsigned short*)pointerFrom;
	int len;

	char * src = pointerFrom + 2,
		 * end = src + lenVar;
	while ( lenVar-- && *(--end) == ' ');
	len = end - src + 1;
	len = MIN(len, MAX(0,(int)to->length-1));

	if( len > 0 )
		memcpy (pointerTo, src, len);

	pointerTo[len] = 0;

	if (len && (long)len > (long)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( indicatorTo )
		*indicatorTo = len;

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

void OdbcConvert::convertStringDataToServerStringData(char * string, int &len)
{
	char * pt, * ptTmp = string;

	if ( !ptTmp || !*ptTmp )
		return;

	while( *ptTmp == ' ' ) ptTmp++;

	if( *ptTmp != '{' )
		return;

	pt = string;

	while( *ptTmp && *ptTmp!='\'' ) ptTmp++;

	if( *ptTmp!='\'' )
		return;

	ptTmp++; // ch \'

	while( *ptTmp && *ptTmp!='\'' )
		*pt++ = *ptTmp++;

	len = pt - string;
	// validate end string check Server
}

}; // end namespace OdbcJdbcLibrary
