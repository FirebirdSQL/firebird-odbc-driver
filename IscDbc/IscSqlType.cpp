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

void IscSqlType::buildType ()
{
	length = bufferLength = lengthIn;

	switch (blrType)
		{
		case blr_text:
		case blr_text2:
			{
			if ( length == 1 && characterId == 1 )
				{
				type = JDBC_TINYINT;
				typeName = "TINYINT";
				bufferLength = length;
				length = MAX_TINYINT_LENGTH;
				}
			else
				{
				type = JDBC_CHAR;
				typeName = "CHAR";

				if ( characterId > 2 && lengthCharIn ) // > ASCII
					{
					length = lengthCharIn;
					if ( characterId == 4 || characterId == 3 ) // UTF8 || UNICODE_FSS
						type = JDBC_WCHAR;
					}
				}
			}
			break;

		case blr_bool:
			{
			type = JDBC_BOOLEAN;
			typeName = "BOOLEAN";
			length = MAX_BOOLEAN_LENGTH;
			bufferLength = sizeof(bool);
			if ( !precision ) // for calculate fields
				precision = length;
			}
			break;

		case blr_short:
			{
			type = JDBC_SMALLINT;
			typeName = "SMALLINT";
			length = MAX_SMALLINT_LENGTH;
			bufferLength = sizeof(short);
			if ( !precision ) // for calculate fields
				precision = length;
			}
			break;

		case blr_long:
			{
			type = JDBC_INTEGER;
			typeName = "INTEGER";
			length = MAX_INT_LENGTH;
			bufferLength = sizeof(int);
			if ( !precision ) // for calculate fields
				precision = length;
			}
			break;

		case blr_quad:
		case blr_int64:
			{
			type = JDBC_BIGINT;
			typeName = "BIGINT";
			length = MAX_QUAD_LENGTH;
			bufferLength = MAX_QUAD_LENGTH + 2;
			if ( !precision ) // for calculate fields
				precision = length;
			}
			break;

		case blr_float:
			{
			type = JDBC_REAL;
			typeName = "FLOAT";
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

			if ( characterId > 2 && lengthCharIn ) // > ASCII
				{
				length = lengthCharIn;
				if ( characterId == 4 || characterId == 3 ) // UTF8 || UNICODE_FSS
					type = JDBC_WVARCHAR;
				}
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
			if ( appOdbcVersion == 2 ) // SQL_OV_ODBC2
				type = JDBC_SQL_DATE;
			else
				type = JDBC_DATE;
			typeName = "DATE";
			length = MAX_DATE_LENGTH;
			bufferLength = 6; // sizeof(tagDATE_STRUCT); 
			}
			break;

		case blr_sql_time:
			{
			if ( appOdbcVersion == 2 ) // SQL_OV_ODBC2
				type = JDBC_SQL_TIME;
			else
				type = JDBC_TIME;
			typeName = "TIME";
			length = MAX_TIME_LENGTH;
			bufferLength = 6; // sizeof(tagTIME_STRUCT); 
			}
			break;

		case blr_timestamp:
			{
			if ( appOdbcVersion == 2 ) // SQL_OV_ODBC2
				type = JDBC_SQL_TIMESTAMP;
			else
				type = JDBC_TIMESTAMP;
			typeName = "TIMESTAMP";
			length = MAX_TIMESTAMP_LENGTH;
			bufferLength = 16; // sizeof(tagTIMESTAMP_STRUCT); 
			}
			break;

		default:
			{
			typeName = "UNKNOWN";
			type = 0;
			}
		}

	switch ( type )
	{	
	case JDBC_SMALLINT:
	case JDBC_INTEGER:
	case JDBC_BIGINT:
	case JDBC_DOUBLE:
		if (subType == 2)
		{
			switch( type )
			{
			case JDBC_SMALLINT:
				bufferLength = MAX_DECIMAL_SHORT_LENGTH + 2;
				break;
			case JDBC_INTEGER:
				bufferLength = MAX_DECIMAL_LONG_LENGTH + 2;
				break;
			case JDBC_DOUBLE:
				bufferLength = MAX_DECIMAL_DOUBLE_LENGTH + 2;
				break;
			default:
				bufferLength = MAX_DECIMAL_LENGTH + 2;
			}

			type = JDBC_DECIMAL;
			typeName = "DECIMAL";
			length = precision;
		}
		else if (subType == 1 || (!subType && scale))
		{
			switch( type )
			{
			case JDBC_SMALLINT:
				bufferLength = MAX_NUMERIC_SHORT_LENGTH + 2;
				if ( !precision )
					precision = MAX_NUMERIC_SHORT_LENGTH;
				break;
			case JDBC_INTEGER:
				bufferLength = MAX_NUMERIC_LONG_LENGTH + 2;
				if ( !precision )
					precision = MAX_NUMERIC_LONG_LENGTH;
				break;
			case JDBC_DOUBLE:
				bufferLength = MAX_NUMERIC_DOUBLE_LENGTH + 2;
				if ( !precision )
					precision = MAX_NUMERIC_DOUBLE_LENGTH;
				break;
			default:
				bufferLength = MAX_NUMERIC_LENGTH + 2;
				if ( !precision )
					precision = MAX_NUMERIC_LENGTH;
			}

			type = JDBC_NUMERIC;
			typeName = "NUMERIC";
			length = precision;
		}
	}
}

}; // end namespace IscDbcLibrary
