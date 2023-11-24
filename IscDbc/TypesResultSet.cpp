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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 */

// TypesResultSet.cpp: implementation of the TypesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "IscDbc.h"
#include "TypesResultSet.h"
#include "Types.h"
#include "IscStatement.h"
#include "Value.h"

using namespace Firebird;

namespace IscDbcLibrary {
/*
#define SET_SQLVAR(index, name, type, prec, offset)			\
{															\
	XSQLVAR *var = ((XSQLDA*)*sqlda)->sqlvar + index - 1;	\
	char *src = var->sqlname;							\
	const char *dst = name;									\
	do														\
		*src++=*dst;										\
	while(*dst++);											\
															\
	var->sqlname_length = src - var->sqlname - 1;			\
	var->sqltype = type + 1;								\
	var->sqllen = prec;										\
	var->sqldata = (char*)offset;							\
}															\

#define SET_INDICATOR_VAL(col,type,isNull)  if ( isNull && (*(type*)(var[col].sqldata + sqldataOffsetPtr)) == -1 ) *var[col].sqlind = -1; else *var[col].sqlind = 0;
#define SET_INDICATOR_STR(col)  if ( ((char*)(var[col].sqldata + sqldataOffsetPtr)) == NULL || !strlen(((char*)(var[col].sqldata + sqldataOffsetPtr))) ) *var[col].sqlind = -1; else *var[col].sqlind = (short)strlen((char*)(var[col].sqldata + sqldataOffsetPtr)) + 1;
*/
struct Types {
	char	label;
	short	lenTypeName;
	char    typeName[50];
	short	typeType;
	int     typePrecision;
	short   lenTypePrefix;
	char    typePrefix[6];
	short   lenTypeSuffix;
	char    typeSuffix[6];
	short   lenTypeParams;
	char    typeParams[20];
	short	typeNullable;
	short	typeCaseSensitive;
	short	typeSearchable;
	short	typeUnsigned;
	short	typeMoney;
	short	typeAutoIncrement;
	short   lenTypeLocalName;
	char    typeLocalName[50];
	short	typeMinScale;
	short	typeMaxScale;
	short	typeSqlDataType;
	short	typeDateTimeSub;
	int     typeNumPrecRadix;
	short	typeIntervalPrecision;
    };

#define F_NO_NULLS					0		// SQL_NO_NULLS
#define F_NULLABLE					1		// SQL_NULLABLE
#define F_CASE_SENSITIVE			1		// SQL_TRUE
#define F_CASE_INSENSITIVE			0		// SQL_FALSE
#define F_IS_SIGNED					1		// SQL_TRUE
#define F_NOT_SIGNED				0		// SQL_FALSE
#define F_NOT_NUMERIC				-1
#define F_SEARCHABLE_EXCEPT_LIKE	2		// SQL_ALL_EXCEPT_LIKE
#define F_SEARCHABLE				3		// SQL_SEARCHABLE
#define F_UNSEARCHABLE				0		// SQL_UNSEARCHABLE
#define F_MONEY						1		// SQL_TRUE
#define F_NOT_MONEY					0		// SQL_FALSE
#define F_NOT_AUTO_INCR				0		// SQL_FALSE
#define F_UNSCALED					-1

#define TYPE_SQL_DATETIME	9

#define ALPHA(type,code,prec) 0,sizeof(type)-1,type,code,prec,1,"'",1,"'",6,"length",F_NULLABLE,F_CASE_SENSITIVE,F_SEARCHABLE,F_NOT_NUMERIC,F_NOT_MONEY,F_NOT_NUMERIC,sizeof(type)-1,type,F_UNSCALED,F_UNSCALED,code,F_NOT_NUMERIC,F_NOT_NUMERIC,F_NOT_NUMERIC
#define BLOB(type,code,prec,prefix,suffix,casesensitive) 0,sizeof(type)-1,type,code,prec,sizeof(prefix)-1,prefix,sizeof(suffix)-1,suffix,0,"",F_NULLABLE,casesensitive,F_UNSEARCHABLE,F_NOT_NUMERIC,F_NOT_MONEY,F_NOT_NUMERIC,sizeof(type)-1,type,F_UNSCALED,F_UNSCALED,code,F_NOT_NUMERIC,F_NOT_NUMERIC,F_NOT_NUMERIC
#define NUMERIC_TINYINT(type,code,prec,attr,min,max,numprecradix) 0,sizeof(type)-1,type,code,prec,0,"",0,"",sizeof(attr)-1,attr,F_NULLABLE,F_CASE_INSENSITIVE,F_SEARCHABLE_EXCEPT_LIKE,F_IS_SIGNED,F_NOT_MONEY,F_NOT_AUTO_INCR,25,"CHAR CHARACTER SET OCTETS",min,max,code,F_NOT_NUMERIC,numprecradix,F_NOT_NUMERIC
#define NUMERIC(type,code,prec,attr,min,max,numprecradix) 0,sizeof(type)-1,type,code,prec,0,"",0,"",sizeof(attr)-1,attr,F_NULLABLE,F_CASE_INSENSITIVE,F_SEARCHABLE_EXCEPT_LIKE,F_NOT_SIGNED,F_NOT_MONEY,F_NOT_AUTO_INCR,sizeof(type)-1,type,min,max,code,F_NOT_NUMERIC,numprecradix,F_NOT_NUMERIC
#define DATE(type,code,prec,prefix,suffix,datetimesub) 0,sizeof(type)-1,type,code,prec,sizeof(prefix)-1,prefix,sizeof(suffix)-1,suffix,0,"",F_NULLABLE,F_CASE_INSENSITIVE,F_SEARCHABLE_EXCEPT_LIKE,F_NOT_NUMERIC,F_NOT_MONEY,F_NOT_NUMERIC,sizeof(type)-1,type,F_UNSCALED,F_UNSCALED,TYPE_SQL_DATETIME,datetimesub,F_NOT_NUMERIC,F_NOT_NUMERIC
#define DATETIME(type,code,prec,prefix,suffix,datetimesub) 0,sizeof(type)-1,type,code,prec,sizeof(prefix)-1,prefix,sizeof(suffix)-1,suffix,0,"",F_NULLABLE,F_CASE_INSENSITIVE,F_SEARCHABLE_EXCEPT_LIKE,F_NOT_NUMERIC,F_NOT_MONEY,F_NOT_NUMERIC,sizeof(type)-1,type,0,4,TYPE_SQL_DATETIME,datetimesub,F_NOT_NUMERIC,F_NOT_NUMERIC

static Types types [] = 
{
	ALPHA ("CHAR", JDBC_CHAR, MAX_CHAR_LENGTH),
	ALPHA ("VARCHAR", JDBC_VARCHAR, MAX_VARCHAR_LENGTH),
	BLOB ("BLOB SUB_TYPE TEXT", JDBC_LONGVARCHAR, MAX_BLOB_LENGTH, "'","'",F_CASE_SENSITIVE),
	ALPHA ("CHAR(x) CHARACTER SET UNICODE_FSS", JDBC_WCHAR, MAX_WCHAR_LENGTH),
	ALPHA ("VARCHAR(x) CHARACTER SET UNICODE_FSS", JDBC_WVARCHAR, MAX_WVARCHAR_LENGTH),
	BLOB ("BLOB SUB_TYPE TEXT CHARACTER SET UNICODE_FSS", JDBC_WLONGVARCHAR, MAX_WLONGVARCHAR_LENGTH, "'","'",F_CASE_SENSITIVE),
	BLOB ("BLOB SUB_TYPE 0", JDBC_LONGVARBINARY, MAX_BLOB_LENGTH, "","",F_CASE_INSENSITIVE),
	BLOB ("BLOB SUB_TYPE 0", JDBC_VARBINARY, MAX_BLOB_LENGTH, "","",F_CASE_INSENSITIVE),
	BLOB ("BLOB SUB_TYPE 0", JDBC_BINARY, MAX_BLOB_LENGTH, "","",F_CASE_INSENSITIVE),
	NUMERIC ("NUMERIC", JDBC_NUMERIC, MAX_NUMERIC_LENGTH, "precision,scale", 0, MAX_NUMERIC_LENGTH, 10),
	NUMERIC ("DECIMAL", JDBC_DECIMAL, MAX_DECIMAL_LENGTH, "precision,scale", 0, MAX_DECIMAL_LENGTH, 10),
	NUMERIC ("INTEGER", JDBC_INTEGER, MAX_INT_LENGTH, "", 0, 0, 10),
	NUMERIC ("SMALLINT", JDBC_TINYINT, MAX_SMALLINT_LENGTH, "", 0, 0, 10),
	NUMERIC ("SMALLINT", JDBC_SMALLINT, MAX_SMALLINT_LENGTH, "", 0, 0, 10),	
	NUMERIC ("DOUBLE PRECISION", JDBC_FLOAT, MAX_DOUBLE_DIGIT_LENGTH, "", F_UNSCALED, F_UNSCALED, 2),
	NUMERIC ("FLOAT", JDBC_REAL, MAX_FLOAT_DIGIT_LENGTH, "", F_UNSCALED, F_UNSCALED, 2),
	NUMERIC ("DOUBLE PRECISION", JDBC_DOUBLE, MAX_DOUBLE_DIGIT_LENGTH, "", F_UNSCALED, F_UNSCALED, 2),
	NUMERIC ("BIGINT", JDBC_BIGINT, MAX_QUAD_LENGTH,"", 0, MAX_QUAD_LENGTH, 10),
	NUMERIC ("BOOLEAN", JDBC_BOOLEAN, MAX_BOOLEAN_LENGTH, "", F_UNSCALED, F_UNSCALED, F_NOT_NUMERIC),
	DATE("DATE",JDBC_DATE,MAX_DATE_LENGTH,"{d'","'}",1),
	DATETIME("TIME",JDBC_TIME,MAX_TIME_LENGTH,"{t'","'}",2),
	DATETIME("TIMESTAMP",JDBC_TIMESTAMP,MAX_TIMESTAMP_LENGTH,"{ts'","'}",3)
};

static constexpr size_t TYPES_SIZE = sizeof (types) / sizeof (types [0]);
static constexpr size_t EXPECTED_TYPES_SIZE = 22;
static_assert( TYPES_SIZE == EXPECTED_TYPES_SIZE, "TypesResultSet.cpp::types array length != 22" );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TypesResultSet::TypesResultSet(int dataType, int appOdbcVersion, int bytesPerCharacter, IscConnection* conn) :
	IscResultSet{nullptr},
	outputSqlda {conn},
	status      {conn->GDS->_status},
	typesOutputHelper{&status, conn->GDS->_master},
	localBuffer {TYPES_SIZE}
{	
	dataTypes = dataType;

	int endRow = TYPES_SIZE;

	if ( appOdbcVersion == 3 ) // SQL_OV_ODBC3
	{
		switch( dataTypes )
		{
		case JDBC_SQL_DATE:
			dataTypes = JDBC_DATE;
			break;

		case JDBC_SQL_TIME:
			dataTypes = JDBC_TIME;
			break;

		case JDBC_SQL_TIMESTAMP:
			dataTypes = JDBC_TIMESTAMP;
			break;
		}

		types[--endRow].typeType = JDBC_TIMESTAMP;
		types[--endRow].typeType = JDBC_TIME;
		types[--endRow].typeType = JDBC_DATE;
	}
	else
	{
		switch( dataTypes )
		{
		case JDBC_DATE:
			dataTypes = JDBC_SQL_DATE;
			break;

		case JDBC_TIME:
			dataTypes = JDBC_SQL_TIME;
			break;

		case JDBC_TIMESTAMP:
			dataTypes = JDBC_SQL_TIMESTAMP;
			break;
		}

		types[--endRow].typeType = JDBC_SQL_TIMESTAMP;
		types[--endRow].typeType = JDBC_SQL_TIME;
		types[--endRow].typeType = JDBC_SQL_DATE;
	}

	types[0].typePrecision = MAX_CHAR_LENGTH / bytesPerCharacter;
	types[1].typePrecision = MAX_VARCHAR_LENGTH / bytesPerCharacter;
	types[2].typePrecision = MAX_BLOB_LENGTH / bytesPerCharacter;

	sqlda = &outputSqlda;
	sqlda->allocBuffer( statement, typesOutputHelper.getMetadata() );

	auto * typePtr = types;

#define	HLP_SET_STR(FLDNAME,LEN,STR,MAX_SIZE) \
		typesOutputHelper->FLDNAME.length = typePtr->LEN; \
		strncpy( typesOutputHelper->FLDNAME.str, typePtr->STR, MAX_SIZE ); \
		typesOutputHelper->FLDNAME##Null = FB_FALSE;

#define HLP_SET_INT(FLDNAME,VAL) \
		typesOutputHelper->FLDNAME = typePtr->VAL; \
		typesOutputHelper->FLDNAME##Null = FB_FALSE;

	for( auto & v : localBuffer )
	{
		typesOutputHelper.clear();

		HLP_SET_STR( TYPE_NAME, lenTypeName, typeName, 50 );
		HLP_SET_INT( DATA_TYPE, typeType );
		HLP_SET_INT( COLUMN_SIZE, typePrecision );
		HLP_SET_STR( LITERAL_PREFIX, lenTypePrefix, typePrefix, 6 );
		HLP_SET_STR( LITERAL_SUFFIX, lenTypeSuffix, typeSuffix, 6 );
		HLP_SET_STR( CREATE_PARAMS, lenTypeParams, typeParams, 20 );
		HLP_SET_INT( NULLABLE, typeNullable );
		HLP_SET_INT( CASE_SENSITIVE, typeCaseSensitive );
		HLP_SET_INT( SEARCHABLE, typeSearchable );
		HLP_SET_INT( UNSIGNED_ATTRIBUTE, typeUnsigned );
		HLP_SET_INT( FIXED_PREC_SCALE, typeMoney );
		HLP_SET_INT( AUTO_UNIQUE_VALUE, typeAutoIncrement );
		HLP_SET_STR( LOCAL_TYPE_NAME, lenTypeLocalName, typeLocalName, 50 );
		HLP_SET_INT( MINIMUM_SCALE, typeMinScale );
		HLP_SET_INT( MAXIMUM_SCALE, typeMaxScale );
		HLP_SET_INT( SQL_DATA_TYPE, typeSqlDataType );
		HLP_SET_INT( SQL_DATETIME_SUB, typeDateTimeSub );
		HLP_SET_INT( NUM_PREC_RADIX, typeNumPrecRadix );
		HLP_SET_INT( INTERVAL_PRECISION, typeIntervalPrecision );

		++typePtr;

		auto * buf = (char *)typesOutputHelper.getData();
		auto buflen = typesOutputHelper.getMetadata()->getMessageLength(&status);
		v.assign( buf, buf + buflen );
	}
	sqlda->buffer.assign( localBuffer[0].begin(), localBuffer[0].end() );	// jump to the first row

#undef	HLP_SET_INT
#undef	HLP_SET_STR

	recordNumber = 0;
	numberColumns = sqlda->columnsCount;
	values.alloc (numberColumns);
	allocConversions();

	//dark magic here, ptr is out of bounds?..
	sqldataOffsetPtr = NULL;
/*
	indicators = (SQLLEN*)calloc( 1, sizeof(SQLLEN) * numberColumns );
	((XSQLDA*)*sqlda)->sqld = numberColumns;
	sqldataOffsetPtr = (uintptr_t)types - sizeof (*types);
	sqlda->orgsqlvar = new CAttrSqlVar [numberColumns];
	CAttrSqlVar * orgvar = sqlda->orgsqlvar;

	SET_SQLVAR( 1, "TYPE_NAME"			, SQL_VARYING	,	52  , OFFSET(Types,lenTypeName)				)
	SET_SQLVAR( 2, "DATA_TYPE"			, SQL_SHORT		,	 5	, OFFSET(Types,typeType)				)
	SET_SQLVAR( 3, "COLUMN_SIZE"		, SQL_LONG		,	10	, OFFSET(Types,typePrecision)			)
	SET_SQLVAR( 4, "LITERAL_PREFIX"		, SQL_VARYING	,	 8	, OFFSET(Types,lenTypePrefix)			)
	SET_SQLVAR( 5, "LITERAL_SUFFIX"		, SQL_VARYING	,	 8	, OFFSET(Types,lenTypeSuffix)			)
	SET_SQLVAR( 6, "CREATE_PARAMS"		, SQL_VARYING	,	22	, OFFSET(Types,lenTypeParams)			)
	SET_SQLVAR( 7, "NULLABLE"			, SQL_SHORT		,	 5	, OFFSET(Types,typeNullable)			)
	SET_SQLVAR( 8, "CASE_SENSITIVE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeCaseSensitive)		)
	SET_SQLVAR( 9, "SEARCHABLE"			, SQL_SHORT		,	 5	, OFFSET(Types,typeSearchable)			)
	SET_SQLVAR(10, "UNSIGNED_ATTRIBUTE"	, SQL_SHORT		,	 5	, OFFSET(Types,typeUnsigned)			)
	SET_SQLVAR(11, "FIXED_PREC_SCALE"	, SQL_SHORT		,	 5	, OFFSET(Types,typeMoney)				)
	SET_SQLVAR(12, "AUTO_UNIQUE_VALUE"	, SQL_SHORT		,	 5	, OFFSET(Types,typeAutoIncrement)		)
	SET_SQLVAR(13, "LOCAL_TYPE_NAME"	, SQL_VARYING	,	52	, OFFSET(Types,lenTypeLocalName)		)
	SET_SQLVAR(14, "MINIMUM_SCALE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeMinScale)			)
	SET_SQLVAR(15, "MAXIMUM_SCALE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeMaxScale)			)
	SET_SQLVAR(16, "SQL_DATA_TYPE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeSqlDataType)			)
	SET_SQLVAR(17, "SQL_DATETIME_SUB"	, SQL_SHORT		,	 5	, OFFSET(Types,typeDateTimeSub)			)
	SET_SQLVAR(18, "NUM_PREC_RADIX"		, SQL_LONG		,	10	, OFFSET(Types,typeNumPrecRadix)		)
	SET_SQLVAR(19, "INTERVAL_PRECISION"	, SQL_SHORT		,	 5	, OFFSET(Types,typeIntervalPrecision)	)

	int i = numberColumns;
	XSQLVAR *var = ((XSQLDA*)*sqlda)->sqlvar;
	SQLLEN *ind = indicators;

	while ( i-- )
	{
		*orgvar++ = var;
		(var++)->sqlind = (short*)ind++;
	}
*/
}

TypesResultSet::~TypesResultSet()
{
	/*
	free(indicators);
	*/
}

bool TypesResultSet::nextFetch()
{
	if (dataTypes != 0)
	{
		if ( recordNumber != 0 )
			return false;

		recordNumber = findType();
		if( recordNumber == -1 )
		{
			recordNumber = 1;
			return false;
		}
	}

	if (++recordNumber > TYPES_SIZE)
		return false;

	//XSQLVAR *var = ((XSQLDA*)*sqlda)->sqlvar;
	auto * var = sqlda->orgsqlvar;
	sqldataOffsetPtr = (size_t)localBuffer[ recordNumber - 1 ].data();

#define SET_INDICATOR_VAL(col,type,isNull) \
	var[col].assignBuffer( localBuffer[ recordNumber - 1 ].data() ); \
	if ( isNull && (*(type*)(var[col].sqldata)) == -1 ) *var[col].sqlind = -1;

#define SET_INDICATOR_STR(col) \
	var[col].assignBuffer( localBuffer[ recordNumber - 1 ].data() ); \
	if ( *(short*)(var[col].sqldata) == 0 ) *var[col].sqlind = -1;

	SET_INDICATOR_STR(0);						// TYPE_NAME
	SET_INDICATOR_VAL(1,short,false);			// DATA_TYPE
	SET_INDICATOR_VAL(2,int,true);				// PRECISION
	SET_INDICATOR_STR(3);						// LITERAL_PREFIX
	SET_INDICATOR_STR(4);						// LITERAL_SUFFIX
	SET_INDICATOR_STR(5);						// CREATE_PARAMS
	SET_INDICATOR_VAL(6,short,false);			// NULLABLE
	SET_INDICATOR_VAL(7,short,false);			// CASE_SENSITIVE
	SET_INDICATOR_VAL(8,short,false);			// SEARCHABLE
	SET_INDICATOR_VAL(9,short,true);			// UNSIGNED_ATTRIBUTE
	SET_INDICATOR_VAL(10,short,false);			// MONEY
	SET_INDICATOR_VAL(11,short,true);			// AUTO_INCREMENT
	SET_INDICATOR_STR(12);						// LOCAL_TYPE_NAME
	SET_INDICATOR_VAL(13,short,true);			// MINIMUM_SCALE
	SET_INDICATOR_VAL(14,short,true);			// MAXIMUM_SCALE
	SET_INDICATOR_VAL(15,short,false);			// SQL_DATA_TYPE
	SET_INDICATOR_VAL(16,short,true);			// SQL_DATETIME_SUB
	SET_INDICATOR_VAL(17,int,true);				// NUM_PREC_RADIX
	SET_INDICATOR_VAL(18,short,true);			// INTERVAL_PRECISION

#undef SET_INDICATOR_STR
#undef SET_INDICATOR_VAL

	return true;
}

bool TypesResultSet::next()
{
	deleteBlobs();
	reset();
	allocConversions();

	if ( !(activePosRowInSet >= 0 && activePosRowInSet < sqlda->getCountRowsStaticCursor()) )
		return false;

	if ( activePosRowInSet )
		copyNextSqldaFromBufferStaticCursor();

	++activePosRowInSet;

	//XSQLVAR *var = sqlda->sqlda->sqlvar;
	//auto * var = sqlda->orgsqlvar;
    Value *value = values.values;

	for (int n = 0; n < sqlda->columnsCount; ++n, ++value)
		statement->setValue (value, n + 1, *sqlda);
		//TODO: what is "statement" here?.. 

	return true;
}

int TypesResultSet::findType()
{	
	for(int i=0; i < TYPES_SIZE; i++)
		if (types[i].typeType == dataTypes)
			return i;		

	return -1;
}

}; // end namespace IscDbcLibrary
