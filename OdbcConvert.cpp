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
#include <math.h>
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

unsigned __int64 listScale[19];

static int init();
static int foo = init();

int init()
{
	listScale[0] = 1;

	for (int i = 1; i < sizeof(listScale)/sizeof(listScale[0]); i++)
		listScale[i] = listScale[i-1] * 10;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcConvert::OdbcConvert(OdbcStatement * parent)
{
	parentStmt = parent;
	bIdentity = false;
	statusReturnData = false;
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
	case SQL_C_BIT:
		if ( to->isIndicatorSqlDa )
			return &OdbcConvert::transferStringToAllowedType;
		break;

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
			return &OdbcConvert::notYetImplemented;
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
			if ( from->scale || to->scale )
				return &OdbcConvert::convShortToFloatWithScale;
			return &OdbcConvert::convShortToFloat;
		case SQL_C_DOUBLE:
			if ( from->scale || to->scale )
				return &OdbcConvert::convShortToDoubleWithScale;
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
			return &OdbcConvert::notYetImplemented;
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
			if ( from->scale || to->scale )
				return &OdbcConvert::convLongToFloatWithScale;
			return &OdbcConvert::convLongToFloat;
		case SQL_C_DOUBLE:
			if ( from->scale || to->scale )
				return &OdbcConvert::convLongToDoubleWithScale;
			return &OdbcConvert::convLongToDouble;
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return &OdbcConvert::convLongToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convLongToString;
		case SQL_DECIMAL:
		case SQL_C_NUMERIC:
			return &OdbcConvert::convLongToTagNumeric;
		default:
			return &OdbcConvert::notYetImplemented;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
			if ( from->scale || to->scale )
				return &OdbcConvert::convBigintToFloatWithScale;
			return &OdbcConvert::convBigintToFloat;
		case SQL_C_DOUBLE:
			if ( from->scale || to->scale )
				return &OdbcConvert::convBigintToDoubleWithScale;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
		default:
			return &OdbcConvert::notYetImplemented;
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
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::convBinaryToBlob;
			return &OdbcConvert::convBlobToBlob;
		case SQL_C_CHAR:
			if ( to->isIndicatorSqlDa && to->isBlobOrArray )
				return &OdbcConvert::convBinaryToBlob;
			return &OdbcConvert::convBlobToString;
		default:
			return &OdbcConvert::notYetImplemented;
		}
		break;

	case SQL_C_CHAR:
		if ( from->type == JDBC_VARCHAR )
		{
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
				if ( parentStmt->isResultSetFromSystemCatalog )
					return &OdbcConvert::convVarStringSystemToString;
				else
					return &OdbcConvert::convVarStringToString;
			case SQL_C_BINARY:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr && from->dataBlobPtr->isArray() )
					return &OdbcConvert::convBlobToBlob;
				return &OdbcConvert::convVarStringToString;
			default:
				return &OdbcConvert::notYetImplemented;
			}
		}
		else 
		{
			if ( to->isIndicatorSqlDa && to->headSqlVarPtr->isReplaceForParamArray() )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferArrayStringToAllowedType;
			}

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
			default:
				return &OdbcConvert::notYetImplemented;
			}
		}
		break;
	default:
		return &OdbcConvert::notYetImplemented;
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

#define ODBCCONVERT_CHECKNULL								\
	if( *(short*)indicatorFrom == SQL_NULL_DATA )			\
	{														\
		if ( indicatorTo )									\
			*indicatorTo = SQL_NULL_DATA;					\
		return SQL_SUCCESS;									\
	}														\

#define ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO)				\
	if ( from->isIndicatorSqlDa )							\
	{														\
		if( *(short*)indicatorFrom == SQL_NULL_DATA )		\
		{													\
			if ( indicatorTo )								\
				*indicatorTo = SQL_NULL_DATA;				\
			return SQL_SUCCESS;								\
		}													\
		else if ( indicatorTo )								\
			*indicatorTo = sizeof(C_TYPE_TO);				\
	}														\
	else /* if ( to->isIndicatorSqlDa ) */					\
	{														\
		if(indicatorFrom && *indicatorFrom==SQL_NULL_DATA)	\
		{													\
			*(short*)indicatorTo = SQL_NULL_DATA;			\
			return SQL_SUCCESS;								\
		}													\
		else												\
			*indicatorTo = 0;								\
	}														\

