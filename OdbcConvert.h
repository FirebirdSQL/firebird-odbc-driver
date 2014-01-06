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

// OdbcConvert.h interface for the OdbcConvert class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_OdbcConvert__INCLUDED_)
#define _OdbcConvert__INCLUDED_

#include "OdbcObject.h"

namespace OdbcJdbcLibrary {

struct shortTime
{
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char hsecond;
};

struct shortDate
{
    int		year;
    char	day;
    char	month;
};

class DescRecord;
class OdbcConvert;
class OdbcStatement;

typedef int (OdbcConvert::*ADRESS_FUNCTION)(DescRecord * from, DescRecord * to);

class OdbcConvert
{
	OdbcStatement	*parentStmt;
	bool			bIdentity;
	SQLLEN			tempBindOffsetPtr;
	SQLLEN			*bindOffsetPtrTo;
	SQLLEN			*bindOffsetPtrIndTo;
	SQLLEN			*bindOffsetPtrFrom;
	SQLLEN			*bindOffsetPtrIndFrom;

public:
	bool			statusReturnData;

private:

	signed int encode_sql_date(SQLUSMALLINT day, SQLUSMALLINT month, SQLSMALLINT year);
	void decode_sql_date(signed int nday, SQLUSMALLINT &mday, SQLUSMALLINT &month, SQLSMALLINT &year);
	signed int encode_sql_time(SQLUSMALLINT hour, SQLUSMALLINT minute, SQLUSMALLINT second);
	void decode_sql_time(signed int ntime, SQLUSMALLINT &hour, SQLUSMALLINT &minute, SQLUSMALLINT &second);
	void convertStringDateTimeToServerStringDateTime (char *& string, int &len);
	void getFirstElementFromArrayString(char * string, char *& firstChar, int &len);
	void setHeadSqlVar ( DescRecord * to );

public:

	OdbcConvert(OdbcStatement * parent);

