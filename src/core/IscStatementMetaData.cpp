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
 */

// IscStatementMetaData.cpp: implementation of the IscStatementMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscBlob.h"
#include "IscArray.h"
#include "IscHeadSqlVar.h"
#include "IscStatement.h"
#include "IscStatementMetaData.h"
#include "IscColumnKeyInfo.h"
#include "Sqlda.h"
#include "SqldaMetadata.h"
#include "MultibyteConvert.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscStatementMetaData::IscStatementMetaData(IscStatement *stmt, Sqlda *ptSqlda)
{
	statement = stmt;
	sqlda = ptSqlda;
}

int IscStatementMetaData::getColumnCount()
{
	return sqlda->getColumnCount();
}

int IscStatementMetaData::getColumnType(int index, int &realSqlType)
{
	return sqlda_get_sql_type(sqlda->Var(index), realSqlType);
}

int IscStatementMetaData::getPrecision(int index)
{
	return sqlda_get_precision(sqlda->Var(index));
}

int IscStatementMetaData::getNumPrecRadix(int index)
{
	return sqlda_get_num_prec_radix(sqlda->Var(index));
}

int IscStatementMetaData::getScale(int index)
{
	auto* var = sqlda->Var(index);
	switch (var->sqltype)
	{
	case SQL_TIMESTAMP:
	case SQL_TYPE_TIME:
		return -ISC_TIME_SECONDS_PRECISION_SCALE;
	}
	return -var->sqlscale;
}

bool IscStatementMetaData::isNullable(int index)
{
	return sqlda->Var(index)->isNullable;
}

int IscStatementMetaData::getColumnDisplaySize(int index)
{
	return sqlda_get_column_display_size(sqlda->effectiveVarProperties(index), sqlda->Var(index));
}

// Helper: return "" instead of nullptr (std::string(nullptr) is UB)
static inline const char* safe_str(const char* s) { return s ? s : ""; }

const char* IscStatementMetaData::getColumnLabel(int index)
{
	auto* var = sqlda->Var(index);
	return safe_str((var->aliasname && *var->aliasname) ? var->aliasname : var->sqlname);
}

const char* IscStatementMetaData::getSqlTypeName(int index)
{
	return safe_str(sqlda_get_sql_type_name(sqlda->Var(index)));
}

const char* IscStatementMetaData::getColumnName(int index)
{
	return safe_str(sqlda->Var(index)->sqlname);
}

const char* IscStatementMetaData::getTableName(int index)
{
	return safe_str(sqlda->getTableName(index));
}

const char* IscStatementMetaData::getColumnTypeName(int index)
{
	return safe_str(sqlda_get_sql_type_name(sqlda->Var(index)));
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
	int type = sqlda_get_sql_type(sqlda->Var(index), realSqlType);

	return type != JDBC_LONGVARCHAR && type != JDBC_LONGVARBINARY;
}

int IscStatementMetaData::isBlobOrArray(int index)
{
	return sqlda->isBlobOrArray(index);
}

bool IscStatementMetaData::isColumnPrimaryKey( int index )
{
	IscColumnKeyInfo keyInfo( (IscDatabaseMetaData*)statement->connection->getMetaData() );

	return keyInfo.getColumnKeyInfo( sqlda->getTableName( index ), sqlda->Var( index )->sqlname );
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
			pt->setType(sqlda->Var(index)->sqlsubtype);
			pt->setMinSegment( DEFAULT_BLOB_BUFFER_LENGTH );
			pt->setConsecutiveRead( true );
			pt->statement = statement;
			ptDataBlob = pt;
		}
		else // if ( isRet == SQL_ARRAY )
		{
			IscArray * pt = new IscArray(statement, sqlda->Var( index ));
			ptDataBlob = pt;
		}
	}
}

void IscStatementMetaData::getSqlData(int index, Blob *& ptDataBlob, HeadSqlVar *& ptHeadSqlVar)
{
	CAttrSqlVar *var = sqlda->Var(index);

	if ( ptHeadSqlVar )
		ptHeadSqlVar->release();

	ptHeadSqlVar = new IscHeadSqlVar(var);

	createBlobDataTransfer(index, ptDataBlob);
}

WCSTOMBS IscStatementMetaData::getAdressWcsToMbs( int index )
{
	int charsetCode = isBlobOrArray( index ) ? statement->connection->getConnectionCharsetCode() : sqlda->Var( index )->sqlsubtype & 0xff;
	return adressWcsToMbs( charsetCode );
}

MBSTOWCS IscStatementMetaData::getAdressMbsToWcs( int index )
{
	int charsetCode = isBlobOrArray( index ) ? statement->connection->getConnectionCharsetCode() : sqlda->Var( index )->sqlsubtype & 0xff;
	return adressMbsToWcs( charsetCode );
}

int IscStatementMetaData::objectVersion()
{
	return STATEMENTMETADATA_VERSION;
}

}; // end namespace IscDbcLibrary
