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
 *
 *	Changes
 *
 *	2002-11-24	IscProcedureColumnsResultSet.cpp
 *				Contributed by C. G. Alvarez
 *				Improve handling of NUMERIC and DECIMAL fields
 *
 *
 *	2002-07-02	IscProcedureColumnsResultSet.cpp
 *				Contributed by C. G. Alvarez
 *				Fixed invalid table alias in typos in 
 *				getProcedureColumns()
 *
 *
 *	2002-05-20	IscProcedureColumnsResultSet.cpp
 *
 *				Contributed by C. G. Alvarez
 *				o qualify the column names in getProcedureColumns()
 *				
 *
 */

// IscProcedureColumnsResultSet.cpp: implementation of the IscProcedureColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscProcedureColumnsResultSet.h"
#include "IscSqlType.h"
#include "IscStatement.h"
#include "IscConnection.h"
#include "IscPreparedStatement.h"
#include "IscDatabaseMetaData.h"

#ifndef SQL_PARAM_INPUT
#define SQL_PARAM_TYPE_UNKNOWN           0
#define SQL_PARAM_INPUT                  1
#define SQL_PARAM_INPUT_OUTPUT           2
#define SQL_RESULT_COL                   3
#define SQL_PARAM_OUTPUT                 4
#define SQL_RETURN_VALUE                 5
#endif

#define TYPE_NAME	7

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscProcedureColumnsResultSet::IscProcedureColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

void IscProcedureColumnsResultSet::getProcedureColumns(const char * catalog, 
													   const char * schemaPattern, 
													   const char * procedureNamePattern, 
													   const char * columnNamePattern)
{
	JString sql = 
		"select cast (NULL as varchar(7)) as procedure_cat,\n"		// 1
				"\tcast (NULL as varchar(7)) as procedure_schem,\n"	// 2
				"\tcast (pp.rdb$procedure_name as varchar(31)) as procedure_name,\n"	// 3
				"\tcast (pp.rdb$parameter_name as varchar(31)) as column_name,\n"		// 4
				"\tpp.rdb$parameter_type as column_type,\n"			// 5
				"\tf.rdb$field_type as data_type,\n"				// 6
				"\tcast (pp.rdb$procedure_name as varchar(31)) as type_name,\n"			// 7
				"\tcast ( f.rdb$field_length as integer ) as column_size,\n"			// 8
				"\tcast ( null as integer ) as buffer_length,\n"	// 9
				"\tcast ( f.rdb$field_scale as smallint) as decimal_digits,\n"			// 10
				"\tcast ( 10 as smallint) as num_prec_radix,\n"		// 11
				"\tcast ( 1 as smallint) as nullable,\n"			// 12 #define SQL_NULLABLE 1
				"\tcast (f.rdb$description as varchar(256)) as remarks,\n"				// 13
				"\tcast (f.rdb$default_value as varchar(512)) as column_def,\n"			// 14
				"\tf.rdb$field_type as sql_data_type,\n"			// 15 - SMALLINT NOT NULL
				"\tf.rdb$field_sub_type as sql_datetime_sub,\n"		// 16 - SMALLINT
				"\tcast ( f.rdb$field_length as integer ) as char_octet_length,\n"		// 17
				"\tcast ( pp.rdb$parameter_number + 1 as integer) as ordinal_position,\n"// 18
				"\tcast ('YES' as varchar(3)) as is_nullable,\n"	// 19
				"\tf.rdb$field_precision as column_precision\n"		// 20
		"from rdb$procedure_parameters pp, rdb$fields f\n"
		"where pp.rdb$field_source = f.rdb$field_name\n";

	if (procedureNamePattern && *procedureNamePattern)
		sql += expandPattern (" and ","pp.rdb$procedure_name", procedureNamePattern);

	if ( !metaData->allTablesAreSelectable() )
		sql += metaData->existsAccess(" and ", "pp", 5, "\n");

	if (columnNamePattern && *columnNamePattern)
		sql += expandPattern (" and ","pp.rdb$parameter_name", columnNamePattern);

	sql += " order by pp.rdb$procedure_name, pp.rdb$parameter_type, pp.rdb$parameter_number";
	prepareStatement (sql);
	numberColumns = 19;
}

bool IscProcedureColumnsResultSet::next()
{
	if (!IscResultSet::next())
		return false;

	int parameterType = sqlda->getShort (5);
	int type = parameterType ? SQL_PARAM_OUTPUT : SQL_PARAM_INPUT;
	sqlda->updateShort (5, type);

	int blrType = sqlda->getShort (6);	// field type
	int subType = sqlda->getShort (16);
	int length = sqlda->getInt (8);
	int scale = sqlda->getShort (10);
	int dialect	= statement->connection->getDatabaseDialect();
	int precision = sqlda->getShort (20);
	IscSqlType sqlType (blrType, subType, length, length, dialect, precision, scale);

	sqlda->updateShort (6, sqlType.type);
	sqlda->updateVarying (7, sqlType.typeName);

	if (sqlType.type != JDBC_VARCHAR &&	sqlType.type != JDBC_CHAR)
		sqlda->updateInt (9, sqlType.bufferLength);
	else
		sqlda->updateInt (9, length);

	switch (sqlType.type)
	{
	case JDBC_NUMERIC:
	case JDBC_DECIMAL:
		sqlda->updateShort ( 10, -scale );
	}
	
	adjustResults (sqlType);

	return true;
}

void IscProcedureColumnsResultSet::adjustResults (IscSqlType &sqlType)
{
	// decimal digits have no meaning for some columns
	// radix - doesn't mean much for some colums either
	switch (sqlType.type)
	{
	case JDBC_CHAR:
	case JDBC_VARCHAR:
	case JDBC_LONGVARCHAR:
	case JDBC_LONGVARBINARY:
	case JDBC_DATE:
	case JDBC_SQL_DATE:
		sqlda->setNull (10);
		sqlda->setNull (11);
		break;
	case JDBC_REAL:
	case JDBC_DOUBLE:
		sqlda->updateShort (11, 2);
		break;
	case JDBC_TIME:
	case JDBC_SQL_TIME:
	case JDBC_TIMESTAMP:
	case JDBC_SQL_TIMESTAMP:
		sqlda->updateShort (10, -ISC_TIME_SECONDS_PRECISION_SCALE);
	default:
		sqlda->updateShort (11, 10);
	}	

	switch (sqlType.type)
	{
	case JDBC_DATE:
	case JDBC_SQL_DATE:
		sqlda->updateShort (15, 9);
		sqlda->updateShort (16, 1);
		break;
	case JDBC_TIME:
	case JDBC_SQL_TIME:
		sqlda->updateShort (15, 9);
		sqlda->updateShort (16, 2);
		break;
	case JDBC_TIMESTAMP:
	case JDBC_SQL_TIMESTAMP:
		sqlda->updateShort (15, 9);
		sqlda->updateShort (16, 3);
		break;
	default:
		sqlda->updateShort (15, sqlda->getShort(6));
		sqlda->setNull (16);
	}

	//Octet length
	switch (sqlType.type)
	{
	case JDBC_LONGVARCHAR:
	case JDBC_LONGVARBINARY:
	case JDBC_VARCHAR:
	case JDBC_CHAR:
		sqlda->updateInt (17, sqlda->getInt (8));
		break;
	default:
		sqlda->setNull (17);
	} 
}

}; // end namespace IscDbcLibrary
