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
#ifdef _WINDOWS
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
	sqlType.appOdbcVersion = metaData->connection->getUseAppOdbcVersion(); // SQL_OV_ODBC2 or SQL_OV_ODBC3
}

void IscColumnsResultSet::initResultSet(IscStatement *stmt)
{
	IscResultSet::initResultSet ( stmt );
	blob.statement = stmt;
}

void IscColumnsResultSet::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	char sql[4096] = "";
	char * pt = sql;
	addString(pt, "select cast( '");
	if (catalog && *catalog)
		addString(pt, catalog);
	addString(pt, "' as varchar(255)) as table_cat,\n"								// 1 - VARCHAR
				"\tcast (tbl.rdb$owner_name as varchar(31)) as table_schem,\n"		// 2 - VARCHAR
				"\tcast (rfr.rdb$relation_name as varchar(31)) as table_name,\n"	// 3 - VARCHAR NOT NULL
				"\tcast (rfr.rdb$field_name as varchar(31)) as column_name,\n"		// 4 - VARCHAR NOT NULL
				"\tfld.rdb$field_type as data_type,\n"				// 5 - SMALLINT NOT NULL
				"\tcast (fld.rdb$field_name as varchar(31)) as type_name,\n"		// 6 - VARCHAR NOT NULL
				"\tcast (fld.rdb$collation_id as integer) as column_size,\n"		// 7 - INTEGER
				"\tcast (fld.rdb$character_set_id as integer) as buffer_length,\n"	// 8 - INTEGER
				"\tcast (fld.rdb$field_scale as smallint) as decimal_digits,\n"		// 9 - SMALLINT
				"\tfld.rdb$field_scale as num_prec_radix,\n"		// 10 - SMALLINT
				"\trfr.rdb$null_flag as nullable,\n"				// 11 - SMALLINT NOT NULL
				"\tcast (NULL as char(10)) as remarks,\n"			// 12 - VARCHAR
				"\tcast (rfr.rdb$field_name as varchar(512)) as column_def,\n"				// 13 - VARCHAR
				"\tfld.rdb$field_type as SQL_DATA_TYPE,\n"			// 14 - SMALLINT NOT NULL
				"\tfld.rdb$field_sub_type as SQL_DATETIME_SUB,\n"	// 15 - SMALLINT
				"\t10 as CHAR_OCTET_LENGTH,\n"						// 16 - INTEGER
				"\t10 as ordinal_position,\n"						// 17 - INTEGER NOT NULL
				"\tcast ('YES' as varchar(3)) as IS_NULLABLE,\n"	// 18 - VARCHAR
				"\tfld.rdb$character_length as char_len,\n"			// 19
				"\tfld.rdb$default_source as f_def_source,\n"		// 20
				"\tfld.rdb$dimensions as array_dim,\n"				// 21
				"\tfld.rdb$null_flag as null_flag,\n"				// 22
				"\trfr.rdb$field_position as column_position,\n"	// 23
				"\tfld.rdb$field_length as column_length,\n"		// 24
				"\tfld.rdb$field_precision as column_precision,\n"	// 25
				"\trfr.rdb$default_source as column_def\n"			// 26
		"from rdb$relation_fields rfr, rdb$fields fld, rdb$relations tbl\n"
		"where rfr.rdb$field_source = fld.rdb$field_name\n"
		"	and rfr.rdb$relation_name = tbl.rdb$relation_name\n");

	char * ptFirst = sql + strlen(sql);

	if (schemaPattern && *schemaPattern)
		expandPattern (ptFirst, " and ","tbl.rdb$owner_name", schemaPattern);

	if (tableNamePattern && *tableNamePattern)
		expandPattern (ptFirst, " and ","rfr.rdb$relation_name", tableNamePattern);

	if (fieldNamePattern && *fieldNamePattern)
		expandPattern (ptFirst, " and ","rfr.rdb$field_name", fieldNamePattern);

	addString(ptFirst, " order by rfr.rdb$relation_name, rfr.rdb$field_position\n");
	
