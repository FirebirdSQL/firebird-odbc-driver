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
 *  2002-11-22  IscSqlType.cpp
 *              Contributed by C G Alvarez
 *				Amended DATE/TIME datatypes 
 *				from JDBC_***** to JDBC_SQL_*****
 *
 *  2002-10-11  IscSqlType.cpp
 *              Contributed by C G Alvarez
 *              Extensive modifications to the getType()
 *              that take advantage of the new MAX_***** 
 *              constants in IscDbc.h
 *
 */

// IscSqlType.cpp: implementation of the IscSqlType class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscSqlType.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscSqlType::IscSqlType(int blrType, int subType, int length, int bufferLen, int dialect, int precision, int scale)
{
	getType (blrType, subType, length, bufferLen, dialect, precision, scale);
}

IscSqlType::~IscSqlType()
{

}

void IscSqlType::getType(int blrType, int subType, int len, int bufferLen, int dialect, int precision, int scale)
{
	length = len;
	bufferLength = bufferLen;

	switch (blrType)
		{
		case blr_text:
		case blr_text2:
			{
			type = JDBC_CHAR;
			typeName = "CHAR";
			bufferLength = length; 
			}
			break;

		case blr_short:
			{
			type = JDBC_SMALLINT;
			typeName = "SMALLINT";
			length = MAX_SMALLINT_LENGTH;
			bufferLength = sizeof(short);
			}
			break;

		case blr_long:
			{
			type = JDBC_INTEGER;
			typeName = "INTEGER";
			length = MAX_INT_LENGTH;
			bufferLength = sizeof(long);
			}
			break;

		case blr_quad:
		case blr_int64:
			{
			type = JDBC_BIGINT;
			typeName = "BIGINT";
			length = MAX_QUAD_LENGTH;
			bufferLength = MAX_QUAD_LENGTH + 2;
			}
			break;

		case blr_float:
			{
			type = JDBC_REAL;
			typeName = "REAL";
			length = MAX_FLOAT_LENGTH;
			bufferLength = sizeof(float);
			}
			break;

		case blr_double:
		case blr_d_float:
			{
			type = JDBC_DOUBLE;
			typeName = "DOUBLE PRECISION";
			length = MAX_DOUBLE_LENGTH;
			bufferLength = sizeof(double);
			}
			break;

		case blr_varying:
		case blr_varying2:
			{
			type = JDBC_VARCHAR;
			typeName = "VARCHAR";
			}
			break;

		case blr_blob:
			{
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
			length = MAX_BLOB_LENGTH;
			bufferLength = length;
			}
			break;

		case blr_sql_date:
			{
//			type = JDBC_DATE;
			type = JDBC_SQL_DATE;
			typeName = "DATE";
			length = MAX_DATE_LENGTH;
			bufferLength = 6; // sizeof(tagDATE_STRUCT); 
			}
			break;

		case blr_sql_time:
			{
//			type = JDBC_TIME;
			type = JDBC_SQL_TIME;
			typeName = "TIME";
			length = MAX_TIME_LENGTH;
			bufferLength = 6; // sizeof(tagTIME_STRUCT); 
			}
			break;

		case blr_timestamp:
			{
//			type = JDBC_TIMESTAMP;
			type = JDBC_SQL_TIMESTAMP;
			typeName = "TIMESTAMP";
			length = MAX_TIMESTAMP_LENGTH;
			bufferLength = 16; // sizeof(tagTIMESTAMP_STRUCT)
			}
			break;

		default:
			{
			typeName = "UNKNOWN";
			type = 0;
			}
		}

	if (type == JDBC_SMALLINT || type == JDBC_INTEGER || 
		type == JDBC_BIGINT || type == JDBC_DOUBLE)
	{	
		if(scale < 0)
		{
			if (subType == 2)
				{
				type = JDBC_DECIMAL;
				typeName = "DECIMAL";
				length = precision;
				bufferLength = MAX_DECIMAL_LENGTH + 2;
				}
			else
				{
				type = JDBC_NUMERIC;
				typeName = "NUMERIC";
				length = precision;
				bufferLength = MAX_NUMERIC_LENGTH + 2;
				}
		}
	}
}

}; // end namespace IscDbcLibrary
