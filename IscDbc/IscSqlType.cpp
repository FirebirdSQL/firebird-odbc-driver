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
 */

// IscSqlType.cpp: implementation of the IscSqlType class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscSqlType.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscSqlType::IscSqlType(int blrType, int subType, int length, int dialect)
{
	getType (blrType, subType, length, dialect);
}

IscSqlType::~IscSqlType()
{

}

void IscSqlType::getType(int blrType, int subType, int len, int dialect)
{
	length = len;

	switch (blrType)
		{
		case blr_text:
		case blr_text2:
			type = JDBC_CHAR;
			typeName = "CHAR";
			break;

		case blr_short:
			type = JDBC_SMALLINT;
			typeName = "SMALLINT";
			length = 6;
			break;

		case blr_long:
			type = JDBC_INTEGER;
			typeName = "INTEGER";
			length = 10;
			break;

		case blr_quad:
		case blr_int64:
			type = JDBC_BIGINT;
			typeName = "BIGINT";
			length = 19;
			break;

		case blr_float:
			type = JDBC_REAL;
			typeName = "REAL";
			length = 14;
			break;

		case blr_double:
		case blr_d_float:
			type = JDBC_DOUBLE;
			typeName = "DOUBLE PRECISION";
			length = 22;
			break;

		case blr_timestamp:
			type = JDBC_TIMESTAMP;
			typeName = "TIMESTAMP";
			length = 19;
			break;

		case blr_varying:
		case blr_varying2:
			type = JDBC_VARCHAR;
			typeName = "VARCHAR";
			break;


		case blr_blob:
			if (subType == 1)
				{
				type = JDBC_LONGVARCHAR;
				typeName = "LONG VARCHAR";
				}
			else
				{
				type = JDBC_LONGVARBINARY;
				typeName = "LONG VARBINARY";
				}
			break;

		case blr_sql_date:
			type = JDBC_DATE;
			typeName = "DATE";
			length = 10;
			break;

		case blr_sql_time:
			type = JDBC_TIME;
			typeName = "TIME";
			length = 8;
			break;

		default:
			typeName = "UNKNOWN";
			type = 0;
		}
	if (type == JDBC_SMALLINT || type ==JDBC_INTEGER || type ==JDBC_BIGINT)
		if (subType == 1)
			{
			type = JDBC_NUMERIC;
			typeName = "NUMERIC";
			}
		else if (subType == 2)
			{
			type = JDBC_DECIMAL;
			typeName = "DECIMAL";
			}
}
