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

// IscStatementMetaData.cpp: implementation of the IscStatementMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscBlob.h"
#include "IscArray.h"
#include "IscHeadSqlVar.h"
#include "IscStatementMetaData.h"
#include "Sqlda.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscStatementMetaData::IscStatementMetaData(IscConnection *connect, Sqlda *ptSqlda)
{
	connection = connect;
	sqlda = ptSqlda;
}

int IscStatementMetaData::getColumnCount()
{
	return sqlda->getColumnCount();
}

int IscStatementMetaData::getColumnType(int index, int &realSqlType)
{
	return sqlda->getColumnType (index, realSqlType);
}

int IscStatementMetaData::getPrecision(int index)
{
	return sqlda->getPrecision (index);
}

int IscStatementMetaData::getScale(int index)
{
	return -sqlda->getScale (index);
}

bool IscStatementMetaData::isNullable(int index)
{
	return sqlda->isNullable (index);
}

int IscStatementMetaData::getColumnDisplaySize(int index)
{
	return sqlda->getColumnDisplaySize(index);
}

const char* IscStatementMetaData::getColumnLabel(int index)
{
	return sqlda->getColumnName(index);
}

const char* IscStatementMetaData::getSqlTypeName(int index)
{
	return sqlda->getColumnTypeName(index);
}

const char* IscStatementMetaData::getColumnName(int index)
{
	return sqlda->getColumnName(index);
}

const char* IscStatementMetaData::getTableName(int index)
{
	return sqlda->getTableName(index);
}

const char* IscStatementMetaData::getColumnTypeName(int index)
{
	return sqlda->getColumnTypeName(index);
}

bool IscStatementMetaData::isSigned(int index)
{
	return true;
}

bool IscStatementMetaData::isReadOnly(int index)
{
	return false;
}

bool IscStatementMetaData::isWritable(int index)
{
	return true;
}

bool IscStatementMetaData::isDefinitelyWritable(int index)
{
	return false;
}

bool IscStatementMetaData::isCurrency(int index)
{
	return false;
}

bool IscStatementMetaData::isCaseSensitive(int index)
{
	return true;
}

bool IscStatementMetaData::isAutoIncrement(int index)
{
	return false;
}

bool IscStatementMetaData::isSearchable(int index)
{
	int realSqlType;
	int type = sqlda->getColumnType (index, realSqlType);

	return type != JDBC_LONGVARCHAR && type != JDBC_LONGVARBINARY;
}

int IscStatementMetaData::isBlobOrArray(int index)
{
	return sqlda->isBlobOrArray(index);
}

const char* IscStatementMetaData::getSchemaName(int index)
{
	return sqlda->getOwnerName (index);
}

const char* IscStatementMetaData::getCatalogName(int index)
{
	return "";	
}

void IscStatementMetaData::createBlobDataTransfer(int index, Blob *& ptDataBlob)
{
	int isRet = sqlda->isBlobOrArray(index);

	if ( ptDataBlob )
	{
		ptDataBlob->release();
		ptDataBlob = NULL;
	}
	if ( isRet )
	{
		if ( isRet == SQL_BLOB )
		{
			IscBlob * pt = new IscBlob;
			pt->setType(sqlda->getSubType(index));
			pt->connection = connection;
			ptDataBlob = pt;
		}
		else // if ( isRet == SQL_ARRAY )
		{
			IscArray * pt = new IscArray(connection, sqlda->Var(index));
			ptDataBlob = pt;
		}
	}
}

void IscStatementMetaData::getSqlData(int index, Blob *& ptDataBlob, HeadSqlVar *& ptHeadSqlVar)
{
	XSQLVAR *var = sqlda->Var(index);

	if ( ptHeadSqlVar )
		ptHeadSqlVar->release();

	ptHeadSqlVar = new IscHeadSqlVar(var);

	createBlobDataTransfer(index, ptDataBlob);
}

int IscStatementMetaData::objectVersion()
{
	return STATEMENTMETADATA_VERSION;
}

}; // end namespace IscDbcLibrary