#define ODBCCONVERT_CHECKNULL_SQLDA							\
	if( indicatorFrom && *indicatorFrom == SQL_NULL_DATA )	\
	{														\
		*(short*)indicatorTo = SQL_NULL_DATA;				\
		return SQL_SUCCESS;									\
	}														\
	else													\
		*indicatorTo = 0;									\

#define GET_LEN_FROM_OCTETLENGTHPTR			\
	if ( octetLengthPtr )					\
	{										\
		if ( *octetLengthPtr == SQL_NTS )	\
			len = strlen ( pointerFrom );	\
		else								\
			len = *octetLengthPtr;			\
	}										\
	else									\
		len = strlen(pointerFrom);			\

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

#define ODBCCONVERTBIGINT_CONV( TYPE_TO, C_TYPE_TO )											\
int OdbcConvert::convBigintTo##TYPE_TO(DescRecord * from, DescRecord * to)						\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	QUAD val = *(QUAD*)getAdressBindDataFrom( (char*)from->dataPtr );							\
																								\
	if ( to->scale != from->scale )																\
	{																							\
		if ( to->scale > from->scale )															\
			val *= listScale[to->scale-from->scale];											\
		else /* if ( to->scale < from->scale )	*/												\
		{																						\
			if ( to->scale )																	\
			{																					\
				QUAD round = 5 * listScale[from->scale - to->scale - 1];						\
																								\
				if ( val < 0 )																	\
					val -= round;																\
				else if ( val > 0 )																\
					val += round;																\
			}																					\
			val /= listScale[from->scale - to->scale];											\
		}																						\
	}																							\
																								\
	*(C_TYPE_TO*)pointer = (C_TYPE_TO)val;														\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_WITH_SCALE_CONV(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)					\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO##WithScale(DescRecord * from, DescRecord * to)	\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLINTEGER *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	double val = (double)*(C_TYPE_FROM*)getAdressBindDataFrom( (char*)from->dataPtr);			\
																								\
	if ( to->scale )																			\
		val *= (QUAD)listScale[to->scale];														\
																								\
	if ( from->scale )																			\
		val /= (QUAD)listScale[from->scale];													\
																								\
	*(C_TYPE_TO*)pointer =	(C_TYPE_TO)val;														\
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
	QUAD * number = (QUAD*)( pointer + 3 );														\
	*number = (QUAD)*(C_TYPE_FROM*)getAdressBindDataFrom( (char*)from->dataPtr );				\
	*pointer++ = (char)from->precision;															\
	*pointer++ = (char)from->scale;																\
																								\
	if ( *number < 0 )																			\
		*number = -*number,																		\
		*pointer++ = 0;																			\
	else																						\
		*pointer++ = 1;																			\
																								\
	*++number = 0;																				\
																								\
	if ( indicatorTo )																			\
		*indicatorTo = sizeof ( tagSQL_NUMERIC_STRUCT );										\
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
	C_TYPE_FROM &valFrom = *(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);			\
	if ( to->scale )																			\
		valFrom *= (C_TYPE_FROM)(QUAD)listScale[to->scale];										\
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

int OdbcConvert::notYetImplemented(DescRecord * from, DescRecord * to)
{ 
	parentStmt->postError ("07006", "Restricted data type attribute violation");
	return SQL_ERROR; 
}

