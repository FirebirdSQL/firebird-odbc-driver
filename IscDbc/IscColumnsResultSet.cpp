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


// IscColumnsResultSet.cpp: implementation of the IscColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnsResultSet::IscColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

IscColumnsResultSet::~IscColumnsResultSet()
{

}

void IscColumnsResultSet::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"						// 1
				"\tNULL as table_schem,\n"				// 2
				"\trdb$relation_name as table_name,"	// 3
				"\trfr.rdb$field_name as column_name,\n"// 4
				"\trdb$field_type as data_type,\n"		// 5
				"\trdb$field_sub_type as type_name,\n"	// 6
				"\trdb$field_length as column_size,\n"	// 7
				"\trdb$field_length as buffer_length,\n"			// 8
				"\trdb$field_scale as decimal_digits,\n"// 9
				"\t10 as num_prec_radix,\n"				// 10
				"\trfr.rdb$null_flag as nullable,\n"		// 11
				"\trfr.rdb$description as remarks,\n"	// 12
				"\trfr.rdb$default_value as column_def,\n"	// 13
				"\trdb$field_type as SQL_DATA_TYPE,\n"			// 14
				"\trdb$field_sub_type as SQL_DATETIME_SUB,\n"			// 15
				"\trdb$field_length as CHAR_OCTET_LENGTH,\n"// 16
				"\trdb$field_position as ordinal_position,\n"	// 17
				"\t'YES' as IS_NULLABLE,\n"				// 18
				"\tfld.rdb$character_length as char_len,\n"	//19
				"\tfld.rdb$default_value as f_def_val,\n"	//20
				"\tfld.rdb$dimensions as array_dim\n,"	//21
				"\tfld.rdb$null_flag as null_flag\n"	//22				
		"from rdb$relation_fields rfr, rdb$fields fld\n"
		" where rfr.rdb$field_source = fld.rdb$field_name\n";

	if (tableNamePattern)
		sql += expandPattern (" and rdb$relation_name %s '%s'", tableNamePattern);

	if (fieldNamePattern)
		sql += expandPattern (" and rfr.rdb$field_name %s '%s'", fieldNamePattern);

	sql += " order by rdb$relation_name, rfr.rdb$field_name";
	prepareStatement (sql);
	numberColumns = 18;
}

bool IscColumnsResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// table name
	trimBlanks (4);							// field name

	//translate to the SQL type information
	int blrType = resultSet->getInt (5);	// field type
	int subType = resultSet->getInt (6);
	int length = resultSet->getInt (7);
	int array = resultSet->getInt (21);
	if (resultSet->valueWasNull)
		array = 0;

	int dialect = resultSet->statement->connection->getDatabaseDialect();
	IscSqlType sqlType (blrType, subType, length, dialect);

	JString type;
	type.Format ("%s%s", (array) ? "ARRAY OF " : "", sqlType.typeName);

	resultSet->setValue (5, sqlType.type);
	resultSet->setValue (6, type);

	setCharLen (19, 7, sqlType);

	adjustResults (sqlType);


	return true;
}

int IscColumnsResultSet::getColumnType(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
		case DEF_VAL:					// Default Value;
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index);
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
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
		case DEF_VAL:					// Default Value;
			return 128;
		}

	return Parent::getPrecision (index);
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
		delete s;
		return false;
		}

	stuff++;
	
	if (*stuff == blr_null)
		{
		resultSet->setValue (indexTarget, "NULL");
		delete s;
		return true;
		}

	if (*stuff != blr_literal)
		{
		resultSet->setValue (indexTarget, "unknown, not literal");
		delete s;
		return false;
		}

	stuff++;	
	long	intVal, temp;
	short	type, scale, mag;
	char	stringTemp [BUFF_LEN];

	JString stringVal;
	
	type = *stuff++;

	switch (type)
		{
		case (blr_short):
		case (blr_long):
			scale = (*stuff++) * -1;
			mag = 1;

			intVal = isc_vax_integer (stuff, (type == blr_short)? 2 : 4);

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
			intVal = isc_vax_integer (stuff, 4);
			temp = isc_vax_integer (&stuff[4], 4);
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
			TimeStamp timestamp;
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
					intVal = isc_vax_integer (stuff, 2);
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
	delete s;
	return true;
}								

void IscColumnsResultSet::setCharLen (int charLenInd, 
								      int fldLenInd, 
									  IscSqlType sqlType)
{

	int charLen = resultSet->getInt (charLenInd);
	if (resultSet->valueWasNull)
		charLen = resultSet->getInt (fldLenInd);
	if (sqlType.type != JDBC_VARCHAR &&
		sqlType.type != JDBC_CHAR)
		charLen = sqlType.length;
	
	if (!charLen)
		resultSet->setNull (fldLenInd);
	else
		resultSet->setValue (fldLenInd, charLen);
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
		case JDBC_TIMESTAMP:
		case JDBC_TIME:
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
	// adjust the storage length for VARCHAR
	if (sqlType.type == JDBC_VARCHAR)
		resultSet->setValue (8, (resultSet->getInt (8)) + 2);

	// decimal digits have no meaning for some columns
	// radix - doesn't mean much for some colums either
	switch (sqlType.type)
		{
		case JDBC_CHAR:
		case JDBC_VARCHAR:
		case JDBC_LONGVARCHAR:
		case JDBC_LONGVARBINARY:
		case JDBC_DATE:
			resultSet->setNull (9);
			resultSet->setNull (10);
			break;
		case JDBC_TIME:
		case JDBC_TIMESTAMP:
			resultSet->setValue (9, (long)0);
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

	// SQL_DATA_TYPE & DateTimeSubType - why make this easy?
	switch (sqlType.type)
		{
		case JDBC_DATE:
			resultSet->setValue (14, (long) 9);
			resultSet->setValue (15, (long) 1);
		case JDBC_TIME:
			resultSet->setValue (14, (long) 9);
			resultSet->setValue (15, (long) 2);
		case JDBC_TIMESTAMP:
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