	void setZeroColumn(DescRecord * to, int rowNumber);
	void setBindOffsetPtrTo(SQLLEN *bindOffsetPtr, SQLLEN *bindOffsetPtrInd);
	void setBindOffsetPtrFrom(SQLLEN *bindOffsetPtr, SQLLEN *bindOffsetPtrInd);
	ADRESS_FUNCTION getAdressFunction(DescRecord * from, DescRecord * to);
	inline SQLPOINTER getAdressBindDataFrom(char * pointer);
	inline SQLLEN *getAdressBindIndFrom(char * pointer);
	inline SQLPOINTER getAdressBindDataTo(char * pointer);
	inline SQLLEN *getAdressBindIndTo(char * pointer);

public:
	bool isIdentity(){ return bIdentity; }
	SQLLEN &getBindOffsetPtrTo() { return *bindOffsetPtrTo; }
	int notYetImplemented(DescRecord * from, DescRecord * to);

// Guid
	int convGuidToString(DescRecord * from, DescRecord * to);
	int convGuidToStringW(DescRecord * from, DescRecord * to);

// TinyInt
	int convTinyIntToBoolean(DescRecord * from, DescRecord * to);
	int convTinyIntToTinyInt(DescRecord * from, DescRecord * to);
	int convTinyIntToShort(DescRecord * from, DescRecord * to);
	int convTinyIntToLong(DescRecord * from, DescRecord * to);
	int convTinyIntToFloat(DescRecord * from, DescRecord * to);
	int convTinyIntToDouble(DescRecord * from, DescRecord * to);
	int convTinyIntToBigint(DescRecord * from, DescRecord * to);
	int convTinyIntToString(DescRecord * from, DescRecord * to);
	int convTinyIntToStringW(DescRecord * from, DescRecord * to);
	int convTinyIntToTagNumeric(DescRecord * from, DescRecord * to);

// Short
	int convShortToBoolean(DescRecord * from, DescRecord * to);
	int convShortToTinyInt(DescRecord * from, DescRecord * to);
	int convShortToShort(DescRecord * from, DescRecord * to);
	int convShortToLong(DescRecord * from, DescRecord * to);
	int convShortToFloat(DescRecord * from, DescRecord * to);
	int convShortToDouble(DescRecord * from, DescRecord * to);
	int convShortToBigint(DescRecord * from, DescRecord * to);
	int convShortToTinyIntWithScale(DescRecord * from, DescRecord * to);
	int convShortToShortWithScale(DescRecord * from, DescRecord * to);
	int convShortToLongWithScale(DescRecord * from, DescRecord * to);
	int convShortToFloatWithScale(DescRecord * from, DescRecord * to);
	int convShortToDoubleWithScale(DescRecord * from, DescRecord * to);
	int convShortToBigintWithScale(DescRecord * from, DescRecord * to);
	int convShortToString(DescRecord * from, DescRecord * to);
	int convShortToStringW(DescRecord * from, DescRecord * to);
	int convShortToTagNumeric(DescRecord * from, DescRecord * to);

// Long
	int convLongToBoolean(DescRecord * from, DescRecord * to);
	int convLongToTinyInt(DescRecord * from, DescRecord * to);
	int convLongToShort(DescRecord * from, DescRecord * to);
	int convLongToLong(DescRecord * from, DescRecord * to);
	int convLongToFloat(DescRecord * from, DescRecord * to);
	int convLongToDouble(DescRecord * from, DescRecord * to);
	int convLongToBigint(DescRecord * from, DescRecord * to);
	int convLongToLongWithScale(DescRecord * from, DescRecord * to);
	int convLongToTinyIntWithScale(DescRecord * from, DescRecord * to);
	int convLongToShortWithScale(DescRecord * from, DescRecord * to);
	int convLongToFloatWithScale(DescRecord * from, DescRecord * to);
	int convLongToDoubleWithScale(DescRecord * from, DescRecord * to);
	int convLongToBigintWithScale(DescRecord * from, DescRecord * to);
	int convLongToString(DescRecord * from, DescRecord * to);
	int convLongToStringW(DescRecord * from, DescRecord * to);
	int convLongToTagNumeric(DescRecord * from, DescRecord * to);

// Float
	int convFloatToBoolean(DescRecord * from, DescRecord * to);
	int convFloatToTinyInt(DescRecord * from, DescRecord * to);
	int convFloatToShort(DescRecord * from, DescRecord * to);
	int convFloatToLong(DescRecord * from, DescRecord * to);
	int convFloatToFloat(DescRecord * from, DescRecord * to);
	int convFloatToDouble(DescRecord * from, DescRecord * to);
	int convFloatToBigint(DescRecord * from, DescRecord * to);
	int convFloatToString(DescRecord * from, DescRecord * to);
	int convFloatToStringW(DescRecord * from, DescRecord * to);

// Double
	int convDoubleToBoolean(DescRecord * from, DescRecord * to);
	int convDoubleToTinyInt(DescRecord * from, DescRecord * to);
	int convDoubleToShort(DescRecord * from, DescRecord * to);
	int convDoubleToLong(DescRecord * from, DescRecord * to);
	int convDoubleToFloat(DescRecord * from, DescRecord * to);
	int convDoubleToDouble(DescRecord * from, DescRecord * to);
	int convDoubleToBigint(DescRecord * from, DescRecord * to);
	int convDoubleToString(DescRecord * from, DescRecord * to);
	int convDoubleToStringW(DescRecord * from, DescRecord * to);
	int convDoubleToTagNumeric(DescRecord * from, DescRecord * to);

// Bigint
	int convBigintToBoolean(DescRecord * from, DescRecord * to);
	int convBigintToTinyInt(DescRecord * from, DescRecord * to);
	int convBigintToShort(DescRecord * from, DescRecord * to);
	int convBigintToLong(DescRecord * from, DescRecord * to);
	int convBigintToFloat(DescRecord * from, DescRecord * to);
	int convBigintToDouble(DescRecord * from, DescRecord * to);
	int convBigintToFloatWithScale(DescRecord * from, DescRecord * to);
	int convBigintToDoubleWithScale(DescRecord * from, DescRecord * to);
	int convBigintToBigint(DescRecord * from, DescRecord * to);
	int convBigintToBinary(DescRecord * from, DescRecord * to);
	int convBigintToString(DescRecord * from, DescRecord * to);
	int convBigintToStringW(DescRecord * from, DescRecord * to);
	int convBigintToTagNumeric(DescRecord * from, DescRecord * to);

// Numeric, Decimal
	int convNumericToBoolean(DescRecord * from, DescRecord * to);
	int convNumericToTinyInt(DescRecord * from, DescRecord * to);
	int convNumericToShort(DescRecord * from, DescRecord * to);
	int convNumericToLong(DescRecord * from, DescRecord * to);
	int convNumericToFloat(DescRecord * from, DescRecord * to);
	int convNumericToDouble(DescRecord * from, DescRecord * to);
	int convNumericToBigint(DescRecord * from, DescRecord * to);
	int convNumericToTagNumeric(DescRecord * from, DescRecord * to);

// TagNumeric
	int convTagNumericToTinyInt(DescRecord * from, DescRecord * to);
	int convTagNumericToShort(DescRecord * from, DescRecord * to);
	int convTagNumericToLong(DescRecord * from, DescRecord * to);
	int convTagNumericToFloat(DescRecord * from, DescRecord * to);
	int convTagNumericToDouble(DescRecord * from, DescRecord * to);
	int convTagNumericToBigint(DescRecord * from, DescRecord * to);

// Date
	int convDateToLong(DescRecord * from, DescRecord * to);
	int convDateToFloat(DescRecord * from, DescRecord * to);
	int convDateToDouble(DescRecord * from, DescRecord * to);
	int convDateToBigint(DescRecord * from, DescRecord * to);
	int convDateToTagDate(DescRecord * from, DescRecord * to);
	int convDateToTagTimestamp(DescRecord * from, DescRecord * to);
	int convDateToString(DescRecord * from, DescRecord * to);
	int convDateToStringW(DescRecord * from, DescRecord * to);
	int convDateToBinary(DescRecord * from, DescRecord * to);