#ifdef DEBUG
	OutputDebugString (sql);
#endif
	prepareStatement (sql);

// SELECT returns 26 columns,
// But all interests only 18 
// This line is forbidden for modifying!!!
	numberColumns = 18;
}

bool IscColumnsResultSet::nextFetch()
{
	if (!IscResultSet::nextFetch())
	{
		blob.clear();
		return false;
	}

	if ( !metaData->getUseSchemaIdentifier() )
		sqlda->setNull(2);

	int &charLength = sqlType.lengthCharIn;
	int &len = sqlType.lengthIn;
	
	charLength = sqlda->getShort (19);
	len = sqlda->getShort (24);

	sqlType.collationId = sqlda->getInt (7);		// COLLATION_ID
	sqlType.characterId = sqlda->getInt (8);		// CHARACTER_SET_ID

	sqlda->updateInt (7, len);						// COLUMN_SIZE
	sqlda->updateInt (8, len);						// BUFFER_LENGTH
	sqlda->updateShort (10, 10);					// NUM_PREC_RADIX
	sqlda->updateInt (16, len);						// CHAR_OCTET_LENGTH
	sqlda->updateInt (17, sqlda->getShort (23)+1);	// ORDINAL_POSITION
	
	//translate to the SQL type information
	sqlType.blrType	  = sqlda->getShort (5);		// DATA_TYPE
	sqlType.subType	  = sqlda->getShort (15);		// SUB_TYPE
	sqlType.scale	  = sqlda->getShort (9);		// DECIMAL_DIGITS
	int array	  = sqlda->getShort (21);			// ARRAY_DIMENSION
	sqlType.precision = sqlda->getShort (25);		// COLUMN_PRECISION
	sqlType.dialect = statement->connection->getDatabaseDialect();

	sqlType.buildType();

	if ( array )
	{
		int len;
		char * relation_name = sqlda->getVarying ( 3, len);
		relation_name[len] = '\0';
		char * field_name = sqlda->getVarying ( 4, len);
		field_name[len] = '\0';

		arrAttr.loadAttributes ( statement, relation_name, field_name, sqlType.subType );

		sqlda->updateVarying (6, arrAttr.getFbSqlType());
		sqlda->updateInt (7, arrAttr.arrOctetLength );
		sqlda->updateInt (8, arrAttr.getBufferLength());

		if ( arrAttr.arrOctetLength < MAX_VARCHAR_LENGTH )
			sqlda->updateShort (5, JDBC_VARCHAR);
		else
			sqlda->updateShort (5, JDBC_LONGVARCHAR);

		sqlda->updateInt (16, arrAttr.arrOctetLength );
	}
	else
	{
		sqlda->updateVarying (6, sqlType.typeName);
		setCharLen (7, 8, sqlType);
		sqlda->updateShort (5, sqlType.type);

		//Octet length
		switch (sqlType.type)
		{
		case JDBC_VARCHAR:
		case JDBC_CHAR:
			sqlda->updateInt (16, sqlda->getInt (8));
			break;
		case JDBC_WVARCHAR:
		case JDBC_WCHAR:
			sqlda->updateInt (16, sqlType.bufferLength);
			break;
		default:
			sqlda->setNull (16);
		} 
	}

	adjustResults (sqlType);

	return true;
}

bool IscColumnsResultSet::getDefSource (int indexIn, int indexTarget)
{
	if ( sqlda->isNull (indexIn) )
	{
		sqlda->updateVarying (indexTarget, "NULL");
		return false;
	}

	XSQLVAR *var = sqlda->Var(indexIn);
	char buffer[1024];
	char * beg = buffer + 7; // sizeof("default")
	char * end;
	int lenRead;

	blob.directOpenBlob ((char*)var->sqldata);
	blob.directFetchBlob (buffer, 1024, lenRead);
	blob.directCloseBlob();
	
	end = buffer + lenRead;

	while ( *++beg == ' ' );
	while ( *end == ' ' ) end--;

	if ( *beg == '\'' && *(beg + 1) != '\'' )
	{
		++beg;
		--end;
	}

	*end = '\0';
	
	sqlda->updateVarying (indexTarget, beg);

	return true;
}								

