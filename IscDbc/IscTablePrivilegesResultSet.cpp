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
 *     2002-08-12	File Created by Carlos G. Alvarez
 *
 *     The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *     Copyright (c) 1999, 2000, 2001 James A. Starkey
 *     All Rights Reserved.
 */

// IscTablePrivilegesResultSet.cpp: implementation of the IscTablePrivilegesResultSet class.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscTablePrivilegesResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"

#define TABLE_TYPE    4


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscTablePrivilegesResultSet::IscTablePrivilegesResultSet(IscDatabaseMetaData *metaData)
        : IscMetaDataResultSet(metaData)
{
    resultSet = NULL;
}

IscTablePrivilegesResultSet::~IscTablePrivilegesResultSet()
{
    if (resultSet)
        resultSet->release();
}

void IscTablePrivilegesResultSet::getTablePrivileges(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	JString sql = "select NULL as table_cat,"						//1
				          "NULL as table_schem,"					//2
						  "tbl.rdb$relation_name as table_name,"	//3
						  "usp.rdb$grantor as grantor,"				//4
						  "usp.rdb$user as grantee,"				//5
						  "usp.rdb$privilege as privilege,"			//6
						  "'YES' as isgrantable, "					//7
						  "usp.rdb$grant_option as GRANT_OPTION "	//8
                          "from rdb$relations tbl, rdb$user_privileges usp\n"
                          " where tbl.rdb$relation_name = usp.rdb$relation_name\n";

    if (tableNamePattern && *tableNamePattern)
        sql += expandPattern (" and tbl.rdb$relation_name %s '%s'", tableNamePattern);

    sql += " order by tbl.rdb$relation_name, usp.rdb$privilege, usp.rdb$user";

    prepareStatement (sql);
    numberColumns = 7;
}

bool IscTablePrivilegesResultSet::next()
{
    if (!resultSet->next())
        return false;

	trimBlanks(3);
	trimBlanks(4);
	trimBlanks(5);

    const char *privilege = resultSet->getString(6);

    switch ( *privilege )
    {
        case 'S':
            resultSet->setValue( 6, "SELECT" );
            break;

        case 'I':
            resultSet->setValue( 6, "INSERT" );
            break;

        case 'U':
            resultSet->setValue( 6, "UPDATE" );
            break;

        case 'D':
            resultSet->setValue( 6, "DELETE" );
            break;

        case 'R':
            resultSet->setValue( 6, "REFERENCES" );
            break;
    }

	int isGrantable = resultSet->getInt(8);
	if ( isGrantable )
		resultSet->setValue( 7, "YES" );
	else
		resultSet->setValue( 7, "NO" );

    return true;
}

int IscTablePrivilegesResultSet::getColumnDisplaySize(int index)
{
    switch (index)
        {
        case TABLE_TYPE:                // change from blob to text
            return 128;
        }

    return Parent::getColumnDisplaySize (index);
}

int IscTablePrivilegesResultSet::getColumnType(int index)
{
    switch (index)
        {
        case TABLE_TYPE:                // change from blob to text
            return JDBC_VARCHAR;
        }

    return Parent::getColumnType (index);
}

int IscTablePrivilegesResultSet::getColumnPrecision(int index)
{
    switch (index)
        {
        case TABLE_TYPE:                // change from blob to text
            return 128;
        }

    return Parent::getPrecision (index);
}