////////////////////////////////////////////////////////////////////////
// TinyInt
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(TinyInt,char,TinyInt,char);
ODBCCONVERT_CONV(TinyInt,unsigned char,Short,short);
ODBCCONVERT_CONV(TinyInt,unsigned char,Long,long);
ODBCCONVERT_CONV(TinyInt,unsigned char,Float,float);
ODBCCONVERT_CONV(TinyInt,unsigned char,Double,double);
ODBCCONVERT_CONV(TinyInt,unsigned char,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(TinyInt,char,3);
ODBCCONVERT_CONVTAGNUMERIC(TinyInt,unsigned char);

////////////////////////////////////////////////////////////////////////
// Short
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Short,short,TinyInt,char);
ODBCCONVERT_CONV(Short,short,Short,short);
ODBCCONVERT_CONV(Short,short,Long,long);
ODBCCONVERT_CONV(Short,short,Float,float);
ODBCCONVERT_CONV(Short,short,Double,double);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Float,float);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Double,double);
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
ODBCCONVERT_WITH_SCALE_CONV(Long,long,Float,float);
ODBCCONVERT_WITH_SCALE_CONV(Long,long,Double,double);
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
ODBCCONVERT_WITH_SCALE_CONV(Bigint,QUAD,Float,float);
ODBCCONVERT_WITH_SCALE_CONV(Bigint,QUAD,Double,double);
ODBCCONVERTBIGINT_CONV(Bigint,QUAD);
ODBCCONVERT_CONV_TO_BINARY(Bigint,QUAD,18);
ODBCCONVERT_CONV_TO_STRING(Bigint,QUAD,18);
ODBCCONVERT_CONVTAGNUMERIC(Bigint,QUAD);

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
ODBCCONVERT_CONVTAGNUMERIC(Numeric,QUAD);

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
	ntime += tagTs->fraction / STD_TIME_SECONDS_PRECISION;

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

	ntime += tagTs->fraction / STD_TIME_SECONDS_PRECISION;

	*(QUAD*)pointer = MAKEQUAD( nday, ntime );

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
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	Blob *& blob = from->dataBlobPtr;
	int dataRemaining = 0;

	if ( blob )
	{
		bool directOpen = false;
		bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

		if ( !fetched || !from->dataOffset )
		{ // attach new blob
			from->dataOffset = 0;
			if ( !(fetched && !blob->getOffset()) )
			{
				if ( parentStmt->isStaticCursor() )
					blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );
				else if ( blob->isArray() || !statusReturnData )
					blob->bind ( *parentStmt, ptBlob );
				else
				{
					blob->directOpenBlob ( ptBlob );
					directOpen = true;
				}
			}
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
		else
		{
			int len = MIN(dataRemaining, MAX(0, (long)to->length));
			int lenRead;
			 
			if ( pointer )
			{
				if ( len > 0 ) 
				{
					if ( blob->isArray() )
						blob->getBinary (from->dataOffset, len, pointer);
					else if ( directOpen )
						blob->directFetchBlob((char*)pointer, len, lenRead);
					else 
						blob->getBytes (from->dataOffset, len, pointer);
				}

				if ( !statusReturnData )
					from->dataOffset += len;

				if ( len && len < dataRemaining )
				{
					OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
					ret = SQL_SUCCESS_WITH_INFO;
				}
			}
		}

		if ( directOpen )
			blob->directCloseBlob();
	}

	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBlobToBinary(DescRecord * from, DescRecord * to)
{
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	Blob *& blob = from->dataBlobPtr;
	int dataRemaining = 0;

	if ( blob )
	{
		bool directOpen = false;
		bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

		if ( !fetched || !from->dataOffset )
		{ // attach new blob
			from->dataOffset = 0;
			if ( !(fetched && !blob->getOffset()) )
			{
				if ( parentStmt->isStaticCursor() )
					blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );
				else if ( blob->isArray() || !statusReturnData )
					blob->bind ( *parentStmt, ptBlob );
				else
				{
					blob->directOpenBlob ( ptBlob );
					directOpen = true;
				}
			}
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
			int lenRead;
		 
			if ( len > 0 ) 
			{
				if ( directOpen )
					blob->directFetchBlob((char*)pointer, len, lenRead);
				else
					blob->getBinary (from->dataOffset, len, pointer);
			}

			if ( !statusReturnData )
				from->dataOffset += len;

			if ( len && len < dataRemaining )
			{
				OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
				ret = SQL_SUCCESS_WITH_INFO;
			}
		}

		if ( directOpen )
			blob->directCloseBlob();
	}

	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBlobToString(DescRecord * from, DescRecord * to)
{
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;

	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	Blob *& blob = from->dataBlobPtr;
	int dataRemaining = 0;

	if ( blob )
	{
		bool directOpen = false;
		bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

		if ( !fetched || !from->dataOffset )
		{ // attach new blob
			from->dataOffset = 0;
			if ( !(fetched && !blob->getOffset()) )
			{
				if ( parentStmt->isStaticCursor() )
					blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );
				else if ( blob->isArray() || !statusReturnData )
					blob->bind ( *parentStmt, ptBlob );
				else
				{
					blob->directOpenBlob ( ptBlob );
					directOpen = true;
				}
			}
			from->currentFetched = parentStmt->getCurrentFetched();
		}

		int length;

		if ( blob->isArray() )
			length = ((BinaryBlob*)blob)->getLength();
		else
		{
			length = blob->length();
			
			if ( blob->isBlob() )
				length *= 2;
		}

		dataRemaining = length - from->dataOffset;

		if ( !to->length )
			;
		else if (!dataRemaining && from->dataOffset)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else
		{
			int len = MIN(dataRemaining, MAX(0, (long)to->length-1));
			int lenRead;
			 
			if ( pointer )
			{
				if ( len > 0 ) 
				{
					if ( !directOpen )
					{
						if ( blob->isBlob() )
							blob->getHexString (from->dataOffset/2, len/2, pointer);
						else
							blob->getBytes (from->dataOffset, len, pointer);
					}
					else
					{
						if ( blob->isBlob() )
							blob->directGetSegmentToHexStr((char*)pointer, len/2, lenRead);
						else
							blob->directFetchBlob((char*)pointer, len, lenRead);
					}

					((char*) (pointer)) [len] = 0;
				}

				if ( !statusReturnData )
					from->dataOffset += len;

				if ( len && len < dataRemaining )
				{
					OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
					ret = SQL_SUCCESS_WITH_INFO;
				}
			}
		}

		if ( directOpen )
			blob->directCloseBlob();
	}

	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBinaryToBlob(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);

	SQLRETURN ret = SQL_SUCCESS;
	int len;

	GET_LEN_FROM_OCTETLENGTHPTR;

	len = MIN( len, (int)MAX(0, (int)to->length));

	if( len > 0 )
	{
		Blob *& blob = to->dataBlobPtr;
		
		if ( blob->isArray() )
		{
			blob->clear();
			blob->putLongSegment (len, pointerFrom);
			blob->writeBlob( pointerTo );
		}
		else
		{
			blob->directCreateBlob( pointerTo );
			blob->directWriteBlob( pointerFrom, len );
			blob->directCloseBlob();
		}
	}
	else		
		*(short*)indicatorTo = SQL_NULL_DATA;

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
			divisor /= (QUAD)listScale[-scale];													\
		else /* if (scale > 0)	*/																\
			divisor *= (QUAD)listScale[scale];													\
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

	ODBCCONVERT_CHECKNULL;

	if ( from->currentFetched != parentStmt->getCurrentFetched() )
	{ // new row read
		from->dataOffset = 0;
		from->currentFetched = parentStmt->getCurrentFetched();
	}

	SQLRETURN ret = SQL_SUCCESS;
	int length = from->length;
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
		 
		if ( !pointerTo )
			length = dataRemaining;
		else
		{
			if( len > 0 )
				memcpy ( pointerTo, pointerFrom + from->dataOffset, len );

			pointerTo[ len ] = '\0';

			if ( !statusReturnData )
				from->dataOffset += len;

			if ( len && len < dataRemaining )
			{
				OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
				ret = SQL_SUCCESS_WITH_INFO;
			}
				
			length = dataRemaining;
		}
	}

	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStringToVarString(DescRecord * from, DescRecord * to)
{	
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);

	SQLRETURN ret = SQL_SUCCESS;
	unsigned short &lenVar = *(unsigned short*)pointerTo;
	int len;

	GET_LEN_FROM_OCTETLENGTHPTR;

	lenVar = MIN( len, (int)MAX(0, (int)to->length));

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
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);

	SQLRETURN ret = SQL_SUCCESS;
	int len;

	GET_LEN_FROM_OCTETLENGTHPTR;

	len = MIN( len, (int)MAX(0, (int)to->length));

	if( len > 0 )
		to->dataBlobPtr->writeStringHexToBlob(pointerTo, pointerFrom, len);
	else		
		*(short*)indicatorTo = SQL_NULL_DATA;

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
		blob->writeStreamHexToBlob( pointerTo );
	else
		blob->writeBlob( pointerTo );

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

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferStringToDateTime(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);

	int len = 0;
	convertStringDateTimeToServerStringDateTime ( pointerFrom, len );

	if ( !len )
	{
		GET_LEN_FROM_OCTETLENGTHPTR;
		len = MIN( len, (int)MAX(0, (int)to->length));
	}

	if ( !from->data_at_exec )
	{
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( pointerFrom );
	}
	else
	{
		if ( !to->isLocalDataPtr )
			to->allocateLocalDataPtr();

		memcpy(to->localDataPtr, pointerFrom, len);
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( to->localDataPtr );
	}

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferStringToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);

	int len;
	SQLRETURN ret = SQL_SUCCESS;

	GET_LEN_FROM_OCTETLENGTHPTR;

	if ( !from->data_at_exec )
	{
		len = MIN( len, to->octetLength );
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( pointerFrom );
	}
	else
	{
		if ( !to->isLocalDataPtr )
		{
			to->allocateLocalDataPtr();
			to->headSqlVarPtr->setSqlData( to->localDataPtr );
		}

		if ( len + from->dataOffset > to->octetLength )
		{
			OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
			ret = SQL_SUCCESS_WITH_INFO;
		}

		len = MIN( len, MAX( 0, to->octetLength - from->dataOffset) );
		memcpy(to->localDataPtr + from->dataOffset, pointerFrom, len);
		from->dataOffset += len;
		to->headSqlVarPtr->setSqlLen( (short)from->dataOffset );
	}

	return ret;
}

