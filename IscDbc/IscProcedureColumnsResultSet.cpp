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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscProcedureColumnsResultSet::IscProcedureColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

IscProcedureColumnsResultSet::~IscProcedureColumnsResultSet()
{

}

void IscProcedureColumnsResultSet::getProcedureColumns(const char * catalog, 
													   const char * schemaPattern, 
													   const char * procedureNamePattern, 
													   const char * columnNamePattern)
{
	JString sql = 
		"select NULL as procedure_cat,\n"							// 1
				"\tNULL as procedure_schem,\n"						// 2
				"\tpp.rdb$procedure_name as procedure_name,\n"		// 3
				"\tpp.rdb$parameter_name as column_name,\n"			// 4
				"\tpp.rdb$parameter_type as column_type,\n"			// 5
				"\tf.rdb$field_type as data_type,\n"				// 6
				"\tf.rdb$field_sub_type as type_name,\n"			// 7
				"\tf.rdb$field_length as column_size,\n"			// 8
				"\tnull as buffer_length,\n"						// 9
				"\tf.rdb$field_scale as decimal_digits,\n"			// 10
				"\t10 as num_prec_radix,\n"							// 11
				"\t1 as nullable,\n"								// 12 #define SQL_NULLABLE 1
//				"\tf.rdb$null_flag as nullable,\n"					// 12
				"\tf.rdb$description as remarks,\n"					// 13
				"\tf.rdb$default_value as column_def,\n"			// 14
				"\tnull as sql_data_type,\n"						// 15
				"\tnull as sql_datetime_sub,\n"						// 16
				"\tf.rdb$field_length as char_octet_length,\n"		// 17
				"\tpp.rdb$parameter_number as ordinal_position,\n"	// 18
				"\t'YES' as is_nullable,\n"							// 19
				"\tf.rdb$field_precision as column_precision\n"		// 20
		"from rdb$procedure_parameters pp, rdb$fields f\n"
		"where pp.rdb$field_source = f.rdb$field_name\n";

	if (procedureNamePattern && *procedureNamePattern)
		sql += expandPattern (" and ","pp.rdb$procedure_name", procedureNamePattern);

	if ( !metaData->allTablesAreSelectable() )
		sql += metaData->existsAccess(" and ", "pp", 5, "\n");

	if (columnNamePattern && *columnNamePattern)
		sql += expandPattern (" and ","pp.rdb$parameter_name", columnNamePattern);

	sql += " order by pp.rdb$procedure_name, pp.rdb$parameter_number";
	prepareStatement (sql);
	numberColumns = 19;
}

bool IscProcedureColumnsResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// procedure name
	trimBlanks (4);							// parameter name

	int parameterType = resultSet->getInt (5);
	int type = (parameterType) ? SQL_PARAM_INPUT : SQL_PARAM_OUTPUT;
	resultSet->setValue (5, type);

	int blrType = resultSet->getInt (6);	// field type
	int subType = resultSet->getInt (7);
	int length = resultSet->getInt (8);
	int scale = resultSet->getInt (10);
	int dialect	= resultSet->statement->connection->getDatabaseDialect();
	int precision = resultSet->getInt (20);
	IscSqlType sqlType (blrType, subType, length, length, dialect, precision, scale);

	resultSet->setValue (6, sqlType.type);
	resultSet->setValue (7, sqlType.typeName);
	resultSet->setValue (9, length);

	return true;
}

int IscProcedureColumnsResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscProcedureColumnsResultSet::getColumnType(int index, int &realSqlType)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index, realSqlType);
}

int IscProcedureColumnsResultSet::getPrecision(int index)
{
	return 31;
/*
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getPrecision (index);
*/
}
