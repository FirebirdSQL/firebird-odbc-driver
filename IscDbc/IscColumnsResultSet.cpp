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
 *	2003-03-24	IscColumnsResultSet.cpp
 *				Contributed by Norbert Meyer
 *				o Add some breaks to case statements in adjustResults()
 *				  VARCHAR storage length is set in function ::setCharLen
 *				o In IscColumnsResultSet::getBLRLiteral use delete[] s
 *				  instead of delete.s
 *
 *	2002-11-24	IscColumnsResultSet.cpp
 *				Contributed by C. G. Alvarez
 *				Improve handling of NUMERIC and DECIMAL fields
 *
 */


// IscColumnsResultSet.cpp: implementation of the IscColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#ifdef _WIN32
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscColumnsResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"
#include "IscSqlType.h"
#include "JString.h"

#define TYPE_NAME	6
#define DEF_VAL		13
#define BUFF_LEN	256

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnsResultSet::IscColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

void IscColumnsResultSet::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"								// 1 - VARCHAR
				"\tNULL as table_schem,\n"							// 2 - VARCHAR
				"\trfr.rdb$relation_name as table_name,"			// 3 - VARCHAR NOT NULL
				"\trfr.rdb$field_name as column_name,\n"			// 4 - VARCHAR NOT NULL
				"\tfld.rdb$field_type as data_type,\n"				// 5 - SMALLINT NOT NULL
				"\tfld.rdb$field_sub_type as type_name,\n"			// 6 - VARCHAR NOT NULL
				"\t10 as column_size,\n"							// 7 - INTEGER
				"\t10 as buffer_length,\n"							// 8 - INTEGER
				"\tcast (fld.rdb$field_scale as smallint) as decimal_digits,\n"		// 9 - SMALLINT
				"\tfld.rdb$field_scale as num_prec_radix,\n"		// 10 - SMALLINT
				"\trfr.rdb$null_flag as nullable,\n"				// 11 - SMALLINT NOT NULL
				"\tNULL as remarks,\n"								// 12 - VARCHAR
				"\trfr.rdb$default_value as column_def,\n"			// 13 - VARCHAR
				"\tfld.rdb$field_type as SQL_DATA_TYPE,\n"			// 14 - SMALLINT NOT NULL
				"\tfld.rdb$field_sub_type as SQL_DATETIME_SUB,\n"	// 15 - SMALLINT
				"\t10 as CHAR_OCTET_LENGTH,\n"						// 16 - INTEGER
				"\t10 as ordinal_position,\n"						// 17 - INTEGER NOT NULL
				"\t'YES' as IS_NULLABLE,\n"							// 18 - VARCHAR
				"\tfld.rdb$character_length as char_len,\n"			// 19
				"\tfld.rdb$default_value as f_def_val,\n"			// 20
				"\tfld.rdb$dimensions as array_dim,\n"				// 21
				"\tfld.rdb$null_flag as null_flag,\n"				// 22
				"\trfr.rdb$field_position as column_position,\n"	// 23
				"\tfld.rdb$field_length as column_length,\n"		// 24
				"\tfld.rdb$field_precision as column_precision\n"	// 25
		"from rdb$relation_fields rfr, rdb$fields fld\n"
		"where rfr.rdb$field_source = fld.rdb$field_name\n";

	if ( !metaData->allTablesAreSelectable() )
		sql += metaData->existsAccess(" and ", "rfr", 0, "\n");

	if (tableNamePattern && *tableNamePattern)
		sql += expandPattern (" and ","rfr.rdb$relation_name", tableNamePattern);

	if (fieldNamePattern && *fieldNamePattern)
		sql += expandPattern (" and ","rfr.rdb$field_name", fieldNamePattern);

	sql += " order by rfr.rdb$relation_name, rfr.rdb$field_position\n";
	
#ifdef DEBUG
	OutputDebugString (sql.getString());
#endif
	prepareStatement (sql);

// SELECT returns 25 columns,
// But all interests only 18 
// This line is forbidden for modifying!!!
	numberColumns = 18;
}

bool IscColumnsResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// table name
	trimBlanks (4);							// field name

	int len = resultSet->getInt (24);

	resultSet->setValue (7, len);						// COLUMN_SIZE
	resultSet->setValue (8, len);						// BUFFER_LENGTH
	resultSet->setValue (10, 10);						// NUM_PREC_RADIX
	resultSet->setValue (16, len);						// CHAR_OCTET_LENGTH
	resultSet->setValue (17, resultSet->getInt (23)+1);	// ORDINAL_POSITION
	
	//translate to the SQL type information
	int blrType	  = resultSet->getInt (5);	// DATA_TYPE
	int subType	  = resultSet->getInt (6);	// SUB_TYPE
	int length	  = resultSet->getInt (7);	// COLUMN_SIZE
	int scale	  = resultSet->getInt (9);	// DECIMAL_DIGITS
	int array	  = resultSet->getInt (21);	// ARRAY_DIMENSION
	int precision = resultSet->getInt (25);	// COLUMN_PRECISION

