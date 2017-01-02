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
#ifndef _WINDOWS
#include <wchar.h>
#endif
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

#include "TemplateConvert.h"

#ifndef _WINDOWS
// for Linux
#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff
#define IS_HIGH_SURROGATE(wch) (((wch) >= HIGH_SURROGATE_START) && ((wch) <= HIGH_SURROGATE_END))
#define IS_LOW_SURROGATE(wch)  (((wch) >= LOW_SURROGATE_START) && ((wch) <= LOW_SURROGATE_END))
#define IS_SURROGATE_PAIR(hs, ls) (IS_HIGH_SURROGATE(hs) && IS_LOW_SURROGATE(ls))
#endif

#ifdef _BIG_ENDIAN // Big endian architectures (IBM PowerPC, Sun Sparc, HP PA-RISC, ... )
#define MAKEQUAD(b, a)      ((QUAD)(((int)(a)) | ((UQUAD)((int)(b))) << 32))
#define HI_LONG(l)			((int)(l))
#define LO_LONG(l)          ((int)(((UQUAD)(l) >> 32) & 0xFFFFFFFF))
#else
#define MAKEQUAD(a, b)      ((QUAD)(((unsigned int)(a)) | ((UQUAD)((int)(b))) << 32))
#define LO_LONG(l)          ((int)(l))
#define HI_LONG(l)          ((int)(((UQUAD)(l) >> 32) & 0xFFFFFFFF))
#endif

size_t wcscch(const wchar_t* s, size_t len)
{
  size_t ret = len;
  while (len--)
  {
    // we count the high surrogates as usual,
    // therefore we ignore the low surrogates
    if (IS_LOW_SURROGATE(*s))
      ret--;
    s++;
  }
  return ret;
}

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

void OdbcConvert::setZeroColumn(DescRecord * to, int rowNumber)
{
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	*(int*)pointer = rowNumber + 1;
	if ( indicatorTo )
		*indicatorTo = sizeof(int);
}

void OdbcConvert::setBindOffsetPtrTo(SQLLEN	*bindOffsetPtr, SQLLEN *bindOffsetPtrInd)
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

void OdbcConvert::setBindOffsetPtrFrom(SQLLEN *bindOffsetPtr, SQLLEN *bindOffsetPtrInd)
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