// for use App to SqlDa
int OdbcConvert::transferArrayStringToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	char * firstChar;
	int len;

	if ( !from->data_at_exec )
	{
		getFirstElementFromArrayString ( pointerFrom, firstChar, len);
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( firstChar );
	}
	else
	{
		if ( !to->isLocalDataPtr )
			to->allocateLocalDataPtr();

		getFirstElementFromArrayString ( pointerFrom, firstChar, len);
		memcpy(to->localDataPtr, firstChar, len);
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( to->localDataPtr );
	}

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferBinaryStringToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLINTEGER * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	int len;

	GET_LEN_FROM_OCTETLENGTHPTR;

	len >>= 1;
	len = MIN( len, (int)MAX(0, (int)to->length));

	if ( len )
	{
		if ( !to->isLocalDataPtr )
			to->allocateLocalDataPtr();

		to->headSqlVarPtr->setSqlLen( (short)len );

		char * dst = to->localDataPtr;
		char * src = pointerFrom;
		while ( len-- )
		{
			*dst++ = *src;
			src += 2;
		}

		to->headSqlVarPtr->setSqlData( to->localDataPtr );
	}

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

	if ( from->currentFetched != parentStmt->getCurrentFetched() )
	{ // new row read
		from->dataOffset = 0;
		from->currentFetched = parentStmt->getCurrentFetched();
	}

	SQLRETURN ret = SQL_SUCCESS;
	int length = *(unsigned short*)pointerFrom;
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
		 
		if ( !pointerTo )
			length = dataRemaining;
		else
		{
			pointerFrom += sizeof( short );
			if( len > 0 )
				memcpy ( pointerTo, pointerFrom + from->dataOffset, len );

			pointerTo[ len ] = '\0';

			if ( !statusReturnData )
				from->dataOffset += len;

			if ( len && len < dataRemaining )
			{
				OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
				ret = SQL_SUCCESS_WITH_INFO;
			}
				
			length = dataRemaining;
		}
	}

	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

