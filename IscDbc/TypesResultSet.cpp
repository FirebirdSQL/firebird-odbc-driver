// TypesResultSet.cpp: implementation of the TypesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "TypesResultSet.h"
#include "Types.h"

#define SET_VALUE(col,value)	if (value == -1) setNull(col); else setValue (col, value);

struct Types {
    char    *typeName;
    int    typeType;
    long    typePrecision;
    char    *typePrefix;
    char    *typeSuffix;
    char    *typeParams;
    int    typeNullable;
    int    typeCaseSensitive;
    int    typeSearchable;
    int    typeUnsigned;
    int    typeMoney;
    int    typeAutoIncrement;
    char    *typeLocalName;
    int    typeMinScale;
    int    typeMaxScale;
    };

#define NO_NULLS			0		// SQL_NO_NULLS
#define NULLABLE			1		// SQL_NULLABLE
#define CASE_SENSITIVE		true
#define CASE_INSENSITIVE	false
#define IS_SIGNED			true
#define NOT_SIGNED			false
#define NOT_NUMERIC			-1
#define SEARCHABLE_EXCEPT_LIKE  2	// SQL_ALL_EXCEPT_LIKE
#define SEARCHABLE			3		// SQL_SEARCHABLE
#define UNSEARCHABLE		0		// SQL_UNSEARCHABLE
#define MONEY				true
#define NOT_MONEY			false
#define NOT_AUTO_INCR		false
#define UNSCALED			-1

#define ALPHA(type,code,prec) type,code,prec,"'","'","length",NULLABLE,CASE_SENSITIVE,SEARCHABLE,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC,type,UNSCALED,UNSCALED
#define BLOB(type,code,prefix,suffix) type,code,2147483647,prefix,suffix,NULL,NULLABLE,CASE_SENSITIVE,UNSEARCHABLE,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC,type,UNSCALED,UNSCALED
#define NUMERIC(type,code,prec,attr,min,max) type,code,prec,NULL,NULL,attr,NULLABLE,CASE_INSENSITIVE,SEARCHABLE_EXCEPT_LIKE,IS_SIGNED,NOT_MONEY,NOT_AUTO_INCR,type,min,max
#define DATETIME(type,code,prec,prefix,suffix) type,code,prec,prefix,suffix,NULL,NULLABLE,CASE_INSENSITIVE,SEARCHABLE_EXCEPT_LIKE,NOT_NUMERIC,NOT_NUMERIC,NOT_AUTO_INCR,type,NULL,NULL

static const Types types [] = {
    ALPHA ("CHAR", jdbcCHAR,32767),
    ALPHA ("VARCHAR", VARCHAR,32765),
    NUMERIC ("NUMERIC", TYPE_SQL_NUMERIC, 18, "precision,scale", 0, 18),
    NUMERIC ("DECIMAL", TYPE_SQL_DECIMAL, 18, "precision,scale", 0, 18),
    NUMERIC ("SMALLINT", SMALLINT, 5, NULL, 0, 0),
    NUMERIC ("INTEGER", INTEGER, 10, NULL, 0, 0),
    NUMERIC ("FLOAT", jdbcFLOAT, 15, NULL, 0, 0),
    NUMERIC ("DOUBLE PRECISION", jdbcDOUBLE, 15, NULL, 0, 0),
    BLOB ("LONG VARCHAR", TYPE_SQL_LONGVARCHAR,"'","'"),
    BLOB ("LONG VARBINARY", TYPE_SQL_LONGVARBINARY,NULL,NULL),
    DATETIME("DATE",jdbcDATE,10,"{d'","'}"),
    DATETIME("TIME",TIME,8,"{t'","'}"),
    DATETIME("TIMESTAMP",TIMESTAMP,23,"{ts'","'}"),
    };


struct Fields {
   const char	*name;
   int			type;
   int			precision;
	};

#define FIELD(name,type,prec)	name, type, prec

static const Fields fields [] = {
	FIELD ("TYPE_NAME", VARCHAR, 128),
	FIELD ("DATA_TYPE", SMALLINT, 5),
	FIELD ("PRECISION", INTEGER, 10),
	FIELD ("LITERAL_PREFIX", VARCHAR, 128),
	FIELD ("LITERAL_SUFFIX", VARCHAR, 128),
	FIELD ("CREATE_PARAMS", VARCHAR, 128),
	FIELD ("NULLABLE", SMALLINT, 5),
	FIELD ("CASE_SENSITIVE", SMALLINT, 5),
	FIELD ("SEARCHABLE", SMALLINT, 5),
	FIELD ("UNSIGNED_ATTRIBUTE", SMALLINT, 5),
	FIELD ("MONEY", SMALLINT, 5),
	FIELD ("AUTO_INCREMENT", SMALLINT, 5),
	FIELD ("LOCAL_TYPE_NAME", VARCHAR, 128),
	FIELD ("MINIMUM_SCALE", SMALLINT, 5),
	FIELD ("MAXIMUM_SCALE", SMALLINT, 5),
	FIELD ("SQL_DATA_TYPE", SMALLINT, 5),
	FIELD ("SQL_DATETIME_SUB", SMALLINT, 5),
	FIELD ("NUM_PREC_RADIX", INTEGER, 10),
	FIELD ("SQL_INTERVAL_PRECISION", SMALLINT, 5),
	};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TypesResultSet::TypesResultSet() : IscResultSet (NULL)
{
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
	if (++recordNumber > sizeof (types) / sizeof (types [0]))
		return false;

	//deleteBlobs();
	reset();
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
	setValue (col++, type->typeType);			// SQL_DATA_TYPE
	setValue (col++, 0L);						// SQL_DATETIME_SUB
	setValue (col++, 10);						// NUM_PREC_RADIX
	setValue (col++, 0L);						// INTERVAL_PRECISION

	return true;
}

const char* TypesResultSet::getColumnName(int index)
{
	return fields [index - 1].name;
}

int TypesResultSet::getColumnType(int index)
{
	return fields [index - 1].type;
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