ADRESS_FUNCTION OdbcConvert::getAdressFunction(DescRecord * from, DescRecord * to)
{
	bIdentity = false;

	if ( to->isIndicatorSqlDa )
		setHeadSqlVar ( to );

	switch(from->conciseType)
	{
	case SQL_C_BIT:
	case SQL_C_TINYINT:
	case SQL_C_UTINYINT:
	case SQL_C_STINYINT:
		switch(to->conciseType)
		{
		case SQL_C_BIT:
			bIdentity = true;
			return &OdbcConvert::convTinyIntToBoolean;
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
		case SQL_C_WCHAR:
			return &OdbcConvert::convTinyIntToStringW;
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
		case SQL_C_BIT:
			return &OdbcConvert::convShortToBoolean;
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			if ( from->scale || to->scale )
				return &OdbcConvert::convShortToTinyIntWithScale;
			return &OdbcConvert::convShortToTinyInt;
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			bIdentity = true;
			if ( from->scale || to->scale )
				return &OdbcConvert::convShortToShortWithScale;
			return &OdbcConvert::convShortToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			if ( from->scale || to->scale )
				return &OdbcConvert::convShortToLongWithScale;
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
		case SQL_C_WCHAR:
			return &OdbcConvert::convShortToStringW;
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
		case SQL_C_BIT:
			return &OdbcConvert::convLongToBoolean;
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			if ( from->scale || to->scale )
				return &OdbcConvert::convLongToTinyIntWithScale;
			return &OdbcConvert::convLongToTinyInt;
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			if ( from->scale || to->scale )
				return &OdbcConvert::convLongToShortWithScale;
			return &OdbcConvert::convLongToShort;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
			bIdentity = true;
			if ( from->scale || to->scale )
				return &OdbcConvert::convLongToLongWithScale;
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
			if ( from->scale || to->scale )
				return &OdbcConvert::convLongToBigintWithScale;
			return &OdbcConvert::convLongToBigint;
		case SQL_C_CHAR:
			return &OdbcConvert::convLongToString;
		case SQL_C_WCHAR:
			return &OdbcConvert::convLongToStringW;
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
		case SQL_C_BIT:
			return &OdbcConvert::convFloatToBoolean;
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
		case SQL_C_WCHAR:
			return &OdbcConvert::convFloatToStringW;
		default:
			return &OdbcConvert::notYetImplemented;
		}
		break;

	case SQL_C_DOUBLE:
		switch(to->conciseType)
		{
		case SQL_C_BIT:
			return &OdbcConvert::convDoubleToBoolean;
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
		case SQL_C_WCHAR:
			return &OdbcConvert::convDoubleToStringW;
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
		case SQL_C_BIT:
			return &OdbcConvert::convBigintToBoolean;
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
		case SQL_C_WCHAR:
			return &OdbcConvert::convBigintToStringW;
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
		case SQL_C_BIT:
			return &OdbcConvert::convNumericToBoolean;
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
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagDateToDateTime;
			return &OdbcConvert::convDateToTagTimestamp;
		case SQL_C_BINARY:
			return &OdbcConvert::convDateToBinary;
		case SQL_C_CHAR:
			return &OdbcConvert::convDateToString;
		case SQL_C_WCHAR:
			return &OdbcConvert::convDateToStringW;
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
			if ( to->isIndicatorSqlDa )
				return &OdbcConvert::transferTagTimeToDateTime;
			return &OdbcConvert::convTimeToTagTimestamp;
		case SQL_C_BINARY:
			return &OdbcConvert::convTimeToBinary;
		case SQL_C_CHAR:
			return &OdbcConvert::convTimeToString;
		case SQL_C_WCHAR:
			return &OdbcConvert::convTimeToStringW;
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
		case SQL_C_BINARY:
			return &OdbcConvert::convDateTimeToBinary;
		case SQL_C_CHAR:
			return &OdbcConvert::convDateTimeToString;
		case SQL_C_WCHAR:
			return &OdbcConvert::convDateTimeToStringW;
		default:
			return &OdbcConvert::notYetImplemented;
		}
		break;

	case SQL_C_BINARY:
		switch(to->conciseType)
		{
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
			if ( to->isIndicatorSqlDa )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferBinaryStringToAllowedType;
			}
			return &OdbcConvert::convBlobToTinyInt;
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
			if ( to->isIndicatorSqlDa )
			{
				to->headSqlVarPtr->setTypeText();
				return &OdbcConvert::transferBinaryStringToAllowedType;
			}
			return &OdbcConvert::convBlobToShort;
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
			else if ( to->isZeroColumn )
				return &OdbcConvert::convLongToLong;
			return &OdbcConvert::convBlobToBlob;
		case SQL_C_CHAR:
			if ( to->isIndicatorSqlDa && to->isBlobOrArray )
				return &OdbcConvert::convBinaryToBlob;
			return &OdbcConvert::convBlobToString;
		case SQL_C_WCHAR:
			return &OdbcConvert::convBlobToStringW;
		default:
			return &OdbcConvert::notYetImplemented;
		}
		break;

	case SQL_C_CHAR:
		if ( from->type == JDBC_VARCHAR || from->type == JDBC_LONGVARCHAR )
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
				if ( from->isIndicatorSqlDa && from->dataBlobPtr )
					return &OdbcConvert::convBlobToString;
				bIdentity = true;
				if ( parentStmt->isResultSetFromSystemCatalog )
					return &OdbcConvert::convVarStringSystemToString;
				else
					return &OdbcConvert::convVarStringToString;
			case SQL_C_WCHAR:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr )
					return &OdbcConvert::convBlobToStringW;
				if ( parentStmt->isResultSetFromSystemCatalog )
					return &OdbcConvert::convVarStringSystemToStringW;
				else
					return &OdbcConvert::convVarStringToStringW;
			case SQL_C_BINARY:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr )
					return &OdbcConvert::convBlobToBlob;
				return &OdbcConvert::convVarStringToBinary;
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
				if ( to->type == SQL_VARCHAR || to->type == SQL_LONGVARCHAR )
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
			case SQL_C_WCHAR:
				if ( to->type == SQL_VARCHAR || to->type == SQL_LONGVARCHAR )
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
				return &OdbcConvert::convStringToStringW;
			case SQL_C_BINARY:
				if ( to->isIndicatorSqlDa )
					return &OdbcConvert::convStringToBlob;
				return &OdbcConvert::convStringToBinary;
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
			default:
				return &OdbcConvert::notYetImplemented;
			}
		}
		break;

	case SQL_C_WCHAR:
		if ( from->type == JDBC_VARCHAR || from->type == JDBC_WVARCHAR || from->type == JDBC_LONGVARCHAR || from->type == JDBC_WLONGVARCHAR )
		{
			switch(to->conciseType) // Varying
			{
			case SQL_C_TINYINT:
			case SQL_C_UTINYINT:
			case SQL_C_STINYINT:
				return &OdbcConvert::convVarStringWToTinyInt;
			case SQL_C_LONG:
			case SQL_C_ULONG:
			case SQL_C_SLONG:
				return &OdbcConvert::convVarStringWToLong;
			case SQL_C_SHORT:
			case SQL_C_USHORT:
			case SQL_C_SSHORT:
				return &OdbcConvert::convVarStringWToShort;
			case SQL_C_FLOAT:
				return &OdbcConvert::convVarStringWToFloat;
			case SQL_C_DOUBLE:
				return &OdbcConvert::convVarStringWToDouble;
			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
				return &OdbcConvert::convVarStringWToBigint;
			case SQL_C_CHAR:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr )
					return &OdbcConvert::convBlobToString;
				if ( parentStmt->isResultSetFromSystemCatalog )
					return &OdbcConvert::convVarStringSystemToString;
				else
					return &OdbcConvert::convVarStringToString;
			case SQL_C_WCHAR:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr )
					return &OdbcConvert::convBlobToStringW;
				bIdentity = true;
				if ( parentStmt->isResultSetFromSystemCatalog )
					return &OdbcConvert::convVarStringSystemToStringW;
				else
					return &OdbcConvert::convVarStringToStringW;
			case SQL_C_BINARY:
				if ( from->isIndicatorSqlDa && from->dataBlobPtr )
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
					return &OdbcConvert::transferStringWToAllowedType;
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
					return &OdbcConvert::transferStringWToAllowedType;
				}
				return &OdbcConvert::convStringToShort;
			case SQL_C_FLOAT:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringWToAllowedType;
				}
				return &OdbcConvert::convStringToFloat;
			case SQL_C_DOUBLE:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringWToAllowedType;
				}
				return &OdbcConvert::convStringToDouble;
			case SQL_C_SBIGINT:
			case SQL_C_UBIGINT:
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringWToAllowedType;
				}
				return &OdbcConvert::convStringToBigint;
			case SQL_C_CHAR:
				if ( to->type == SQL_VARCHAR || to->type == SQL_LONGVARCHAR )
				{
					if ( to->isIndicatorSqlDa )
					{
						if ( to->isBlobOrArray )
							return &OdbcConvert::convStringWToBlob;
						to->headSqlVarPtr->setTypeText();
						return &OdbcConvert::transferStringWToAllowedType;
					}
					return &OdbcConvert::convStringToVarString;
				}
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					return &OdbcConvert::transferStringWToAllowedType;
				}
				return &OdbcConvert::convStringToString;
			case SQL_C_WCHAR:
				if ( to->type == SQL_VARCHAR || to->type == SQL_LONGVARCHAR )
				{
					if ( to->isIndicatorSqlDa )
					{
						if ( to->isBlobOrArray )
							return &OdbcConvert::convStringWToBlob;
						to->headSqlVarPtr->setTypeText();
						return &OdbcConvert::transferStringWToAllowedType;
					}
					return &OdbcConvert::convStringToVarString;
				}
				if ( to->isIndicatorSqlDa )
				{
					to->headSqlVarPtr->setTypeText();
					if ( to->type == JDBC_WVARCHAR || to->type == JDBC_WCHAR )
						return &OdbcConvert::transferStringWToAllowedType;
					return &OdbcConvert::transferStringToAllowedType;
				}
				bIdentity = true;
				return &OdbcConvert::convStringToStringW;
			case SQL_C_BINARY:
				if ( to->isIndicatorSqlDa )
					return &OdbcConvert::convStringWToBlob;
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
					return &OdbcConvert::transferStringWToDateTime;
				}
			default:
				return &OdbcConvert::notYetImplemented;
			}
		}
		break;
	case SQL_C_GUID:
		switch(to->conciseType)
		{
		case SQL_C_CHAR:
			return &OdbcConvert::convGuidToString;
		case SQL_C_WCHAR:
			return &OdbcConvert::convGuidToStringW;
		default:
			return &OdbcConvert::notYetImplemented;
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
SQLLEN * OdbcConvert::getAdressBindIndFrom(char * pointer)
{
	return (SQLLEN *)(pointer + *bindOffsetPtrIndFrom);
}

inline 
SQLPOINTER OdbcConvert::getAdressBindDataTo(char * pointer)
{
	return (SQLPOINTER)(pointer + *bindOffsetPtrTo);
}

inline
SQLLEN * OdbcConvert::getAdressBindIndTo(char * pointer)
{
	return (SQLLEN *)(pointer + *bindOffsetPtrIndTo);
}

#define ODBCCONVERT_CHECKNULL(pointerTo)					\
	if( indicatorFrom && *(short*)indicatorFrom == SQL_NULL_DATA )			\
	{														\
		if ( indicatorTo )									\
			*indicatorTo = SQL_NULL_DATA;					\
		if ( pointerTo )									\
			*(char*)pointerTo = 0;                          \
		return SQL_SUCCESS;									\
	}														\
	if ( !pointerTo )										\
		return SQL_SUCCESS;

#define ODBCCONVERT_CHECKNULLW(pointerTo)					\
	if( indicatorFrom && *(short*)indicatorFrom == SQL_NULL_DATA )			\
	{														\
		if ( indicatorTo )									\
			*indicatorTo = SQL_NULL_DATA;					\
		if ( pointerTo )									\
			*(wchar_t*)pointerTo = 0;                          \
		return SQL_SUCCESS;									\
	}														\
	if ( !pointerTo )										\
		return SQL_SUCCESS;

#define ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO)				\
	if ( from->isIndicatorSqlDa )							\
	{														\
		if( *(short*)indicatorFrom == SQL_NULL_DATA )		\
		{													\
			if ( indicatorTo )								\
				*indicatorTo = SQL_NULL_DATA;				\
			if ( pointer )									\
				*(C_TYPE_TO*)pointer = 0;                   \
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
	if ( !pointer )											\
		return SQL_SUCCESS;

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
			len = (int)strlen ( pointerFrom );	\
		else								\
			len = *octetLengthPtr;			\
	}										\
	else									\
		len = (int)strlen(pointerFrom);		\

#define GET_WLEN_FROM_OCTETLENGTHPTR		\
	if ( octetLengthPtr )					\
	{										\
		if ( *octetLengthPtr == SQL_NTS )	\
			len = (int)wcslen ( pointerFrom );	\
		else								\
			len = *octetLengthPtr / 2;		\
	}										\
	else									\
		len = (int)wcslen( pointerFrom );	\

#define ODBCCONVERT_CONV(TYPE_FROM,C_TYPE_FROM,TYPE_TO,C_TYPE_TO)								\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	*(C_TYPE_TO*)pointer=(C_TYPE_TO)*(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);	\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_BIGINT_CONV( TYPE_TO, C_TYPE_TO )											\
int OdbcConvert::convBigintTo##TYPE_TO(DescRecord * from, DescRecord * to)						\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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
	C_TYPE_TO * pointer = (C_TYPE_TO*)getAdressBindDataTo((char*)to->dataPtr);					\
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
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

#define ODBCCONVERTTAG_NUMERIC_CONV_FLOAT(TYPE_TO,C_TYPE_TO)									\
int OdbcConvert::convTagNumericTo##TYPE_TO(DescRecord * from, DescRecord * to)					\
{																								\
	C_TYPE_TO * pointer = (C_TYPE_TO*)getAdressBindDataTo((char*)to->dataPtr);					\
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	QUAD val;																					\
	tagSQL_NUMERIC_STRUCT * nm =																\
					(tagSQL_NUMERIC_STRUCT *)getAdressBindDataFrom((char*)from->dataPtr);		\
																								\
	val = *(QUAD*)nm->val;																		\
																								\
	if ( !nm->sign )																			\
		val = -val;																				\
																								\
	*pointer = (C_TYPE_TO)val / (QUAD)listScale[nm->scale];										\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONVTAGNUMERIC(TYPE_FROM,C_TYPE_FROM)										\
int OdbcConvert::conv##TYPE_FROM##ToTagNumeric(DescRecord * from, DescRecord * to)				\
{																								\
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL( pointer );															\
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
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL_COMMON(C_TYPE_TO);													\
																								\
	C_TYPE_FROM valFrom = *(C_TYPE_FROM*)getAdressBindDataFrom((char*)from->dataPtr);			\
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
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL( pointer );															\
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
	if ( to->isIndicatorSqlDa ) {																\
		to->headSqlVarPtr->setSqlLen(len);														\
	} else																						\
	if ( indicatorTo )																			\
		*indicatorTo = len;																		\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONV_TO_STRINGW(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)							\
int OdbcConvert::conv##TYPE_FROM##ToStringW(DescRecord * from, DescRecord * to)					\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULLW( pointer );															\
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
		{																						\
			char tempBuf [256];																	\
			strcpy( tempBuf, string );															\
			from->MbsToWcs( (wchar_t *)string, tempBuf, len );									\
			((wchar_t *)string)[len] = L'\0';													\
			len *=2;																			\
		}																						\
	}																							\
																								\
	if ( to->isIndicatorSqlDa ) {																\
		to->headSqlVarPtr->setSqlLen(len);														\
	} else																						\
	if ( indicatorTo )																			\
		*indicatorTo = len;																		\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_CONV_TO_BINARY(TYPE_FROM,C_TYPE_FROM,DEF_SCALE)								\
int OdbcConvert::conv##TYPE_FROM##ToBinary(DescRecord * from, DescRecord * to)					\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL( pointer );															\
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
	if ( to->isIndicatorSqlDa ) {																\
		to->headSqlVarPtr->setSqlLen(len << 1);													\
	} else																						\
	if ( indicatorTo )																			\
		*indicatorTo = len << 1;																\
																								\
	return SQL_SUCCESS;																			\
}																								\

#define ODBCCONVERT_BLOB_CONV(TYPE_TO,C_TYPE_TO)												\
int OdbcConvert::convBlob##To##TYPE_TO(DescRecord * from, DescRecord * to)						\
{																								\
	SQLRETURN ret = SQL_SUCCESS;																\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL( pointer );															\
																								\
	char * ptBlob = (char*)getAdressBindDataFrom((char*)from->dataPtr);							\
	Blob *& blob = from->dataBlobPtr;															\
	int dataRemaining = 0;																		\
																								\
	if ( blob )																					\
	{																							\
		bool directOpen = false;																\
		bool fetched = from->currentFetched == parentStmt->getCurrentFetched();					\
																								\
		if ( !fetched || !from->dataOffset )													\
		{																						\
			from->dataOffset = 0;																\
			from->startedReturnSQLData = false;													\
			if ( !fetched || blob->getOffset() )												\
			{																					\
				if ( parentStmt->isStaticCursor() )												\
					blob->attach ( ptBlob, parentStmt->isStaticCursor(), false );				\
				else if ( blob->isArray() || !statusReturnData )								\
					blob->bind ( *parentStmt, ptBlob );											\
				else																			\
				{																				\
					blob->directOpenBlob ( ptBlob );											\
					directOpen = true;															\
				}																				\
			}																					\
			from->currentFetched = parentStmt->getCurrentFetched();								\
		}																						\
																								\
		dataRemaining = blob->length() - from->dataOffset;										\
																								\
		if ( !to->length )																		\
			;																					\
		else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)	\
		{																						\
			from->dataOffset = 0;																\
			ret = SQL_NO_DATA;																	\
		}																						\
		else																					\
		{																						\
			from->startedReturnSQLData = true;													\
			int len = MIN(dataRemaining, MAX(0, (int)sizeof(C_TYPE_TO)));						\
			int lenRead;																		\
																								\
			if ( pointer )																		\
			{																					\
				if ( len > 0 )																	\
				{																				\
					if ( blob->isArray() )														\
						blob->getBinary (from->dataOffset, len, pointer);						\
					else if ( directOpen )														\
						blob->directFetchBlob((char*)pointer, len, lenRead);					\
					else																		\
						blob->getBytes (from->dataOffset, len, pointer);						\
				}																				\
																								\
				if ( len && len < dataRemaining )												\
				{																				\
					OdbcError *error = parentStmt->postError( new OdbcError( 0, "01004", "Data truncated" ) ); \
					ret = SQL_SUCCESS_WITH_INFO;												\
				}																				\
			}																					\
		}																						\
																								\
		if ( directOpen )																		\
			blob->directCloseBlob();															\
	}																							\
																								\
	if ( indicatorTo && !to->isIndicatorSqlDa )													\
		*indicatorTo = sizeof(C_TYPE_TO);														\
																								\
	return ret;																					\
}																								\

int OdbcConvert::notYetImplemented(DescRecord * from, DescRecord * to)
{ 
	parentStmt->postError ("07006", "Restricted data type attribute violation");
	return SQL_ERROR; 
}

////////////////////////////////////////////////////////////////////////
// Guid
////////////////////////////////////////////////////////////////////////

int OdbcConvert::convGuidToString(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	SQLGUID *g = (SQLGUID*)getAdressBindDataFrom((char*)from->dataPtr);
	int len, outlen = to->length;

	len = snprintf(pointer, outlen, "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		(unsigned int) g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

	if ( len == -1 ) len = outlen;

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convGuidToStringW(DescRecord * from, DescRecord * to)
{
	wchar_t *pointer = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointer );

	SQLGUID *g = (SQLGUID*)getAdressBindDataFrom((char*)from->dataPtr);
	int len, outlen = to->length / sizeof( wchar_t );

	len = swprintf(pointer, outlen, L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		(unsigned int) g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

	len = len == -1 ? outlen * sizeof( wchar_t ) : len * sizeof( wchar_t );

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// TinyInt
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(TinyInt,char,Boolean,bool);
ODBCCONVERT_CONV(TinyInt,char,TinyInt,char);
ODBCCONVERT_CONV(TinyInt,unsigned char,Short,short);
ODBCCONVERT_CONV(TinyInt,unsigned char,Long,int);
ODBCCONVERT_CONV(TinyInt,unsigned char,Float,float);
ODBCCONVERT_CONV(TinyInt,unsigned char,Double,double);
ODBCCONVERT_CONV(TinyInt,unsigned char,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(TinyInt,char,3);
ODBCCONVERT_CONV_TO_STRINGW(TinyInt,char,3);
ODBCCONVERT_CONVTAGNUMERIC(TinyInt,unsigned char);

////////////////////////////////////////////////////////////////////////
// Short
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Short,short,Boolean,bool);
ODBCCONVERT_CONV(Short,short,TinyInt,char);
ODBCCONVERT_CONV(Short,short,Short,short);
ODBCCONVERT_CONV(Short,short,Long,int);
ODBCCONVERT_CONV(Short,short,Float,float);
ODBCCONVERT_CONV(Short,short,Double,double);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,TinyInt,char);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Short,short);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Long,int);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Float,float);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Double,double);
ODBCCONVERT_WITH_SCALE_CONV(Short,short,Bigint,QUAD);
ODBCCONVERT_CONV(Short,short,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(Short,short,5);
ODBCCONVERT_CONV_TO_STRINGW(Short,short,5);
ODBCCONVERT_CONVTAGNUMERIC(Short,short);

////////////////////////////////////////////////////////////////////////
// Long
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Long,int,Boolean,bool);
ODBCCONVERT_CONV(Long,int,TinyInt,char);
ODBCCONVERT_CONV(Long,int,Short,short);
ODBCCONVERT_CONV(Long,int,Long,int);
ODBCCONVERT_CONV(Long,int,Float,float);
ODBCCONVERT_CONV(Long,int,Double,double);
ODBCCONVERT_CONV(Long,int,Bigint,QUAD);
ODBCCONVERT_WITH_SCALE_CONV(Long,int,TinyInt,char);
ODBCCONVERT_WITH_SCALE_CONV(Long,int,Short,short);
ODBCCONVERT_WITH_SCALE_CONV(Long,int,Long,int);
ODBCCONVERT_WITH_SCALE_CONV(Long,int,Float,float);
ODBCCONVERT_WITH_SCALE_CONV(Long,int,Double,double);
ODBCCONVERT_WITH_SCALE_CONV(Long,int,Bigint,QUAD);
ODBCCONVERT_CONV_TO_STRING(Long,int,10);
ODBCCONVERT_CONV_TO_STRINGW(Long,int,10);
ODBCCONVERT_CONVTAGNUMERIC(Long,int);

