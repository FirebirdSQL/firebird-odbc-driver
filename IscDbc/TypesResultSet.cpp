// TypesResultSet.cpp: implementation of the TypesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "TypesResultSet.h"
#include "Types.h"

#define SET_VALUE(col,value)	if (value == -1) setNull(col); else setValue (col, value);

struct Types {
	char	*typeName;
	int		typeType;
	int		typePrecision;
	char	*typePrefix;
	char	*typeSuffix;
	char	*typeParams;
	int		typeNullable;
	int		typeCaseSensitive;
	int		typeSearchable;
	int		typeUnsigned;
	int		typeMoney;
	int		typeAutoIncrement;
	//typeLocalName	char*;
	int		typeMinScale;
	int		typeMaxScale;
	};

#define NULLABLE			0		// SQL_NULLABLE
#define CASE_SENSITIVE		true
#define CASE_INSENSITIVE	false
#define IS_SIGNED			true
#define NOT_SIGNED			false
#define NOT_NUMERIC			-1
#define SEARCHABLE			3		// SQL_SEARCHABLE
#define UNSEARCHABLE		0		// SQL_UNSEARCHABLE
#define MONEY				true
#define NOT_MONEY			false
#define NOT_AUTO_INCR		false
#define UNSCALED			-1

#define ALPHA(type,code) type,code,65535,"'","'",NULL,NULLABLE,CASE_SENSITIVE,SEARCHABLE,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC,UNSCALED,UNSCALED
#define BLOB(type,code) type,code,2^31,NULL,NULL,NULL,NULLABLE,CASE_SENSITIVE,UNSEARCHABLE,NOT_NUMERIC,NOT_NUMERIC,NOT_NUMERIC,UNSCALED,UNSCALED
#define NUMERIC(type,code,prec,attr,min,max) type,code,prec,"'","'",attr,NULLABLE,CASE_INSENSITIVE,SEARCHABLE,IS_SIGNED,NOT_MONEY,NOT_AUTO_INCR,min,max

static const Types types [] = {
	ALPHA ("CHAR", jdbcCHAR),
	ALPHA ("VARCHAR", VARCHAR),
	NUMERIC ("TINYINT", TINYINT, 3, NULL, 0, 0),
	NUMERIC ("SMALLINT", SMALLINT, 5, NULL, 0, 0),
	NUMERIC ("INTEGER", INTEGER, 10, NULL, 0, 0),
	NUMERIC ("BIGINT", BIGINT, 19, NULL, 0, 0),
	NUMERIC ("FLOAT", jdbcFLOAT, 15, NULL, 0, 0),
	NUMERIC ("DOUBLE PRECISION", jdbcDOUBLE, 15, NULL, 0, 0),
	BLOB ("LONG VARCHAR", jdbcCLOB),
	BLOB ("LONG VARBINARY", jdbcBLOB)
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
	FIELD ("SQL_DATA_TYPE", VARCHAR, 128),
	FIELD ("SQL_DATETIME_SUB", SMALLINT, 5),
	FIELD ("NUM_PREC_RADIX", SMALLINT, 5),
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
	setValue (col++, type->typeName);
	setValue (col++, type->typeType);
	setValue (col++, type->typePrecision);
	setValue (col++, type->typePrefix);
	setValue (col++, type->typeSuffix);
	setValue (col++, type->typeNullable);
	SET_VALUE (col++, type->typeCaseSensitive);
	SET_VALUE (col++, type->typeSearchable);
	SET_VALUE (col++, type->typeUnsigned);
	SET_VALUE (col++, type->typeMoney);
	SET_VALUE (col++, type->typeAutoIncrement);
	setValue (col++, (const char*) NULL);		// LOCAL_TYPE_NAME
	SET_VALUE (col++, type->typeMinScale);
	SET_VALUE (col++, type->typeMaxScale);
	setValue (18, 10);							// NUM_PREC_RADIX

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
