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

#include "IscDbc.h"
#include "TypesResultSet.h"
#include "Types.h"

namespace IscDbcLibrary {

#define SET_VALUE(col,value)	if (value == -1) setNull(col); else setValue (col, value);

struct Types {
    char    *typeName;
	long	typeType;
    long    typePrecision;
    char    *typePrefix;
    char    *typeSuffix;
    char    *typeParams;
	long	typeNullable;
	long	typeCaseSensitive;
	long	typeSearchable;
	long	typeUnsigned;
	long	typeMoney;
	long	typeAutoIncrement;
    char    *typeLocalName;
	long	typeMinScale;
	long	typeMaxScale;
	long	typeSqlDataType;
	long	typeDateTimeSub;
	long	typeNumPrecRadix;
	long	typeIntervalPrecision;
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

#define ALPHA(type,code,prec) type,code,prec,"'","'","length",NULLABLE,CASE_SENSITIVE,SEARCHABLE,NOT_NUMERIC,NOT_MONEY,NOT_NUMERIC,type,UNSCALED,UNSCALED,code,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC
#define BLOB(type,code,prefix,suffix,casesensitive) type,code,MAX_BLOB_LENGTH,prefix,suffix,NULL,NULLABLE,casesensitive,UNSEARCHABLE,NOT_NUMERIC,NOT_MONEY,NOT_NUMERIC,type,UNSCALED,UNSCALED,code,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC
#define NUMERIC(type,code,prec,attr,min,max,numprecradix) type,code,prec,"<n/a>","<n/a>",attr,NULLABLE,CASE_INSENSITIVE,SEARCHABLE_EXCEPT_LIKE,IS_SIGNED,NOT_MONEY,NOT_AUTO_INCR,type,min,max,code,NOT_NUMERIC,numprecradix,NOT_NUMERIC
#define DATETIME(type,code,prec,prefix,suffix,datetimesub) type,code,prec,prefix,suffix,NULL,NULLABLE,CASE_INSENSITIVE,SEARCHABLE_EXCEPT_LIKE,NOT_NUMERIC,NOT_MONEY,NOT_AUTO_INCR,type,UNSCALED,UNSCALED,TYPE_SQL_DATETIME,datetimesub,NOT_NUMERIC,NOT_NUMERIC

static const Types types [] = {
	BLOB ("BLOB", JDBC_LONGVARBINARY,NULL,NULL,CASE_INSENSITIVE),
	BLOB ("BLOB SUB_TYPE 1", JDBC_LONGVARCHAR,"'","'",CASE_SENSITIVE),
	ALPHA ("CHAR", JDBC_CHAR,MAX_CHAR_LENGTH),
	NUMERIC ("NUMERIC", JDBC_NUMERIC, MAX_NUMERIC_LENGTH, "precision,scale", 0, MAX_NUMERIC_LENGTH, 10),
	NUMERIC ("DECIMAL", JDBC_DECIMAL, MAX_DECIMAL_LENGTH, "precision,scale", 0, MAX_DECIMAL_LENGTH, 10),
	NUMERIC ("INTEGER", JDBC_INTEGER, MAX_INT_LENGTH, NULL, 0, 0, 10),	
	NUMERIC ("SMALLINT", JDBC_SMALLINT, MAX_SMALLINT_LENGTH, NULL, 0, 0, 10),	
	NUMERIC ("FLOAT", JDBC_REAL, MAX_FLOAT_DIGIT_LENGTH, NULL, UNSCALED, UNSCALED, 2),
	NUMERIC ("DOUBLE PRECISION", JDBC_DOUBLE, MAX_DOUBLE_DIGIT_LENGTH, NULL, UNSCALED, UNSCALED, 2),
	NUMERIC ("BIGINT", JDBC_BIGINT, MAX_QUAD_LENGTH,NULL, 0, 0, 10),
	ALPHA ("VARCHAR", JDBC_VARCHAR,MAX_VARCHAR_LENGTH),
	DATETIME("DATE",JDBC_DATE,MAX_DATE_LENGTH,"'","'",1),
	DATETIME("TIME",JDBC_TIME,MAX_TIME_LENGTH,"'","'",2),
	DATETIME("TIMESTAMP",JDBC_TIMESTAMP,MAX_TIMESTAMP_LENGTH,"'","'",3)
//    DATETIME("TIMESTAMP",TIMESTAMP,23,"{ts'","'}"),
    };


struct Fields {
   const char	*name;
   int			type;
   int			precision;
	};

#define FIELD(name,type,prec)	name, type, prec

static const Fields fields [] = {
	FIELD ("TYPE_NAME"			, JDBC_VARCHAR	, 128),
	FIELD ("DATA_TYPE"			, JDBC_SMALLINT	, 5),
	FIELD ("PRECISION"			, JDBC_INTEGER	, 10),
	FIELD ("LITERAL_PREFIX"		, JDBC_VARCHAR	, 128),
	FIELD ("LITERAL_SUFFIX"		, JDBC_VARCHAR	, 128),
	FIELD ("CREATE_PARAMS"		, JDBC_VARCHAR	, 128),
	FIELD ("NULLABLE"			, JDBC_SMALLINT	, 5),
	FIELD ("CASE_SENSITIVE"		, JDBC_SMALLINT	, 5),
	FIELD ("SEARCHABLE"			, JDBC_SMALLINT	, 5),
	FIELD ("UNSIGNED_ATTRIBUTE"	, JDBC_SMALLINT	, 5),
	FIELD ("MONEY"				, JDBC_SMALLINT	, 5),
	FIELD ("AUTO_INCREMENT"		, JDBC_SMALLINT	, 5),
	FIELD ("LOCAL_TYPE_NAME"	, JDBC_VARCHAR	, 128),
	FIELD ("MINIMUM_SCALE"		, JDBC_SMALLINT	, 5),
	FIELD ("MAXIMUM_SCALE"		, JDBC_SMALLINT	, 5),
	FIELD ("SQL_DATA_TYPE"		, JDBC_SMALLINT	, 5),
	FIELD ("SQL_DATETIME_SUB"	, JDBC_SMALLINT	, 5),
	FIELD ("NUM_PREC_RADIX"		, JDBC_INTEGER	, 10),	
	FIELD ("SQL_INTERVAL_PRECISION", JDBC_SMALLINT, 5),	
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
	numberColumns = sizeof (fields) / sizeof (fields [0]);
	values.alloc (numberColumns);
	allocConversions();
}

TypesResultSet::~TypesResultSet()
{

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

	reset();
	allocConversions();

	const Types *type = types + recordNumber - 1;
	int col = 1;

	setValue (col++, type->typeName);			// TYPE_NAME
	setValue (col++, type->typeType);			// DATA_TYPE
	setValue (col++, type->typePrecision);		// PRECISION
	setValue (col++, type->typePrefix);			// LITERAL_PREFIX
	setValue (col++, type->typeSuffix);			// LITERAL_SUFFIX
	setValue (col++, type->typeParams);			// CREATE_PARAMS
	setValue (col++, type->typeNullable);		// NULLABLE
	SET_VALUE (col++, type->typeCaseSensitive);	// CASE_SENSITIVE
	SET_VALUE (col++, type->typeSearchable);	// SEARCHABLE
	SET_VALUE (col++, type->typeUnsigned);		// UNSIGNED_ATTRIBUTE
	SET_VALUE (col++, type->typeMoney);			// MONEY
	SET_VALUE (col++, type->typeAutoIncrement); // AUTO_INCREMENT
	setValue (col++, type->typeLocalName);		// LOCAL_TYPE_NAME
	SET_VALUE (col++, type->typeMinScale);		// MINIMUM_SCALE
	SET_VALUE (col++, type->typeMaxScale);		// MAXIMUM_SCALE
	setValue (col++, type->typeSqlDataType);	// SQL_DATA_TYPE
	SET_VALUE (col++, type->typeDateTimeSub);	// SQL_DATETIME_SUB
	SET_VALUE (col++, type->typeNumPrecRadix);	// NUM_PREC_RADIX
	SET_VALUE (col++, type->typeIntervalPrecision);	// INTERVAL_PRECISION	

	return true;
}

const char* TypesResultSet::getColumnName(int index)
{
	return fields [index - 1].name;
}

const char* TypesResultSet::getColumnTypeName(int index)
{
	return fields [index - 1].name;
}

const char* TypesResultSet::getSqlTypeName(int index)
{
	return fields [index - 1].name;
}

int TypesResultSet::getColumnType(int index, int &realSqlType)
{
	return (realSqlType = fields [index - 1].type);
}

const char* TypesResultSet::getColumnLabel(int index)
{
	return fields [index - 1].name;
}

int TypesResultSet::getColumnDisplaySize(int index)
{
	return fields [index - 1].precision;
}

int TypesResultSet::getScale(int index)
{
	return 0;
}

int TypesResultSet::getPrecision(int index)
{
	return fields [index - 1].precision;
}

bool TypesResultSet::isNullable(int index)
{
	return true;
}

const char* TypesResultSet::getTableName(int index)
{
	return "";
}

int TypesResultSet::findType()
{	
	for(int i=0;i<sizeof (fields)/sizeof (fields [0]);i++)
	{
		if (types[i].typeType == dataTypes)
			return i;		
	}

	return 0;
}

}; // end namespace IscDbcLibrary