	int transferTagDateToDate(DescRecord * from, DescRecord * to);
	int transferTagDateToDateTime(DescRecord * from, DescRecord * to);

// Time
	int convTimeToLong(DescRecord * from, DescRecord * to);
	int convTimeToFloat(DescRecord * from, DescRecord * to);
	int convTimeToDouble(DescRecord * from, DescRecord * to);
	int convTimeToBigint(DescRecord * from, DescRecord * to);
	int convTimeToTagTime(DescRecord * from, DescRecord * to);
	int convTimeToTagTimestamp(DescRecord * from, DescRecord * to);
	int convTimeToString(DescRecord * from, DescRecord * to);
	int convTimeToStringW(DescRecord * from, DescRecord * to);
	int convTimeToBinary(DescRecord * from, DescRecord * to);

	int transferTagTimeToTime(DescRecord * from, DescRecord * to);
	int transferTagTimeToDateTime(DescRecord * from, DescRecord * to);

// DateTime
	int convDateTimeToDouble(DescRecord * from, DescRecord * to);
	int convDateTimeToBigint(DescRecord * from, DescRecord * to);
	int convDateTimeToTagDate(DescRecord * from, DescRecord * to);
	int convDateTimeToTagTime(DescRecord * from, DescRecord * to);
	int convDateTimeToTagDateTime(DescRecord * from, DescRecord * to);
	int convDateTimeToString(DescRecord * from, DescRecord * to);
	int convDateTimeToStringW(DescRecord * from, DescRecord * to);
	int convDateTimeToBinary(DescRecord * from, DescRecord * to);

