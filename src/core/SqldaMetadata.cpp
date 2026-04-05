#include "IscDbc.h"
#include "SQLError.h"
#include "SqldaMetadata.h"

namespace IscDbcLibrary {

#define SET_INFO_FROM_SUBTYPE( a, b, c ) \
		var->sqlsubtype == 1 || (!var->sqlsubtype && var->sqlscale) ? (a) : \
		var->sqlsubtype == 2 ? (b) : (c)

int sqlda_get_sql_type(const CAttrSqlVar* var, int& realSqlType)
{
	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return (realSqlType = JDBC_TINYINT);
		else if ( var->sqllen == 16 && var->sqlsubtype == 1 )
			return (realSqlType = JDBC_GUID);
		else if ( ( var->sqlsubtype == 3 // UNICODE_FSS
				    || var->sqlsubtype == 4 ) // UTF8
			&& !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return (realSqlType = JDBC_WCHAR);
		return (realSqlType = JDBC_CHAR);

	case SQL_VARYING:
		if ( ( var->sqlsubtype == 3 // UNICODE_FSS
				    || var->sqlsubtype == 4 ) // UTF8
			&& !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return (realSqlType = JDBC_WVARCHAR);
		return (realSqlType = JDBC_VARCHAR);

	case SQL_BOOLEAN:
		return (realSqlType = JDBC_BOOLEAN);

	case SQL_SHORT:
		realSqlType = JDBC_SMALLINT;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_LONG:
		realSqlType = JDBC_INTEGER;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_FLOAT:
		return (realSqlType = JDBC_REAL);

	case SQL_DOUBLE:
		realSqlType = JDBC_DOUBLE;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_QUAD:
		return JDBC_BIGINT;

	case SQL_INT64:
		realSqlType = JDBC_BIGINT;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_BLOB:
		if (var->sqlsubtype == 1)
			return (realSqlType = JDBC_LONGVARCHAR);
		return (realSqlType = JDBC_LONGVARBINARY);

	case SQL_TIMESTAMP:
		return (realSqlType = JDBC_TIMESTAMP);

	case SQL_TYPE_TIME:
		return (realSqlType = JDBC_TIME);

	case SQL_TYPE_DATE:
		return (realSqlType = JDBC_DATE);

	case SQL_ARRAY:
		if ( var->array->arrOctetLength < MAX_VARCHAR_LENGTH )
			return (realSqlType = JDBC_VARCHAR);
		return (realSqlType = JDBC_LONGVARCHAR);
	}

	return (realSqlType = 0);
}

const char* sqlda_get_sql_type_name(const CAttrSqlVar* var)
{
	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return "TINYINT";
		else if ( var->sqllen == 16 && var->sqlsubtype == 1 )
			return "GUID";
		return "CHAR";

	case SQL_VARYING:
		return "VARCHAR";

	case SQL_BOOLEAN:
		return "BOOLEAN";

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "SMALLINT");

	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "INTEGER");

	case SQL_FLOAT:
		return "FLOAT";

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "DOUBLE PRECISION");

	case SQL_QUAD:
		return "BIGINT";

	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "BIGINT");

	case SQL_BLOB:
		if ( var->sqlsubtype == 1 )
			return "BLOB SUB_TYPE TEXT";
		return "BLOB SUB_TYPE 0";

	case SQL_TIMESTAMP:
		return "TIMESTAMP";

	case SQL_TYPE_TIME:
		return "TIME";

	case SQL_TYPE_DATE:
		return "DATE";

	case SQL_ARRAY:
		return "ARRAY";

	default:
		NOT_YET_IMPLEMENTED;
	}

	return "*unknown type*";
}

int sqlda_get_column_display_size(const SqlProperties* var, const CAttrSqlVar* fullVar)
{
	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return MAX_TINYINT_LENGTH + 1;
		if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return var->sqllen / getCharsetSize( var->sqlsubtype );
		return var->sqllen;

	case SQL_BOOLEAN:
		return MAX_BOOLEAN_LENGTH;

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH + 2,
										MAX_DECIMAL_SHORT_LENGTH + 2,
										MAX_SMALLINT_LENGTH + 1);
		
	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH + 2,
										MAX_DECIMAL_LONG_LENGTH + 2,
										MAX_INT_LENGTH + 1);

	case SQL_FLOAT:
		return MAX_FLOAT_LENGTH + 4;			

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH + 2,
										MAX_DECIMAL_DOUBLE_LENGTH + 2,
										MAX_DOUBLE_LENGTH + 4);

	case SQL_QUAD:
	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH + 2,
										MAX_DECIMAL_LENGTH + 2,
										MAX_QUAD_LENGTH + 1);
		
	case SQL_ARRAY:
		return fullVar->array->arrOctetLength;

	case SQL_BLOB:
		return MAX_BLOB_LENGTH;

	case SQL_TYPE_TIME:
		return MAX_TIME_LENGTH;

	case SQL_TYPE_DATE:
		return MAX_DATE_LENGTH;

	case SQL_TIMESTAMP:
		return MAX_TIMESTAMP_LENGTH;
	}

	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
		return var->sqllen / getCharsetSize( var->sqlsubtype );

	return var->sqllen;
}

int sqlda_get_precision(const CAttrSqlVar* var)
{
	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return MAX_TINYINT_LENGTH;
		if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return var->sqllen / getCharsetSize( var->sqlsubtype );
		return var->sqllen;

	case SQL_BOOLEAN:
		return MAX_BOOLEAN_LENGTH;

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH,
										MAX_DECIMAL_SHORT_LENGTH,
										MAX_SMALLINT_LENGTH);

	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH,
										MAX_DECIMAL_LONG_LENGTH,
										MAX_INT_LENGTH);

	case SQL_FLOAT:
		return MAX_FLOAT_LENGTH;

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH,
										MAX_DECIMAL_DOUBLE_LENGTH,
										MAX_DOUBLE_LENGTH);

	case SQL_QUAD:
	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH,
										MAX_DECIMAL_LENGTH,
										MAX_QUAD_LENGTH);

	case SQL_ARRAY:	
		return var->array->arrOctetLength;
	
	case SQL_BLOB:		
		return MAX_BLOB_LENGTH;

	case SQL_TYPE_TIME:
		return MAX_TIME_LENGTH;

	case SQL_TYPE_DATE:
		return MAX_DATE_LENGTH;

	case SQL_TIMESTAMP:
		return MAX_TIMESTAMP_LENGTH;
	}

	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
		return var->sqllen / getCharsetSize( var->sqlsubtype );

	return var->sqllen;
}

int sqlda_get_num_prec_radix(const CAttrSqlVar* var)
{
	switch (var->sqltype)
	{
	case SQL_SHORT:
	case SQL_LONG:
	case SQL_QUAD:
	case SQL_INT64:
		return 10;
	case SQL_FLOAT:
	case SQL_DOUBLE:
	case SQL_D_FLOAT:
		return 2;
	}

	return 0;
}

#undef SET_INFO_FROM_SUBTYPE

} // end namespace IscDbcLibrary
