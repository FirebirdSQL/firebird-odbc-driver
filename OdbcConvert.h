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

// OdbcConvert.h interface for the OdbcConvert class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_OdbcConvert__INCLUDED_)
#define _OdbcConvert__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class DescRecord;
class OdbcConvert;

typedef int (OdbcConvert::*ADRESS_FUNCTION)(DescRecord * from, DescRecord * to);

class OdbcConvert
{
	bool			bIdentity;
	SQLINTEGER		**bindOffsetPtr;

private:

	signed long encode_sql_date(SQLUSMALLINT day, SQLUSMALLINT month, SQLSMALLINT year);
	void decode_sql_date(signed long nday, SQLUSMALLINT &mday, SQLUSMALLINT &month, SQLSMALLINT &year);
	signed long encode_sql_time(SQLUSMALLINT hour, SQLUSMALLINT minute, SQLUSMALLINT second);
	void decode_sql_time(signed long ntime, SQLUSMALLINT &hour, SQLUSMALLINT &minute, SQLUSMALLINT &second);

public:

	OdbcConvert();
	void setBindOffsetPtr(SQLINTEGER **ptBindOffsetPtr);
	ADRESS_FUNCTION getAdresFunction(DescRecord * from, DescRecord * to);
	inline SQLPOINTER getAdressData(char * pointer);

public:
	bool isIdentity(){ return bIdentity; }

// Short
	int convShortToShort(DescRecord * from, DescRecord * to);
	int convShortToLong(DescRecord * from, DescRecord * to);
	int convShortToFloat(DescRecord * from, DescRecord * to);
	int convShortToDouble(DescRecord * from, DescRecord * to);
	int convShortToBigint(DescRecord * from, DescRecord * to);
	int convShortToString(DescRecord * from, DescRecord * to);
	int convShortToTagNumeric(DescRecord * from, DescRecord * to);

// Long
	int convLongToLong(DescRecord * from, DescRecord * to);
	int convLongToShort(DescRecord * from, DescRecord * to);
	int convLongToFloat(DescRecord * from, DescRecord * to);
	int convLongToDouble(DescRecord * from, DescRecord * to);
	int convLongToBigint(DescRecord * from, DescRecord * to);
	int convLongToString(DescRecord * from, DescRecord * to);
	int convLongToTagNumeric(DescRecord * from, DescRecord * to);

// Float
	int convFloatToLong(DescRecord * from, DescRecord * to);
	int convFloatToShort(DescRecord * from, DescRecord * to);
	int convFloatToFloat(DescRecord * from, DescRecord * to);
	int convFloatToDouble(DescRecord * from, DescRecord * to);
	int convFloatToBigint(DescRecord * from, DescRecord * to);
	int convFloatToString(DescRecord * from, DescRecord * to);

// Double
	int convDoubleToLong(DescRecord * from, DescRecord * to);
	int convDoubleToShort(DescRecord * from, DescRecord * to);
	int convDoubleToFloat(DescRecord * from, DescRecord * to);
	int convDoubleToDouble(DescRecord * from, DescRecord * to);
	int convDoubleToBigint(DescRecord * from, DescRecord * to);
	int convDoubleToString(DescRecord * from, DescRecord * to);
	int convDoubleToTagNumeric(DescRecord * from, DescRecord * to);

// Bigint
	int convBigintToLong(DescRecord * from, DescRecord * to);
	int convBigintToShort(DescRecord * from, DescRecord * to);
	int convBigintToFloat(DescRecord * from, DescRecord * to);
	int convBigintToDouble(DescRecord * from, DescRecord * to);
	int convBigintToBigint(DescRecord * from, DescRecord * to);
	int convBigintToString(DescRecord * from, DescRecord * to);
	int convBigintToTagNumeric(DescRecord * from, DescRecord * to);

// Numeric, Decimal
	int convNumericToLong(DescRecord * from, DescRecord * to);
	int convNumericToShort(DescRecord * from, DescRecord * to);
	int convNumericToFloat(DescRecord * from, DescRecord * to);
	int convNumericToDouble(DescRecord * from, DescRecord * to);
	int convNumericToBigint(DescRecord * from, DescRecord * to);
	int convNumericToTagNumeric(DescRecord * from, DescRecord * to);

// Date
	int convDateToLong(DescRecord * from, DescRecord * to);
	int convDateToFloat(DescRecord * from, DescRecord * to);
	int convDateToDouble(DescRecord * from, DescRecord * to);
	int convDateToBigint(DescRecord * from, DescRecord * to);
	int convDateToTagDate(DescRecord * from, DescRecord * to);
	int convDateToString(DescRecord * from, DescRecord * to);

// Time
	int convTimeToLong(DescRecord * from, DescRecord * to);
	int convTimeToFloat(DescRecord * from, DescRecord * to);
	int convTimeToDouble(DescRecord * from, DescRecord * to);
	int convTimeToBigint(DescRecord * from, DescRecord * to);
	int convTimeToTagTime(DescRecord * from, DescRecord * to);
	int convTimeToString(DescRecord * from, DescRecord * to);

// DateTime
	int convDateTimeToDouble(DescRecord * from, DescRecord * to);
	int convDateTimeToBigint(DescRecord * from, DescRecord * to);
	int convDateTimeToTagDateTime(DescRecord * from, DescRecord * to);
	int convDateTimeToString(DescRecord * from, DescRecord * to);

// Blob
	int convBlobToLong(DescRecord * from, DescRecord * to);
	int convBlobToFloat(DescRecord * from, DescRecord * to);
	int convBlobToDouble(DescRecord * from, DescRecord * to);
	int convBlobToBigint(DescRecord * from, DescRecord * to);
	int convBlobToBlob(DescRecord * from, DescRecord * to);
	int convBlobToString(DescRecord * from, DescRecord * to);

// String
	int convStringToLong(DescRecord * from, DescRecord * to);
	int convStringToShort(DescRecord * from, DescRecord * to);
	int convStringToFloat(DescRecord * from, DescRecord * to);
	int convStringToDouble(DescRecord * from, DescRecord * to);
	int convStringToBigint(DescRecord * from, DescRecord * to);
	int convStringToString(DescRecord * from, DescRecord * to);
	int convVarStringToString(DescRecord * from, DescRecord * to);
};

#endif // !defined(_OdbcConvert__INCLUDED_)