//	if (resultSet->valueWasNull)
//		array = 0;

	int dialect = resultSet->statement->connection->getDatabaseDialect();
//	IscSqlType sqlType (blrType, subType, length, length, dialect, precision);
	IscSqlType sqlType (blrType, subType, length, length, dialect, precision, scale);

	JString type;
	type.Format ("%s%s", (array) ? "ARRAY OF " : "", sqlType.typeName);

	resultSet->setValue (5, sqlType.type);
	resultSet->setValue (6, type);

	setCharLen (7, 8, sqlType);

	adjustResults (sqlType);


	return true;
}

int IscColumnsResultSet::getColumnType(int index, int &realSqlType)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
		case DEF_VAL:					// Default Value;
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index, realSqlType);
}

int IscColumnsResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
		case DEF_VAL:					// Default Value;
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscColumnsResultSet::getPrecision(int index)
{
	return 31;
/*
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
		case DEF_VAL:					// Default Value;
			return 128;
		}

	return Parent::getPrecision (index);
*/
}


bool IscColumnsResultSet::getBLRLiteral (int indexIn, 
										 int indexTarget,
										 IscSqlType sqlType)
{

	Blob *blob = resultSet->getBlob (indexIn);
	
	if (resultSet->valueWasNull)
	{
		resultSet->setValue (indexTarget, "NULL");
		return false;
	}

	char * stuff = new char [blob->length()];
	char * s = stuff;

	for (int offset = 0, length; 
			length = blob->getSegmentLength(offset); 
			offset += length)
		memcpy (stuff + offset, blob->getSegment(offset), length);

	if ((*stuff != blr_version4) && (*stuff != blr_version5))
		{
		resultSet->setValue (indexTarget, "unknown, not BLR");
		delete[] s;
		return false;
		}

	stuff++;
	
	if (*stuff == blr_null)
		{
		resultSet->setValue (indexTarget, "NULL");
		delete[] s;
		return true;
		}

	if (*stuff != blr_literal)
		{
		resultSet->setValue (indexTarget, "unknown, not literal");
		delete[] s;
		return false;
		}

	stuff++;	
	long	intVal, temp;
	short	type, scale, mag;
	char	stringTemp [BUFF_LEN];

	CFbDll * GDS = resultSet->statement->connection->GDS;
	JString stringVal;
	TimeStamp timestamp;	
	type = *stuff++;

	switch (type)
		{
		case (blr_short):
		case (blr_long):
			scale = (*stuff++) * -1;
			mag = 1;

			intVal = GDS->_vax_integer (stuff, (type == blr_short)? 2 : 4);

			if (!scale)
				stringVal.Format ("%d", intVal);
			else
				{
				for (temp = intVal; scale; scale--)
					{
					temp /= 10;	
					mag *= 10;
					}
				intVal %= mag;
				scale = *--stuff * -1;			
				stringVal.Format ("%d.%0*d", temp, scale, intVal);
				}

			break;

		case (blr_quad):
		case (blr_int64):
			scale = (*stuff++) * -1;
			intVal = GDS->_vax_integer (stuff, 4);
			temp = GDS->_vax_integer (&stuff[4], 4);
			stringVal.Format ("0x%x%x scale %d", intVal, temp, scale);
			break;

		case (blr_float):
		case (blr_double):
			stringVal.Format ("%g", stuff);
			break;

		case (blr_d_float):
			stringVal.Format ("d_float is not an ODBC concept");
			break;
			
		case (blr_timestamp):
			timestamp.date = (long) stuff;
			timestamp.nanos = (long) &stuff[4];
			timestamp.getTimeString (BUFF_LEN, stringTemp);
			stringVal.Format ("\'%s\'", stringTemp);
			break;

		case (blr_sql_date):
			DateTime date;
			date.date = (long) stuff;
			date.getString (BUFF_LEN, stringTemp);
			stringVal.Format ("\'%s\'", stringTemp);
			break;

		case (blr_sql_time):
			SqlTime time;
			time.timeValue = (long) stuff;
			time.getString (BUFF_LEN, stringTemp);
			stringVal.Format ("\'%s\'", stringTemp);
			break;

		case (blr_text2):
		case (blr_varying2):
		case (blr_cstring2):
			stuff += 2;   // skip the type info
		case (blr_text):
		case (blr_varying):
		case (blr_cstring):
			switch (type)
				{
				case (blr_cstring):
				case (blr_cstring2):
					intVal = strlen (stuff);
					break;
				default:
					intVal = GDS->_vax_integer (stuff, 2);
				}
			if ((intVal + 4) >= BUFF_LEN)
				{
				stringVal = "TRUNCATED";
				break;
				}
			stringVal.setString (&stuff[2], intVal);
			checkQuotes (sqlType, stringVal); 
			break;
			
		case (blr_blob_id):
		case (blr_blob):
			stringVal = "blob type is not compatible";
			break;

		}
	resultSet->setValue (indexTarget, stringVal);
	delete[] s;
	return true;
}								

