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
 *	2002-08-12	IscMetaDataResultSet.cpp
 *				Added changes from C. G. Alvarez to so that
 *				SQLColAttributes() called with SQL_COLUMN_TYPE_NAME returns 
 *				the name of the type instead of the number of the type
 *
 */

// IscMetaDataResultSet.cpp: implementation of the IscMetaDataResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscMetaDataResultSet.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "SQLError.h"
#include "IscConnection.h"
#include "Value.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscMetaDataResultSet::IscMetaDataResultSet(IscDatabaseMetaData *meta) : IscResultSet (NULL)
{
	metaData = meta;
	resultSet = NULL;
	statement = NULL;
}

IscMetaDataResultSet::~IscMetaDataResultSet()
{

}


int IscMetaDataResultSet::findColumn(const char * columnName)
{
	return resultSet->findColumn (columnName);
}

Value* IscMetaDataResultSet::getValue(int index)
{
	Value *value = resultSet->getValue (index);
	valueWasNull = value->type == Null;

	return value;
}

void IscMetaDataResultSet::prepareStatement(const char * sql)
{
	statement = metaData->connection->prepareStatement (sql);
	resultSet = (IscResultSet*) statement->executeQuery();
	sqlda = resultSet->sqlda;
	numberColumns = resultSet->numberColumns;
	allocConversions();
}

void IscMetaDataResultSet::trimBlanks(int id)
{
	Value *value = getValue (id);

	if (value->type == String)
		{
		char *data = value->data.string.string;
		int l = value->data.string.length;
		while (l && data [l - 1] == ' ')
			data [--l] = 0;
		value->data.string.length = l;
		}
}

bool IscMetaDataResultSet::isWildcarded(const char * pattern)
{
	for (const char *p = pattern; *p; ++p)
		if (*p == '%' || *p == '\\' || *p == '*')
			return true;

	return false;
}

JString IscMetaDataResultSet::expandPattern(const char *prefix, const char * string, const char * pattern)
{
	char temp [256];

	if (isWildcarded (pattern))
		sprintf (temp, "%s (%s like '%s %%' ESCAPE '\\' or %s like '%s' ESCAPE '\\')\n",
							prefix, string, pattern, string, pattern);
	else
		sprintf (temp, "%s %s = \'%s\'\n",prefix, string, pattern);

	return temp;
}

int IscMetaDataResultSet::getColumnType(int index, int &realSqlType)
{
	return resultSet->getColumnType (index, realSqlType);
}

const char* IscMetaDataResultSet::getColumnTypeName(int index)
{
    return resultSet->getColumnTypeName (index);
}

int IscMetaDataResultSet::getColumnDisplaySize(int index)
{
	return resultSet->getColumnDisplaySize (index);
}

const char* IscMetaDataResultSet::getColumnName(int index)
{
	return resultSet->getColumnName (index);
}

const char* IscMetaDataResultSet::getTableName(int index)
{
	return resultSet->getTableName (index);
}

int IscMetaDataResultSet::getPrecision(int index)
{
	return resultSet->getPrecision (index);
}

int IscMetaDataResultSet::getScale(int index)
{
	return resultSet->getScale (index);
}

bool IscMetaDataResultSet::isNullable(int index)
{
	return resultSet->isNullable (index);
}
