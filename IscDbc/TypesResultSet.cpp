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

namespace IscDbcLibrary {

#define SET_SQLVAR(index, name, type, prec, offset)			\
{															\
	XSQLVAR *var = ((XSQLDA*)*sqlda)->sqlvar + index - 1;	\
	char	*src = var->sqlname,							\
			*dst = name;									\
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
#define SET_INDICATOR_STR(col)  if ( ((char*)(var[col].sqldata + sqldataOffsetPtr)) == NULL || !strlen(((char*)(var[col].sqldata + sqldataOffsetPtr))) ) *var[col].sqlind = -1; else *var[col].sqlind = strlen((char*)(var[col].sqldata + sqldataOffsetPtr)) + 1;

struct Types {
	char	label;
    short   lenTypeName;
    char    typeName[31];
	short	typeType;
    long    typePrecision;
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
    char    typeLocalName[31];
	short	typeMinScale;
	short	typeMaxScale;
	short	typeSqlDataType;
	short	typeDateTimeSub;
	long	typeNumPrecRadix;
	short	typeIntervalPrecision;
    };

#define NO_NULLS			0		// SQL_NO_NULLS
#define NULLABLE			1		// SQL_NULLABLE
#define CASE_SENSITIVE		1		// SQL_TRUE
#define CASE_INSENSITIVE	0		// SQL_FALSE
#define IS_SIGNED			1		// SQL_TRUE
#define NOT_SIGNED			0		// SQL_FALSE
#define NOT_NUMERIC			-1
#define SEARCHABLE_EXCEPT_LIKE  2	// SQL_ALL_EXCEPT_LIKE
#define SEARCHABLE			3		// SQL_SEARCHABLE
#define UNSEARCHABLE		0		// SQL_UNSEARCHABLE
#define MONEY				1		// SQL_TRUE
#define NOT_MONEY			0		// SQL_FALSE
#define NOT_AUTO_INCR		0		// SQL_FALSE
#define UNSCALED			-1

#define TYPE_SQL_DATETIME	9

#define ALPHA(type,code,prec) 0,sizeof(type)-1,type,code,prec,1,"'",1,"'",6,"length",NULLABLE,CASE_SENSITIVE,SEARCHABLE,NOT_NUMERIC,NOT_MONEY,NOT_NUMERIC,sizeof(type)-1,type,UNSCALED,UNSCALED,code,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC
#define BLOB(type,code,prefix,suffix,casesensitive) 0,sizeof(type)-1,type,code,MAX_BLOB_LENGTH,sizeof(prefix)-1,prefix,sizeof(suffix)-1,suffix,0,"",NULLABLE,casesensitive,UNSEARCHABLE,NOT_NUMERIC,NOT_MONEY,NOT_NUMERIC,sizeof(type)-1,type,UNSCALED,UNSCALED,code,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC
//#define NUMERIC_TINYINT(type,code,prec,attr,min,max,numprecradix) 0,sizeof(type)-1,type,code,prec,5,"<n/a>",5,"<n/a>",sizeof(attr)-1,attr,NULLABLE,CASE_INSENSITIVE,SEARCHABLE,NOT_SIGNED,NOT_MONEY,NOT_AUTO_INCR,4,"CHAR",min,max,code,NOT_NUMERIC,numprecradix,NOT_NUMERIC
#define NUMERIC(type,code,prec,attr,min,max,numprecradix) 0,sizeof(type)-1,type,code,prec,0,"",0,"",sizeof(attr)-1,attr,NULLABLE,CASE_INSENSITIVE,SEARCHABLE_EXCEPT_LIKE,NOT_SIGNED,NOT_MONEY,NOT_AUTO_INCR,sizeof(type)-1,type,min,max,code,NOT_NUMERIC,numprecradix,NOT_NUMERIC
#define DATETIME(type,code,prec,prefix,suffix,datetimesub) 0,sizeof(type)-1,type,code,prec,sizeof(prefix)-1,prefix,sizeof(suffix)-1,suffix,0,"",NULLABLE,CASE_INSENSITIVE,SEARCHABLE_EXCEPT_LIKE,NOT_NUMERIC,NOT_MONEY,NOT_NUMERIC,sizeof(type)-1,type,UNSCALED,UNSCALED,TYPE_SQL_DATETIME,datetimesub,NOT_NUMERIC,NOT_NUMERIC

static Types types [] = {
	BLOB ("BLOB", JDBC_LONGVARBINARY,"","",CASE_INSENSITIVE),
	BLOB ("BLOB SUB_TYPE 1", JDBC_LONGVARCHAR,"'","'",CASE_SENSITIVE),
	ALPHA ("CHAR", JDBC_CHAR,MAX_CHAR_LENGTH),
	NUMERIC ("NUMERIC", JDBC_NUMERIC, MAX_NUMERIC_LENGTH, "precision,scale", 0, MAX_NUMERIC_LENGTH, 10),
	NUMERIC ("DECIMAL", JDBC_DECIMAL, MAX_DECIMAL_LENGTH, "precision,scale", 0, MAX_DECIMAL_LENGTH, 10),
	NUMERIC ("INTEGER", JDBC_INTEGER, MAX_INT_LENGTH, "", 0, 0, 10),	
//	NUMERIC_TINYINT ("TINYINT", JDBC_TINYINT, 3, NULL, 0, 0, 10),
	NUMERIC ("SMALLINT", JDBC_SMALLINT, MAX_SMALLINT_LENGTH, "", 0, 0, 10),	
	NUMERIC ("FLOAT", JDBC_REAL, MAX_FLOAT_DIGIT_LENGTH, "", UNSCALED, UNSCALED, 2),
	NUMERIC ("DOUBLE PRECISION", JDBC_DOUBLE, MAX_DOUBLE_DIGIT_LENGTH, "", UNSCALED, UNSCALED, 2),
	NUMERIC ("BIGINT", JDBC_BIGINT, MAX_QUAD_LENGTH,"", 0, MAX_QUAD_LENGTH, 10),
	ALPHA ("VARCHAR", JDBC_VARCHAR,MAX_VARCHAR_LENGTH),
	DATETIME("DATE",JDBC_DATE,MAX_DATE_LENGTH,"'","'",1),
	DATETIME("TIME",JDBC_TIME,MAX_TIME_LENGTH,"'","'",2),
	DATETIME("TIMESTAMP",JDBC_TIMESTAMP,MAX_TIMESTAMP_LENGTH,"'","'",3)
//    DATETIME("TIMESTAMP",TIMESTAMP,23,"{ts'","'}"),
    };

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TypesResultSet::TypesResultSet(int dataType) : IscResultSet (NULL)
{	
	dataTypes = dataType;

	switch( dataTypes )
	{
		case 9:
			dataTypes = JDBC_DATE;
			break;

		case 10:
			dataTypes = JDBC_TIME;
			break;

		case 11:
			dataTypes = JDBC_TIMESTAMP;
			break;
	}

	recordNumber = 0;
	numberColumns = 19;
	values.alloc (numberColumns);
	allocConversions();

	indicators = (short*)calloc(1,sizeof(short)*numberColumns);
	sqlda = &outputSqlda;
	((XSQLDA*)*sqlda)->sqld = numberColumns;
	sqldataOffsetPtr = (unsigned long)types - sizeof (*types);
	sqlda->orgsqlvar = (ORGSQLVAR *)malloc ( numberColumns * sizeof(ORGSQLVAR) );
	ORGSQLVAR * orgvar = sqlda->orgsqlvar;

	SET_SQLVAR( 1, "TYPE_NAME"			, SQL_VARYING	,	33  , OFFSET(Types,lenTypeName)				)
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
	SET_SQLVAR(13, "LOCAL_TYPE_NAME"	, SQL_VARYING	,	33	, OFFSET(Types,lenTypeLocalName)		)
	SET_SQLVAR(14, "MINIMUM_SCALE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeMinScale)			)
	SET_SQLVAR(15, "MAXIMUM_SCALE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeMaxScale)			)
	SET_SQLVAR(16, "SQL_DATA_TYPE"		, SQL_SHORT		,	 5	, OFFSET(Types,typeSqlDataType)			)
	SET_SQLVAR(17, "SQL_DATETIME_SUB"	, SQL_SHORT		,	 5	, OFFSET(Types,typeDateTimeSub)			)
	SET_SQLVAR(18, "NUM_PREC_RADIX"		, SQL_LONG		,	10	, OFFSET(Types,typeNumPrecRadix)		)
	SET_SQLVAR(19, "INTERVAL_PRECISION"	, SQL_SHORT		,	 5	, OFFSET(Types,typeIntervalPrecision)	)

	int i = numberColumns;
	XSQLVAR *var = ((XSQLDA*)*sqlda)->sqlvar;
	short *ind = indicators;

	for( ; i-- ; ++var, ++orgvar, ++ind )
	{
		*(QUAD*)orgvar = *(QUAD*)var;
		var->sqlind = ind;
	}
}

TypesResultSet::~TypesResultSet()
{
	free(indicators);
}

bool TypesResultSet::next()
{
	if (dataTypes != 0)
	{
		if ( recordNumber != 0 )
			return false;

		recordNumber = findType();
		if( recordNumber == 0 )
		{
			recordNumber = 1;
			return false;
		}
	}

	if (++recordNumber > sizeof (types) / sizeof (types [0]))
		return false;

	XSQLVAR *var = ((XSQLDA*)*sqlda)->sqlvar;
	sqldataOffsetPtr += sizeof (*types);

	SET_INDICATOR_STR(0);						// TYPE_NAME
	SET_INDICATOR_VAL(1,short,false);			// DATA_TYPE
	SET_INDICATOR_VAL(2,long,true);				// PRECISION
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
	SET_INDICATOR_VAL(17,long,true);			// NUM_PREC_RADIX
	SET_INDICATOR_VAL(18,short,true);			// INTERVAL_PRECISION	

	return true;
}

int TypesResultSet::findType()
{	
	for(int i=0; i<sizeof (types)/sizeof (types [0]) ; i++)
		if (types[i].typeType == dataTypes)
			return i;		

	return 0;
}

}; // end namespace IscDbcLibrary