void IscColumnsResultSet::setCharLen (int charLenInd, 
								      int fldLenInd, 
									  IscSqlType sqlType)
{
	int fldLen = resultSet->getInt (fldLenInd);
	int charLen = resultSet->getInt (charLenInd);
	if (resultSet->valueWasNull)
		charLen = fldLen;

	if (sqlType.type != JDBC_VARCHAR &&
		sqlType.type != JDBC_CHAR)
	{
		charLen = sqlType.length;
		fldLen  = sqlType.bufferLength;
	}
	
	resultSet->setValue (fldLenInd, fldLen);

	if (!charLen)
		resultSet->setNull (charLenInd);
	else
		resultSet->setValue (charLenInd, charLen);
}

void IscColumnsResultSet::checkQuotes (IscSqlType sqlType, JString stringVal)
{
	// Revolting ODBC wants the default value quoted unless its a 
	// number or a pseudo-literal
	
	JString	string = stringVal;
	string.upcase (string);

	switch (sqlType.type)
		{
		case JDBC_DATE:
		case JDBC_SQL_DATE:
		case JDBC_TIME:
		case JDBC_SQL_TIME:
		case JDBC_TIMESTAMP:
		case JDBC_SQL_TIMESTAMP:
			if (string == "CURRENT DATE" ||
				string == "CURRENT TIME" ||
				string == "CURRENT TIMESTAMP" ||
				string == "CURRENT ROLE")
				{
				stringVal = string;
				return;
				}
		case JDBC_CHAR:
		case JDBC_VARCHAR:
			if (string == "USER")
				{
				stringVal = string;
				return;
				}
	}
	stringVal.Format ("\'%s\'", (const char *) stringVal);
	return;
}

void IscColumnsResultSet::adjustResults (IscSqlType sqlType)

{
	// Data source–dependent data type name
	switch (sqlType.type)
		{
		case JDBC_LONGVARCHAR:
			resultSet->setValue (6, "BLOB SUB_TYPE 1");
			break;
		case JDBC_LONGVARBINARY:
			resultSet->setValue (6, "BLOB");
			break;
		} 

	// adjust the storage length for VARCHAR
//	if (sqlType.type == JDBC_VARCHAR)							// NOMEY -
//		resultSet->setValue (8, (resultSet->getInt (8)) + 2);	// NOMEY -

	// decimal digits have no meaning for some columns
	// radix - doesn't mean much for some colums either
	switch (sqlType.type)
		{
		case JDBC_NUMERIC:
		case JDBC_DECIMAL:
			resultSet->setValue (9, resultSet->getInt(9)*-1); 	// Scale > 0
			break;
		case JDBC_REAL:
		case JDBC_DOUBLE:
			resultSet->setNull (9);
			resultSet->setValue (10, 2);
			break;
		case JDBC_CHAR:
		case JDBC_VARCHAR:
		case JDBC_LONGVARCHAR:
		case JDBC_LONGVARBINARY:
		case JDBC_DATE:
		case JDBC_SQL_DATE:
			resultSet->setNull (9);
			resultSet->setNull (10);
			break;
		case JDBC_TIME:
		case JDBC_SQL_TIME:
		case JDBC_TIMESTAMP:
		case JDBC_SQL_TIMESTAMP:
			resultSet->setValue (9, (long)-ISC_TIME_SECONDS_PRECISION_SCALE);
			resultSet->setNull (10);
		}	

	// nullable
	int nullable = resultSet->getInt (11);
	if (!nullable || resultSet->valueWasNull)
		resultSet->setValue (11, (long) 1);
	else resultSet->setValue (11,(long) 0);
	

	// default values
	if (!getBLRLiteral (13, 13, sqlType))
		getBLRLiteral (20, 13, sqlType);

	switch (sqlType.type)
		{
		case JDBC_DATE:
		case JDBC_SQL_DATE:
			resultSet->setValue (14, (long) 9);
			resultSet->setValue (15, (long) 1);
			break;
		case JDBC_TIME:
		case JDBC_SQL_TIME:
			resultSet->setValue (14, (long) 9);
			resultSet->setValue (15, (long) 2);
			break;
		case JDBC_TIMESTAMP:
		case JDBC_SQL_TIMESTAMP:
			resultSet->setValue (14, (long) 9);
			resultSet->setValue (15, (long) 3);
			break;
		default:
			resultSet->setValue (14, resultSet->getInt(5));
			resultSet->setNull (15);
		}

	//Octet length
	switch (sqlType.type)
		{
		case JDBC_VARCHAR:
		case JDBC_CHAR:
			resultSet->setValue (16, resultSet->getInt (8));
			break;
		default:
			resultSet->setNull (16);
		} 

	// Is Nullable - I'm seeing everything twice

	resultSet->setValue (18, ((resultSet->getInt (11)) == 0) ? "NO" : "YES");

}

}; // end namespace IscDbcLibrary