	int transferTagDateTimeToDate(DescRecord * from, DescRecord * to);
	int transferTagDateTimeToTime(DescRecord * from, DescRecord * to);
	int transferTagDateTimeToDateTime(DescRecord * from, DescRecord * to);

// Blob
	int convBlobToTinyInt(DescRecord * from, DescRecord * to);
	int convBlobToShort(DescRecord * from, DescRecord * to);
	int convBlobToLong(DescRecord * from, DescRecord * to);
	int convBlobToFloat(DescRecord * from, DescRecord * to);
	int convBlobToDouble(DescRecord * from, DescRecord * to);
	int convBlobToBigint(DescRecord * from, DescRecord * to);
	int convBlobToBlob(DescRecord * from, DescRecord * to);
	int convBlobToBinary(DescRecord * from, DescRecord * to);
	int convBlobToString(DescRecord * from, DescRecord * to);
	int convBlobToStringW(DescRecord * from, DescRecord * to);
	int convStreamToBlob(DescRecord * from, DescRecord * to);
	int convBinaryToBlob(DescRecord * from, DescRecord * to);

// Text
	int convStringToLong(DescRecord * from, DescRecord * to);
	int convStringToTinyInt(DescRecord * from, DescRecord * to);
	int convStringToShort(DescRecord * from, DescRecord * to);
	int convStringToFloat(DescRecord * from, DescRecord * to);
	int convStringToDouble(DescRecord * from, DescRecord * to);
	int convStringToBigint(DescRecord * from, DescRecord * to);
	int convStringWToLong(DescRecord * from, DescRecord * to);
	int convStringWToTinyInt(DescRecord * from, DescRecord * to);
	int convStringWToShort(DescRecord * from, DescRecord * to);
	int convStringWToFloat(DescRecord * from, DescRecord * to);
	int convStringWToDouble(DescRecord * from, DescRecord * to);
	int convStringWToBigint(DescRecord * from, DescRecord * to);
	int convStringToString(DescRecord * from, DescRecord * to);
	int convStringToStringW(DescRecord * from, DescRecord * to);
	int convStringToVarString(DescRecord * from, DescRecord * to);
	int convStringToBlob(DescRecord * from, DescRecord * to);
	int convStringWToBlob(DescRecord * from, DescRecord * to);
	int convStringToBinary(DescRecord * from, DescRecord * to);

	int transferStringToTinyInt(DescRecord * from, DescRecord * to);
	int transferStringToDateTime(DescRecord * from, DescRecord * to);
	int transferStringWToDateTime(DescRecord * from, DescRecord * to);
	int transferStringToAllowedType(DescRecord * from, DescRecord * to);
	int transferStringWToAllowedType(DescRecord * from, DescRecord * to);
	int transferArrayStringToAllowedType(DescRecord * from, DescRecord * to);
	int transferBinaryStringToAllowedType(DescRecord * from, DescRecord * to);
	int convStreamHexStringToBlob(DescRecord * from, DescRecord * to);

// Varying
	int convVarStringToLong(DescRecord * from, DescRecord * to);
	int convVarStringToTinyInt(DescRecord * from, DescRecord * to);
	int convVarStringToShort(DescRecord * from, DescRecord * to);
	int convVarStringToFloat(DescRecord * from, DescRecord * to);
	int convVarStringToDouble(DescRecord * from, DescRecord * to);
	int convVarStringToBigint(DescRecord * from, DescRecord * to);
	int convVarStringWToLong(DescRecord * from, DescRecord * to);
	int convVarStringWToTinyInt(DescRecord * from, DescRecord * to);
	int convVarStringWToShort(DescRecord * from, DescRecord * to);
	int convVarStringWToFloat(DescRecord * from, DescRecord * to);
	int convVarStringWToDouble(DescRecord * from, DescRecord * to);
	int convVarStringWToBigint(DescRecord * from, DescRecord * to);
	int convVarStringToString(DescRecord * from, DescRecord * to);
	int convVarStringToStringW(DescRecord * from, DescRecord * to);
	int convVarStringSystemToString(DescRecord * from, DescRecord * to);
	int convVarStringSystemToStringW(DescRecord * from, DescRecord * to);
	int convVarStringToBinary(DescRecord * from, DescRecord * to);
};

}; // end namespace OdbcJdbcLibrary

#endif // !defined(_OdbcConvert__INCLUDED_)