int OdbcConvert::convVarStringSystemToString(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLINTEGER * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLINTEGER * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL;
	
	SQLRETURN ret = SQL_SUCCESS;
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

void OdbcConvert::roundStringNumber ( char *& strNumber, int numDigits, int &realDigits )
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

void OdbcConvert::convertFloatToString(double value, char *string, int size, int *length, int digit, char POINT_DIV)
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

void OdbcConvert::convertStringDateTimeToServerStringDateTime (char *& string, int &len)
{
	char * ptBeg = string;

	if ( !ptBeg || !*ptBeg )
		return;

	while( *ptBeg == ' ' ) ptBeg++;

	if( *ptBeg != '{' )
		return;

	while( *++ptBeg == ' ' );

	int offset, offsetPoint;
	
	if ( UPPER(*ptBeg) == 'D' )
	{
		offsetPoint = 0;
		offset = 6; // for bad variant '99-1-1'
	}
	else if ( UPPER(*ptBeg) == 'T' )
	{
		++ptBeg;
		if ( UPPER(*ptBeg) == 'S' )
		{
			offsetPoint = 19;
			offset = 12; // for bad variant '99-1-1 0:0:0'
		}
		else
		{
			offsetPoint = 8;
			offset = 5; // for bad variant '0:0:0'
		}
	}
	else
		return;

	while( *ptBeg && *ptBeg != '\'' ) ptBeg++;

	if( *ptBeg != '\'' )
		return;

	// ASSERT ( ptBeg == '\'' );
	char * ptEnd = ++ptBeg + offset;

	while( *ptEnd && *ptEnd != '\'' ) ptEnd++;

	if( *ptEnd != '\'' )
		return;

	len = ptEnd - ptBeg;

	if ( offsetPoint )
	{
		ptEnd = ptBeg + offsetPoint;
		if ( len > offsetPoint && *ptEnd == '.' )
		{
			int l = 5;
			while ( l-- && *++ptEnd !='\'' );
			len = ptEnd - ptBeg;
		}
	}

	string = ptBeg;
	// validate string check Server
}

void OdbcConvert::getFirstElementFromArrayString(char * string, char *& firstChar, int &len)
{
	bool delimiter = false;
	char * ptTmp = string;

	if ( !ptTmp || !*ptTmp )
		return;

	while( *ptTmp == '{' || *ptTmp == ' ' ) 
		ptTmp++;

	if ( *ptTmp == '\'' )
	{
		delimiter = true;
		ptTmp++;
	}

	firstChar = ptTmp;

	bool cont = true;

	while( cont )
	{
		switch ( *ptTmp )
		{
		case '\0':
		case '}':
			cont = false;
			break;
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			if ( !delimiter )
				cont = false;
			break;
		case ',':
			cont = false;
			break;
		case '\'':
			if ( delimiter )
			{
				if ( *(ptTmp+1) == '\'' )
					ptTmp +=2;
				else
					cont = false;
			}
			break;
		}
		ptTmp++;
	}

	len = ptTmp - firstChar - 1;
}

void OdbcConvert::setHeadSqlVar ( DescRecord * to )
{
	switch ( to->conciseType )
	{
	case SQL_C_CHAR:
		if ( to->isIndicatorSqlDa && to->dataBlobPtr )
		{
			if ( to->dataBlobPtr->isArray() )
				to->headSqlVarPtr->setTypeArray();
			else
				to->headSqlVarPtr->setTypeBlob();
		}
		else
			to->headSqlVarPtr->setTypeText();
		break;

	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
	case SQL_C_UTINYINT:
		to->headSqlVarPtr->setTypeText();
		to->headSqlVarPtr->setSqlLen( 1 );
		break;

	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	case SQL_C_USHORT:
		to->headSqlVarPtr->setTypeShort();
		to->headSqlVarPtr->setSqlScale ( -to->scale );
		break;

	case SQL_C_LONG:
	case SQL_C_SLONG:
	case SQL_C_ULONG:
		to->headSqlVarPtr->setTypeLong();
		to->headSqlVarPtr->setSqlScale ( -to->scale );
		break;

	case SQL_C_FLOAT:
		to->headSqlVarPtr->setTypeFloat();
		break;

	case SQL_C_DOUBLE:
		to->headSqlVarPtr->setTypeDouble();
		to->headSqlVarPtr->setSqlScale ( -to->scale );
		break;

	case SQL_C_SBIGINT:
	case SQL_C_UBIGINT:
		to->headSqlVarPtr->setTypeInt64();
		to->headSqlVarPtr->setSqlScale ( -to->scale );
		break;

	case SQL_C_BIT:
		to->headSqlVarPtr->setTypeText();
		to->headSqlVarPtr->setSqlLen( 1 );
		break;

	case SQL_C_BINARY:
		break;

	case SQL_C_DATE:
	case SQL_TYPE_DATE:
		to->headSqlVarPtr->setTypeDate();
		break;

	case SQL_C_TIME:
	case SQL_TYPE_TIME:
		to->headSqlVarPtr->setTypeTime();
		break;

	case SQL_C_TIMESTAMP:
	case SQL_TYPE_TIMESTAMP:
		to->headSqlVarPtr->setTypeTimestamp();
		break;

	case SQL_C_NUMERIC:
	case SQL_DECIMAL:
		break;
	}
}

}; // end namespace OdbcJdbcLibrary
