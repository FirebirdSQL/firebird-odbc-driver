// IscSpecialColumnsResultSet.cpp: implementation of the IscSpecialColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"
#include "IscSqlType.h"

#include "IscSpecialColumnsResultSet.h"

#define TYPE_NAME  4


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
IscSpecialColumnsResultSet::IscSpecialColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)

{

}

IscSpecialColumnsResultSet::~IscSpecialColumnsResultSet()
{

}

void IscSpecialColumnsResultSet::specialColumns (const char * catalog, const char * schema, const char * table, int scope, int nullable)
{
	JString sql = 
		"select 1 as scope,\n"			// 1 scope is always transaction for us
				"\trfr.rdb$field_name as column_name, \n"		// 2
				"\tf.rdb$field_type as data_type,\n"		// 3
				"\tf.rdb$field_sub_type as type_name,\n"	// 4
				"\tf.rdb$character_length as column_size,\n"	// 5
				"\tf.rdb$field_length as buffer_length,\n"	// 6
				"\t(f.rdb$field_scale * -1) as decimal_digits,\n"	// 7
				"\t1 as pseudo_column,\n"	// 8
				"\trel.rdb$constraint_type,\n"	// 9
				"\ti.rdb$index_id\n"						//10
		"from rdb$fields f\n"
			"\tjoin rdb$relation_fields rfr\n" 
				"\t\ton rfr.rdb$field_source = f.rdb$field_name\n"
			"\tjoin rdb$indices i\n"
				"\t\ton rfr.rdb$relation_name = i.rdb$relation_name\n"
			"\tjoin rdb$index_segments s\n"
				"\t\ton rfr.rdb$field_name = s.rdb$field_name\n"
				"\t\tand i.rdb$index_name = s.rdb$index_name\n"
			"\tleft outer join rdb$relation_constraints rel\n"
				"\t\ton rel.rdb$constraint_type = 'PRIMARY KEY'\n"
				"\t\tand rel.rdb$index_name = i.rdb$index_name\n"
		"where i.rdb$unique_flag = 1\n";
	sql += expandPattern ("\tand rfr.rdb$relation_name %s '%s'\n", table);
	sql += " order by rel.rdb$constraint_type, rdb$index_name, rdb$field_position";

	prepareStatement (sql);
	numberColumns = 9;
	index_id = -1;

}

bool IscSpecialColumnsResultSet::next ()
{

	if (!resultSet->next())
		return false;

	int	idx_id = resultSet->getInt (10);
	if (index_id == -1) 
		index_id = idx_id;
	else if (idx_id != index_id)
		return false;

	trimBlanks (2);

	//translate to the SQL type information
	int blrType = resultSet->getInt (3);	// field type
	int subType = resultSet->getInt (4);
	int length = resultSet->getInt (6);

	int dialect = resultSet->statement->connection->getDatabaseDialect();
	IscSqlType sqlType (blrType, subType, length, dialect);

	char *type, t[50];
	type = t;
	sprintf (type, "%s", sqlType.typeName);

	resultSet->setValue (3, sqlType.type);
	resultSet->setValue (4, type);

	setCharLen (5, 6, sqlType);

	adjustResults (sqlType);


	return true;
}

int IscSpecialColumnsResultSet::getColumnType(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index);
}

int IscSpecialColumnsResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscSpecialColumnsResultSet::getPrecision(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getPrecision (index);
}



void IscSpecialColumnsResultSet::setCharLen (int charLenInd, 
								      int fldLenInd, 
									  IscSqlType sqlType)
{

	int fldLen = resultSet->getInt (fldLenInd);
	int charLen = resultSet->getInt (charLenInd);
	if (resultSet->valueWasNull)
		charLen = fldLen;

	if (sqlType.type != JDBC_VARCHAR &&
		sqlType.type != JDBC_CHAR)
		charLen = sqlType.length;
	
	if (sqlType.type == JDBC_VARCHAR)
		resultSet->setValue (fldLenInd, fldLen + 2);
	else
		resultSet->setValue (fldLenInd, fldLen);


	if (!charLen)
		resultSet->setNull (charLenInd);
	else
		resultSet->setValue (charLenInd, charLen);

}



void IscSpecialColumnsResultSet::adjustResults (IscSqlType sqlType)

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
			resultSet->setNull (7);
			break;
		case JDBC_TIME:
		case JDBC_TIMESTAMP:
			resultSet->setValue (7, (long)0);
		}	

}