void IscColumnsResultSet::setCharLen (int charLenInd, 
								      int fldLenInd, 
									  IscSqlType &sqlType)
{
	int fldLen = sqlda->getInt (fldLenInd);
	int charLen = sqlda->getInt (charLenInd);

	if ( sqlda->isNull(charLenInd) )
		charLen = fldLen;

	if (sqlType.type != JDBC_VARCHAR &&
		sqlType.type != JDBC_CHAR)
	{
		charLen = sqlType.length;
		fldLen  = sqlType.bufferLength;
	}
	
	sqlda->updateInt (fldLenInd, fldLen);

	if (!charLen)
		sqlda->setNull (charLenInd);
	else
		sqlda->updateInt (charLenInd, charLen);
}

void IscColumnsResultSet::checkQuotes (IscSqlType &sqlType, JString stringVal)
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

void IscColumnsResultSet::adjustResults (IscSqlType &sqlType)
{
	// Data source–dependent data type name
	switch (sqlType.type)
	{
	case JDBC_LONGVARCHAR:
		sqlda->updateVarying (6, "BLOB SUB_TYPE TEXT");
		break;
	case JDBC_LONGVARBINARY:
		sqlda->updateVarying (6, "BLOB SUB_TYPE BLR");
		break;
	} 

	// decimal digits have no meaning for some columns
	// radix - doesn't mean much for some colums either
	switch (sqlType.type)
	{
	case JDBC_NUMERIC:
	case JDBC_DECIMAL:
		sqlda->updateShort (9, sqlda->getShort(9)*-1); 	// Scale > 0
		break;
	case JDBC_REAL:
	case JDBC_FLOAT:
	case JDBC_DOUBLE:
		sqlda->setNull (9);
		sqlda->updateShort (10, 2);
		break;
	case JDBC_CHAR:
	case JDBC_WCHAR:
	case JDBC_VARCHAR:
	case JDBC_WVARCHAR:
	case JDBC_LONGVARCHAR:
	case JDBC_LONGVARBINARY:
	case JDBC_DATE:
	case JDBC_SQL_DATE:
		sqlda->setNull (9);
		sqlda->setNull (10);
		break;
	case JDBC_TIME:
	case JDBC_SQL_TIME:
	case JDBC_TIMESTAMP:
	case JDBC_SQL_TIMESTAMP:
		sqlda->updateShort (9, -ISC_TIME_SECONDS_PRECISION_SCALE);
		sqlda->setNull (10);
	}	

	// nullable
	short nullable = !sqlda->getShort (11) || sqlda->isNull(11);
	sqlda->updateShort (11, nullable);

	// default source
	if (!getDefSource (26, 13))
		getDefSource (20, 13);

	switch (sqlType.type)
		{
		case JDBC_DATE:
		case JDBC_SQL_DATE:
			sqlda->updateShort (14, 9);
			sqlda->updateShort (15, 1);
			break;
		case JDBC_TIME:
		case JDBC_SQL_TIME:
			sqlda->updateShort (14, 9);
			sqlda->updateShort (15, 2);
			break;
		case JDBC_TIMESTAMP:
		case JDBC_SQL_TIMESTAMP:
			sqlda->updateShort (14, 9);
			sqlda->updateShort (15, 3);
			break;
		default:
			sqlda->updateShort (14, sqlda->getShort(5));
			sqlda->setNull (15);
		}

	// Is Nullable - I'm seeing everything twice
	if ( !nullable )
		sqlda->updateVarying (18, "NO");
}

}; // end namespace IscDbcLibrary
