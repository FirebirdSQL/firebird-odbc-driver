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
 *	2002-08-12	IscResultSetMetaData.cpp
 *				Added changes from C. G. Alvarez to so that
 *				SQLColAttributes() called with SQL_COLUMN_TYPE_NAME returns 
 *				the name of the type instead of the number of the type
 *
 */

// IscResultSetMetaData.cpp: implementation of the IscResultSetMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscResultSetMetaData.h"
#include "IscResultSet.h"
#include "SQLError.h"
#include "Sqlda.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscResultSetMetaData::IscResultSetMetaData(IscResultSet *results, int columns)
{
	resultSet = results;
	numberColumns = columns;
}

IscResultSetMetaData::~IscResultSetMetaData()
{

}

int IscResultSetMetaData::getColumnCount()
{
	return numberColumns;
}

int IscResultSetMetaData::getColumnType(int index)
{
	int realSqlType;
	return resultSet->getColumnType (index, realSqlType);
}

const char* IscResultSetMetaData::getColumnTypeName(int index)
{
    return resultSet->getColumnTypeName (index);
}

int IscResultSetMetaData::getColumnDisplaySize(int index)
{
	return resultSet->getColumnDisplaySize (index);
}

const char* IscResultSetMetaData::getColumnName(int index)
{
	return resultSet->getColumnName (index);
}

const char* IscResultSetMetaData::getTableName(int index)
{
	return resultSet->getTableName (index);
}

int IscResultSetMetaData::getPrecision(int index)
{
	return resultSet->getPrecision (index);
}

int IscResultSetMetaData::getNumPrecRadix(int index)
{
	return resultSet->getNumPrecRadix (index);
}

int IscResultSetMetaData::getScale(int index)
{
	return resultSet->getScale (index);
}

bool IscResultSetMetaData::isNullable(int index)
{
	return resultSet->isNullable (index);
}

int IscResultSetMetaData::objectVersion()
{
	return RESULTSETMETADATA_VERSION;
}

const char* IscResultSetMetaData::getColumnLabel(int index)
{
	return resultSet->getColumnLabel (index);
}

bool IscResultSetMetaData::isSigned(int index)
{
	return true;
}

bool IscResultSetMetaData::isWritable(int index)
{
	return true;
}

bool IscResultSetMetaData::isReadOnly(int index)
{
	return false;
}

bool IscResultSetMetaData::isDefinitelyWritable(int index)
{
	return false;
}

bool IscResultSetMetaData::isCurrency(int index)
{
	return false;
}

bool IscResultSetMetaData::isAutoIncrement(int index)
{
	return false;
}

bool IscResultSetMetaData::isSearchable(int index)
{
	int type = getColumnType (index);

	return type != JDBC_LONGVARCHAR && type != JDBC_LONGVARBINARY;
}

const char* IscResultSetMetaData::getSchemaName(int index)
{
	return resultSet->getSchemaName (index);
}

const char* IscResultSetMetaData::getCatalogName(int index)
{
	return "";	
}

bool IscResultSetMetaData::isCaseSensitive(int index)
{
	return true;
}

}; // end namespace IscDbcLibrary