////////////////////////////////////////////////////////////////////////
// Float
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONVROUND(Float,float,TinyInt,char);
ODBCCONVERT_CONVROUND(Float,float,Short,short);
ODBCCONVERT_CONVROUND(Float,float,Long,int);
ODBCCONVERT_CONV(Float,float,Boolean,bool);
ODBCCONVERT_CONV(Float,float,Float,float);
ODBCCONVERT_CONV(Float,float,Double,double);
ODBCCONVERT_CONVROUND(Float,float,Bigint,QUAD);

int OdbcConvert::convFloatToString(DescRecord * from, DescRecord * to)
{
	char * pointerTo = (char *)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );

	int len = to->length;

	if ( len )
		ConvertFloatToString<char>(*(float*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len, &len);

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convFloatToStringW(DescRecord * from, DescRecord * to)
{
	wchar_t * pointerTo = (wchar_t *)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointerTo );

	int len = to->length;

	if ( len )
	{
		ConvertFloatToString<wchar_t>(*(float*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len/2, &len);
		len *= sizeof( wchar_t );
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Double
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONVROUND(Double,double,TinyInt,char);
ODBCCONVERT_CONVROUND(Double,double,Short,short);
ODBCCONVERT_CONVROUND(Double,double,Long,int);
ODBCCONVERT_CONV(Double,double,Boolean,bool);
ODBCCONVERT_CONV(Double,double,Float,float);
ODBCCONVERT_CONV(Double,double,Double,double);
ODBCCONVERT_CONVROUND(Double,double,Bigint,QUAD);

int OdbcConvert::convDoubleToTagNumeric(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	QUAD *number = (QUAD*)( pointer + 3 );
	*number = (QUAD)(*(double*)getAdressBindDataFrom( (char*)from->dataPtr ) * (QUAD)listScale[from->scale] );

	if ( to->scale != from->scale )
	{
		if ( to->scale > from->scale )
			*number *= listScale[to->scale-from->scale];
		else /* if ( to->scale < from->scale )	*/
		{
			if ( to->scale )
			{
				QUAD round = 5 * listScale[from->scale - to->scale - 1];

				if ( *number > 0 )
					*number += round;
				else if ( *number < 0 )
					*number -= round;
			}
			*number /= listScale[from->scale - to->scale];
		}
	}

	*pointer++ = (char)to->precision;
	*pointer++ = (char)to->scale;

	if ( *number < 0 )
		*number = -*number,
		*pointer++ = 0;
	else
		*pointer++ = 1;

	*++number = 0;

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof ( tagSQL_NUMERIC_STRUCT );

	return SQL_SUCCESS;
}

int OdbcConvert::convDoubleToString(DescRecord * from, DescRecord * to)
{
	char * pointerTo = (char *)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );

	int len = to->length;

	if ( len )	// MAX_DOUBLE_DIGIT_LENGTH = 15
		ConvertFloatToString<char>(*(double*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len, &len);

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convDoubleToStringW(DescRecord * from, DescRecord * to)
{
	wchar_t * pointerTo = (wchar_t *)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointerTo );

	int len = to->length;

	if ( len )	// MAX_DOUBLE_DIGIT_LENGTH = 15
	{
		ConvertFloatToString<wchar_t>(*(double*)getAdressBindDataFrom((char*)from->dataPtr), pointerTo, len/2, &len);
		len *= sizeof( wchar_t );
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Bigint
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_BIGINT_CONV(Boolean,bool);
ODBCCONVERT_BIGINT_CONV(TinyInt,char);
ODBCCONVERT_BIGINT_CONV(Short,short);
ODBCCONVERT_BIGINT_CONV(Long,int);
ODBCCONVERT_BIGINT_CONV(Float,float);
ODBCCONVERT_BIGINT_CONV(Double,double);
ODBCCONVERT_WITH_SCALE_CONV(Bigint,QUAD,Float,float);
ODBCCONVERT_WITH_SCALE_CONV(Bigint,QUAD,Double,double);
ODBCCONVERT_BIGINT_CONV(Bigint,QUAD);
ODBCCONVERT_CONV_TO_BINARY(Bigint,QUAD,18);
ODBCCONVERT_CONV_TO_STRING(Bigint,QUAD,18);
ODBCCONVERT_CONV_TO_STRINGW(Bigint,QUAD,18);
ODBCCONVERT_CONVTAGNUMERIC(Bigint,QUAD);

////////////////////////////////////////////////////////////////////////
// Numeric,Decimal
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Numeric,QUAD,Boolean,bool);
ODBCCONVERT_CONV(Numeric,QUAD,TinyInt,char);
ODBCCONVERT_CONV(Numeric,QUAD,Short,short);
ODBCCONVERT_CONV(Numeric,QUAD,Long,int);
ODBCCONVERT_CONV(Numeric,QUAD,Float,float);
ODBCCONVERT_CONV(Numeric,QUAD,Double,double);
ODBCCONVERT_CONV(Numeric,QUAD,Bigint,QUAD);

////////////////////////////////////////////////////////////////////////
// TagNumeric
////////////////////////////////////////////////////////////////////////

ODBCCONVERTTAG_NUMERIC_CONV(TinyInt,char);
ODBCCONVERTTAG_NUMERIC_CONV(Short,short);
ODBCCONVERTTAG_NUMERIC_CONV(Long,int);
ODBCCONVERTTAG_NUMERIC_CONV_FLOAT(Float,float);
ODBCCONVERTTAG_NUMERIC_CONV_FLOAT(Double,double);
ODBCCONVERTTAG_NUMERIC_CONV(Bigint,QUAD);
ODBCCONVERT_CONVTAGNUMERIC(Numeric,QUAD);

////////////////////////////////////////////////////////////////////////
#define ODBCCONVERT_TEMP_CONV(TYPE_FROM,TYPE_TO,C_TYPE_TO)										\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
																								\
	ODBCCONVERT_CHECKNULL( pointer );															\
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

ODBCCONVERT_CONV(Date,int,Long,int);
ODBCCONVERT_CONV(Date,int,Float,float);
ODBCCONVERT_CONV(Date,int,Double,double);
ODBCCONVERT_CONV(Date,int,Bigint,QUAD);

int OdbcConvert::convDateToString(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	SQLUSMALLINT mday, month;
	SQLSMALLINT year;

	decode_sql_date(*(int*)getAdressBindDataFrom((char*)from->dataPtr), mday, month, year);
	int len, outlen = to->length;

	len = snprintf(pointer, outlen, "%04d-%02d-%02d",year,month,mday);

	if ( len == -1 ) len = outlen;

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToStringW(DescRecord * from, DescRecord * to)
{
	wchar_t *pointer = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointer );

	SQLUSMALLINT mday, month;
	SQLSMALLINT year;

	decode_sql_date(*(int*)getAdressBindDataFrom((char*)from->dataPtr), mday, month, year);
	int len, outlen = to->length / sizeof( wchar_t );

	len = swprintf(pointer, outlen, L"%04d-%02d-%02d",year,month,mday);

	len = len == -1 ? outlen * sizeof( wchar_t ) : len * sizeof( wchar_t );
	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToTagDate(DescRecord * from, DescRecord * to)
{
	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagDt );

	decode_sql_date(*(int*)getAdressBindDataFrom((char*)from->dataPtr), tagDt->day, tagDt->month, tagDt->year);

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagDATE_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateToBinary(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	SQLUSMALLINT mday, month;
	SQLSMALLINT year;

	decode_sql_date(*(int*)getAdressBindDataFrom((char*)from->dataPtr), mday, month, year);
	int outlen = to->length;

	if ( outlen == sizeof(tagDATE_STRUCT) ) // tagDate
	{
		tagDATE_STRUCT *pt = (tagDATE_STRUCT*)pointer;
		pt->year = year;
		pt->day = mday;
		pt->month = month;
	}
	else if ( outlen == 4 ) // DOS date
	{
		shortDate *pt = (shortDate*)pointer;
		pt->year = year;
		pt->day = (char)mday;
		pt->month = (char)month;
	}
	else
	{
		tagDATE_STRUCT tagDt;
		tagDt.year = year;
		tagDt.day = mday;
		tagDt.month = month;
		memcpy( pointer, &tagDt, outlen );
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(outlen);
	} else
	if ( indicatorTo )
		*indicatorTo = outlen;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateToDate(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	*(int*)pointer = encode_sql_date ( tagDt->day, tagDt->month, tagDt->year );

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateToDateTime(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	int nday = encode_sql_date ( tagDt->day, tagDt->month, tagDt->year );

	*(QUAD*)pointer = MAKEQUAD( nday, 0 );

	return SQL_SUCCESS;
}

// for use App from SqlDa
int OdbcConvert::convDateToTagTimestamp(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagTs );

	decode_sql_date(*(int*)getAdressBindDataFrom((char*)from->dataPtr), tagTs->day, tagTs->month, tagTs->year);
	tagTs->hour = tagTs->minute = tagTs->second = 0;
	tagTs->fraction = 0;

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_CONV(Time,int,Long,int);
ODBCCONVERT_CONV(Time,int,Float,float);
ODBCCONVERT_CONV(Time,int,Double,double);
ODBCCONVERT_CONV(Time,int,Bigint,QUAD);

int OdbcConvert::convTimeToString(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	SQLUSMALLINT hour, minute, second;
	int ntime = *(int*)getAdressBindDataFrom((char*)from->dataPtr);
	int nnano = ntime % ISC_TIME_SECONDS_PRECISION;

	decode_sql_time(ntime, hour, minute, second);

	int len, outlen = to->length;

	if ( nnano )
		len = snprintf(pointer, outlen, "%02d:%02d:%02d.%04lu",hour, minute, second, nnano);
	else
		len = snprintf(pointer, outlen, "%02d:%02d:%02d",hour, minute, second);

	if ( len == -1 ) len = outlen;

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToStringW(DescRecord * from, DescRecord * to)
{
	wchar_t *pointer = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointer );

	SQLUSMALLINT hour, minute, second;
	int ntime = *(int*)getAdressBindDataFrom((char*)from->dataPtr);
	int nnano = ntime % ISC_TIME_SECONDS_PRECISION;

	decode_sql_time(ntime, hour, minute, second);

	int len, outlen = to->length / sizeof( wchar_t );

	if ( nnano )
		len = swprintf(pointer, outlen, L"%02d:%02d:%02d.%04lu",hour, minute, second, nnano);
	else
		len = swprintf(pointer, outlen, L"%02d:%02d:%02d",hour, minute, second);

	len = len == -1 ? outlen * sizeof( wchar_t ) : len * sizeof( wchar_t );

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToTagTime(DescRecord * from, DescRecord * to)
{
	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagTm );

	decode_sql_time(*(int*)getAdressBindDataFrom((char*)from->dataPtr), tagTm->hour, tagTm->minute, tagTm->second);

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagTIME_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToBinary(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	SQLUSMALLINT hour, minute, second;
	int ntime = *(int*)getAdressBindDataFrom((char*)from->dataPtr);
	int nnano = ntime % ISC_TIME_SECONDS_PRECISION;

	decode_sql_time(ntime, hour, minute, second);

	int outlen = to->length;

	if ( outlen == sizeof(tagTIME_STRUCT) ) // tagTime
	{
		tagTIME_STRUCT *pt = (tagTIME_STRUCT*)pointer;
		pt->hour = hour;
		pt->minute = minute;
		pt->second = second;
	}
	else if ( outlen == 4 ) // DOS date
	{
		shortTime *pt = (shortTime*)pointer;
		pt->hour = (unsigned char)hour;
		pt->minute = (unsigned char)minute;
		pt->second = (unsigned char)second;
		if ( nnano ) nnano = (10000 + nnano) / 100 - 100;
		pt->hsecond = (unsigned char)nnano;
	}
	else
	{
		tagTIME_STRUCT tagTm;
		tagTm.hour = hour;
		tagTm.minute = minute;
		tagTm.second = second;
		memcpy( pointer, &tagTm, outlen );
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(outlen);
	} else
	if ( indicatorTo )
		*indicatorTo = outlen;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagTimeToTime(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	*(int*)pointer = encode_sql_time ( tagTm->hour, tagTm->minute, tagTm->second );

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagTimeToDateTime(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	int ntime = encode_sql_time ( tagTm->hour, tagTm->minute, tagTm->second );
	int nday = encode_sql_date ( 1, 1, 100 ); // min validate date for server

	*(QUAD*)pointer = MAKEQUAD( nday, ntime );

	return SQL_SUCCESS;
}

int OdbcConvert::convTimeToTagTimestamp(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagTs );

	int ntime = *(int*)getAdressBindDataFrom((char*)from->dataPtr);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->day = tagTs->month = tagTs->year = 0;
	tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;

	if ( indicatorTo && !to->isIndicatorSqlDa )
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
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	QUAD pointerFrom = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	int ndate = LO_LONG(pointerFrom);
	int ntime = HI_LONG(pointerFrom);
	int nnano = ntime % ISC_TIME_SECONDS_PRECISION;
	SQLUSMALLINT mday, month;
	SQLSMALLINT year;
	SQLUSMALLINT hour, minute, second;

	decode_sql_date(ndate, mday, month, year);
	decode_sql_time(ntime, hour, minute, second);
	int len, outlen = to->length;

	if ( nnano )
		len = snprintf(pointer, outlen, "%04d-%02d-%02d %02d:%02d:%02d.%04lu",year,month,mday,hour, minute, second, nnano);
	else
		len = snprintf(pointer, outlen, "%04d-%02d-%02d %02d:%02d:%02d",year,month,mday,hour, minute, second);

	if ( len == -1 ) len = outlen;

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToStringW(DescRecord * from, DescRecord * to)
{
	wchar_t *pointer = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointer );

	QUAD pointerFrom = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);
	int ndate = LO_LONG(pointerFrom);
	int ntime = HI_LONG(pointerFrom);
	int nnano = ntime % ISC_TIME_SECONDS_PRECISION;
	SQLUSMALLINT mday, month;
	SQLSMALLINT year;
	SQLUSMALLINT hour, minute, second;

	decode_sql_date(ndate, mday, month, year);
	decode_sql_time(ntime, hour, minute, second);
	int len, outlen = to->length / sizeof( wchar_t );

	if ( nnano )
		len = swprintf( pointer, outlen, L"%04d-%02d-%02d %02d:%02d:%02d.%04lu",year,month,mday,hour, minute, second, nnano );
	else
		len = swprintf( pointer, outlen, L"%04d-%02d-%02d %02d:%02d:%02d",year,month,mday,hour, minute, second );

	len = len == -1 ? outlen * sizeof( wchar_t ) : len * sizeof( wchar_t );

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagDate(DescRecord * from, DescRecord * to)
{
	tagDATE_STRUCT * tagDt = (tagDATE_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagDt );

	int nday = LO_LONG ( *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr) );

	decode_sql_date(nday, tagDt->day, tagDt->month, tagDt->year);

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagDATE_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagTime(DescRecord * from, DescRecord * to)
{
	tagTIME_STRUCT * tagTm = (tagTIME_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagTm );

	int ntime = HI_LONG ( *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr) );

	decode_sql_time(ntime, tagTm->hour, tagTm->minute, tagTm->second);

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagTIME_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToTagDateTime(DescRecord * from, DescRecord * to)
{
	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( tagTs );

	QUAD &number = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);

	int nday = LO_LONG(number);
	int ntime = HI_LONG(number);

	if ( ntime < 0 ) 
		ntime = 0;

	decode_sql_date(nday, tagTs->day, tagTs->month, tagTs->year);
	decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
	tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}

int OdbcConvert::convDateTimeToBinary(DescRecord * from, DescRecord * to)
{
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

	QUAD &number = *(QUAD*)getAdressBindDataFrom((char*)from->dataPtr);

	int nday = LO_LONG(number);
	int ntime = HI_LONG(number);
	int outlen = to->length;

	if ( outlen == sizeof(tagTIMESTAMP_STRUCT) ) // tagTimestamp
	{
		tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)pointer;
		decode_sql_date(nday, tagTs->day, tagTs->month, tagTs->year);
		decode_sql_time(ntime, tagTs->hour, tagTs->minute, tagTs->second);
		tagTs->fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;
	}
	else if ( outlen == 8 ) // DOS date/time
	{
		tagTIMESTAMP_STRUCT tagTs;

		decode_sql_date(nday, tagTs.day, tagTs.month, tagTs.year);
		decode_sql_time(ntime, tagTs.hour, tagTs.minute, tagTs.second);
		tagTs.fraction = (ntime % ISC_TIME_SECONDS_PRECISION);

		if ( tagTs.fraction ) 
			tagTs.fraction = (10000 + tagTs.fraction) / 100 - 100;

		shortDate *ptd = (shortDate*)pointer;
		ptd->year = tagTs.year;
		ptd->day = (char)tagTs.day;
		ptd->month = (char)tagTs.month;

		shortTime *ptt = (shortTime*)(pointer + 4);
		ptt->hour = (unsigned char)tagTs.hour;
		ptt->minute = (unsigned char)tagTs.minute;
		ptt->second = (unsigned char)tagTs.second;
		ptt->hsecond = (unsigned char)tagTs.fraction;
	}
	else
	{
		tagTIMESTAMP_STRUCT tagTs;

		decode_sql_date(nday, tagTs.day, tagTs.month, tagTs.year);
		decode_sql_time(ntime, tagTs.hour, tagTs.minute, tagTs.second);
		tagTs.fraction = (ntime % ISC_TIME_SECONDS_PRECISION) * STD_TIME_SECONDS_PRECISION;
		memcpy( pointer, &tagTs, outlen );
	}

	if ( indicatorTo && !to->isIndicatorSqlDa )
		*indicatorTo = sizeof(tagTIMESTAMP_STRUCT);

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateTimeToDate(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	*(int*)pointer = encode_sql_date ( tagTs->day, tagTs->month, tagTs->year );

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateTimeToTime(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	int &ntime = *(int*)getAdressBindDataTo((char*)to->dataPtr);

	ntime = encode_sql_time ( tagTs->hour, tagTs->minute, tagTs->second );
	ntime += tagTs->fraction / STD_TIME_SECONDS_PRECISION;

	return SQL_SUCCESS;
}

// for use App to SqlDa
int OdbcConvert::transferTagDateTimeToDateTime(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	tagTIMESTAMP_STRUCT * tagTs = (tagTIMESTAMP_STRUCT*)getAdressBindDataFrom((char*)from->dataPtr);
	char* pointer = (char*)getAdressBindDataTo((char*)to->dataPtr);

	int nday = encode_sql_date ( tagTs->day, tagTs->month, tagTs->year );
	int ntime = encode_sql_time ( tagTs->hour, tagTs->minute, tagTs->second );

	ntime += tagTs->fraction / STD_TIME_SECONDS_PRECISION;

	*(QUAD*)pointer = MAKEQUAD( nday, ntime );

	return SQL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Blob
////////////////////////////////////////////////////////////////////////

ODBCCONVERT_BLOB_CONV(TinyInt,char);
ODBCCONVERT_BLOB_CONV(Short,short);
ODBCCONVERT_BLOB_CONV(Long,int);
ODBCCONVERT_BLOB_CONV(Float,float);
ODBCCONVERT_BLOB_CONV(Double,double);
ODBCCONVERT_BLOB_CONV(Bigint,QUAD);

int OdbcConvert::convBlobToBlob(DescRecord * from, DescRecord * to)
{
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

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
			from->startedReturnSQLData = false;
			if ( !fetched || blob->getOffset() )
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
		else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else
		{
			from->startedReturnSQLData = true;
			int len = MIN(dataRemaining, MAX(0, (int)to->length));
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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(dataRemaining);
	} else
	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBlobToBinary(DescRecord * from, DescRecord * to)
{
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

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
			from->startedReturnSQLData = false;
			if ( !fetched || blob->getOffset() )
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
		else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else if ( pointer )
		{
			from->startedReturnSQLData = true;
			int len = MIN(dataRemaining, MAX(0, (int)to->length-1)>>1);
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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(dataRemaining);
	} else
	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBlobToString(DescRecord * from, DescRecord * to)
{
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointer );

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
			from->startedReturnSQLData = false;
			if ( !fetched || blob->getOffset() )
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
		else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else
		{
			from->startedReturnSQLData = true;
			int len = MIN(dataRemaining, MAX(0, (int)to->length-1));
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
				}
				((char*) (pointer)) [len] = 0;

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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(dataRemaining);
	} else
	if ( indicatorTo )
		*indicatorTo = dataRemaining;

	return ret;
}

int OdbcConvert::convBlobToStringW( DescRecord * from, DescRecord * to )
{
	SQLRETURN ret = SQL_SUCCESS;
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointer );

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
			from->startedReturnSQLData = false;
			from->freeLocalDataPtr();
			if ( !fetched || blob->getOffset() )
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

		if (blob->isBlob())
		{
			length = blob->length() * 2;
		}
		else if (!from->isLocalDataPtr)
		{
			// we cannot tell the length of the codepage-converted output
			// in advance. so fetch all and convert in advance

			if ( blob->isArray() )
				length = ((BinaryBlob*)blob)->getLength();
			else
				length = blob->length();

			from->allocateLocalDataPtr((length + 1) * sizeof(wchar_t));
			wchar_t *wcs = (wchar_t*) from->localDataPtr;
			char *tmp = new char[length];

			if ( !directOpen )
			{
				blob->getBytes (0, length, tmp);
			}
			else
			{
				int lenRead = 0;
				blob->directFetchBlob(tmp, length, lenRead);
				length = lenRead;
			}

			length = from->MbsToWcs(wcs, tmp, length);
			wcs[length] = L'\0';

			delete [] tmp;
		}
		else
		{
			length = wcslen(((wchar_t*) from->localDataPtr) + from->dataOffset) + from->dataOffset;
		}

		dataRemaining = length - from->dataOffset;

		if ( !to->length )
			;
		else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
		{
			from->dataOffset = 0;
			ret = SQL_NO_DATA;
		}
		else
		{
			from->startedReturnSQLData = true;
			int len = MIN(dataRemaining, MAX(0, to->length / (int)sizeof(wchar_t) - 1));
			 
			if ( pointer )
			{
				if (blob->isBlob())
				{
					len &= ~1;  // we can only return an even number
					if ( len > 0 ) 
					{
						char *tmp = new char[len];

						if ( !directOpen )
						{
							blob->getHexString (from->dataOffset/2, len/2, tmp);
						}
						else
						{
							int lenRead = 0;
							blob->directGetSegmentToHexStr(tmp, len/2, lenRead);
							len = lenRead;
						}

						from->MbsToWcs( (wchar_t *)pointer, tmp, len );

						delete [] tmp;
					}
				}
				else
				{
					wcsncpy((wchar_t*) pointer, ((wchar_t*) from->localDataPtr) + from->dataOffset, len);
				}

				((wchar_t *) (pointer)) [len] = L'\0';

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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(dataRemaining * sizeof(wchar_t));
	} else
	if ( indicatorTo )
		*indicatorTo = dataRemaining * sizeof(wchar_t);

	return ret;
}

int OdbcConvert::convBinaryToBlob(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
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
#define CALC_LEN_STRING  (int)strlen((char*)p)

#define ODBCCONVERT_CONV_STRING_TO(TYPE_FROM,TYPE_TO,C_TYPE_TO)									\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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

#define ODBCCONVERT_CONV_STRINGW_TO(TYPE_FROM,TYPE_TO,C_TYPE_TO)								\
int OdbcConvert::conv##TYPE_FROM##To##TYPE_TO(DescRecord * from, DescRecord * to)				\
{																								\
	SQLPOINTER pointer = getAdressBindDataTo((char*)to->dataPtr);								\
	SQLLEN *indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);						\
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);				\
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
	for (;p < end; ++p )																		\
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
ODBCCONVERT_CONV_STRING_TO(String,Long,int);
ODBCCONVERT_CONV_STRING_TO(String,Float,float);
ODBCCONVERT_CONV_STRING_TO(String,Double,double);
ODBCCONVERT_CONV_STRING_TO(String,Bigint,QUAD);
ODBCCONVERT_CONV_STRINGW_TO(StringW,TinyInt,char);
ODBCCONVERT_CONV_STRINGW_TO(StringW,Short,short);
ODBCCONVERT_CONV_STRINGW_TO(StringW,Long,int);
ODBCCONVERT_CONV_STRINGW_TO(StringW,Float,float);
ODBCCONVERT_CONV_STRINGW_TO(StringW,Double,double);
ODBCCONVERT_CONV_STRINGW_TO(StringW,Bigint,QUAD);

int OdbcConvert::convStringToString(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );

	bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

	if ( !fetched )
	{ // new row read
		from->dataOffset = 0;
		from->startedReturnSQLData = false;
		from->currentFetched = parentStmt->getCurrentFetched();
	}

	SQLRETURN ret = SQL_SUCCESS;
	int length = from->length;
	int dataRemaining = length - from->dataOffset;

	if ( !to->length )
		length = dataRemaining;
	else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
	{
		from->dataOffset = 0;
		ret = SQL_NO_DATA;
	}
	else
	{
		from->startedReturnSQLData = true;
		int len = MIN(dataRemaining, MAX(0, (int)to->length-1));
		 
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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(length);
	} else
	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

int OdbcConvert::convStringToStringW(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	wchar_t * pointerFromWcs;
	wchar_t * pointerTo = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointerTo );

	SQLRETURN ret = SQL_SUCCESS;
	int length;
	bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

	if ( !fetched )
	{ // new row read
		from->dataOffset = 0;
		from->startedReturnSQLData = false;
		from->currentFetched = parentStmt->getCurrentFetched();

		if (!to->isLocalDataPtr)
			to->allocateLocalDataPtr((from->getBufferLength() + 1) * sizeof(wchar_t));

		pointerFromWcs = (wchar_t*) to->localDataPtr;

		length = (int)from->MbsToWcs( pointerFromWcs, pointerFrom, from->length * from->headSqlVarPtr->getSqlMultiple() );
		if ( length < 0 )
			length = 0;

		while (length < from->length)       // safety-code - should not happen
			pointerFromWcs[length++] = L' ';

		length = from->length;
		pointerFromWcs[length] = L'\0';
	}
	else
	{
		pointerFromWcs = (wchar_t*) to->localDataPtr;
		length = wcslen(pointerFromWcs + from->dataOffset) + from->dataOffset;
	}

	int dataRemaining = length - from->dataOffset;

	if ( !to->length )
		;
	else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
	{
		from->dataOffset = 0;
		ret = SQL_NO_DATA;
	}
	else
	{
		from->startedReturnSQLData = true;
		int len = MIN(dataRemaining, MAX(0, (int)(to->length / sizeof( wchar_t )) - 1 ));
		 
		if ( pointerTo )
		{
			wcsncpy(pointerTo, pointerFromWcs + from->dataOffset, len);
			pointerTo[len] = L'\0';

			from->dataOffset += len;

			if ( len && len < dataRemaining )
			{
				OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
				ret = SQL_SUCCESS_WITH_INFO;
			}
		}
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(dataRemaining * sizeof( wchar_t ));
	} else
	if ( indicatorTo )
		*indicatorTo = dataRemaining * sizeof( wchar_t );

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStringToVarString(DescRecord * from, DescRecord * to)
{	
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);

	SQLRETURN ret = SQL_SUCCESS;
	unsigned short &lenVar = *(unsigned short*)pointerTo;
	int len;

	GET_LEN_FROM_OCTETLENGTHPTR;

	lenVar = MIN( len, (int)MAX(0, (int)to->length));

	if( lenVar > 0 )
		memcpy ( pointerTo+2, pointerFrom, lenVar);

	if (lenVar && (int)lenVar > (int)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(lenVar);
	} else
	if ( indicatorTo )
		*(short*)indicatorTo = lenVar;

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStringToBlob(DescRecord * from, DescRecord * to)
{	
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
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
int OdbcConvert::convStringWToBlob(DescRecord * from, DescRecord * to)
{	
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	wchar_t * pointerFrom = (wchar_t*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);

	SQLINTEGER len;
	SQLINTEGER lenMbs;
	SQLRETURN ret = SQL_SUCCESS;

	GET_WLEN_FROM_OCTETLENGTHPTR;

	lenMbs = (SQLUINTEGER)to->WcsToMbs( NULL, pointerFrom, 0 );
	lenMbs = MIN( lenMbs, (int)MAX(0, (int)to->length));

	if ( lenMbs > 0 )
	{
		char* tempValue = new char[lenMbs + 1];
		lenMbs = (SQLUINTEGER)to->WcsToMbs( tempValue, pointerFrom, lenMbs );
		to->dataBlobPtr->writeStringHexToBlob(pointerTo, tempValue, lenMbs);
		delete [] tempValue;
	}
	else		
		*(short*)indicatorTo = SQL_NULL_DATA;

	return ret;
}

// for use App to SqlDa
int OdbcConvert::convStreamHexStringToBlob(DescRecord * from, DescRecord * to)
{	
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

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
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	from->dataBlobPtr->writeBlob( pointerTo );

	return SQL_SUCCESS;
}

int OdbcConvert::convStringToBinary(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );

	bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

	if ( !fetched )
	{ // new row read
		from->dataOffset = 0;
		from->startedReturnSQLData = false;
		from->currentFetched = parentStmt->getCurrentFetched();
	}

	SQLRETURN ret = SQL_SUCCESS;
	int length = from->length;
	int dataRemaining = length - from->dataOffset;

	if ( !to->length )
		length = dataRemaining;
	else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
	{
		from->dataOffset = 0;
		ret = SQL_NO_DATA;
	}
	else
	{
		from->startedReturnSQLData = true;
		int len = MIN(dataRemaining, MAX(0, (int)to->length));
		 
		if ( !pointerTo )
			length = dataRemaining;
		else
		{
			if( len > 0 )
				memcpy ( pointerTo, pointerFrom + from->dataOffset, len );

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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(length);
	} else
	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

// for use App to SqlDa
int OdbcConvert::transferStringToTinyInt(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

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
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
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

int OdbcConvert::transferStringWToDateTime(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	wchar_t * pointerFrom = (wchar_t*)getAdressBindDataFrom((char*)from->dataPtr);

	int len = 0;

	if ( !to->isLocalDataPtr )
		to->allocateLocalDataPtr();

	GET_WLEN_FROM_OCTETLENGTHPTR

	int n = len;
	char * beg = (char*)pointerFrom + 1;
	short * next = (short*)(beg + 1);
	
	while ( n-- )
		*beg++ = (char)*next++;

	convertStringDateTimeToServerStringDateTime ( (char*&)pointerFrom, len );

	if ( !len )
		return SQL_SUCCESS;

	if ( !from->data_at_exec )
	{
		to->headSqlVarPtr->setSqlLen( (short)len );
		to->headSqlVarPtr->setSqlData( (char*)pointerFrom );
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
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);

	SQLINTEGER len;
	SQLRETURN ret = SQL_SUCCESS;

	GET_LEN_FROM_OCTETLENGTHPTR;

	if ( !from->data_at_exec )
	{
		if ( len > to->octetLength )
		{
			OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
			ret = SQL_SUCCESS_WITH_INFO;
		}

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

		if ( len + from->dataOffset > to->length )
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
int OdbcConvert::transferStringWToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	wchar_t * pointerFrom = (wchar_t *)getAdressBindDataFrom((char*)from->dataPtr);

	SQLINTEGER len;
	SQLINTEGER cch;
	SQLINTEGER lenMbs;
	SQLRETURN ret = SQL_SUCCESS;

	GET_WLEN_FROM_OCTETLENGTHPTR;
	cch = wcscch(pointerFrom, len);

	if ( !to->isLocalDataPtr )
	{
		to->allocateLocalDataPtr();
		to->headSqlVarPtr->setSqlData( to->localDataPtr );
	}

	if (from->dataOffset == 0)
		to->dataOffset = 0;

	if ( cch + from->dataOffset > to->octetLength )
	{
  		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
		ret = SQL_SUCCESS_WITH_INFO;
		do
		{
			len--;
			if (!IS_LOW_SURROGATE(pointerFrom[len-1]))
				cch--;
		} while (cch + from->dataOffset > to->octetLength);
	}

	if ( len < 0 )
	{
		cch = len = 0;
		lenMbs = 0;
	}
	else
	{
		wchar_t &wcEnd = *(pointerFrom + len);
		wchar_t saveEnd = wcEnd;
		wcEnd = L'\0';	// We guarantee the end L'\0'
		SQLUINTEGER spaceLeft = (to->octetLength - from->dataOffset) * to->headSqlVarPtr->getSqlMultiple();
		lenMbs = (SQLUINTEGER)to->WcsToMbs( to->localDataPtr + to->dataOffset, pointerFrom, spaceLeft);
		wcEnd = saveEnd;
	}

	if ( from->data_at_exec )
	{
		from->dataOffset += cch;
		to->dataOffset += lenMbs;
		to->headSqlVarPtr->setSqlLen( (short)to->dataOffset );
	}
    else
		to->headSqlVarPtr->setSqlLen( (short)lenMbs );

	return ret;
}

// for use App to SqlDa
int OdbcConvert::transferArrayStringToAllowedType(DescRecord * from, DescRecord * to)
{
	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

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
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);

	ODBCCONVERT_CHECKNULL_SQLDA;

	SQLLEN * octetLengthPtr = getAdressBindIndFrom((char*)from->octetLengthPtr);
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
ODBCCONVERT_CONV_STRING_TO(VarString,Long,int);
ODBCCONVERT_CONV_STRING_TO(VarString,Float,float);
ODBCCONVERT_CONV_STRING_TO(VarString,Double,double);
ODBCCONVERT_CONV_STRING_TO(VarString,Bigint,QUAD);
ODBCCONVERT_CONV_STRINGW_TO(VarStringW,TinyInt,char);
ODBCCONVERT_CONV_STRINGW_TO(VarStringW,Short,short);
ODBCCONVERT_CONV_STRINGW_TO(VarStringW,Long,int);
ODBCCONVERT_CONV_STRINGW_TO(VarStringW,Float,float);
ODBCCONVERT_CONV_STRINGW_TO(VarStringW,Double,double);
ODBCCONVERT_CONV_STRINGW_TO(VarStringW,Bigint,QUAD);

int OdbcConvert::convVarStringToBinary(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );

	bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

	if ( !fetched )
	{ // new row read
		from->dataOffset = 0;
		from->startedReturnSQLData = false;
		from->currentFetched = parentStmt->getCurrentFetched();
	}

	SQLRETURN ret = SQL_SUCCESS;
	int length = *(unsigned short*)pointerFrom;
	int dataRemaining = length - from->dataOffset;

	if ( !to->length )
		length = dataRemaining;
	else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
	{
		from->dataOffset = 0;
		ret = SQL_NO_DATA;
	}
	else
	{
		from->startedReturnSQLData = true;
		int len = MIN(dataRemaining, MAX(0, (int)to->length));
		 
		if ( !pointerTo )
			length = dataRemaining;
		else
		{
			pointerFrom += sizeof( short );
			if( len > 0 )
				memcpy ( pointerTo, pointerFrom + from->dataOffset, len );

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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(length);
	} else
	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

int OdbcConvert::convVarStringToString(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );

	bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

	if ( !fetched )
	{ // new row read
		from->dataOffset = 0;
		from->startedReturnSQLData = false;
		from->currentFetched = parentStmt->getCurrentFetched();
	}

	SQLRETURN ret = SQL_SUCCESS;
	int length = *(unsigned short*)pointerFrom;
	int dataRemaining = length - from->dataOffset;

	if ( !to->length )
		length = dataRemaining;
	else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
	{
		from->dataOffset = 0;
		ret = SQL_NO_DATA;
	}
	else
	{
		from->startedReturnSQLData = true;
		int len = MIN(dataRemaining, MAX(0, (int)to->length-1));
		 
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

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(length);
	} else
	if ( indicatorTo )
		*indicatorTo = length;

	return ret;
}

int OdbcConvert::convVarStringToStringW(DescRecord * from, DescRecord * to)
{
	short * pointerFromLen = (short*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerFrom = (char*)(pointerFromLen + 1);
	wchar_t * pointerFromWcs;
	wchar_t * pointerTo = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointerTo );

	SQLRETURN ret = SQL_SUCCESS;
	int length;
	bool fetched = from->currentFetched == parentStmt->getCurrentFetched();

	if ( !fetched )
	{ // new row read
		from->dataOffset = 0;
		from->startedReturnSQLData = false;
		from->currentFetched = parentStmt->getCurrentFetched();

		if (!to->isLocalDataPtr)
			to->allocateLocalDataPtr((from->getBufferLength() + 1) * sizeof(wchar_t));

		pointerFromWcs = (wchar_t*) to->localDataPtr;

		length = (int)from->MbsToWcs( pointerFromWcs, pointerFrom, *pointerFromLen );
		if ( length < 0 )
			length = 0;
		pointerFromWcs[length] = L'\0';
	}
	else
	{
		pointerFromWcs = (wchar_t*) to->localDataPtr;
		length = wcslen(pointerFromWcs + from->dataOffset) + from->dataOffset;
	}

	int dataRemaining = length - from->dataOffset;

	if ( !to->length )
		;
	else if (!dataRemaining && ( from->dataOffset || fetched ) && from->startedReturnSQLData)
	{
		from->dataOffset = 0;
		ret = SQL_NO_DATA;
	}
	else
	{
		from->startedReturnSQLData = true;
		int len = MIN(dataRemaining, MAX(0, (int)(to->length / sizeof( wchar_t )) - 1 ));
		 
		if ( pointerTo )
		{
			wcsncpy(pointerTo, pointerFromWcs + from->dataOffset, len);
			pointerTo[len] = L'\0';

			from->dataOffset += len;

			if ( len && len < dataRemaining )
			{
				OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
				ret = SQL_SUCCESS_WITH_INFO;
			}
		}
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(dataRemaining * sizeof( wchar_t ));
	} else
		if ( indicatorTo )
			*indicatorTo = dataRemaining * sizeof( wchar_t );

	return ret;
}

int OdbcConvert::convVarStringSystemToString(DescRecord * from, DescRecord * to)
{
	char * pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	char * pointerTo = (char*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULL( pointerTo );
	
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

	if (len && (int)len > (int)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return ret;
}

int OdbcConvert::convVarStringSystemToStringW(DescRecord * from, DescRecord * to)
{
	char *pointerFrom = (char*)getAdressBindDataFrom((char*)from->dataPtr);
	wchar_t *pointerTo = (wchar_t*)getAdressBindDataTo((char*)to->dataPtr);
	SQLLEN * indicatorTo = getAdressBindIndTo((char*)to->indicatorPtr);
	SQLLEN * indicatorFrom = getAdressBindIndFrom((char*)from->indicatorPtr);

	ODBCCONVERT_CHECKNULLW( pointerTo );
	
	SQLRETURN ret = SQL_SUCCESS;
	unsigned short lenVar = *(unsigned short*)pointerFrom;
	int len;

	char * src = pointerFrom + 2,
		 * end = src + lenVar;

	while ( lenVar-- && *(--end) == ' ');
	len = end - src + 1;
	len = MIN( len, MAX(0,(int)(to->length / sizeof( wchar_t )) - 1 ));

	if( len > 0 )
		mbstowcs( pointerTo, src, len );
	
	pointerTo[len] = (wchar_t)'\0';
	len *= sizeof( wchar_t );

	if (len && (int)len > (int)to->length)
	{
		OdbcError *error = parentStmt->postError (new OdbcError (0, "01004", "Data truncated"));
//		if (error)
//			error->setColumnNumber (column, rowNumber);
		ret = SQL_SUCCESS_WITH_INFO;
	}

	if ( to->isIndicatorSqlDa ) {
		to->headSqlVarPtr->setSqlLen(len);
	} else
	if ( indicatorTo )
		*indicatorTo = len;

	return ret;
}

signed int OdbcConvert::encode_sql_date(SQLUSMALLINT day, SQLUSMALLINT month, SQLSMALLINT year)
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
	signed int	c, ya;

	if (month > 2)
		month -= 3;
	else
	{
		month += 9;
		year -= 1;
	}

	c = year / 100;
	ya = year - 100 * c;

	return (signed int) (((QUAD) 146097 * c) / 4 + 
		(1461 * ya) / 4 + 
		(153 * month + 2) / 5 + 
		day - 678882); //	day + 1721119 - 2400001);
}

void OdbcConvert::decode_sql_date(signed int nday, SQLUSMALLINT &mday, SQLUSMALLINT &month, SQLSMALLINT &year)
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
	signed int	day;
	signed int	century;

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

signed int OdbcConvert::encode_sql_time(SQLUSMALLINT hour, SQLUSMALLINT minute, SQLUSMALLINT second)
{
	return ((hour * 60 + minute) * 60 +
				 second) * ISC_TIME_SECONDS_PRECISION;
}

void OdbcConvert::decode_sql_time(signed int ntime, SQLUSMALLINT &hour, SQLUSMALLINT &minute, SQLUSMALLINT &second)
{
	int minutes;

	minutes = ntime / (ISC_TIME_SECONDS_PRECISION * 60);
	hour = (SQLUSMALLINT)(minutes / 60);
	minute = (SQLUSMALLINT)(minutes % 60);
	second = (SQLUSMALLINT)((ntime / ISC_TIME_SECONDS_PRECISION) % 60);
}

void OdbcConvert::convertStringDateTimeToServerStringDateTime (char *& string, int &len)
{
	char * ptBeg = string;

	if ( !ptBeg || !*ptBeg )
		return;

	while( *ptBeg == ' ' ) ptBeg++;

	int offset, offsetPoint;

	if( *ptBeg != '{' )
	{
		while ( *ptBeg && !ISDIGIT( *ptBeg ) ) ptBeg++;

		if ( !ISDIGIT( *ptBeg ) )
			return;

		char * ptEnd = ptBeg + 1;

		while ( *ptEnd )
		{
			if ( *ptEnd == '.' )
			{
				int l = 5;
				while ( l-- && *++ptEnd );
				break;
			}
			ptEnd++;
		}

		len = ptEnd - ptBeg;
	}
	else
	{
		while( *++ptBeg == ' ' );
		
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

	case SQL_C_WCHAR:
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
		to->headSqlVarPtr->setTypeBoolean();
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
